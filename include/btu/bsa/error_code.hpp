#pragma once

#include <btu/common/error.hpp>

namespace btu::bsa {
using common::Error;

enum class BsaErr : std::uint8_t
{
    Success = 0,
    Unknown = 1,
    FailedToReadArchive,
    FailedToRemoveArchive,
    UnknownFormat,
    FailedToWriteFile,
    FailedToReadFile,
};
} // namespace btu::bsa

template<>
struct std::is_error_code_enum<btu::bsa::BsaErr> : true_type
{
}; // namespace std

namespace btu::bsa {
struct BsaErrCategory final : std::error_category
{
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::bsa error"; }
    [[nodiscard]] auto message(int ev) const -> std::string override
    {
        switch (static_cast<BsaErr>(ev))
        {
            case BsaErr::Success: return "no error";
            case BsaErr::Unknown: return "default error";
        }
        return "(unrecognized error)";
    };
};

inline const BsaErrCategory k_bsa_err_category{};
inline auto make_error_code(BsaErr e) -> std::error_code
{
    return {static_cast<int>(e), k_bsa_err_category};
}

enum class FailureSource : std::uint8_t
{
    BadUserInput = 1,
    SystemError  = 2,
};
} // namespace btu::bsa

template<>
struct std::is_error_condition_enum<btu::bsa::FailureSource> : true_type
{
}; // namespace std

namespace btu::bsa {
class FailureSourceCategory final : public std::error_category
{
public:
    [[nodiscard]] auto name() const noexcept -> const char * override { return "btu::bsa failure-source"; }
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
            case FailureSource::SystemError: return ec == BsaErr::Unknown;
            default: return false;
        }
    }
};

inline const FailureSourceCategory k_failure_source_category{};
inline auto make_error_condition(FailureSource e) -> std::error_condition
{
    return {static_cast<int>(e), k_failure_source_category};
}
} // namespace btu::bsa
