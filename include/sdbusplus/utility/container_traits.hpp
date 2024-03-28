#pragma once

#include <iterator>
#include <utility>

namespace sdbusplus
{

template <typename T>
concept IsDbusArray = requires(T v) {
                          {
                              v.begin()
                          } -> std::forward_iterator;
                          !std::is_same_v<std::decay<T>, std::string>;
                      };

template <typename T, typename... U>
concept IsAnyOf = (std::same_as<T, U> || ...);

template <typename T>
concept CanAppendArrayValue =
    requires(T v) {
        // Require that the container be contiguous
        {
            v.begin()
        } -> std::contiguous_iterator;
        // The sd-bus append array docs are very clear about what types are
        // allowed; verify this is one of those types.
        requires(IsAnyOf<typename T::value_type, uint8_t, int16_t, uint16_t,
                         int32_t, uint32_t, int64_t, uint64_t, double>);
    };

template <typename T>
concept HasEmplaceBack = requires(T v) { v.emplace_back(); };

template <typename T>
concept HasEmplace = requires(T v) { v.emplace(); };

} // namespace sdbusplus
