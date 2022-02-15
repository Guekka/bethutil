/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/tex/texture.hpp"

#include "btu/tex/dimension.hpp"

#include <DirectXTex.h>
#include <btu/common/string.hpp>

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

    const auto end = lhs.GetPixels() + lhs.GetPixelsSize(); // NOLINT
    return std::equal(lhs.GetPixels(), end, rhs.GetPixels());
}
} // namespace DirectX

namespace btu::tex {
namespace detail {
static_assert(k_sizeof_scratchimage == sizeof(ScratchImage));
static_assert(k_alignof_scratchimage == alignof(ScratchImage));

void initialize_com()
{
    const auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        throw btu::common::Exception(error_from_hresult(hr));
}

ScratchImagePimpl::ScratchImagePimpl()
{
    new (&storage_) ScratchImage();
    initialize_com();
}

ScratchImagePimpl::ScratchImagePimpl(ScratchImagePimpl &&other) noexcept
{
    new (&storage_) ScratchImage(std::move(other).get());
    initialize_com();
}

auto ScratchImagePimpl::operator=(ScratchImagePimpl &&other) noexcept -> ScratchImagePimpl &
{
    get() = std::move(other).get();
    return *this;
}

ScratchImagePimpl::~ScratchImagePimpl()
{
    get().~ScratchImage();
}

auto ScratchImagePimpl::get() &noexcept -> ScratchImage &
{
    return *std::launder(reinterpret_cast<ScratchImage *>(&storage_));
}

auto ScratchImagePimpl::get() &&noexcept -> ScratchImage
{
    return std::move(*std::launder(reinterpret_cast<ScratchImage *>(&storage_)));
}

auto ScratchImagePimpl::get() const &noexcept -> const ScratchImage &
{
    return *std::launder(reinterpret_cast<const ScratchImage *>(&storage_));
}

auto operator==(const ScratchImagePimpl &lhs, const ScratchImagePimpl &rhs) noexcept -> bool
{
    return lhs.get() == rhs.get();
}

} // namespace detail

void Texture::set(DirectX::ScratchImage &&tex) noexcept
{
    tex_.get() = std::move(tex);
}

auto Texture::get() noexcept -> ScratchImage &
{
    return tex_.get();
}

auto Texture::get() const noexcept -> const ScratchImage &
{
    return tex_.get();
}

auto Texture::get_images() const noexcept -> std::span<const Image>
{
    const auto begin = get().GetImages();
    const auto end   = begin + get().GetImageCount(); // NOLINT
    return std::span(begin, end);
}

auto Texture::get_dimension() const noexcept -> Dimension
{
    const auto info = get().GetMetadata();
    return {info.width, info.height};
}

auto Texture::get_load_path() const noexcept -> const Path &
{
    return load_path_;
}

void Texture::set_load_path(Path path) noexcept
{
    load_path_ = std::move(path);
}

tl::expected<Texture, Error> load(Path path) noexcept
{
    Texture tex;
    tex.set_load_path(std::move(path));

    DirectX::TexMetadata info{};
    const auto load_path = tex.get_load_path().wstring();
    auto hr = DirectX::LoadFromDDSFile(load_path.c_str(), DirectX::DDS_FLAGS_NONE, &info, tex.get());
    if (FAILED(hr))
    {
        // Maybe it's a TGA then?
        const auto hr2 = DirectX::LoadFromTGAFile(load_path.c_str(),
                                                  DirectX::TGA_FLAGS_NONE,
                                                  &info,
                                                  tex.get());
        if (FAILED(hr2))
            return tl::make_unexpected(error_from_hresult(hr)); // preserve original error
    }
    return tex;
}

ResultError save(const Texture &tex, Path path) noexcept
{
    const auto res = DirectX::SaveToDDSFile(tex.get().GetImages(),
                                            tex.get().GetImageCount(),
                                            tex.get().GetMetadata(),
                                            DirectX::DDS_FLAGS_NONE,
                                            path.wstring().c_str());
    if (FAILED(res))
        return tl::make_unexpected(error_from_hresult(res));
    return {};
}

} // namespace btu::tex
