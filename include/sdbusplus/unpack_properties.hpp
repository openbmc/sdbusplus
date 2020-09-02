#pragma once

#include <algorithm>
#include <array>
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
    static constexpr std::string_view reasonPropertyAppearsMoreThanOnce =
        "Property appears more than once";
    static constexpr std::string_view reasonTypeNotMatched = "Type not matched";

    const std::string propertyName;
    const std::string reason;
};

namespace detail
{

struct ErrorInfo
{
    std::string propertyName;
    uint8_t propertyCount;
};

template <typename T, typename... V, typename ValueType>
uint8_t getIf(std::variant<V...>& variant, ValueType& outValue)
{
    if (T* value = std::get_if<T>(&variant))
    {
        outValue = std::move(*value);
        return 1u;
    }

    return 0u;
}

template <size_t Index, typename... V, size_t N, typename ValueType,
          typename... Args>
void readSingleProperty(std::string& key, std::variant<V...>& variant,
                        std::array<uint8_t, N>& assigned,
                        const std::string& expectedKey, ValueType& outValue,
                        Args&&... args)
{
    static_assert(Index < N);

    if (key == expectedKey)
    {
        assigned[Index] += getIf<ValueType>(variant, outValue);
    }
    else if constexpr (sizeof...(Args) > 0)
    {
        return readSingleProperty<Index + 1>(key, variant, assigned,
                                             std::forward<Args>(args)...);
    }
}

template <size_t Index, size_t N, typename ValueType, typename... Args>
std::optional<ErrorInfo> findError(std::array<uint8_t, N>& assigned,
                                   const std::string& key, ValueType&,
                                   Args&&... args)
{
    static_assert(Index < N);

    if (assigned[Index] != 1u)
    {
        return ErrorInfo{key, assigned[Index]};
    }

    if constexpr (sizeof...(Args) > 0)
    {
        return findError<Index + 1>(assigned, std::forward<Args>(args)...);
    }

    return std::nullopt;
}

} // namespace detail

template <typename... V, typename... Args>
std::optional<UnpackPropertyError> unpackProperties(
    std::vector<std::pair<std::string, std::variant<V...>>>& ret,
    Args&&... args)
{
    static_assert(sizeof...(Args) % 2 == 0);

    auto assigned = std::array<uint8_t, sizeof...(Args) / 2>();

    for (auto& [key, value] : ret)
    {
        detail::readSingleProperty<0>(key, value, assigned,
                                      std::forward<Args>(args)...);
    }

    if (auto errorInfo =
            detail::findError<0>(assigned, std::forward<Args>(args)...))
    {
        if (errorInfo->propertyCount > 1u)
        {
            return UnpackPropertyError(
                errorInfo->propertyName,
                UnpackPropertyError::reasonPropertyAppearsMoreThanOnce);
        }
        else if (errorInfo->propertyCount == 0u)
        {
            auto keyIt = std::find_if(
                ret.begin(), ret.end(), [&errorInfo](const auto& item) {
                    return errorInfo->propertyName == item.first;
                });

            if (keyIt == ret.end())
            {
                return UnpackPropertyError(
                    errorInfo->propertyName,
                    UnpackPropertyError::reasonMissingProperty);
            }

            return UnpackPropertyError(
                errorInfo->propertyName,
                UnpackPropertyError::reasonTypeNotMatched);
        }
    }

    return std::nullopt;
}

} // namespace sdbusplus
