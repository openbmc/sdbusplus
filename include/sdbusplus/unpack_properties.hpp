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
namespace detail
{

template <typename Variant, typename ValueType>
bool getIf(Variant&& variant, ValueType& outValue)
{
    if (auto value = std::get_if<ValueType>(&variant))
    {
        outValue = std::move(*value);
        return true;
    }

    return false;
}

template <typename Container>
auto findProperty(Container&& container, const std::string& key)
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
bool containsProperty(Container&& container, const std::string& key)
{
    if constexpr (utility::has_member_contains_v<Container>)
    {
        return container.contains(key);
    }
    else
    {
        return findProperty(std::forward<Container>(container), key) !=
               std::end(container);
    }
}

template <size_t Index, typename Container, size_t N, typename ValueType,
          typename... Args>
void readProperties(Container&& container, std::bitset<N>& assigned,
                    const std::string& expectedKey, ValueType& outValue,
                    Args&&... args)
{
    static_assert(Index < N);

    auto it = findProperty(std::forward<Container>(container), expectedKey);

    if (it != std::end(container))
    {
        if (getIf(it->second, outValue))
        {
            assigned.set(Index);
        }
    }

    if constexpr (sizeof...(Args) > 0)
    {
        readProperties<Index + 1>(std::forward<Container>(container), assigned,
                                  std::forward<Args>(args)...);
    }
}

template <size_t Index, size_t N, typename ValueType, typename... Args>
std::string findMissingProperty(std::bitset<N>& assigned,
                                const std::string& key, ValueType&,
                                Args&&... args)
{
    static_assert(Index < N);

    if (!assigned.test(Index))
    {
        return key;
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return findMissingProperty<Index + 1>(assigned,
                                              std::forward<Args>(args)...);
    }

    return {};
}

} // namespace detail

template <typename Container, typename... Args>
void unpackProperties(Container&& input, Args&&... args)
{
    static_assert(sizeof...(Args) % 2 == 0);

    auto assigned = std::bitset<sizeof...(Args) / 2>();

    detail::readProperties<0>(input, assigned, std::forward<Args>(args)...);

    if (!assigned.all())
    {
        std::string missingProperty = detail::findMissingProperty<0>(
            assigned, std::forward<Args>(args)...);

        if (detail::containsProperty(std::forward<Container>(input),
                                     missingProperty))
        {
            throw exception::UnpackPropertyError(
                missingProperty,
                exception::UnpackPropertyError::reasonTypeNotMatched);
        }
        else
        {
            throw exception::UnpackPropertyError(
                missingProperty,
                exception::UnpackPropertyError::reasonMissingProperty);
        }
    }
}

} // namespace sdbusplus
