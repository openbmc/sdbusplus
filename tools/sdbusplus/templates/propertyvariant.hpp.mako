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

using PropertyVariant = sdbusplus::utility::dedup_variant_t<
    ${",\n    ".join(variant.types)}>;

} // namespace sdbusplus
