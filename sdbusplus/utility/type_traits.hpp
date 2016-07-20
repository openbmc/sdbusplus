#pragma once

#include <type_traits>

namespace sdbusplus
{

namespace utility
{

/** @struct array_to_ptr
 *
 *  @brief Convert T[N] to T* if is_same<Tbase,T>.
 *  @tparam Tbase - The base type expected.
 *  @tparam T - The type to convert.
 */
template <typename Tbase, typename T> struct array_to_ptr
{
    // T is not an array type, so reuse T.
    typedef T type;
};

template <typename Tbase, typename T, std::size_t N>
struct array_to_ptr<Tbase, T[N]>
{
    // T is an array type, so if is_same<Tbase,T> use T* instead of T[N].
    typedef std::conditional_t<std::is_same<Tbase, T>::value, T*, T[N]> type;
};

template <typename Tbase, typename T>
using array_to_ptr_t = typename array_to_ptr<Tbase, T>::type;

} // namespace utility

} // namespace sdbusplus
