/* Copyright (C) 2019 - 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#pragma once

#include <btu/common/threading.hpp>

#include <string>

class ID3D11Device;

namespace btu::tex {
namespace detail {
struct DxAdapter;
} // namespace detail

/// Recommended way to use is to make it static
class CompressionDevice
{
public:
    struct AdapterInfo
    {
        uint32_t index{};
        std::u8string name;
    };

    using Callback = std::function<void(ID3D11Device *dev)>;

    CompressionDevice();
    ~CompressionDevice(); // = default;

    [[nodiscard]] auto list_adapters() const noexcept -> const std::vector<AdapterInfo> &;

    // I don't like platform specific API,, but I can't think of a better way
#ifdef _WIN32
    void apply(const Callback &callback) noexcept(noexcept(callback));
#endif

    // This one will always return false on non-windows platforms
    [[nodiscard]] auto try_apply(const Callback &callback) noexcept(noexcept(callback)) -> bool;

private:
    std::vector<std::unique_ptr<common::synchronized<detail::DxAdapter>>> devices_;
    std::vector<AdapterInfo> cached_info_;

    std::mutex apply_mutex_;
    std::condition_variable cv_;
};
} // namespace btu::tex
