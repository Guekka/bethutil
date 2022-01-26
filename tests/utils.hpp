#include "btu/common/path.hpp"

#include <catch2/catch.hpp>

#include <fstream>

using btu::common::Path;

using namespace std::literals;

inline bool compare_files(const Path &filename1, const Path &filename2)
{
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end

    if (file1.tellg() != file2.tellg())
    {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1,
                      std::istreambuf_iterator<char>(),
                      begin2); //Second argument is end-of-range iterator
}
