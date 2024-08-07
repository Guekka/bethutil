#pragma once

#include "btu/common/filesystem.hpp"
#include "btu/common/path.hpp"

#include <catch.hpp>
#include <tl/expected.hpp>

#include <fstream>

#ifdef __unix__
constexpr std::string_view k_fail_on_linux_tag = "[!nonportable] [!shouldfail]";
#else
constexpr std::string_view k_fail_on_linux_tag = "[!nonportable]";
#endif

using btu::Path;               // NOLINT(google-global-names-in-headers)
using namespace std::literals; // NOLINT(google-global-names-in-headers)

namespace Catch {
template<typename T, typename E>
struct StringMaker<tl::expected<T, E>>
{
    static auto convert(const tl::expected<T, E> &expected) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return expected.has_value() ? ts(expected.value()) : ts(expected.error());
    }
};

template<typename E>
struct StringMaker<tl::expected<void, E>>
{
    static auto convert(const tl::expected<void, E> &expected) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return expected.has_value() ? "void value" : ts(expected.error());
    }
};

template<>
struct StringMaker<Path>
{
    static auto convert(const Path &in) -> std::string { return btu::common::as_ascii_string(in.u8string()); }
};

template<>
struct StringMaker<std::u8string_view>
{
    static auto convert(const std::u8string_view &v) -> std::string
    {
        namespace bc = btu::common;
        using bc::UTF8Iterator, bc::U8Unit;
        return std::string(bc::as_ascii(v)) + " ("
               + StringMaker<std::vector<U8Unit>>::convert(std::vector(UTF8Iterator(v), UTF8Iterator::end(v)))
               + ")";
    }
};

template<>
struct StringMaker<std::u8string>
{
    static auto convert(const std::u8string &v) -> std::string
    {
        return StringMaker<std::u8string_view>::convert(v);
    }
};
} // namespace Catch

class TempPath
{
    Path path_;

public:
    explicit TempPath(const Path &dir, std::u8string_view ext = u8"")
        : path_(dir / btu::common::str_random(10) += ext)
    {
    }

    TempPath(const TempPath &) = delete;
    TempPath(TempPath &&other) noexcept
        : path_(std::exchange(other.path_, {}))
    {
    }

    [[nodiscard]] auto operator=(const TempPath &) = delete;
    [[nodiscard]] auto operator=(TempPath &&) noexcept -> TempPath &
    {
        path_ = std::exchange(path_, {});
        return *this;
    }

    ~TempPath()
    {
        if (btu::fs::exists(path_) && path_.native().size() > 10) // just in case
            btu::fs::remove_all(path_);
    }

    [[nodiscard]] auto path() const noexcept -> const Path & { return path_; }
};

template<class T, class E>
[[nodiscard]] auto require_expected(tl::expected<T, E> res) -> T
{
    if (!res)
        FAIL(res.error());

    return std::move(res).value();
}

template<class E>
void require_expected(tl::expected<void, E> res)
{
    if (!res)
        FAIL(res.error());
}

inline void create_file(const Path &path, std::span<const std::byte> content)
{
    require_expected(btu::common::write_file(path, content));
}

inline void create_file(const Path &path, const std::string &content = "")
{
    create_file(path, std::span{reinterpret_cast<const std::byte *>(content.data()), content.size()});
}
