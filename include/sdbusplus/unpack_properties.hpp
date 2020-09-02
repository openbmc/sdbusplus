#pragma once

#include <algorithm>
#include <bitset>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace sdbusplus
{

struct UnpackPropertyError : public std::runtime_error
{
    explicit UnpackPropertyError(std::string propertyName,
                                 std::string_view reason) :
        std::runtime_error("UnpackPropertyError: " + propertyName +
                           ", reason: " + reason.data()),
        propertyName(propertyName), reason(reason)
    {}

    static constexpr std::string_view reasonMissingProperty =
        "Missing property";
    static constexpr std::string_view reasonTypeNotMatched = "Type not matched";

    const std::string propertyName;
    const std::string reason;
};

namespace detail
{

template <typename T, typename... V, typename ValueType>
bool getIf(std::variant<V...>& variant, ValueType& outValue)
{
    if (T* value = std::get_if<T>(&variant))
    {
        outValue = std::move(*value);
        return true;
    }

    return false;
}

template <size_t Index, typename... V, size_t N, typename ValueType,
          typename... Args>
void readSingleProperty(const std::string& key, std::variant<V...>& variant,
                        std::bitset<N>& assigned,
                        const std::string& expectedKey, ValueType& outValue,
                        Args&&... args)
{
    static_assert(Index < N);

    if (key == expectedKey)
    {
        if (!assigned.test(Index))
        {
            if (getIf<ValueType>(variant, outValue))
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

    return std::string();
}

} // namespace detail

template <typename Container, typename... Args>
std::optional<UnpackPropertyError> unpackProperties(Container&& input,
                                                    Args&&... args)
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
            return UnpackPropertyError(
                missingProperty, UnpackPropertyError::reasonMissingProperty);
        }

        return UnpackPropertyError(missingProperty,
                                   UnpackPropertyError::reasonTypeNotMatched);
    }

    return std::nullopt;
}

} // namespace sdbusplus
