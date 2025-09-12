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
    std::tuple<boost::system::error_code, sdbusplus::message_t, int> foo;
    auto x = make_dbus_args_tuple(foo);
    static_assert(std::is_same_v<std::tuple_element_t<0, decltype(x)>, int&>,
                  "Second type wasn't int");
    static_assert(std::tuple_size_v<decltype(x)> == 1, "Size was wrong");
}
TEST(MakeDbusArgsTuple, ArgFirst)
{
    std::tuple<boost::system::error_code, int> foo;
    auto x = make_dbus_args_tuple(foo);
    static_assert(std::is_same_v<std::tuple_element_t<0, decltype(x)>, int&>,
                  "Second type wasn't int");
    static_assert(std::tuple_size_v<decltype(x)> == 1, "Size was wrong");
}
TEST(MakeDbusArgsTuple, NoArgs)
{
    std::tuple<boost::system::error_code> foo;
    auto x = make_dbus_args_tuple(foo);
    static_assert(std::tuple_size_v<decltype(x)> == 0, "Size was wrong");
}
} // namespace utility
} // namespace sdbusplus
