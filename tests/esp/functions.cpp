#include "../utils.hpp"

#include <btu/esp/functions.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
template<typename T>
void append_bytes(std::vector<std::byte> &destination, const T &value)
{
    const auto bytes = std::as_bytes(std::span{&value, size_t{1}});
    destination.insert(destination.end(), bytes.begin(), bytes.end());
}

auto bytes_of(std::string_view value) -> std::span<const std::byte>
{
    return std::as_bytes(std::span{value.data(), value.size()});
}

auto make_field(std::string_view type, std::span<const std::byte> payload) -> std::vector<std::byte>
{
    auto header = btu::esp::detail::PluginFieldHeader{};
    std::memcpy(header.type, type.data(), sizeof header.type);
    header.data_size = static_cast<std::uint16_t>(payload.size());

    auto field = std::vector<std::byte>{};
    append_bytes(field, header);
    field.insert(field.end(), payload.begin(), payload.end());
    return field;
}

auto make_record(std::string_view type,
                 const std::uint32_t id,
                 std::span<const std::byte> fields) -> std::vector<std::byte>
{
    auto header = btu::esp::detail::PluginRecordHeader{};
    std::memcpy(header.record.type, type.data(), sizeof header.record.type);
    header.record.data_size = static_cast<std::uint32_t>(fields.size());
    header.record.id        = id;

    auto record = std::vector<std::byte>{};
    append_bytes(record, header);
    record.insert(record.end(), fields.begin(), fields.end());
    return record;
}

auto make_group(std::string_view label, std::span<const std::byte> records) -> std::vector<std::byte>
{
    auto header = btu::esp::detail::PluginRecordHeader{};
    std::memcpy(header.plugin.type, "GRUP", sizeof header.plugin.type);
    header.plugin.group_size = static_cast<std::uint32_t>(sizeof header + records.size());
    std::memcpy(header.plugin.label, label.data(), sizeof header.plugin.label);

    auto group = std::vector<std::byte>{};
    append_bytes(group, header);
    group.insert(group.end(), records.begin(), records.end());
    return group;
}

auto make_plugin(std::span<const std::byte> groups) -> std::vector<std::byte>
{
    auto plugin = make_record("TES4", 0, {});
    plugin.insert(plugin.end(), groups.begin(), groups.end());
    return plugin;
}

auto write_temp_plugin(const std::vector<std::byte> &plugin) -> TempPath
{
    auto path = TempPath{".", u8".esp"};
    require_expected(btu::common::write_file(path.path(), std::span<const std::byte>{plugin}));
    return path;
}
} // namespace

TEST_CASE("list_headparts", "[src]")
{
    const auto res = btu::esp::list_headparts("esp/headparts.esp");
    REQUIRE(res);
    REQUIRE(res->size() == 52);

    // no need to check all of them, hopefully this is enough
    CHECK(res->at(00) == u8"actors/character/character assets/eyesfemale.nif");
    CHECK(res->at(10) == u8"actors/character/character assets/hair/femaleredguardhair02.nif");
    CHECK(res->at(20) == u8"actors/character/ranaline/character assets/childmaskrightside.nif");
    CHECK(res->at(30) == u8"actors/character/ranaline/hair/children/female02.nif");
    CHECK(res->at(40) == u8"actors/character/ranaline/hair/children/male03.nif");
    CHECK(res->at(50) == u8"actors/character/ranaline/hair/hairginko04.nif");
}

TEST_CASE("list_landscape_textures", "[src]")
{
    const auto res = btu::esp::list_landscape_textures("esp/landscape.esp");
    REQUIRE(res);
    const auto expected = std::vector<std::u8string>{
        u8"dlc01/landscape/glowingforestdirt01.dds",
        u8"dlc01/landscape/soulcairnbones01.dds",
        u8"dlc01/landscape/soulcairndirt01.dds",
        u8"dlc01/landscape/winterforestleaves02.dds",
    };
    CHECK(*res == expected);
}

TEST_CASE("ESP parsers skip oversized and malformed fields")
{
    SECTION("headpart models")
    {
        const auto oversized_path = std::string(2049, 'x');
        auto fields               = make_field("MODL", bytes_of(oversized_path));
        const auto valid_path     = make_field("MODL", bytes_of("meshes/example.nif"));
        fields.insert(fields.end(), valid_path.begin(), valid_path.end());

        const auto record = make_record("HDPT", 1, fields);
        const auto group  = make_group("HDPT", record);
        const auto plugin = make_plugin(group);
        auto path         = write_temp_plugin(plugin);

        auto file         = std::fstream{path.path(), std::ios::binary | std::ios::in};
        const auto result = btu::esp::list_headparts(std::move(file));
        REQUIRE(result);
        CHECK(*result == std::vector<std::u8string>{u8"example.nif"});
    }

    SECTION("landscape texture IDs and paths")
    {
        const auto invalid_id          = std::array<std::byte, 8>{};
        auto ltex_fields               = make_field("TNAM", invalid_id);
        const std::uint32_t texture_id = 42;
        const auto valid_id            = make_field("TNAM", std::as_bytes(std::span{&texture_id, size_t{1}}));
        ltex_fields.insert(ltex_fields.end(), valid_id.begin(), valid_id.end());

        const auto ltex_record = make_record("LTEX", 1, ltex_fields);
        const auto ltex_group  = make_group("LTEX", ltex_record);

        const auto oversized_path = std::vector<std::byte>(2049, std::byte{'x'});
        auto txst_fields          = make_field("TX00", oversized_path);
        const auto valid_path     = make_field("TX00", bytes_of("textures/test.dds"));
        txst_fields.insert(txst_fields.end(), valid_path.begin(), valid_path.end());

        const auto txst_record = make_record("TXST", texture_id, txst_fields);
        const auto txst_group  = make_group("TXST", txst_record);

        auto groups = ltex_group;
        groups.insert(groups.end(), txst_group.begin(), txst_group.end());
        const auto plugin = make_plugin(groups);
        auto path         = write_temp_plugin(plugin);

        auto file         = std::fstream{path.path(), std::ios::binary | std::ios::in};
        const auto result = btu::esp::list_landscape_textures(std::move(file));
        REQUIRE(result);
        CHECK(*result == std::vector<std::u8string>{u8"test.dds"});
    }
}