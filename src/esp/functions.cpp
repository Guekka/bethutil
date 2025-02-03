#include "btu/esp/error_code.hpp"

#include <btu/common/algorithms.hpp>
#include <btu/common/metaprogramming.hpp>
#include <btu/esp/functions.hpp>

#include <cstring>
#include <fstream>
#include <map>

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

auto read_headers(std::fstream &file, detail::PluginRecordHeader &header) noexcept -> bool
{
    file.read(reinterpret_cast<char *>(&header), sizeof header);
    return header.plugin.type == detail::k_group_grup;
}

void read_field_plugin_header(std::fstream &file, detail::PluginFieldHeader &plugin_field_header) noexcept
{
    file.read(reinterpret_cast<char *>(&plugin_field_header), sizeof plugin_field_header);
}

auto list_headparts(const Path &input) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    return load_file(input).and_then([](auto &&file) { return list_headparts(BTU_FWD(file)); });
}

auto list_headparts(std::fstream file) noexcept -> tl::expected<std::vector<std::u8string>, Error>
{
    detail::PluginRecordHeader header{};
    detail::PluginFieldHeader plugin_field_header{};

    auto headparts = std::vector<std::u8string>{};
    headparts.reserve(500); // unlikely to have more than 500 headparts

    read_headers(file, header);
    if (header.plugin.type != detail::k_group_tes4)
        return {}; //Not a plugin file

    //Skip TES4 record
    file.seekg(header.record.data_size, std::ios::cur);

    // Reading all groups
    while (read_headers(file, header) && file)
    {
        // skip non headpart groups
        if (header.plugin.label != detail::k_group_hdpt)
        {
            file.seekg(static_cast<std::streamsize>(header.plugin.group_size - sizeof header), std::ios::cur);
            continue;
        }

        //Reading all headpart records
        const auto group_end_pos = static_cast<std::streamsize>(header.plugin.group_size - sizeof header)
                                   + file.tellg();
        while (file.tellg() < group_end_pos)
        {
            read_headers(file, header);

            // reading all record fields
            const int64_t rec_end_pos = header.record.data_size + file.tellg();
            while (file.tellg() < rec_end_pos)
            {
                read_field_plugin_header(file, plugin_field_header);
                // skip everything but MODL
                if (plugin_field_header.type != detail::k_group_modl)
                {
                    file.seekg(plugin_field_header.data_size, std::ios::cur);
                    continue;
                }

                // prevent reading of corrupted files
                if (plugin_field_header.data_size > reasonable_max_path)
                    continue;

                auto headpart = std::string(plugin_field_header.data_size, '\0');
                file.read(headpart.data(), plugin_field_header.data_size);

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

    read_headers(file, header);
    if (header.plugin.type != detail::k_group_tes4)
        return {}; //Not a plugin file

    //Skip TES4 record
    file.seekg(header.record.data_size, std::ios::cur);

    std::vector<uint32_t> tnam_form_ids;
    std::map<uint32_t, std::u8string> txst_textures;

    //Reading all groups
    while (read_headers(file, header) && file)
    {
        // skip other groups
        if (header.plugin.label != detail::k_group_ltex && header.plugin.label != detail::k_group_txst)
        {
            file.seekg(static_cast<std::streamsize>(header.plugin.group_size - sizeof header), std::ios::cur);
            continue;
        }

        std::array<char, 4> signature_group{};
        std::copy_n(std::begin(header.plugin.label), signature_group.size(), signature_group.begin());

        //Reading all records
        const auto group_end_pos = static_cast<std::streamsize>(header.plugin.group_size - sizeof header)
                                   + file.tellg();
        while (file.tellg() < group_end_pos)
        {
            read_headers(file, header);

            // reading all record fields
            const int64_t rec_end_pos = header.record.data_size + file.tellg();
            while (file.tellg() < rec_end_pos)
            {
                read_field_plugin_header(file, plugin_field_header);
                // read FormID of landscape TXST record
                if (signature_group == detail::k_group_ltex
                    && plugin_field_header.type == detail::k_group_tnam)
                {
                    uint32_t form_id = 0;
                    file.read(reinterpret_cast<char *>(&form_id), plugin_field_header.data_size);
                    tnam_form_ids.emplace_back(form_id);
                }
                // read diffuse texture name from TXST record
                else if (signature_group == detail::k_group_txst
                         && plugin_field_header.type == detail::k_group_tx00)
                {
                    // prevent reading of corrupted files
                    if (plugin_field_header.data_size > reasonable_max_path)
                        continue;

                    auto texture = std::string(plugin_field_header.data_size, '\0');
                    file.read(texture.data(), plugin_field_header.data_size);

                    constexpr auto cleanup_path = common::make_path_canonizer(u8"textures/");
                    txst_textures.try_emplace(header.record.id, cleanup_path(texture));
                }
                else
                { // skip other fields
                    file.seekg(plugin_field_header.data_size, std::ios::cur);
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
