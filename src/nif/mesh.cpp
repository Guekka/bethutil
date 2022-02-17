/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/nif/mesh.hpp"

namespace btu::nif {
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

auto load(Path path) noexcept -> tl::expected<Mesh, Error>
{
    Mesh m;
    m.set_load_path(std::move(path));

    try
    {
        const int res = m.get().Load(m.get_load_path());
        if (res != 0)
            return tl::make_unexpected(Error(std::error_code(res, std::generic_category())));
    }
    catch (const std::exception &)
    {
        return tl::make_unexpected(Error(std::error_code(1, std::generic_category())));
    }

    return m;
}

auto save(Mesh mesh, const Path &path) noexcept -> ResultError
{
    const int res = mesh.get().Save(path);
    if (res != 0)
        return tl::make_unexpected(Error(std::error_code(res, std::generic_category())));
    return {};
}

} // namespace btu::nif
