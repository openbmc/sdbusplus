#pragma once

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>

namespace sdbusplus
{

namespace utility
{

/** @brief Retrieve the first type from a parameter pack
 *
 * @tparam Types - the parameter pack
 */
template <typename... Types>
using first_type_t = std::tuple_element_t<0, std::tuple<Types...>>;

/** @brief Convert T[N] to T* if is_same<Tbase,T>
 *
 *  @tparam Tbase - The base type expected.
 *  @tparam T - The type to convert.
 */
template <typename Tbase, typename T>
using array_to_ptr_t = typename std::conditional_t<
    std::is_array_v<T>,
    std::conditional_t<std::is_same_v<Tbase, std::remove_extent_t<T>>,
                       std::add_pointer_t<std::remove_extent_t<T>>, T>,
    T>;

template <std::size_t N, typename FirstArg, typename... Rest>
struct strip_first_n_args;

template <std::size_t N, typename FirstArg, typename... Rest>
struct strip_first_n_args<N, std::tuple<FirstArg, Rest...>> :
    strip_first_n_args<N - 1, std::tuple<Rest...>>
{};

template <typename FirstArg, typename... Rest>
struct strip_first_n_args<0, std::tuple<FirstArg, Rest...>>
{
    using type = std::tuple<FirstArg, Rest...>;
};
template <std::size_t N>
struct strip_first_n_args<N, std::tuple<>>
{
    using type = std::tuple<>;
};

template <std::size_t N, typename... Args>
using strip_first_n_args_t = typename strip_first_n_args<N, Args...>::type;

// Small helper class for stripping off the error code from the function
// argument definitions so unpack can be called appropriately
template <typename T>
using strip_first_arg = strip_first_n_args<1, T>;

template <typename T>
using strip_first_arg_t = typename strip_first_arg<T>::type;

// matching helper class to only return the first type
template <typename T>
struct get_first_arg
{
    using type = void;
};

template <typename FirstArg, typename... Rest>
struct get_first_arg<std::tuple<FirstArg, Rest...>>
{
    using type = FirstArg;
};

template <typename... Args>
using get_first_arg_t = typename get_first_arg<Args...>::type;

// helper class to remove const and reference from types
template <typename T>
struct decay_tuple
{};

template <typename... Args>
struct decay_tuple<std::tuple<Args...>>
{
    using type = std::tuple<typename std::decay<Args>::type...>;
};

template <typename... Args>
using decay_tuple_t = typename decay_tuple<Args...>::type;

// Small helper class for stripping off the first + last character of a char
// array
template <std::size_t N, std::size_t... Is>
constexpr std::array<char, N - 2> strip_ends(const std::array<char, N>& s,
                                             std::index_sequence<Is...>)
{
    return {(s[1 + Is])..., static_cast<char>(0)};
}

template <std::size_t N>
constexpr std::array<char, N - 2> strip_ends(const std::array<char, N>& s)
{
    return strip_ends(s, std::make_index_sequence<N - 3>{});
}

template <typename T>
concept has_member_find =
    requires(std::decay_t<T> t,
             std::tuple_element_t<0, typename std::decay_t<T>::value_type> val)
{
    t.find(val);
};

template <typename T>
concept has_member_contains =
    requires(std::decay_t<T> t,
             std::tuple_element_t<0, typename std::decay_t<T>::value_type> val)
{
    t.contains(val);
};

template <typename T>
struct is_optional : public std::false_type
{};

template <typename T>
struct is_optional<std::optional<T>> : public std::true_type
{};

template <class T>
concept an_optional = is_optional<std::decay_t<T>>::value;

template <class T>
struct functor_traits : public functor_traits<decltype(&T::operator())>
{};

template <class T, class R, class... Args>
struct functor_traits<R (T::*)(Args...) const>
{
    template <size_t Index>
    using arg_t = std::remove_const_t<std::remove_reference_t<
        std::tuple_element_t<Index, std::tuple<Args...>>>>;
};

} // namespace utility

} // namespace sdbusplus
