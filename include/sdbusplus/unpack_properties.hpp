#pragma once

#include <sdbusplus/exception.hpp>

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

template <size_t Index, typename Variant, size_t N, typename ValueType,
          typename... Args>
void readSingleProperty(const std::string& key, Variant&& variant,
                        std::bitset<N>& assigned,
                        const std::string& expectedKey, ValueType& outValue,
                        Args&&... args)
{
    static_assert(Index < N);

    if (key == expectedKey)
    {
        if (!assigned.test(Index))
        {
            if (getIf(variant, outValue))
            {
                assigned.set(Index);
            }
        }
    }
    else if constexpr (sizeof...(Args) > 0)
    {
        readSingleProperty<Index + 1>(key, variant, assigned,
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

    for (auto& [key, value] : input)
    {
        detail::readSingleProperty<0>(key, value, assigned,
                                      std::forward<Args>(args)...);
    }

    if (!assigned.all())
    {
        std::string missingProperty = detail::findMissingProperty<0>(
            assigned, std::forward<Args>(args)...);
        auto keyIt = std::find_if(input.begin(), input.end(),
                                  [&missingProperty](const auto& item) {
                                      return missingProperty == item.first;
                                  });

        if (keyIt == input.end())
        {
            throw exception::UnpackPropertyError(
                missingProperty,
                exception::UnpackPropertyError::reasonMissingProperty);
        }

        throw exception::UnpackPropertyError(
            missingProperty,
            exception::UnpackPropertyError::reasonTypeNotMatched);
    }
}

} // namespace sdbusplus
