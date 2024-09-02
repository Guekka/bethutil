/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <btu/common/string.hpp>
#include <btu/tex/crunch_texture.hpp>
#include <btu/tex/dimension.hpp>
#include <btu/tex/formats.hpp>
#include <crunch/crn_cfile_stream.h>
#include <crunch/crn_core.h>
#include <crunch/crn_dynamic_stream.h>
#include <crunch/crn_image.h>
#include <crunch/crn_mipmapped_texture.h>
#include <winerror.h>

#include <mutex>

namespace crnlib {
auto operator==(const image_u8 &lhs, const image_u8 &rhs) noexcept -> bool
{
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *lhs_end = lhs.get_ptr() + lhs.get_total();
    auto *rhs_end = rhs.get_ptr() + rhs.get_total();
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::equal(lhs.get_ptr(), lhs_end, rhs.get_ptr(), rhs_end);
}

auto operator==(const mipmapped_texture &lhs, const mipmapped_texture &rhs) noexcept -> bool
{
    auto cmp = [&](auto... ptrs) { return ((std::invoke(ptrs, lhs) == std::invoke(ptrs, rhs)) && ...); };
    const bool first = cmp(&mipmapped_texture::get_num_faces,
                           &mipmapped_texture::get_num_levels,
                           &mipmapped_texture::get_total_pixels);
    if (!first)
        return false;

    image_u8 lhs_temp, rhs_temp;
    for (uint face = 0; face < lhs.get_num_faces(); face++)
    {
        for (uint level = 0; level < lhs.get_num_levels(); level++)
        {
            image_u8 *lhs_img = lhs.get_level_image(face, level, lhs_temp);
            image_u8 *rhs_img = rhs.get_level_image(face, level, rhs_temp);
            if (*lhs_img != *rhs_img)
                return false;
        }
    }

    return true;
}
} // namespace crnlib

namespace btu::tex {
using namespace crnlib;

void CrunchTexture::set(mipmapped_texture &&tex) noexcept
{
    tex_ = std::move(tex);
}

auto CrunchTexture::get() noexcept -> mipmapped_texture &
{
    return tex_;
}

auto CrunchTexture::get() const noexcept -> const mipmapped_texture &
{
    return tex_;
}

auto CrunchTexture::get_dimension() const noexcept -> Dimension
{
    return {tex_.get_width(), tex_.get_height()};
}

auto CrunchTexture::get_load_path() const noexcept -> const Path &
{
    return load_path_;
}

void CrunchTexture::set_load_path(Path path) noexcept
{
    load_path_ = std::move(path);
}

auto CrunchTexture::get_texture_type() const noexcept -> const TextureType
{
    const auto filename = load_path_.filename().u8string();

    const auto guessed = guess_texture_type(filename);

    return guessed.value_or(TextureType::Diffuse);
}

auto CrunchTexture::get_format_as_dxgi() const noexcept -> const DXGI_FORMAT
{
    switch (tex_.get_format())
    {
        case PIXEL_FMT_DXT1:
        case PIXEL_FMT_DXT1A: return DXGI_FORMAT_BC1_UNORM_SRGB;
        case PIXEL_FMT_DXT2:
        case PIXEL_FMT_DXT3: return DXGI_FORMAT_BC2_UNORM_SRGB;
        case PIXEL_FMT_DXT4:
        case PIXEL_FMT_DXT5: return DXGI_FORMAT_BC3_UNORM_SRGB;
        case PIXEL_FMT_DXT5A: return DXGI_FORMAT_BC4_UNORM;
        case PIXEL_FMT_3DC: return DXGI_FORMAT_BC5_UNORM;
        case PIXEL_FMT_R8G8B8: return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case PIXEL_FMT_A8R8G8B8: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

auto load_crunch(Path path) noexcept -> tl::expected<CrunchTexture, Error>
{
    CrunchTexture tex;
    tex.set_load_path(std::move(path));

    mipmapped_texture tex_;

    const auto load_path = tex.get_load_path().string();

    texture_file_types::format src_file_format = texture_file_types::determine_file_format(load_path.c_str());

    const auto success = tex_.read_from_file(load_path.c_str(), src_file_format);
    if (!success)
    {
        // Crunch saves loading errors in the texture object as a string.
        // Also it is being cleared in a weird fashion so not super helpful.
        return tl::make_unexpected(error_from_hresult(ERROR_READ_FAULT));
    }

    tex.set(std::move(tex_));

    return tex;
}

auto load_crunch(Path relative_path, std::span<std::byte> data) noexcept -> tl::expected<CrunchTexture, Error>
{
    CrunchTexture tex;
    tex.set_load_path(std::move(relative_path));

    const auto load_path = tex.get_load_path().string();

    mipmapped_texture tex_;
    dynamic_stream in_stream(data.data(), static_cast<crnlib::uint>(data.size()), load_path.c_str());
    data_stream_serializer serializer(in_stream);
    const auto success = tex_.read_from_stream(serializer);
    if (!success)
    {
        // Crunch saves loading errors in the texture object as a string.
        // Also it is being cleared in a weird fashion so not super helpful.
        return tl::make_unexpected(error_from_hresult(ERROR_READ_FAULT));
    }

    tex.set(std::move(tex_));

    return tex;
}

auto save(const CrunchTexture &tex, const Path &path) noexcept -> ResultError
{
    const auto filename = path.string();

    cfile_stream write_stream;
    if (!write_stream.open(filename.c_str(), cDataStreamWritable | cDataStreamSeekable))
    {
        return tl::make_unexpected(error_from_hresult(ERROR_WRITE_FAULT));
    }
    data_stream_serializer serializer(write_stream);

    if (!tex.get().write_dds(serializer))
        return tl::make_unexpected(error_from_hresult(ERROR_WRITE_FAULT));
    return {};
}

auto save(const CrunchTexture &tex) noexcept -> tl::expected<std::vector<std::byte>, Error>
{
    dynamic_stream out_stream;
    data_stream_serializer serializer(out_stream);

    if (!tex.get().write_dds(serializer))
        return tl::make_unexpected(error_from_hresult(ERROR_WRITE_FAULT));

    auto buf = out_stream.get_buf();
    // NOLINTBEGIN(*pointer-arithmetic): needed for the conversion to work properly
    return std::vector(static_cast<std::byte *>(static_cast<void *>(buf.get_ptr())),
                       static_cast<std::byte *>(static_cast<void *>(buf.get_ptr())) + buf.size_in_bytes());
    // NOLINTEND(*pointer-arithmetic)
}

} // namespace btu::tex