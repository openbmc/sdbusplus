#include <sdbusplus/utility/dedup_variant.hpp>
#include <sdbusplus/utility/merge_variants.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <type_traits>

#include <gmock/gmock.h>

namespace
{

using sdbusplus::utility::array_to_ptr_t;
using std::is_same_v;

TEST(TypeTraits, Basic)
{
    static_assert(is_same_v<char, array_to_ptr_t<char, char>>,
                  "array_to_ptr_t<char, char> != char");

    static_assert(is_same_v<char*, array_to_ptr_t<char, char*>>,
                  "array_to_ptr_t<char, char*> != char*");

    static_assert(is_same_v<char*, array_to_ptr_t<char, char[100]>>,
                  "array_to_ptr_t<char, char[100]> != char*");

    static_assert(is_same_v<char[100], array_to_ptr_t<int, char[100]>>,
                  "array_to_ptr_t<int, char[100]> != char[100]");
}

TEST(TypeTraits, HasMemberFind)
{
    using sdbusplus::utility::has_member_find_v;
    using namespace testing;

    ASSERT_THAT((has_member_find_v<std::map<std::string, int>>), Eq(true));
    ASSERT_THAT((has_member_find_v<std::vector<std::pair<std::string, int>>>),
                Eq(false));

    struct Foo
    {
        using value_type = std::pair<int, int>;

        void find(std::tuple_element_t<0, value_type>) {}
    };

    struct Bar
    {};

    ASSERT_THAT(has_member_find_v<Foo>, Eq(true));
    ASSERT_THAT(has_member_find_v<Foo&>, Eq(true));
    ASSERT_THAT(has_member_find_v<const Foo&>, Eq(true));

    ASSERT_THAT(has_member_find_v<Bar>, Eq(false));
}

TEST(TypeTraits, HasMemberContains)
{
    using sdbusplus::utility::has_member_contains_v;
    using namespace testing;

    // std::map has member_contains from c++20
    ASSERT_THAT((has_member_contains_v<std::map<std::string, int>>), Eq(true));
    ASSERT_THAT(
        (has_member_contains_v<std::vector<std::pair<std::string, int>>>),
        Eq(false));

    struct Foo
    {
        using value_type = std::pair<int, int>;

        void contains(std::tuple_element_t<0, value_type>) {}
    };

    struct Bar
    {};

    ASSERT_THAT(has_member_contains_v<Foo>, Eq(true));
    ASSERT_THAT(has_member_contains_v<Foo&>, Eq(true));
    ASSERT_THAT(has_member_contains_v<const Foo&>, Eq(true));

    ASSERT_THAT(has_member_contains_v<Bar>, Eq(false));
}

TEST(TypeTraits, IsOptional)
{
    using sdbusplus::utility::is_optional;
    using sdbusplus::utility::is_optional_v;

    ASSERT_TRUE(is_optional<std::optional<int>>::value);
    ASSERT_TRUE(is_optional<std::optional<int>&>::value);
    ASSERT_FALSE(is_optional<int>::value);

    ASSERT_TRUE(is_optional_v<std::optional<int>>);
    ASSERT_TRUE(is_optional_v<std::optional<int>&>);
    ASSERT_FALSE(is_optional_v<int>);
}

// Tests for dedup_variant.
static_assert(std::is_same_v<std::variant<size_t>,
                             sdbusplus::utility::dedup_variant_t<size_t>>);
static_assert(
    std::is_same_v<std::variant<char, size_t>,
                   sdbusplus::utility::dedup_variant_t<char, size_t>>);
static_assert(std::is_same_v<
              std::variant<uint32_t, uint64_t>,
              sdbusplus::utility::dedup_variant_t<uint32_t, uint64_t, size_t>>);

// Tests for merge_variants.
static_assert(
    std::is_same_v<std::variant<size_t>,
                   sdbusplus::utility::merge_variants_t<std::variant<size_t>>>);
static_assert(std::is_same_v<std::variant<char, uint32_t, uint64_t>,
                             sdbusplus::utility::merge_variants_t<
                                 std::variant<char, uint32_t>,
                                 std::variant<uint32_t, uint64_t, size_t>>>);

} // namespace
