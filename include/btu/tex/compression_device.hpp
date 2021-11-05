/* Copyright (C) 2019 - 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

class ID3D11Device;
class IDXGIFactory1;

#include <memory>
#include <optional>
#include <string>

namespace Microsoft::WRL { // NOLINT
template<typename>
class ComPtr;
} // namespace Microsoft::WRL

namespace btu::tex {

class CompressionDevice
{
public:
    explicit operator bool() const;
    [[nodiscard]] auto is_valid() const -> bool;

    [[nodiscard]] auto get_device() const -> ID3D11Device *;
    [[nodiscard]] auto gpu_name() const -> const std::u8string &;

    static auto make(uint32_t adapter_index, bool allow_software = false) -> std::optional<CompressionDevice>;

    CompressionDevice(const CompressionDevice &) = delete;
    CompressionDevice(CompressionDevice &&other) noexcept;

    auto operator=(const CompressionDevice &) -> CompressionDevice & = delete;
    auto operator=(CompressionDevice &&other) noexcept -> CompressionDevice &;

    ~CompressionDevice();

private:
    CompressionDevice();

    // Kinda ridiculous to have a smart pointer own another one,
    // But I don't want to leak Windows headers
    std::unique_ptr<Microsoft::WRL::ComPtr<ID3D11Device>> device_;
    std::u8string gpu_name_;
};
} // namespace btu::tex
