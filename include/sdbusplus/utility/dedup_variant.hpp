#pragma once
#include <type_traits>
#include <variant>

/** See `sdbusplus::utility::dedup_variant` below. */

namespace sdbusplus
{
namespace utility
{

namespace details
{

/** Find the deduplicated variant type.
 *
 *  @tparam T - A type of the form 'variant<...>'.
 *  @tparam Unused - An empty type set (non-empty sets are matched in
 *                   specialization).
 *
 *  This template is only matched when Unused is empty, which means all
 *  types have been processed (deduplicated).
 */
template <typename T, typename... Unused>
struct dedup_variant
{
    using type = T;
};

/** Find the deduplicated variant type.
 *
 * @tparam Done - The types which have already been deduplicated.
 * @tparam First - The first type to be deduplicated.
 * @tparam Rest - The remaining types to be deduplicated.
 *
 * This template specialization is matched when there are remaining
 * items to be analyzed, since the 'First' is a stronger match than the
 * (empty) 'Types' in the previous template.
 *
 * Check for First in Done and recursively create the variant type as
 * appropriate (add First if First not in Done, skip otherwise).
 */
template <typename... Done, typename First, typename... Rest>
struct dedup_variant<std::variant<Done...>, First, Rest...> :
    public std::conditional_t<
        (std::is_same_v<First, Done> || ...),
        dedup_variant<std::variant<Done...>, Rest...>,
        dedup_variant<std::variant<Done..., First>, Rest...>>
{};

} // namespace details

/** This type is useful for generated code which may inadvertantly contain
 *  duplicate types if specified simply as `std::variant<A, B, C>`.  Some
 *  types, such as `uint32_t` and `size_t` are the same on some architectures
 *  and different on others.  `dedup_variant_t<uint32_t, size_t>` will evalute
 *  to `std::variant<uint32_t>` on architectures where there is a collision.
 */
template <typename T, typename... Types>
using dedup_variant =
    typename details::dedup_variant<std::variant<T>, Types...>::type;

} // namespace utility
} // namespace sdbusplus
