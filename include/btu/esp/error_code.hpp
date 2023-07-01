#pragma once

#include <btu/common/error.hpp>

#include <optional>

namespace btu::esp {
using btu::common::Error;

enum class EspErr
{
    Success = 0,
    Unknown = 1,
    FailedToReadFile
};
} // namespace btu::esp

namespace std {
template<>
struct is_error_code_enum<btu::esp::EspErr> : true_type
{
};
} // namespace std

namespace btu::esp {
struct EspErrCategory : std::error_category
{
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::esp error"; }
    [[nodiscard]] auto message(int ev) const -> std::string override
    {
        switch (static_cast<EspErr>(ev))
        {
            case EspErr::Success: return "no error";
            case EspErr::Unknown: return "default error";
            case EspErr::FailedToReadFile: return "failed to read file";
        }
        return "(unrecognized error)";
    };
};

inline const EspErrCategory k_Esp_err_category{};
inline auto make_error_code(EspErr e) -> std::error_code
{
    return {static_cast<int>(e), k_Esp_err_category};
}

enum class FailureSource
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::esp

namespace std {
template<>
struct is_error_condition_enum<btu::esp::FailureSource> : true_type
{
};
} // namespace std

namespace btu::esp {
class FailureSourceCategory : public std::error_category
{
public:
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::esp failure-source"; }
    [[nodiscard]] auto message(int ev) const -> std::string override
    {
        switch (static_cast<FailureSource>(ev))
        {
            case FailureSource::BadUserInput: return "invalid user request";
            case FailureSource::SystemError: return "internal error";
            default: return "(unrecognized condition)";
        }
    }
    [[nodiscard]] auto equivalent(const std::error_code &ec, int cond) const noexcept -> bool override
    {
        switch (static_cast<FailureSource>(cond))
        {
            case FailureSource::SystemError: return ec == EspErr::Unknown;
            default: return false;
        }
    }
};

inline const FailureSourceCategory k_failure_source_category{};
inline auto make_error_condition(FailureSource e) -> std::error_condition
{
    return {static_cast<int>(e), k_failure_source_category};
}
} // namespace btu::esp
