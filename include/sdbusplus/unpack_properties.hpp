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

struct UnpackError
{
    UnpackErrorReason reason;
    size_t index;
};

namespace details
{

template <typename Container>
inline auto findProperty(const Container& container,
                         const std::string& key) noexcept
{
    if constexpr (utility::has_member_find<Container>)
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

template <typename Container, typename ValueType>
inline std::optional<UnpackErrorReason>
    readProperty(const Container& container, const std::string& expectedKey,
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
                return UnpackErrorReason::wrongType;
            }
        }
        else if constexpr (utility::an_optional<ValueType>)
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
                    return UnpackErrorReason::wrongType;
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
                return UnpackErrorReason::wrongType;
            }
        }
    }
    else if constexpr (!utility::an_optional<ValueType> &&
                       !std::is_pointer_v<ValueType>)
    {
        return UnpackErrorReason::missingProperty;
    }

    return {};
}

template <size_t Index, typename Container, typename ValueType,
          typename... Args>
inline std::optional<UnpackError>
    readProperties(const Container& container, const std::string& expectedKey,
                   ValueType& outValue, Args&&... args) noexcept
{
    if (auto reason = readProperty(container, expectedKey, outValue))
    {
        return UnpackError{*reason, Index};
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return readProperties<Index + 1>(container,
                                         std::forward<Args>(args)...);
    }

    return {};
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

template <typename Container, typename... Args>
inline auto unpackPropertiesCommon(const Container& input,
                                   Args&&... args) noexcept
{
    static_assert(sizeof...(Args) % 2 == 0);

    return details::readProperties<0>(input, std::forward<Args>(args)...);
}

} // namespace details

template <typename Container, typename... Args>
inline void unpackProperties(const Container& input, Args&&... args)
{
    std::optional<UnpackError> error =
        details::unpackPropertiesCommon(input, std::forward<Args>(args)...);

    if (error)
    {
        std::string key =
            details::getNthKey(error->index, std::forward<Args>(args)...);
        if (error->reason == UnpackErrorReason::wrongType)
        {
            throw exception::UnpackPropertyError(
                std::move(key),
                exception::UnpackPropertyError::reasonTypeNotMatched);
        }
        else
        {
            throw exception::UnpackPropertyError(
                std::move(key),
                exception::UnpackPropertyError::reasonMissingProperty);
        }
    }
}

template <typename Container, typename... Args>
inline std::optional<UnpackError>
    unpackPropertiesNoThrow(const Container& input, Args&&... args) noexcept
{
    return details::unpackPropertiesCommon(input, std::forward<Args>(args)...);
}

} // namespace sdbusplus
