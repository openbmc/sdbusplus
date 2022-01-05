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

template <typename VariantType>
inline auto findProperty(
    const std::vector<std::pair<std::string, VariantType>>& container,
    const std::string& key) noexcept
{
    return std::find_if(
        container.begin(), container.end(),
        [&key](const auto& keyValue) { return keyValue.first == key; });
}

template <size_t Index, typename Bitset, typename OnErrorCallback,
          typename VariantType, typename ValueType, typename... Args>
inline void readProperty(
    Bitset& assigned, const OnErrorCallback& onErrorCallback,
    const std::string& key, const VariantType& valueVariant,
    const std::string& expectedKey, ValueType& outValue,
    Args&&... args) noexcept(noexcept(onErrorCallback(sdbusplus::
                                                          UnpackErrorReason{},
                                                      std::string{})))
{
    if (key == expectedKey)
    {
        if constexpr (std::is_pointer_v<ValueType>)
        {
            if (const auto* value = std::get_if<
                    std::remove_cv_t<std::remove_pointer_t<ValueType>>>(
                    &valueVariant))
            {
                assigned.set(Index);
                outValue = value;
            }
            else
            {
                assigned.reset(Index);
                onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
            }
        }
        else if constexpr (utility::is_optional_v<ValueType>)
        {
            using InnerType = typename ValueType::value_type;
            static_assert(!std::is_pointer_v<InnerType>,
                          "std::optional<T*> is not supported");
            if (const auto value = std::get_if<InnerType>(&valueVariant))

            {
                assigned.set(Index);
                outValue = *value;
            }
            else
            {
                assigned.reset(Index);
                onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
            }
        }
        else
        {
            if (const auto value = std::get_if<ValueType>(&valueVariant))
            {
                assigned.set(Index);
                outValue = *value;
            }
            else
            {
                assigned.reset(Index);
                onErrorCallback(UnpackErrorReason::wrongType, expectedKey);
            }
        }
    }
    else if constexpr (sizeof...(Args) > 0)
    {
        readProperty<Index + 1>(assigned, onErrorCallback, key, valueVariant,
                                std::forward<Args>(args)...);
    }
}

template <size_t Index, typename Bitset, typename OnErrorCallback,
          typename ValueType, typename... Args>
void reportMissingProperties(const Bitset& assigned,
                             const OnErrorCallback& onErrorCallback,
                             const std::string& expectedKey, const ValueType&,
                             Args&&... args)
{
    if (!assigned.test(Index))
    {
        onErrorCallback(UnpackErrorReason::missingProperty, expectedKey);
    }
    else if constexpr (sizeof...(Args) > 0)
    {
        reportMissingProperties<Index + 1>(assigned, onErrorCallback,
                                           std::forward<Args>(args)...);
    }
}

template <typename Bitset, typename OnErrorCallback, typename VariantType,
          typename... Args>
inline void readProperties(
    Bitset& assigned, OnErrorCallback&& onErrorCallback,
    const std::vector<std::pair<std::string, VariantType>>& container,
    Args&&... args) noexcept(noexcept(onErrorCallback(sdbusplus::
                                                          UnpackErrorReason{},
                                                      std::string{})))
{
    for (const auto& [key, value] : container)
    {
        readProperty<0>(assigned,
                        std::forward<OnErrorCallback>(onErrorCallback), key,
                        value, std::forward<Args>(args)...);
    }

    reportMissingProperties<0>(assigned,
                               std::forward<OnErrorCallback>(onErrorCallback),
                               std::forward<Args>(args)...);
}

template <class... Args>
struct InitialAssigned
{
    template <size_t Index, typename KeyType, typename ValueType, class... Rest>
    static constexpr uint64_t calculateValue(uint64_t v)
    {
        if constexpr (std::is_pointer_v<ValueType> ||
                      utility::is_optional_v<ValueType>)
        {
            v |= 1 << Index;
        }

        if constexpr (sizeof...(Rest) > 0)
        {
            return calculateValue<Index + 1, Rest...>(v);
        }
        else
        {
            return v;
        }
    }

    static constexpr uint64_t value{calculateValue<0, Args...>(0u)};
    static constexpr std::bitset<sizeof...(Args) / 2> bitset{value};
};

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

    std::bitset<sizeof...(Args) / 2> assigned =
        InitialAssigned<Args...>::bitset;

    details::readProperties(assigned,
                            std::forward<OnErrorCallback>(onErrorCallback),
                            input, std::forward<Args>(args)...);

    return assigned.all();
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
