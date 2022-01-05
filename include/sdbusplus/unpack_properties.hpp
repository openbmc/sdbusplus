#pragma once

#include <sdbusplus/exception.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <algorithm>
#include <bitset>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace sdbusplus
{
namespace details
{

template <typename Container>
inline auto findProperty(const Container& container,
                         const std::string& key) noexcept
{
    if constexpr (utility::has_member_find_v<Container>)
    {
        return container.find(key);
    }
    else
    {
        return std::find_if(
            std::begin(container), std::end(container),
            [&key](const auto& keyValue) { return keyValue.first == key; });
    }
}

template <typename Container>
inline bool containsProperty(const Container& container,
                             const std::string& key) noexcept
{
    if constexpr (utility::has_member_contains_v<Container>)
    {
        return container.contains(key);
    }
    else
    {
        return findProperty(container, key) != std::end(container);
    }
}

template <typename Container, typename ValueType>
inline std::optional<std::string> readProperty(const Container& container,
                                               const std::string& expectedKey,
                                               ValueType& outValue) noexcept
{
    auto it = findProperty(container, expectedKey);

    if (it != std::end(container))
    {
        if constexpr (utility::is_optional_v<ValueType>)
        {
            using InnerType = typename ValueType::value_type;
            if (const auto value = std::get_if<InnerType>(&it->second))

            {
                outValue = *value;
            }
            else
            {
                return expectedKey;
            }
        }
        else
        {
            if (const auto value = std::get_if<ValueType>(&it->second))

            {
                outValue = *value;
            }
            else
            {
                return expectedKey;
            }
        }
    }
    else if constexpr (!utility::is_optional_v<ValueType>)
    {
        return expectedKey;
    }

    return {};
}

template <typename Container, typename ValueType, typename... Args>
inline std::optional<std::string>
    readProperties(const Container& container, const std::string& expectedKey,
                   ValueType& outValue, Args&&... args) noexcept
{
    if (auto badProperty = readProperty(container, expectedKey, outValue))

    {
        return badProperty;
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return readProperties(container, std::forward<Args>(args)...);
    }

    return {};
}

template <bool ReturnBadProperty, typename Container, typename... Args>
inline auto unpackPropertiesCommon(const Container& input,
                                   Args&&... args) noexcept(ReturnBadProperty)
{
    static_assert(sizeof...(Args) % 2 == 0);

    std::optional<std::string> badProperty =
        details::readProperties(input, std::forward<Args>(args)...);

    if (badProperty)
    {
        if constexpr (ReturnBadProperty)
        {
            return badProperty;
        }
        else
        {
            if (details::containsProperty(input, *badProperty))
            {
                throw exception::UnpackPropertyError(
                    *badProperty,
                    exception::UnpackPropertyError::reasonTypeNotMatched);
            }
            else
            {
                throw exception::UnpackPropertyError(
                    *badProperty,
                    exception::UnpackPropertyError::reasonMissingProperty);
            }
        }
    }

    return std::conditional_t<ReturnBadProperty, std::optional<std::string>,
                              void>();
}

} // namespace details

template <typename Container, typename... Args>
inline void unpackProperties(const Container& input, Args&&... args)
{
    details::unpackPropertiesCommon<false, Container, Args...>(
        input, std::forward<Args>(args)...);
}

template <typename Container, typename... Args>
inline std::optional<std::string>
    unpackPropertiesNoThrow(const Container& input, Args&&... args) noexcept
{
    return details::unpackPropertiesCommon<true, Container, Args...>(
        input, std::forward<Args>(args)...);
}

} // namespace sdbusplus
