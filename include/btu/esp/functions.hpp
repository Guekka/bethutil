#pragma once

#include <btu/common/error.hpp>
#include <btu/common/path.hpp>
#include <tl/expected.hpp>

#include <array>
#include <cstring>
#include <vector>

namespace btu::esp {
using common::Error;

namespace detail {
#ifdef __GNUC__
#define PACKED(datastructure) datastructure __attribute__((__packed__))
#else
#define PACKED(datastructure) __pragma(pack(push, 1)) datastructure __pragma(pack(pop))
#endif

PACKED(struct PluginHeader {
    char type[4];
    uint32_t group_size;
    char label[4];
    int32_t group_type;
    unsigned short stamp;
    char vc1;
    char vc2;
    uint32_t unknown;
};)

PACKED(struct RecordHeader {
    char type[4];
    uint32_t data_size;
    uint32_t flags;
    uint32_t id;
    unsigned short stamp;
    char vc1;
    char vc2;
    uint16_t version;
    uint16_t unknown;
};)

PACKED(union PluginRecordHeader {
    RecordHeader record;
    PluginHeader plugin;
};)

PACKED(struct PluginFieldHeader {
    char type[4];
    uint16_t data_size;
};)

#undef PACKED

class GroupType
{
public:
    constexpr static size_t k_length = 4;

    constexpr explicit GroupType(std::string_view value) noexcept
        : value_(value)
    {
        assert(value.size() == k_length);
    }

    [[nodiscard]] auto operator==(const char (&other)[k_length]) const noexcept -> bool
    {
        return std::memcmp(value_.data(), other, k_length) == 0;
    }

    [[nodiscard]] auto operator==(const std::array<char, k_length> &other) const noexcept -> bool
    {
        return std::memcmp(value_.data(), other.data(), other.size()) == 0;
    }

private:
    std::string_view value_;
};

constexpr auto k_group_tes4 = GroupType{"TES4"};
constexpr auto k_group_grup = GroupType{"GRUP"};
constexpr auto k_group_hdpt = GroupType{"HDPT"};
constexpr auto k_group_modl = GroupType{"MODL"};
constexpr auto k_group_ltex = GroupType{"LTEX"};
constexpr auto k_group_tnam = GroupType{"TNAM"};
constexpr auto k_group_txst = GroupType{"TXST"};
constexpr auto k_group_tx00 = GroupType{"TX00"};

} // namespace detail

[[nodiscard]] auto list_headparts(const Path &input) noexcept
    -> tl::expected<std::vector<std::u8string>, Error>;
[[nodiscard]] auto list_headparts(std::fstream file) noexcept
    -> tl::expected<std::vector<std::u8string>, Error>;
[[nodiscard]] auto list_landscape_textures(const Path &input) noexcept
    -> tl::expected<std::vector<std::u8string>, Error>;
[[nodiscard]] auto list_landscape_textures(std::fstream file) noexcept
    -> tl::expected<std::vector<std::u8string>, Error>;
} // namespace btu::esp
