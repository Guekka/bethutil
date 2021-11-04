#pragma once

#include <optional>
#include <source_location>
#include <system_error>

namespace btu::tex {

enum class TextureErr
{
    Success = 0,
    Unknown = 1,
    BadInput,
    MemoryAllocation,

};
} // namespace btu::tex

namespace std {
template<>
struct is_error_code_enum<btu::tex::TextureErr> : true_type
{
};
} // namespace std

namespace btu::tex {
struct TextureErrCategory : std::error_category
{
    [[nodiscard]] auto name() const noexcept -> const char * override;
    [[nodiscard]] auto message(int ev) const -> std::string override;
};

inline const TextureErrCategory k_texture_err_category{};
auto make_error_code(TextureErr e) -> std::error_code;

enum class FailureSource
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::tex

namespace std {
template<>
struct is_error_condition_enum<btu::tex::FailureSource> : true_type
{
};
} // namespace std

namespace btu::tex {
class FailureSourceCategory : public std::error_category
{
public:
    [[nodiscard]] auto name() const noexcept -> const char * override;
    [[nodiscard]] auto message(int ev) const -> std::string override;
    [[nodiscard]] auto equivalent(const std::error_code &ec, int cond) const noexcept -> bool override;
};

inline const FailureSourceCategory k_failure_source_category{};
auto make_error_condition(FailureSource e) -> std::error_condition;

struct SourceLocation
#ifndef __clang__ // Missing support for source location in clang
    : public std::source_location
#endif
{
};
#ifndef __clang__
#define BETHUTIL_CURRENT_SOURCE_LOC static_cast<btu::tex::SourceLocation>(std::source_location::current())
#else
#define BETHUTIL_CURRENT_SOURCE_LOC \
    btu::tex::SourceLocation {}
#endif
auto operator<<(std::ostream &os, SourceLocation loc) -> std::ostream &;

struct Error : public std::exception
{
    explicit Error(std::error_code ec, SourceLocation l = BETHUTIL_CURRENT_SOURCE_LOC);
    explicit Error(TextureErr ec, SourceLocation l = BETHUTIL_CURRENT_SOURCE_LOC);

    auto what() const -> const char * override;

    SourceLocation loc;
    std::error_code ec;
};

auto error_from_hresult(int64_t hr,
                        std::error_code default_err = TextureErr::Unknown,
                        SourceLocation loc          = BETHUTIL_CURRENT_SOURCE_LOC) -> Error;

} // namespace btu::tex
