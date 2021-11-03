#include "btu/tex/error_code.hpp"

#include <iostream>
#include <optional>

namespace btu::tex {
auto TextureErrCategory::name() const noexcept -> const char *
{
    return "btu::tex error";
}

auto btu::tex::TextureErrCategory::message(int ev) const -> std::string
{
    switch (static_cast<TextureErr>(ev))
    {
        case btu::tex::TextureErr::Success: return "no error";
        case btu::tex::TextureErr::Unknown: return "default error";
        default: return "(unrecognized error)";
    }
}

auto make_error_code(TextureErr e) -> std::error_code
{
    return {static_cast<int>(e), k_texture_err_category};
}

auto btu::tex::FailureSourceCategory::name() const noexcept -> const char *
{
    return "btu::tex failure-source";
}

auto btu::tex::FailureSourceCategory::message(int ev) const -> std::string
{
    switch (static_cast<FailureSource>(ev))
    {
        case FailureSource::BadUserInput: return "invalid user request";
        case FailureSource::SystemError: return "internal error";
        default: return "(unrecognized condition)";
    }
}

auto btu::tex::FailureSourceCategory::equivalent(const std::error_code &ec, int cond) const noexcept -> bool
{
    switch (static_cast<FailureSource>(cond))
    {
        case FailureSource::BadUserInput: return ec == TextureErr::BadInput;
        case FailureSource::SystemError:
            return ec == TextureErr::Unknown || ec == TextureErr::MemoryAllocation;
        default: return false;
    }
}

auto make_error_condition(FailureSource e) -> std::error_condition
{
    return {static_cast<int>(e), k_failure_source_category};
}

auto error_from_hresult(int64_t hr, std::error_code default_err, SourceLocation loc) -> Error
{
    // MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)
    constexpr auto win32_valid = 0x80070000L;
    constexpr auto mask        = 0xFFFF0000U;
    if ((static_cast<uint64_t>(hr) & mask) == win32_valid)
    {
        // Could have come from many values, but we choose this one
        const auto converted_code = static_cast<int32_t>(static_cast<uint64_t>(hr) & 0xFFFFU);
        const auto ec = std::system_error(converted_code, std::system_category(), "DirectXTex error").code();
        return Error(ec, loc);
    }
    if (hr == 0) // S_OK
    {
        const auto ec = std::system_error(0, std::system_category(), "DirectXTex success").code();
        return Error(ec, loc);
    }
    // otherwise, we got an impossible value
    return Error(default_err, loc);
}

auto operator<<(std::ostream &os, [[maybe_unused]] SourceLocation loc) -> std::ostream &
{
#ifdef __clang__
    return os << std::string_view("source location unsupported with clang");
#else
    return os << "file: " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") `"
              << loc.function_name() << "`";
#endif
}

Error::Error(std::error_code ec, SourceLocation l)
    : loc(l)
    , ec(ec)
{
}

Error::Error(TextureErr ec, SourceLocation l)
    : Error(make_error_code(ec), l)
{
}

} // namespace btu::tex
