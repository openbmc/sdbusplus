#pragma once

#include <algorithm>
#include <cstdint>

namespace sdbusplus::utility
{
namespace details
{

template <std::size_t N>
struct consteval_string_holder
{
    char value[N] = {};

    consteval consteval_string_holder(const char (&str)[N])
    {
        std::ranges::copy(str, value);
    }
};

} // namespace details

/** Type to allow compile-time parameter string comparisons.
 *
 *  Example:
 *      void foo(consteval_string<"FOO">);
 *
 *  If the function is not called with foo("FOO"), it will be a compile error.
 */
template <details::consteval_string_holder V>
struct consteval_string
{
    std::string_view value;

    template <typename T>
    consteval consteval_string(const T& s) : value(s)
    {
        if (value != V.value)
        {
            report_error("String mismatch; check parameter name.");
        }
    }

    // This is not a real function but used to trigger compile errors due to
    // calling a non-constexpr function in a constexpr context.
    static void report_error(const char*);
};

} // namespace sdbusplus::utility
