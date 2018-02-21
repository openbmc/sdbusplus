#pragma once

namespace sdbusplus
{
namespace utility
{

/** has_const_iterator - Determine if type has const iterator
 *
 *  @tparam T - Type to be tested.
 *
 *  @value A value as to whether or not the type supports iteration
 */
template <typename T> struct has_const_iterator
{
  private:
    typedef char yes;
    typedef struct
    {
        char array[2];
    } no;

    template <typename C>
    static constexpr yes test(typename C::const_iterator*);
    template <typename C> static constexpr no test(...);

  public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
};

/** has_emplace_method - Determine if type has a method template named emplace
 *
 *  @tparam T - Type to be tested.
 *
 *  @value A value as to whether or not the type has an emplace method
 */
template <typename T> struct has_emplace_method
{
  private:
    struct dummy
    {
    };

    template <typename C, typename P>
    static constexpr auto test(P* p)
        -> decltype(std::declval<C>().emplace(*p), std::true_type());

    template <typename, typename> static std::false_type test(...);

  public:
    static constexpr bool value =
        std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
};

/** has_emplace_method - Determine if type has a method template named
 * emplace_back
 *
 *  @tparam T - Type to be tested.
 *
 *  @value A value as to whether or not the type has an emplace_back method
 */
template <typename T> struct has_emplace_back_method
{
  private:
    struct dummy
    {
    };

    template <typename C, typename P>
    static constexpr auto test(P* p)
        -> decltype(std::declval<C>().emplace_back(*p), std::true_type());

    template <typename, typename> static std::false_type test(...);

  public:
    static constexpr bool value =
        std::is_same<std::true_type, decltype(test<T, dummy>(nullptr))>::value;
};

} // namespace utility
} // namespace sdbusplus
