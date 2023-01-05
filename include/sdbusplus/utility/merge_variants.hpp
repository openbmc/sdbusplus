#pragma once
#include <sdbusplus/utility/dedup_variant.hpp>

#include <variant>

/** See `sdbusplus::utility::merge_variants` below. */

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
 *  types have been processed (merged).
 */
template <typename T, typename... Unused>
struct merge_variants
{
    using type = T;
    static_assert(sizeof...(Unused) == 0);
};

/** Compute the merged variant type.
 *
 * @tparam D - Done - Head and tail of the alternative types list of the
 * first variant.
 * @tparam Next - The types of the next variant<...> to be merged with the
 * first.
 * @tparam Rest - The remaining variants to be merged.
 *
 * Use dedup_variant_t to recursively combine alternative types of all the
 * supplied variants without duplication.
 */
template <typename D, typename... Done, typename... Next, typename... Rest>
struct merge_variants<std::variant<D, Done...>, std::variant<Next...>,
                      Rest...> :
    public merge_variants<
        sdbusplus::utility::dedup_variant_t<D, Done..., Next...>, Rest...>
{};

} // namespace details

/** This type is useful for processing D-Bus messages containing information for
 * multiple interfaces. For doing sdbusplus::message::message::read() on them
 * one needs to define an std::variant which would be a combination of all the
 * PropertiesVariant's involved. */
template <typename... T>
using merge_variants_t = typename details::merge_variants<T...>::type;

} // namespace utility
} // namespace sdbusplus
