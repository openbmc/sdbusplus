#include <sdbusplus/utility/type_traits.hpp>

#include <type_traits>

#include <gmock/gmock.h>

namespace
{

using sdbusplus::utility::array_to_ptr_t;
using std::is_same;

TEST(TypeTraits, Basic)
{

    static_assert(is_same<char, array_to_ptr_t<char, char>>::value,
                  "array_to_ptr_t<char, char> != char");

    static_assert(is_same<char*, array_to_ptr_t<char, char*>>::value,
                  "array_to_ptr_t<char, char*> != char*");

    static_assert(is_same<char*, array_to_ptr_t<char, char[100]>>::value,
                  "array_to_ptr_t<char, char[100]> != char*");

    static_assert(is_same<char[100], array_to_ptr_t<int, char[100]>>::value,
                  "array_to_ptr_t<int, char[100]> != char[100]");
}

template <class T, class U>
void assertWrapper(T&& t, U&& u, std::string_view message)
{
    ASSERT_THAT(t,
                u)
        << message; // ASSERT macros doesn't like commas when templates are used
}

TEST(TypeTraits, HasMemberFind)
{
    using sdbusplus::utility::has_member_find_v;
    using namespace testing;

    assertWrapper(has_member_find_v<std::map<std::string, int>>, Eq(true),
                  "std::map");
    assertWrapper(has_member_find_v<std::vector<std::pair<std::string, int>>>,
                  Eq(false), "std::vector");

    struct Foo
    {
        using value_type = std::pair<int, int>;

        void find(std::tuple_element_t<0, value_type>)
        {}
    };

    struct Bar
    {};

    assertWrapper(has_member_find_v<Foo>, Eq(true), "Foo");
    assertWrapper(has_member_find_v<Foo&>, Eq(true), "Foo&");
    assertWrapper(has_member_find_v<const Foo&>, Eq(true), "const Foo&");

    assertWrapper(has_member_find_v<Bar>, Eq(false), "Bar");
}

TEST(TypeTraits, HasMemberContains)
{
    using sdbusplus::utility::has_member_contains_v;
    using namespace testing;

    // std::map has member_contains from c++20
    assertWrapper(has_member_contains_v<std::map<std::string, int>>, Eq(false),
                  "std::map");
    assertWrapper(
        has_member_contains_v<std::vector<std::pair<std::string, int>>>,
        Eq(false), "std::vector");

    struct Foo
    {
        using value_type = std::pair<int, int>;

        void contains(std::tuple_element_t<0, value_type>)
        {}
    };

    struct Bar
    {};

    assertWrapper(has_member_contains_v<Foo>, Eq(true), "Foo");
    assertWrapper(has_member_contains_v<Foo&>, Eq(true), "Foo&");
    assertWrapper(has_member_contains_v<const Foo&>, Eq(true), "const Foo&");

    assertWrapper(has_member_contains_v<Bar>, Eq(false), "Bar");
}

} // namespace
