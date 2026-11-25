#include "btu/esp/error_code.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/metaprogramming.hpp>
#include <btu/esp/functions.hpp>

#include <cstring>
#include <fstream>
#include <map>
#include <optional>

namespace btu::esp {

[[nodiscard]] auto load_file(const Path &path) noexcept -> tl::expected<std::fstream, Error>
{
    auto file = std::fstream{path, std::ios::binary | std::ios::in};
    if (!file)
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

    return file;
}

/// The maximum allowed path length. It is way more than enough for any sane path.
constexpr size_t reasonable_max_path = 2048;

auto read_header(std::fstream &file, detail::PluginRecordHeader &header) noexcept -> bool
{
    file.read(reinterpret_cast<char *>(&header), sizeof header);
    return file.gcount() == static_cast<std::streamsize>(sizeof header);
}

auto read_field_plugin_header(std::fstream &file,
                              detail::PluginFieldHeader &plugin_field_header) noexcept -> bool
{
    file.read(reinterpret_cast<char *>(&plugin_field_header), sizeof plugin_field_header);
    return file.gcount() == static_cast<std::streamsize>(sizeof plugin_field_header);
}

auto stream_end_position(std::fstream &file) noexcept -> std::optional<std::streamoff>
{
    file.seekg(0, std::ios::end);
    if (!file)
        return std::nullopt;

    const auto end = file.tellg();
    if (end == std::streampos(-1))
        return std::nullopt;

    file.seekg(0, std::ios::beg);
    if (!file)
        return std::nullopt;

    return static_cast<std::streamoff>(end);
}

auto checked_end_position(std::fstream &file,
                          const std::uint64_t size,
                          const std::streamoff parent_end) noexcept -> std::optional<std::streamoff>
{
    const auto position = file.tellg();
    if (position == std::streampos(-1))
        return std::nullopt;

    const auto start = static_cast<std::streamoff>(position);
    if (start < 0 || start > parent_end)
        return std::nullopt;

    const auto available = static_cast<std::uint64_t>(parent_end - start);
    if (size > available)
        return std::nullopt;

    return start + static_cast<std::streamoff>(size);
}

auto seek_to(std::fstream &file, const std::streamoff position) noexcept -> bool
{
    file.seekg(position, std::ios::beg);
    return static_cast<bool>(file);
}

auto list_headparts(const Path &input) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    return load_file(input).and_then([](auto &&file) { return list_headparts(BTU_FWD(file)); });
}

auto list_headparts(std::fstream file) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    detail::PluginRecordHeader header{};
    detail::PluginFieldHeader plugin_field_header{};

    const auto file_end = stream_end_position(file);
    if (!file_end)
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

    auto headparts = std::vector<std::u8string>{};
    headparts.reserve(500); // unlikely to have more than 500 headparts

    if (!read_header(file, header))
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));
    if (header.plugin.type != detail::k_group_tes4)
        return {}; //Not a plugin file

    const auto tes4_end = checked_end_position(file, header.record.data_size, *file_end);
    if (!tes4_end || !seek_to(file, *tes4_end))
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

    while (true)
    {
        const auto group_start = file.tellg();
        if (group_start == std::streampos(-1))
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));
        if (group_start == std::streampos(*file_end))
            break;
        if (group_start > std::streampos(*file_end) || !read_header(file, header)
            || header.plugin.type != detail::k_group_grup
            || header.plugin.group_size < sizeof header)
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));

        const auto group_end_pos = checked_end_position(
            file, header.plugin.group_size - sizeof header, *file_end);
        if (!group_end_pos)
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));

        // skip non headpart groups
        if (header.plugin.label != detail::k_group_hdpt)
        {
            if (!seek_to(file, *group_end_pos))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));
            continue;
        }

        while (true)
        {
            const auto record_start = file.tellg();
            if (record_start == std::streampos(-1))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));
            if (record_start == std::streampos(*group_end_pos))
                break;
            if (record_start > std::streampos(*group_end_pos) || !read_header(file, header))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));

            const auto record_end_pos =
                checked_end_position(file, header.record.data_size, *group_end_pos);
            if (!record_end_pos)
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));

            while (true)
            {
                const auto field_start = file.tellg();
                if (field_start == std::streampos(-1))
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                if (field_start == std::streampos(*record_end_pos))
                    break;
                if (field_start > std::streampos(*record_end_pos)
                    || !read_field_plugin_header(file, plugin_field_header))
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                const auto field_end_pos =
                    checked_end_position(file, plugin_field_header.data_size, *record_end_pos);
                if (!field_end_pos)
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                // skip everything but MODL
                if (plugin_field_header.type != detail::k_group_modl
                    || plugin_field_header.data_size > reasonable_max_path)
                {
                    if (!seek_to(file, *field_end_pos))
                        return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                    continue;
                }

                auto headpart = std::string(plugin_field_header.data_size, '\0');
                file.read(headpart.data(), plugin_field_header.data_size);
                if (!file)
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                constexpr auto cleanup_path = common::make_path_canonizer(u8"meshes/");
                headparts.emplace_back(cleanup_path(headpart));
            }
        }
    }

    common::remove_duplicates(headparts);
    return headparts;
}

