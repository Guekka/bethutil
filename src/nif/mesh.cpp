/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/nif/mesh.hpp"

namespace btu::nif {
auto canonize_path(std::filesystem::path path) noexcept -> std::u8string
{
    auto str         = btu::common::to_lower(path.generic_u8string());
    const auto start = std::u8string_view(u8"meshes/");
    auto prefix_end  = str.find(start);
    prefix_end       = prefix_end == std::string::npos ? 0 : prefix_end + start.size();
    return str.substr(prefix_end);
}

ResultError Mesh::load(Path path)
{
    load_path_ = std::move(path);

    const int res = get().Load(path);
    if (res != 0)
        return tl::make_unexpected(btu::common::Error(std::error_code(res, std::generic_category())));
    return {};
}

ResultError Mesh::save(const Path &path)
{
    const int res = get().Save(path);
    if (res != 0)
        return tl::make_unexpected(btu::common::Error(std::error_code(res, std::generic_category())));
    return {};
}

auto Mesh::get() noexcept -> nifly::NifFile &
{
    return file_;
}

auto Mesh::get() const noexcept -> const nifly::NifFile &
{
    return file_;
}

auto Mesh::get_load_path() const noexcept -> const Path &
{
    return load_path_;
}

void Mesh::set_load_path(Path path) noexcept
{
    load_path_ = std::move(path);
}

} // namespace btu::nif
