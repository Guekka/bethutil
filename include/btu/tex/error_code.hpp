#pragma once

#include <optional>
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

auto error_from_hresult(int64_t hr, std::error_code default_err = TextureErr::Unknown) -> std::error_code;

} // namespace btu::tex
