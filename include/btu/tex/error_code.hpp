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
    const char *name() const noexcept override;
    std::string message(int ev) const override;
};

inline const TextureErrCategory k_texture_err_category{};
std::error_code make_error_code(TextureErr e);

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
    const char *name() const noexcept override;
    std::string message(int ev) const override;
    bool equivalent(const std::error_code &code, int condition) const noexcept override;
};

inline const FailureSourceCategory theFailureSourceCategory{};
std::error_condition make_error_condition(FailureSource e);

auto error_from_hresult(int64_t hr, std::error_code default_err = TextureErr::Unknown) -> std::error_code;

} // namespace btu::tex
