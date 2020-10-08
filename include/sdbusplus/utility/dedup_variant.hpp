#pragma once
#include <type_traits>
#include <variant>

/** See `sdbusplus::utility::dedup_variant_t` below. */

namespace sdbusplus
{
namespace utility
{

namespace details
{

/** Determine if a type T is in the set of types {First, ...Types}.
 *  @return true if there is a type T is in the set, false if not.
 *
 *  This is a constexpr function that evaluates by template-recursion.
 *  Type T is compared against First and then recursively compared against
 *  the remaining ...Types.
 *
 *  @tparam T - The type being analysed.
 *  @tparam First - The first type in a set of types.
 *  @tparam Types - The rest of the types in the set.
 */
template <typename T, typename First, typename... Types>
constexpr bool dup_type()
{
    if constexpr (std::is_same_v<T, First>)
    {
        return true;
    }
    else if constexpr (sizeof...(Types) == 0)
    {
        return false;
    }
    else
    {
        return dup_type<T, Types...>();
    }
}

/** Return a std::variant<T, ...Types> containing no duplicate types.
 *  @return a std::variant<...> containing no duplicate types.
 *
 *  This is a function which is never actually evaluated but used in
 *  `decltype` analysis to determine the appropriate type of a variant
 *  without any duplicate types.  It is implemented by template-recursion.
 *
 *  @tparam Done - The types having been analysed and non-duplicative from
 *                 previous recursion calls.
 *  @tparam T - The first type to be analysed.
 *  @tparma Types - The remaining types to still be analysed.
 */
template <typename... Done, typename T, typename... Types>
auto dedup_var(std::tuple<Done...>, T, Types...)
{
    if constexpr (sizeof...(Done) == 0)
    {
        if constexpr (sizeof...(Types) == 0)
        {
            return std::variant<T>{};
        }
        else
        {
            return dedup_var(std::tuple<T>{}, Types{}...);
        }
    }
    else if constexpr (sizeof...(Types) == 0)
    {
        if constexpr (dup_type<T, Done...>())
        {
            return std::variant<Done...>{};
        }
        else
        {
            return std::variant<Done..., T>{};
        }
    }
    else
    {
        if constexpr (dup_type<T, Done...>())
        {
            return dedup_var(std::tuple<Done...>{}, Types{}...);
        }
        else
        {
            return dedup_var(std::tuple<Done..., T>{}, Types{}...);
        }
    }
}

} // namespace details

/** This type is useful for generated code which may inadvertantly contain
 *  duplicate types if specified simply as `std::variant<A, B, C>`.  Some
 *  types, such as `uint32_t` and `size_t` are the same on some architectures
 *  and different on others.  `dedup_variant_t<uint32_t, size_t>` will evalute
 *  to `std::variant<uint32_t>` on architectures where there is a collision.
 */
template <typename... Types>
using dedup_variant_t =
    decltype(details::dedup_var(std::tuple<>{}, Types{}...));

} // namespace utility
} // namespace sdbusplus
