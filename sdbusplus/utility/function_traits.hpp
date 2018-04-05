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
#include <tuple>
#include <functional>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{

// primary template.
template <class T>
struct function_traits : function_traits<decltype(&T::operator())>
{
};

// partial specialization for function type
template <class R, class... Args> struct function_traits<R(Args...)>
{
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for function pointer
template <class R, class... Args> struct function_traits<R (*)(Args...)>
{
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for std::function
template <class R, class... Args>
struct function_traits<std::function<R(Args...)>>
{
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for pointer-to-member-function (i.e., operator()'s)
template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...)>
{
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...) const>
{
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

template <class F, size_t... Is>
constexpr auto index_apply_impl(F f, std::index_sequence<Is...>)
{
    return f(std::integral_constant<size_t, Is>{}...);
}

template <size_t N, class F> constexpr auto index_apply(F f)
{
    return index_apply_impl(f, std::make_index_sequence<N>{});
}

template <class Tuple, class F> constexpr auto apply(F f, Tuple& t)
{
    return index_apply<std::tuple_size<Tuple>{}>(
        [&](auto... Is) { return f(std::get<Is>(t)...); });
}

template <class Tuple>
constexpr bool unpack_into_tuple(Tuple& t, message::message& m)
{
    return index_apply<std::tuple_size<Tuple>{}>([&](auto... Is) {
        if (m.is_method_error())
        {
            return false;
        }
        m.read(std::get<Is>(t)...);
        return true;
    });
}

// Specialization for empty tuples.  No need to unpack if no arguments
constexpr bool unpack_into_tuple(std::tuple<>& t, message::message& m)
{
    return true;
}
} // namespace sdbusplus