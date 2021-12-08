#pragma once

#include <type_traits>
#include <variant>

namespace sdbusplus::utility
{

namespace details
{

template <class T, class... Args>
struct holds_type;

template <class T, class... Args>
struct holds_type<T, std::variant<Args...>>
{
  private:
    template <class U, class... Types>
    static constexpr bool evaluate()
    {
        if constexpr (std::is_same_v<T, U>)
        {
            return true;
        }
        else if constexpr (sizeof...(Types) > 0)
        {
            return evaluate<Types...>();
        }

        return false;
    }

  public:
    static constexpr bool value = evaluate<Args...>();
};

} // namespace details

template <class T, class Variant>
constexpr bool holds_type_v = details::holds_type<T, Variant>::value;

} // namespace sdbusplus::utility
