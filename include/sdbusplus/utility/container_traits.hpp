#pragma once

#include <iterator>
#include <utility>

namespace sdbusplus
{
namespace utility
{
template <typename T>
concept is_dbus_array = requires(T v) {
                            { v.begin() } -> std::forward_iterator;
                            !std::is_same_v<std::decay<T>, std::string>;
                        };

template <typename T, typename... U>
concept is_any_of = (std::same_as<T, U> || ...);

template <typename T>
concept can_append_array_value =
    requires(T v) {
        // Require that the container be contiguous
        { v.begin() } -> std::contiguous_iterator;
        // The sd-bus append array docs are very specific about what types are
        // allowed; verify this is one of those types.
        requires(is_any_of<typename T::value_type, uint8_t, int16_t, uint16_t,
                           int32_t, uint32_t, int64_t, uint64_t, double>);
    };

template <typename T>
concept has_emplace_back = requires(T v) { v.emplace_back(); };

template <typename T>
concept has_emplace = requires(T v) { v.emplace(); };

template <typename T>
concept is_dbus_enum = std::is_enum_v<T>;
} // namespace utility
} // namespace sdbusplus
