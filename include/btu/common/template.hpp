/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <type_traits>
#include <vector>

namespace btu::common {
template<typename T, typename U>
using is_equiv = std::is_same<std::remove_cvref<T>, std::remove_cvref<U>>;

template<typename T, typename U>
constexpr bool is_equiv_v = is_equiv<T, U>::value;

} // namespace btu::common
