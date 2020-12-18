#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>

#include <gtest/gtest.h>

template <typename... Args>
auto dbus_string(Args&&... /*args*/)
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
    const char* c = "c";

    ASSERT_EQ(dbus_string(a, std::move(b), c), "sss");
}

TEST(MessageTypes, ObjectPath)
{
    ASSERT_EQ(dbus_string(sdbusplus::message::object_path("/asdf")), "o");
}

TEST(MessageTypes, ObjectPathLeaf)
{
    ASSERT_EQ(sdbusplus::message::object_path("/abc/def").filename(),
              std::optional<std::string>("def"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc/").filename(),
              std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("/abc").filename(),
              std::optional<std::string>("abc"));
    ASSERT_EQ(sdbusplus::message::object_path("/").filename(), std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("").filename(), std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("abc").filename(), std::nullopt);
}

TEST(MessageTypes, ObjectPathParent)
{
    ASSERT_EQ(sdbusplus::message::object_path("/abc/def").parent_path(),
              std::optional<std::string>("/abc"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc/").parent_path(),
              std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("/abc").parent_path(),
              std::optional<std::string>("/"));
    ASSERT_EQ(sdbusplus::message::object_path("/").parent_path(), std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("").parent_path(), std::nullopt);
    ASSERT_EQ(sdbusplus::message::object_path("abc").parent_path(),
              std::nullopt);
}

TEST(MessageTypes, Signature)
{
    ASSERT_EQ(dbus_string(sdbusplus::message::signature("sss")), "g");
}

TEST(MessageTypes, VectorOfString)
{
    std::vector<std::string> s = {"a", "b"};

    ASSERT_EQ(dbus_string(s), "as");
}

TEST(MessageTypes, SetOfString)
{
    std::set<std::string> s = {"a", "b"};

    ASSERT_EQ(dbus_string(s), "as");
}
