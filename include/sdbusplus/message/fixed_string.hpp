#pragma once

#include <array>
#include <string>
#include <string_view>

namespace sdbusplus
{

namespace message
{

template <std::size_t N>
struct fixed_string
{
    // allways contains a terminating null byte
    std::array<char, N> arr{};

    constexpr fixed_string(const char (&in)[N])
    {
        for (std::size_t i = 0; i < N; i++)
        {
            arr[i] = in[i];
        }

        arr[N - 1] = '\0';
    }
    template <std::size_t N1, std::size_t N2>
    constexpr fixed_string(const std::array<char, N1> a1,
                           const std::array<char, N2> a2)
    {
        std::size_t j = 0;
        for (std::size_t i = 0; i < N1 - 1; i++)
        {
            arr[j] = a1[i];
            j++;
        }
        for (std::size_t i = 0; i < N2; i++)
        {
            arr[j] = a2[i];
            j++;
        }
    }
    constexpr fixed_string(const std::array<char, N> in, bool _)
    {
        for (std::size_t i = 0; i < N; i++)
        {
            arr[i] = in[i];
        }
    }

    operator std::string() const
    {
        return std::string(arr.data());
    }
    constexpr operator std::string_view() const
    {
        return std::string_view(arr.data());
    }
    constexpr operator const char*() const
    {
        return arr.data();
    }
    template <std::size_t N2>
    constexpr auto operator+(fixed_string<N2> other) const
    {
        return fixed_string<N + N2 - 1>(arr, other.arr);
    }
    static constexpr std::size_t size()
    {
        return N;
    } // includes '\0'
    constexpr std::string_view view() const
    {
        return {arr.data(), N - 1};
    }
};

} // namespace message

} // namespace sdbusplus
