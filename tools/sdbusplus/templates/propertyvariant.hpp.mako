#pragma once

// Generated from all non-recursive property types in the YAML collection.
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>

namespace sdbusplus
{

% if variant.aliases:
namespace details
{

// Resolve aliases on the target before merging matching wire shapes.
template <${", ".join("typename" for a in variant.aliases)}>
struct property_variant;

% for arguments, types in variant.specializations:
template <>
struct property_variant<${", ".join(arguments)}>
{
    using type = sdbusplus::utility::dedup_variant_t<
        ${",\n        ".join(types)}>;
};

% endfor
} // namespace details

using PropertyVariant = details::property_variant<${", ".join(variant.alias_cpp_types)}>::type;
% else:
using PropertyVariant = sdbusplus::utility::dedup_variant_t<
    ${",\n    ".join(variant.types)}>;
% endif

} // namespace sdbusplus
