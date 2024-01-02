#include <btu/common/json.hpp>
#include <btu/common/string.hpp>
#include <catch.hpp>

TEST_CASE("std::u8string is converted to json as a string", "[src]")
{
    SECTION("To json")
    {
        const auto str  = std::u8string(u8"👍 👍 👍");
        const auto json = nlohmann::json(str);
        REQUIRE(json.is_string());
        REQUIRE(json.get<std::string>() == btu::common::as_ascii(str));
    }

    SECTION("From json")
    {
        const auto json = R"("👍 👍 👍")"_json;
        REQUIRE(json.is_string());
        REQUIRE(json.get<std::u8string>() == u8"👍 👍 👍");
    }
}