auto list_landscape_textures(const Path &input) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    return load_file(input).and_then([](auto &&file) { return list_landscape_textures(BTU_FWD(file)); });
}

auto list_landscape_textures(std::fstream file) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    detail::PluginRecordHeader header{};
    detail::PluginFieldHeader plugin_field_header{};

    const auto file_end = stream_end_position(file);
    if (!file_end)
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

    if (!read_header(file, header))
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));
    if (header.plugin.type != detail::k_group_tes4)
        return {}; //Not a plugin file

    const auto tes4_end = checked_end_position(file, header.record.data_size, *file_end);
    if (!tes4_end || !seek_to(file, *tes4_end))
        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

    std::vector<uint32_t> tnam_form_ids;
    std::map<uint32_t, std::u8string> txst_textures;

    while (true)
    {
        const auto group_start = file.tellg();
        if (group_start == std::streampos(-1))
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));
        if (group_start == std::streampos(*file_end))
            break;
        if (group_start > std::streampos(*file_end) || !read_header(file, header)
            || header.plugin.type != detail::k_group_grup
            || header.plugin.group_size < sizeof header)
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));

        const auto group_end_pos = checked_end_position(
            file, header.plugin.group_size - sizeof header, *file_end);
        if (!group_end_pos)
            return tl::make_unexpected(Error(EspErr::FailedToReadFile));

        // skip other groups
        if (header.plugin.label != detail::k_group_ltex && header.plugin.label != detail::k_group_txst)
        {
            if (!seek_to(file, *group_end_pos))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));
            continue;
        }

        std::array<char, 4> signature_group{};
        std::copy_n(std::begin(header.plugin.label), signature_group.size(), signature_group.begin());

        while (true)
        {
            const auto record_start = file.tellg();
            if (record_start == std::streampos(-1))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));
            if (record_start == std::streampos(*group_end_pos))
                break;
            if (record_start > std::streampos(*group_end_pos) || !read_header(file, header))
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));

            const auto record_end_pos =
                checked_end_position(file, header.record.data_size, *group_end_pos);
            if (!record_end_pos)
                return tl::make_unexpected(Error(EspErr::FailedToReadFile));

            while (true)
            {
                const auto field_start = file.tellg();
                if (field_start == std::streampos(-1))
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                if (field_start == std::streampos(*record_end_pos))
                    break;
                if (field_start > std::streampos(*record_end_pos)
                    || !read_field_plugin_header(file, plugin_field_header))
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                const auto field_end_pos =
                    checked_end_position(file, plugin_field_header.data_size, *record_end_pos);
                if (!field_end_pos)
                    return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                // read FormID of landscape TXST record
                if (signature_group == detail::k_group_ltex
                    && plugin_field_header.type == detail::k_group_tnam)
                {
                    if (plugin_field_header.data_size != sizeof(uint32_t))
                    {
                        if (!seek_to(file, *field_end_pos))
                            return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                        continue;
                    }

                    uint32_t form_id = 0;
                    file.read(reinterpret_cast<char *>(&form_id), sizeof form_id);
                    if (!file)
                        return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                    tnam_form_ids.emplace_back(form_id);
                }
                // read diffuse texture name from TXST record
                else if (signature_group == detail::k_group_txst
                         && plugin_field_header.type == detail::k_group_tx00)
                {
                    // prevent reading of corrupted files
                    if (plugin_field_header.data_size > reasonable_max_path)
                    {
                        if (!seek_to(file, *field_end_pos))
                            return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                        continue;
                    }

                    auto texture = std::string(plugin_field_header.data_size, '\0');
                    file.read(texture.data(), plugin_field_header.data_size);
                    if (!file)
                        return tl::make_unexpected(Error(EspErr::FailedToReadFile));

                    constexpr auto cleanup_path = common::make_path_canonizer(u8"textures/");
                    txst_textures.try_emplace(header.record.id, cleanup_path(texture));
                }
                else
                { // skip other fields
                    if (!seek_to(file, *field_end_pos))
                        return tl::make_unexpected(Error(EspErr::FailedToReadFile));
                }
            }
        }
    }

    // go over landscape texture set FormIDs and find matching diffuse textures
    auto ret = std::vector<std::u8string>{};
    ret.reserve(tnam_form_ids.size());
    for (auto form_id : tnam_form_ids)
    {
        if (auto it = txst_textures.find(form_id); it != txst_textures.end())
            ret.emplace_back(it->second);
    }
    common::remove_duplicates(ret);
    return ret;
}

} // namespace btu::esp
