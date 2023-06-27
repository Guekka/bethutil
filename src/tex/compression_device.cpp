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
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
template<typename Func>
auto get_api_from_dll(const wchar_t *dll, const char *func) -> Func
{
    const HMODULE h_mod_dll = LoadLibraryW(dll);
    if (h_mod_dll == nullptr)
        return nullptr;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
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

CompressionDevice::operator bool() const
{
    return is_valid();
}

auto CompressionDevice::is_valid() const -> bool
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    return static_cast<bool>(device_);
#else
    return false;
#endif
}

auto CompressionDevice::get_device() const -> ID3D11Device *
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    return device_->Get();
#else
    return nullptr;
#endif
}

auto CompressionDevice::gpu_name() const -> const std::u8string &
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    return gpu_name_;
#else
    static const std::u8string empty;
    return empty;
#endif
}

auto CompressionDevice::make([[maybe_unused]] uint32_t adapter_index, [[maybe_unused]] bool allow_software)
    -> std::optional<CompressionDevice>
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    CompressionDevice ret;
    ID3D11Device **device = ret.device_->GetAddressOf();
    if (device == nullptr)
        return std::nullopt;

    *device = nullptr;

    static PFN_D3D11_CREATE_DEVICE s_dynamic_d3_d11_create_device = make_device_creator();
    if (s_dynamic_d3_d11_create_device == nullptr)
        return std::nullopt;

    const auto feature_levels = std::to_array({
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

    if (((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U) && !allow_software)
    {
        return std::nullopt;
    }

    ret.gpu_name_ = btu::common::to_utf8(static_cast<const wchar_t *>(desc.Description));

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
        if ((device == nullptr) || ((*device) == nullptr))
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

    return std::make_optional(std::move(ret));
#else
    return std::nullopt;
#endif
}

CompressionDevice::CompressionDevice([[maybe_unused]] CompressionDevice &&other) noexcept
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    : device_(std::move(other.device_))
    , gpu_name_(std::move(other.gpu_name_))
#endif
{
}

auto CompressionDevice::operator=([[maybe_unused]] CompressionDevice &&other) noexcept -> CompressionDevice &
{
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    if (this != &other)
    {
        device_   = std::move(other.device_);
        gpu_name_ = std::move(other.gpu_name_);
    }
#endif
    return *this;
}

CompressionDevice::CompressionDevice()
#if defined(__d3d11_h__) || defined(__d3d11_x_h__)
    : device_(std::make_unique<Microsoft::WRL::ComPtr<ID3D11Device>>())
#endif
{
}
CompressionDevice::~CompressionDevice() = default;
} // namespace btu::tex
