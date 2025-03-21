/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <iostream>
#include <source_location>
#include <system_error>

namespace btu::common {
inline auto operator<<(std::ostream &os, std::source_location loc) -> std::ostream &
{
    return os << "file: " << loc.file_name() << "(" << loc.line() << ":" << loc.column() << ") `"
              << loc.function_name() << "`";
}

struct Error : std::error_code
{
    explicit Error(std::error_code ec, std::source_location l = std::source_location::current())
        : std::error_code(ec)
        , loc(l)
    {
    }

    std::source_location loc;
};

inline auto operator<<(std::ostream &os, const Error &err) -> std::ostream &
{
    return os << err.value() << ":" << err.category().name() << ":" << err.message() << "; " << err.loc;
}

class Exception : public std::exception
{
    Error err_;

public:
    explicit Exception(Error err)
        : err_(std::move(err))
    {
    }

    [[nodiscard]] auto what() const noexcept -> const char * override { return "btu::common::Exception"; }

    [[nodiscard]] auto error() const noexcept -> const Error & { return err_; }
};
} // namespace btu::common
