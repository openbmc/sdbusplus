#pragma once

#include <sdbusplus/message.hpp>

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>

namespace sdbusplus
{
namespace utility
{
namespace details
{
template <std::size_t FirstArgIndex, typename Tuple, std::size_t... Is>
auto make_sub_tuple_impl(Tuple& t, std::index_sequence<Is...>)
{
    return std::tie(std::get<FirstArgIndex + Is>(t)...);
}

template <typename TupleType>
constexpr size_t args_to_skip()
{
    constexpr std::size_t input_size = std::tuple_size_v<TupleType>;
    if constexpr (input_size > 1)
    {
        if (std::is_same_v<std::tuple_element_t<1, TupleType>,
                           sdbusplus::message_t>)
        {
            return 2;
        }
    }
    return 1;
}

} // namespace details

/*
 * Given a tuple of function args to a method callback, return a tuple
 * with references to only the dbus message arguments.
 * */
template <typename... Args>
constexpr auto make_dbus_args_tuple(std::tuple<Args...>& t)
{
    using tuple_type = std::remove_reference_t<decltype(t)>;
    constexpr std::size_t input_size = std::tuple_size_v<tuple_type>;
    constexpr size_t skip_args = details::args_to_skip<tuple_type>();
    constexpr std::size_t new_size = input_size - skip_args;
    std::index_sequence seq = std::make_index_sequence<new_size>{};
    return details::make_sub_tuple_impl<skip_args>(t, seq);
}
} // namespace utility
} // namespace sdbusplus
