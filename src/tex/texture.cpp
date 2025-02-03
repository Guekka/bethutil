/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <btu/common/string.hpp>
#include <btu/tex/dimension.hpp>
#include <btu/tex/dxtex.hpp>
#include <btu/tex/texture.hpp>

#include <mutex>

namespace DirectX {
auto operator==(const TexMetadata &lhs, const TexMetadata &rhs) noexcept -> bool
{
    auto cmp = [&](auto... ptrs) { return ((std::invoke(ptrs, lhs) == std::invoke(ptrs, rhs)) && ...); };
    return cmp(&TexMetadata::arraySize,
               &TexMetadata::depth,
               &TexMetadata::format,
               &TexMetadata::height,
               &TexMetadata::mipLevels,
               &TexMetadata::miscFlags,
               &TexMetadata::miscFlags2,
               &TexMetadata::width);
}

auto operator==(const ScratchImage &lhs, const ScratchImage &rhs) noexcept -> bool
{
    auto cmp = [&](auto... ptrs) { return ((std::invoke(ptrs, lhs) == std::invoke(ptrs, rhs)) && ...); };
    const bool first = cmp(&ScratchImage::GetMetadata,
                           &ScratchImage::GetImageCount,
                           &ScratchImage::GetPixelsSize);
    if (!first)
        return false;

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto *lhs_end = lhs.GetPixels() + lhs.GetPixelsSize();
    auto *rhs_end = rhs.GetPixels() + rhs.GetPixelsSize();
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::equal(lhs.GetPixels(), lhs_end, rhs.GetPixels(), rhs_end);
}
} // namespace DirectX

void initialize_com()
{
#ifdef _WIN32
    const auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        throw btu::common::Exception(btu::tex::error_from_hresult(hr));
#endif
}

namespace btu::tex {
Texture::Texture()
{
    static std::once_flag wic_initialized;
    std::call_once(wic_initialized, initialize_com);
}

void Texture::set(ScratchImage &&tex) noexcept
{
    tex_ = std::move(tex);
}

auto Texture::get() noexcept -> ScratchImage &
{
    return tex_;
}

auto Texture::get() const noexcept -> const ScratchImage &
{
    return tex_;
}

auto Texture::get_images() const noexcept -> std::span<const Image>
{
    const auto *begin = get().GetImages();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto *end = begin + get().GetImageCount();
    return {begin, end};
}

auto Texture::get_dimension() const noexcept -> Dimension
{
    const auto info = get().GetMetadata();
    return Dimension{.w = info.width, .h = info.height};
}

auto Texture::get_load_path() const noexcept -> const Path &
{
    return load_path_;
}

void Texture::set_load_path(Path path) noexcept
{
    load_path_ = std::move(path);
}

auto load(Path path) noexcept -> tl::expected<Texture, Error>
{
    Texture tex;
    tex.set_load_path(std::move(path));

    TexMetadata info{};
    const auto load_path = tex.get_load_path().wstring();
    auto hr              = LoadFromDDSFile(load_path.c_str(), DirectX::DDS_FLAGS_NONE, &info, tex.get());
    if (FAILED(hr))
    {
        // Maybe it's a TGA then?
        const auto hr2 = LoadFromTGAFile(load_path.c_str(), DirectX::TGA_FLAGS_NONE, &info, tex.get());
        if (FAILED(hr2))
            return tl::make_unexpected(error_from_hresult(hr)); // preserve original error
    }
    return tex;
}

    auto load(Path relative_path, const std::span<std::byte> data) noexcept -> tl::expected<Texture, Error>
{
    Texture tex;
    tex.set_load_path(std::move(relative_path));

    TexMetadata info{};
    const auto hr = LoadFromDDSMemory(data.data(), data.size(), DirectX::DDS_FLAGS_NONE, &info, tex.get());
    if (FAILED(hr))
    {
        // Maybe it's a TGA then?
        const auto hr2 = LoadFromTGAMemory(data.data(), data.size(), DirectX::TGA_FLAGS_NONE, &info, tex.get());
        if (FAILED(hr2))
            return tl::make_unexpected(error_from_hresult(hr)); // preserve original error
    }
    return tex;
}

auto save(const Texture &tex, const Path &path) noexcept -> ResultError
{
    const auto res = SaveToDDSFile(tex.get().GetImages(),
                                   tex.get().GetImageCount(),
                                   tex.get().GetMetadata(),
                                   DirectX::DDS_FLAGS_NONE,
                                   path.wstring().c_str());
    if (FAILED(res))
        return tl::make_unexpected(error_from_hresult(res));
    return {};
}

auto save(const Texture &tex) noexcept -> tl::expected<std::vector<std::byte>, Error>
{
    auto blob      = DirectX::Blob{};
    const auto res = SaveToDDSMemory(tex.get().GetImages(),
                                     tex.get().GetImageCount(),
                                     tex.get().GetMetadata(),
                                     DirectX::DDS_FLAGS_NONE,
                                     blob);
    if (FAILED(res))
        return tl::make_unexpected(error_from_hresult(res));

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return std::vector(reinterpret_cast<std::byte *>(blob.GetBufferPointer()),
                       reinterpret_cast<std::byte *>(blob.GetBufferPointer()) + blob.GetBufferSize());
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}
} // namespace btu::tex
