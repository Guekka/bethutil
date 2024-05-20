#pragma once

#include <btu/common/error.hpp>

#include <source_location>

namespace btu::tex {
using common::Error;

enum class TextureErr : std::uint8_t
{
    Success = 0,
    Unknown = 1,
    BadInput,
    MemoryAllocation,

};
} // namespace btu::tex

template<>
struct std::is_error_code_enum<btu::tex::TextureErr> : true_type
{
}; // namespace std

namespace btu::tex {
struct TextureErrCategory final : std::error_category
{
    [[nodiscard]] auto name() const noexcept -> const char * override;
    [[nodiscard]] auto message(int ev) const -> std::string override;
};

inline const TextureErrCategory k_texture_err_category{};
auto make_error_code(TextureErr e) -> std::error_code;

enum class FailureSource : std::uint8_t
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::tex

template<>
struct std::is_error_condition_enum<btu::tex::FailureSource> : true_type
{
}; // namespace std

namespace btu::tex {
class FailureSourceCategory final : public std::error_category
{
public:
    [[nodiscard]] auto name() const noexcept -> const char * override;
    [[nodiscard]] auto message(int ev) const -> std::string override;
    [[nodiscard]] auto equivalent(const std::error_code &ec, int cond) const noexcept -> bool override;
};

inline const FailureSourceCategory k_failure_source_category{};
auto make_error_condition(FailureSource e) -> std::error_condition;

auto error_from_hresult(int64_t hr,
                        std::error_code default_err = TextureErr::Unknown,
                        std::source_location loc    = std::source_location::current()) -> Error;

} // namespace btu::tex
