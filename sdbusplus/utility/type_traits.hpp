#pragma once

#include <type_traits>
#include <tuple>

namespace sdbusplus
{

namespace utility
{

/** @brief Convert T[N] to T* if is_same<Tbase,T>
 *
 *  @tparam Tbase - The base type expected.
 *  @tparam T - The type to convert.
 */
template <typename Tbase, typename T>
using array_to_ptr_t = typename std::conditional_t<
    std::is_array<T>::value,
    std::conditional_t<std::is_same<Tbase, std::remove_extent_t<T>>::value,
                       std::add_pointer_t<std::remove_extent_t<T>>, T>,
    T>;

// Small helper class for stripping off the error code from the function
// argument definitions so unpack can be called appropriately
template <typename T> struct strip_first_arg
{
};

template <typename FirstArg, typename... Rest>
struct strip_first_arg<std::tuple<FirstArg, Rest...>>
{
    using type = std::tuple<Rest...>;
};

} // namespace utility

} // namespace sdbusplus
