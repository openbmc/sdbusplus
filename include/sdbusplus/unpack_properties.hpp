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
#include <vector>

namespace sdbusplus
{

namespace details
{

template <typename VariantType>
inline auto findProperty(
    const std::vector<std::pair<std::string, VariantType>>& container,
    const std::string& key) noexcept
{
    return std::find_if(
        container.begin(), container.end(),
        [&key](const auto& keyValue) { return keyValue.first == key; });
}

template <typename OnErrorCallback, typename VariantType, typename ValueType>
inline bool readProperty(
    const OnErrorCallback& onErrorCallback,
    const std::vector<std::pair<std::string, VariantType>>& container,
    const std::string& expectedKey,
    ValueType&
        outValue) noexcept(noexcept(onErrorCallback(sdbusplus::
                                                        UnpackErrorReason{},
                                                    std::string{})))
{
    auto it = findProperty(container, expectedKey);

    if (it != container.end())
    {
        if constexpr (std::is_pointer_v<ValueType>)
        {
            if (const auto* value = std::get_if<
                    std::remove_cv_t<std::remove_pointer_t<ValueType>>>(
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
        else if constexpr (utility::is_optional_v<ValueType>)
        {
            using InnerType = typename ValueType::value_type;
            static_assert(!std::is_pointer_v<InnerType>,
                          "std::optional<T*> is not supported");
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
    else if constexpr (!utility::is_optional_v<ValueType> &&
                       !std::is_pointer_v<ValueType>)
    {
        onErrorCallback(UnpackErrorReason::missingProperty, expectedKey);
        return false;
    }

    return true;
}

template <typename OnErrorCallback, typename VariantType, typename ValueType,
          typename... Args>
inline bool readProperties(
    OnErrorCallback&& onErrorCallback,
    const std::vector<std::pair<std::string, VariantType>>& container,
    const std::string& expectedKey, ValueType& outValue,
    Args&&... args) noexcept(noexcept(onErrorCallback(sdbusplus::
                                                          UnpackErrorReason{},
                                                      std::string{})))
{
    if (!readProperty(onErrorCallback, container, expectedKey, outValue))
    {
        return false;
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return readProperties(std::forward<OnErrorCallback>(onErrorCallback),
                              container, std::forward<Args>(args)...);
    }

    return true;
}

template <typename OnErrorCallback, typename VariantType, typename... Args>
inline auto unpackPropertiesCommon(
    OnErrorCallback&& onErrorCallback,
    const std::vector<std::pair<std::string, VariantType>>& input,
    Args&&... args) noexcept(noexcept(onErrorCallback(sdbusplus::
                                                          UnpackErrorReason{},
                                                      std::string{})))
{
    static_assert(
        sizeof...(Args) % 2 == 0,
        "Expected number of arguments to be even, but got odd number instead");

    return details::readProperties(
        std::forward<OnErrorCallback>(onErrorCallback), input,
        std::forward<Args>(args)...);
}

} // namespace details

template <typename VariantType, typename... Args>
inline void unpackProperties(
    const std::vector<std::pair<std::string, VariantType>>& input,
    Args&&... args)
{
    details::unpackPropertiesCommon(
        [](const UnpackErrorReason reason, const std::string& property) {
            throw exception::UnpackPropertyError(property, reason);
        },
        input, std::forward<Args>(args)...);
}

template <typename OnErrorCallback, typename VariantType, typename... Args>
inline bool unpackPropertiesNoThrow(
    OnErrorCallback&& onErrorCallback,
    const std::vector<std::pair<std::string, VariantType>>& input,
    Args&&... args) noexcept
{
    return details::unpackPropertiesCommon(
        std::forward<OnErrorCallback>(onErrorCallback), input,
        std::forward<Args>(args)...);
}

} // namespace sdbusplus
