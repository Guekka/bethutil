
#include <btu/common/filesystem.hpp>
#include <flux.hpp>

#include <fstream>

namespace btu {
auto common::read_file(const Path &a_path) noexcept -> tl::expected<std::vector<std::byte>, common::Error>
{
    std::error_code ec;

    const auto size = fs::file_size(a_path, ec);
    if (ec)
        return tl::make_unexpected(Error(ec));

    std::vector<std::byte> data(size);

    std::ifstream in{a_path, std::ios_base::in | std::ios_base::binary};
    if (!in)
        return tl::make_unexpected(Error(std::error_code{errno, std::system_category()}));
    in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!in)
        return tl::make_unexpected(Error(std::error_code{errno, std::system_category()}));

    return data;
}

auto common::write_file(const Path &a_path, std::span<const std::byte> data) noexcept
    -> tl::expected<void, common::Error>
{
    std::ofstream out{a_path, std::ios_base::binary};
    if (!out)
        return tl::make_unexpected(Error(std::error_code{errno, std::system_category()}));
    out.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out)
        return tl::make_unexpected(Error(std::error_code{errno, std::system_category()}));

    return {};
}

auto common::compare_files(const Path &filename1, const Path &filename2) noexcept -> bool
{
    try
    {
        std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); // open file at the end
        std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); // open file at the end

        if (file1.tellg() != file2.tellg())
        {
            return false; //different file size
        }

        file1.seekg(0); //rewind
        file2.seekg(0); //rewind

        const std::istreambuf_iterator begin1(file1);
        const std::istreambuf_iterator begin2(file2);

        return std::equal(begin1,
                          std::istreambuf_iterator<char>(),
                          begin2); //Second argument is end-of-range iterator}
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto common::compare_directories(const Path &dir1, const Path &dir2) noexcept -> bool
{
    try
    {
        // sort before comparing, as the directory iteration order is not guaranteed
        auto files1 = std::vector(fs::recursive_directory_iterator(dir1), fs::recursive_directory_iterator{});
        auto files2 = std::vector(fs::recursive_directory_iterator(dir2), fs::recursive_directory_iterator{});

        if (files1.size() != files2.size())
            return false;

        std::ranges::sort(files1);
        std::ranges::sort(files2);

        auto beg1 = files1.begin();
        auto beg2 = files2.begin();

        while (beg1 != files1.end()) // no need to check beg2, as we already checked the size
        {
            auto path1 = beg1->path();
            auto path2 = beg2->path();

            if (path1.lexically_relative(dir1) != path2.lexically_relative(dir2))
                return false;

            if (beg1->is_directory() != beg2->is_directory())
                return false;

            // Skip directories, we only care about files
            if (beg1->is_directory())
            {
                ++beg1;
                ++beg2;
                continue;
            }

            if (!compare_files(path1, path2))
                return false;

            ++beg1;
            ++beg2;
        }
        return beg1 == files1.end() && beg2 == files2.end();
    }
    catch (const std::exception &)
    {
        return false;
    }
}

auto common::hard_link(const Path &from, const Path &to) noexcept -> tl::expected<void, common::Error>
{
    try
    {
        // simple case
        if (!is_directory(from))
        {
            auto ec = std::error_code{};
            create_hard_link(from, to, ec);

            if (ec)
            {
                // we have to make a copy, unfortunately
                fs::copy(from, to, ec);
                if (ec)
                    return tl::make_unexpected(Error(ec));
            }
            return {};
        }

        // we cannot hard link directories on Windows, so we create a directory and hardlink files inside
        create_directories(to);
        for (const auto &e : fs::recursive_directory_iterator(from))
        {
            if (const auto success = hard_link(e.path(), to / relative(e.path(), from)); !success)
                return success;
        }

        return {};
    }
    catch (const std::exception &)
    {
        return tl::make_unexpected(Error(std::error_code{errno, std::system_category()}));
    }
}

auto find_matching_paths_icase(const btu::Path &directory, std::span<const btu::Path> paths) noexcept
    -> std::vector<btu::Path>
{
#ifdef _WIN32
    // Windows is case-insensitive by default. Only check if the file exists
    return flux::from(paths)
        .map([&directory](const btu::Path &path) { return directory / path; })
        .filter([](const auto &path) { return btu::fs::exists(path); })
        .to<std::vector<btu::Path>>();
#else
    // On Linux, we need to make a case-insensitive map of the files in the directory. This is slow.
    // We also assume there is only one file with the same lowercase path. This is probably fine since
    // mods were made for Windows and Windows is case-insensitive.
    const auto files_in_directory
        = flux::from_range(btu::fs::recursive_directory_iterator(directory))
              .map([&directory](const btu::fs::directory_entry &entry) {
                  const auto &path         = entry.path();
                  const auto relative_path = btu::fs::relative(path, directory);
                  return std::pair{btu::common::to_lower(relative_path.u8string()), path};
              })
              .to<std::unordered_map<std::u8string, btu::Path>>();

    return flux::from(paths)
        .map([&files_in_directory](const btu::Path &path) -> std::optional<btu::Path> {
            const auto lower_path = btu::common::to_lower(path.u8string());
            if (const auto it = files_in_directory.find(lower_path); it != files_in_directory.end())
                return it->second;
            return {};
        })
        .filter([](const auto &path) { return path.has_value(); })
        .map([](const auto &path) { return path.value(); })
        .to<std::vector<btu::Path>>();
#endif
}

} // namespace btu
