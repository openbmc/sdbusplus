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

template <typename T>
class has_member_find
{
  private:
    template <typename U>
    static U& ref();

    template <typename U>
    static std::true_type check(decltype(ref<U>().find(
        ref<std::tuple_element_t<0, typename U::value_type>>()))*);
    template <typename>
    static std::false_type check(...);

  public:
    static constexpr bool value =
        decltype(check<std::decay_t<T>>(nullptr))::value;
};

template <typename T>
constexpr bool has_member_find_v = has_member_find<T>::value;

template <typename T>
class has_member_contains
{
  private:
    template <typename U>
    static U& ref();

    template <typename U>
    static std::true_type check(decltype(ref<U>().contains(
        ref<std::tuple_element_t<0, typename U::value_type>>()))*);
    template <typename>
    static std::false_type check(...);

  public:
    static constexpr bool value =
        decltype(check<std::decay_t<T>>(nullptr))::value;
};

template <typename T>
constexpr bool has_member_contains_v = has_member_contains<T>::value;

template <typename T>
struct is_optional : public std::false_type
{};

template <typename T>
struct is_optional<std::optional<T>> : public std::true_type
{};

template <typename T>
struct is_optional<const std::optional<T>> : public std::true_type
{};

template <typename T>
struct is_optional<std::optional<T>&> : public std::true_type
{};

template <typename T>
struct is_optional<const std::optional<T>&> : public std::true_type
{};

template <typename T>
constexpr bool is_optional_v = is_optional<T>::value;

} // namespace utility

} // namespace sdbusplus
