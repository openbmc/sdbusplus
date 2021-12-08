#pragma once

#include <sdbusplus/utility/dedup_variant.hpp>
#include <sdbusplus/utility/holds_type.hpp>

#include <type_traits>
#include <variant>

namespace sdbusplus::utility
{

namespace details
{

using CommonVariant =
    utility::dedup_variant_t<std::monostate, std::string, double, bool, uint8_t,
                             uint16_t, uint32_t, uint64_t, int16_t, int32_t,
                             int64_t>;

} // namespace details

template <class T>
using CommonVariant =
    std::conditional_t<holds_type_v<T, details::CommonVariant>,
                       details::CommonVariant, std::variant<std::monostate, T>>;

} // namespace sdbusplus::utility
