/* Copyright (C) 2019 - 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/compression_device.hpp"

#include <btu/tex/dxtex.hpp>

#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
#include <btu/common/string.hpp>
#include <wrl/client.h>

#include <array>
#endif

namespace btu::tex {
namespace detail {
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
template<typename Func>
auto get_api_from_dll(const wchar_t *dll, const char *func) -> Func
{
    const HMODULE h_mod_dll = LoadLibraryW(dll);
    if (h_mod_dll == nullptr)
        return nullptr;

    auto ptr = reinterpret_cast<Func>(GetProcAddress(h_mod_dll, func));
    return ptr;
}

auto make_device_creator() -> PFN_D3D11_CREATE_DEVICE
{
    return get_api_from_dll<PFN_D3D11_CREATE_DEVICE>(L"d3d11.dll", "D3D11CreateDevice");
}

auto get_dxgi_factory(IDXGIFactory1 **factory) -> bool
{
    if (factory == nullptr)
        return false;

    *factory = nullptr;

    using pfn_CreateDXGIFactory1 = HRESULT(WINAPI *)(REFIID riid, _Out_ void **pp_factory);

    static auto create = get_api_from_dll<pfn_CreateDXGIFactory1>(L"dxgi.dll", "CreateDXGIFactory1");
    if (create == nullptr)
        return false;

    return SUCCEEDED(create(IID_PPV_ARGS(factory)));
}
#endif

struct DxAdapter
{
#ifdef _WIN32
    // Kinda ridiculous to have a smart pointer own another one,
    // But I don't want to leak Windows headers
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    std::u8string gpu_name;
#endif
};

[[nodiscard]] auto make_dx_adapter([[maybe_unused]] uint32_t adapter_index,
                                   [[maybe_unused]] bool allow_software = true) -> std::optional<DxAdapter>
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    DxAdapter ret;
    ID3D11Device **device = ret.device.GetAddressOf();
    if (device == nullptr)
        return std::nullopt;

    *device = nullptr;

    static PFN_D3D11_CREATE_DEVICE s_dynamic_d3_d11_create_device = make_device_creator();
    if (s_dynamic_d3_d11_create_device == nullptr)
        return std::nullopt;

    constexpr auto feature_levels = std::to_array({
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    });

    constexpr UINT create_device_flags = 0;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> p_adapter;
    Microsoft::WRL::ComPtr<IDXGIFactory1> dxgi_factory;

    if (!get_dxgi_factory(dxgi_factory.GetAddressOf()))
        return std::nullopt;

    auto hr = dxgi_factory->EnumAdapters1(adapter_index, p_adapter.GetAddressOf());
    if (FAILED(hr) || hr == DXGI_ERROR_NOT_FOUND)
    {
        return std::nullopt;
    }

    DXGI_ADAPTER_DESC1 desc = {};
    if (FAILED(p_adapter->GetDesc1(&desc)))
        return std::nullopt;

    if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U && !allow_software)
    {
        return std::nullopt;
    }

    ret.gpu_name = common::to_utf8(desc.Description);

    D3D_FEATURE_LEVEL fl{};
    hr = s_dynamic_d3_d11_create_device(p_adapter.Get(),
                                        p_adapter != nullptr ? D3D_DRIVER_TYPE_UNKNOWN
                                                             : D3D_DRIVER_TYPE_HARDWARE,
                                        nullptr,
                                        create_device_flags,
                                        feature_levels.data(),
                                        static_cast<UINT>(feature_levels.size()),
                                        D3D11_SDK_VERSION,
                                        device,
                                        &fl,
                                        nullptr);
    if (FAILED(hr))
        return std::nullopt;

    if (fl < D3D_FEATURE_LEVEL_11_0)
    {
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
        if (device == nullptr || *device == nullptr)
            return std::nullopt;

        hr = (*device)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
        if (FAILED(hr))
            memset(&hwopts, 0, sizeof(hwopts));

        if (hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x == 0)
        {
            if (*device != nullptr)
            {
                (*device)->Release();
                *device = nullptr;
            }
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    if (FAILED(hr))
        return std::nullopt;

    Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
    hr = (*device)->QueryInterface(IID_PPV_ARGS(dxgi_device.GetAddressOf()));

    if FAILED (hr)
        return std::nullopt;

    return ret;
#else
    return std::nullopt;
#endif
}
} // namespace detail

CompressionDevice::CompressionDevice()
{
#ifdef _WIN32
    // NOTE: this code was first implemented as a while loop, which, for some reason, made MSVC stuck on linkage
    for (auto dev = detail::make_dx_adapter(0); dev;
         dev      = detail::make_dx_adapter(static_cast<uint32_t>(cached_info_.size())))
    {
        cached_info_.emplace_back(static_cast<uint32_t>(cached_info_.size()), dev->gpu_name);
        devices_.emplace_back(std::make_unique<common::synchronized<detail::DxAdapter>>(std::move(*dev)));
    }
#endif
}

auto CompressionDevice::list_adapters() const noexcept -> const std::vector<AdapterInfo> &
{
    return cached_info_;
}

#ifdef _WIN32
void CompressionDevice::apply(const Callback &callback) noexcept(noexcept(callback))
{
    std::unique_lock lock(apply_mutex_);

    // Try to find an available device
    while (true)
    {
        for (auto &dev : devices_)
        {
            if (auto locked_dev = dev->try_wlock())
            {
                lock.unlock(); // Release the lock on runnersMutex before calling the callback
                callback((*locked_dev)->device.Get()); // Run the callback on the available device
                cv_.notify_one(); // Notify any waiting threads that a device may now be available
                return;
            }
        }

        // If no device is available, wait until one becomes available
        cv_.wait(lock);
    }
}
#endif

// ReSharper disable once CppMemberFunctionMayBeStatic - only used in Windows
auto CompressionDevice::try_apply([[maybe_unused]] const Callback &callback) noexcept(noexcept(callback))
    -> bool
{
#ifdef _WIN32
    for (auto &dev : devices_)
    {
        if (auto locked = dev->try_wlock())
        {
            callback((*locked)->device.Get());
            return true;
        }
    }
#endif

    return false;
}

CompressionDevice::~CompressionDevice() = default;
} // namespace btu::tex
