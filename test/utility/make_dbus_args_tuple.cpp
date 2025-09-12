#include <boost/system/error_code.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/make_dbus_args_tuple.hpp>

#include <gtest/gtest.h>

namespace sdbusplus
{
namespace utility
{

TEST(MakeDbusArgsTuple, MessageFirst)
{
    std::tuple<boost::system::error_code, sdbusplus::message_t, int>
        input_tuple;
    auto tuple_out = make_dbus_args_tuple(input_tuple);
    static_assert(
        std::is_same_v<std::tuple_element_t<0, decltype(tuple_out)>, int&>,
        "Second type wasn't int");
    static_assert(std::tuple_size_v<decltype(tuple_out)> == 1,
                  "Size was wrong");
    // Verify the output reference is now the first member, and references the 2
    // index tuple arg
    EXPECT_EQ(&std::get<2>(input_tuple), &std::get<0>(tuple_out));
}
TEST(MakeDbusArgsTuple, ArgFirst)
{
    std::tuple<boost::system::error_code, int> input_tuple{
        boost::system::error_code(), 42};
    auto tuple_out = make_dbus_args_tuple(input_tuple);
    static_assert(
        std::is_same_v<std::tuple_element_t<0, decltype(tuple_out)>, int&>,
        "Second type wasn't int");
    static_assert(std::tuple_size_v<decltype(tuple_out)> == 1,
                  "Size was wrong");
    // Verify the output reference is now the first member, and references the 1
    // index tuple arg
    EXPECT_EQ(&std::get<1>(input_tuple), &std::get<0>(tuple_out));
    EXPECT_EQ(std::get<0>(tuple_out), 42);
}
TEST(MakeDbusArgsTuple, NoArgs)
{
    std::tuple<boost::system::error_code> input_tuple;
    auto tuple_out = make_dbus_args_tuple(input_tuple);
    static_assert(std::tuple_size_v<decltype(tuple_out)> == 0,
                  "Size was wrong");
}
} // namespace utility
} // namespace sdbusplus
