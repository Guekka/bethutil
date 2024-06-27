#pragma once

#include <btu/common/string.hpp>
#include <nlohmann/json.hpp>

#include <optional>
#include <variant>

// variant from https://github.com/nlohmann/json/issues/1261#issuecomment-426209912

namespace detail {
template<std::size_t N>
struct variant_switch
{
    template<typename Variant>
    void operator()(int index, nlohmann::json const &value, Variant &v) const
    {
        if (index == N)
            v = value.get<std::variant_alternative_t<N, Variant>>();
        else
            variant_switch<N - 1>{}(index, value, v);
    }
};

template<>
struct variant_switch<0>
{
    template<typename Variant>
    void operator()(int index, nlohmann::json const &value, Variant &v) const
    {
        if (index == 0)
            v = value.get<std::variant_alternative_t<0, Variant>>();
        else
        {
            throw std::runtime_error("while converting json to variant: invalid index");
        }
    }
};
} // namespace detail

// yes, we are polluting std. But it's for the greater good (hopefully)
namespace std {
inline void to_json(nlohmann::json &j, const u8string &str)
{
    j = btu::common::as_ascii(str);
}

inline void from_json(const nlohmann::json &j, u8string &str)
{
    str = btu::common::as_utf8(j.get<std::string>());
}

inline void to_json(nlohmann::json &j, const u8string_view &str)
{
    j = btu::common::as_ascii(str);
}

inline void from_json(const nlohmann::json &j, u8string_view &str)
{
    str = btu::common::as_utf8(j.get<std::string>());
}

} // namespace std

namespace nlohmann {

template<typename T>
struct adl_serializer<std::optional<T>>
{
    static void to_json(json &j, const std::optional<T> &opt)
    {
        if (opt == std::nullopt)
            j = nullptr;
        else
            j = *opt;
    }

    static void from_json(const json &j, std::optional<T> &opt)
    {
        if (j.is_null())
            opt = std::nullopt;
        else
            opt = j.get<T>();
    }
};

template<typename... Args>
struct adl_serializer<std::variant<Args...>>
{
    static void to_json(json &j, std::variant<Args...> const &v)
    {
        std::visit(
            [&](auto &&value) {
                j["index"] = v.index();
                j["value"] = std::forward<decltype(value)>(value);
            },
            v);
    }

    static void from_json(json const &j, std::variant<Args...> &v)
    {
        auto const index = j.at("index").get<int>();
        ::detail::variant_switch<sizeof...(Args) - 1>{}(index, j.at("value"), v);
    }
};

template<>
struct adl_serializer<std::monostate>
{
    static void to_json(json &j, std::monostate const &) { j = nullptr; }

    static void from_json(json const &j, std::monostate &)
    {
        if (!j.is_null())
            throw std::runtime_error("while converting json to variant: invalid index");
    }
};

} // namespace nlohmann
