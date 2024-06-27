#pragma once

#include <btu/common/error.hpp>

namespace btu::hkx {
using common::Error;

enum class AnimErr : std::uint8_t
{
    Success = 0,
    Unknown = 1,
    NoAppropriateExe,
    ExeFailed,
    NoExeFound,
    OsNotSupported,
};
} // namespace btu::hkx

template<>
struct std::is_error_code_enum<btu::hkx::AnimErr> : true_type
{
}; // namespace std

namespace btu::hkx {
struct AnimErrCategory final : std::error_category
{
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::hkx error"; }
    [[nodiscard]] auto message(int ev) const -> std::string override
    {
        switch (static_cast<AnimErr>(ev))
        {
            case AnimErr::Success: return "no error";
            case AnimErr::Unknown: return "default error";
            case AnimErr::NoAppropriateExe: return "no appropriate exe found";
            case AnimErr::ExeFailed: return "exe failed";
            case AnimErr::NoExeFound: return "no exe found at all";
            case AnimErr::OsNotSupported: return "os not supported by exe";
        }
        return "(unrecognized error)";
    };
};

inline const AnimErrCategory k_anim_err_category{};
inline auto make_error_code(AnimErr e) -> std::error_code
{
    return {static_cast<int>(e), k_anim_err_category};
}

enum class FailureSource : std::uint8_t
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::hkx

template<>
struct std::is_error_condition_enum<btu::hkx::FailureSource> : true_type
{
}; // namespace std

namespace btu::hkx {
class FailureSourceCategory final : public std::error_category
{
public:
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::hkx failure-source"; }
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
            case FailureSource::SystemError: return ec == AnimErr::Unknown;
            default: return false;
        }
    }
};

inline const FailureSourceCategory k_failure_source_category{};
inline auto make_error_condition(FailureSource e) -> std::error_condition
{
    return {static_cast<int>(e), k_failure_source_category};
}
} // namespace btu::hkx
