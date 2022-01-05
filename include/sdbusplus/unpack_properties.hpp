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

enum class UnpackErrorReason
{
    missingProperty,
    wrongType
};

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
            container.begin(), container.end(),
            [&key](const auto& keyValue) { return keyValue.first == key; });
    }
}

template <typename OnErrorCallback, typename Container, typename ValueType>
inline bool readProperty(const OnErrorCallback& onErrorCallback,
                         const Container& container,
                         const std::string& expectedKey,
                         ValueType& outValue) noexcept
{
    auto it = findProperty(container, expectedKey);

    if (it != container.end())
    {
        if constexpr (std::is_pointer_v<ValueType>)
        {
            if (const auto* value = std::get_if<
                    std::remove_const_t<std::remove_pointer_t<ValueType>>>(
                    &it->second))
            {
                outValue = value;
            }
            else
            {
                onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
                return false;
            }
        }
        else if constexpr (utility::is_optional_v<std::decay_t<ValueType>>)
        {
            using InnerType = typename ValueType::value_type;
            if constexpr (std::is_pointer_v<InnerType>)
            {
                static_assert(!std::is_pointer_v<InnerType>,
                              "std::optional<T*> is not supported");
            }
            else
            {
                if (const auto value = std::get_if<InnerType>(&it->second))

                {
                    outValue = *value;
                }
                else
                {
                    onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
                    return false;
                }
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
                onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
                return false;
            }
        }
    }
    else if constexpr (!utility::is_optional_v<std::decay_t<ValueType>> &&
                       !std::is_pointer_v<ValueType>)
    {
        onErrorCallback(UnpackErrorReason::missingProperty, expectedKey);
        return false;
    }

    return true;
}

template <size_t Index, typename OnErrorCallback, typename Container,
          typename ValueType, typename... Args>
inline bool readProperties(OnErrorCallback&& onErrorCallback,
                           const Container& container,
                           const std::string& expectedKey, ValueType& outValue,
                           Args&&... args) noexcept
{
    if (!readProperty(onErrorCallback, container, expectedKey, outValue))
    {
        return false;
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return readProperties<Index + 1>(
            std::forward<OnErrorCallback>(onErrorCallback), container,
            std::forward<Args>(args)...);
    }

    return true;
}

template <class V, class... Args>
inline std::string getNthKey(size_t index, const std::string& key, V&&,
                             Args&&... args) noexcept
{
    if (index == 0)
    {
        return key;
    }

    if constexpr (sizeof...(Args))
    {
        return getNthKey(index - 1, std::forward<Args>(args)...);
    }

    return "";
}

template <typename OnErrorCallback, typename Container, typename... Args>
inline auto unpackPropertiesCommon(OnErrorCallback&& onErrorCallback,
                                   const Container& input,
                                   Args&&... args) noexcept
{
    static_assert(sizeof...(Args) % 2 == 0);

    return details::readProperties<0>(
        std::forward<OnErrorCallback>(onErrorCallback), input,
        std::forward<Args>(args)...);
}

} // namespace details

template <typename Container, typename... Args>
inline void unpackProperties(const Container& input, Args&&... args)
{
    std::optional<exception::UnpackPropertyError> exception;

    details::unpackPropertiesCommon(
        [&exception](const UnpackErrorReason reason,
                     const std::string& property) {
            if (reason == UnpackErrorReason::wrongType)
            {
                exception.emplace(
                    property,
                    exception::UnpackPropertyError::reasonTypeNotMatched);
            }
            else
            {
                exception.emplace(
                    property,
                    exception::UnpackPropertyError::reasonMissingProperty);
            }
        },
        input, std::forward<Args>(args)...);

    if (exception)
    {
        throw *exception;
    }
}

template <typename OnErrorCallback, typename Container, typename... Args>
inline bool unpackPropertiesNoThrow(OnErrorCallback&& onErrorCallback,
                                    const Container& input,
                                    Args&&... args) noexcept
{
    return details::unpackPropertiesCommon(
        std::forward<OnErrorCallback>(onErrorCallback), input,
        std::forward<Args>(args)...);
}

} // namespace sdbusplus
