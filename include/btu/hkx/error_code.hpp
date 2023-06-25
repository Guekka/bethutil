#pragma once

#include <btu/common/error.hpp>

#include <optional>

namespace btu::hkx {
using btu::common::Error, btu::common::SourceLocation;

enum class AnimErr
{
    Success = 0,
    Unknown = 1,
    NoAppropriateExe,
    ExeFailed,
    NoExeFound,
};
} // namespace btu::hkx

namespace std {
template<>
struct is_error_code_enum<btu::hkx::AnimErr> : true_type
{
};
} // namespace std

namespace btu::hkx {
struct AnimErrCategory : std::error_category
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
        }
        return "(unrecognized error)";
    };
};

inline const AnimErrCategory k_anim_err_category{};
auto make_error_code(AnimErr e) -> std::error_code
{
    return {static_cast<int>(e), k_anim_err_category};
}

enum class FailureSource
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::hkx

namespace std {
template<>
struct is_error_condition_enum<btu::hkx::FailureSource> : true_type
{
};
} // namespace std

namespace btu::hkx {
class FailureSourceCategory : public std::error_category
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
auto make_error_condition(FailureSource e) -> std::error_condition
{
    return {static_cast<int>(e), k_failure_source_category};
}
} // namespace btu::hkx
