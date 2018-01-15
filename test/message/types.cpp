#include <gtest/gtest.h>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>

template <typename... Args> auto dbus_string(Args &&... args)
{
    return std::string(sdbusplus::utility::tuple_to_array(
                           sdbusplus::message::types::type_id<Args...>())
                           .data());
}

TEST(MessageTypes, Integer)
{
    ASSERT_EQ(dbus_string(1), "i");
}

TEST(MessageTypes, Double)
{
    ASSERT_EQ(dbus_string(1.0), "d");
}

TEST(MessageTypes, MultipleParameter)
{
    ASSERT_EQ(dbus_string(false, true), "bb");
    ASSERT_EQ(dbus_string(1, 2, 3, true, 1.0), "iiibd");
}

TEST(MessageTypes, StringReferences)
{
    std::string a = "a";
    std::string b = "b";
    const char *c = "c";

    ASSERT_EQ(dbus_string(a, std::move(b), c), "sss");
}

TEST(MessageTypes, ObjectPath)
{
    ASSERT_EQ(dbus_string(sdbusplus::message::object_path("/asdf")), "o");
}

TEST(MessageTypes, Signature)
{
    ASSERT_EQ(dbus_string(sdbusplus::message::signature("sss")), "g");
}
