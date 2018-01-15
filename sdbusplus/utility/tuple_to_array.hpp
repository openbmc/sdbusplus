#pragma once

#include <array>
#include <tuple>
#include <utility>

namespace sdbusplus
{

namespace utility
{

namespace details
{

/** tuple_to_array - Create std::array from std::tuple.
 *
 *  @tparam V - Type of the first tuple element.
 *  @tparam Types - Sequence of types for remaining elements, which must
 *                  be automatically castable to V.
 *  @tparam I - Sequence of integral indexes (0...N) for each tuple elemeent.
 *
 *  @param tuple - Tuple of N same-typed elements.
 *  @param [unnamed] - std::integer_sequence of tuple's index values.
 *
 *  @return A std::array where each I-th element is tuple's I-th element.
 */
template <typename V, typename... Types, std::size_t... I>
constexpr auto tuple_to_array(std::tuple<V, Types...> &&tuple,
                              std::integer_sequence<std::size_t, I...>)
{
    return std::array<V, sizeof...(I)>({
        std::get<I>(tuple)...,
    });
}

} // namespace details

/** tuple_to_array - Create std::array from std::tuple.
 *
 *  @tparam Types - Sequence of types of the tuple elements.
 *  @tparam I - std::integer_sequence of indexes (0..N) for each tuple element.
 *
 *  @param tuple - Tuple of N same-typed elements.
 *
 *  @return A std::array where each I-th element is tuple's I-th element.
 */
template <typename... Types,
          typename I = std::make_index_sequence<sizeof...(Types)>>
constexpr auto tuple_to_array(std::tuple<Types...> &&tuple)
{
    return details::tuple_to_array(std::move(tuple), I());
}

} // namespace utility

} // namespace sdbusplus
