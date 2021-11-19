/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once
#include <sdbusplus/message.hpp>

#include <tuple>

namespace sdbusplus
{
namespace utility
{

namespace detail
{
template <class F, size_t... Is>
constexpr auto index_apply_impl(F f, std::index_sequence<Is...>)
{
    return f(std::integral_constant<size_t, Is>{}...);
}
template <size_t N, class F>
constexpr auto index_apply(F f)
{
    return index_apply_impl(f, std::make_index_sequence<N>{});
}
} // namespace detail
template <class Tuple>
constexpr bool read_into_tuple(Tuple& t, message_t& m)
{
    return detail::index_apply<std::tuple_size<Tuple>{}>([&](auto... Is) {
        if (m.is_method_error())
        {
            return false;
        }
        m.read(std::get<Is>(t)...);
        return true;
    });
}
} // namespace utility
} // namespace sdbusplus
