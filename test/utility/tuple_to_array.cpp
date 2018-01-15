#include <gtest/gtest.h>
#include <sdbusplus/utility/tuple_to_array.hpp>

TEST(TupleToArray, Test3Chars)
{
    std::array<char, 3> a{'a', 'b', 'c'};
    auto t = std::make_tuple('a', 'b', 'c');

    ASSERT_EQ(a, sdbusplus::utility::tuple_to_array(std::move(t)));
}

TEST(TupleToArray, Test4Ints)
{
    std::array<int, 4> b{1, 2, 3, 4};
    auto t2 = std::make_tuple(1, 2, 3, 4);

    ASSERT_EQ(b, sdbusplus::utility::tuple_to_array(std::move(t2)));
}
