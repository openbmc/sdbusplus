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
} // namespace details

/*
 * Given a tuple of function args to a method callback, return a tuple
 * with references to only the dbus message arguments.
 * */
template <typename Tuple>
auto make_dbus_args_tuple(Tuple& t)
{
    constexpr size_t skipArgs = []() {
        if constexpr ((std::tuple_size_v<Tuple>) > 1)
        {
            if (std::is_same_v<std::tuple_element_t<1, Tuple>,
                               sdbusplus::message_t>)
            {
                return 2;
            }
        }
        return 1;
    }();
    constexpr std::size_t original_size = std::tuple_size<Tuple>::value;
    constexpr std::size_t new_size = original_size - skipArgs;
    return details::make_sub_tuple_impl<skipArgs>(
        t, std::make_index_sequence<new_size>{});
}
} // namespace utility
} // namespace sdbusplus
