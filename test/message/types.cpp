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

TEST(MessageTypes, ObjectPathFilename)
{
    ASSERT_EQ(sdbusplus::message::object_path("/abc/def").filename(), "def");
    ASSERT_EQ(sdbusplus::message::object_path("/abc/").filename(), "");
    ASSERT_EQ(sdbusplus::message::object_path("/abc").filename(), "abc");
    ASSERT_EQ(sdbusplus::message::object_path("/_61bc").filename(), "abc");
    ASSERT_EQ(sdbusplus::message::object_path("/").filename(), "");
    ASSERT_EQ(sdbusplus::message::object_path("").filename(), "");
    ASSERT_EQ(sdbusplus::message::object_path("abc").filename(), "");
    ASSERT_EQ(sdbusplus::message::object_path("/_2d").filename(), "-");
    ASSERT_EQ(sdbusplus::message::object_path("/_20").filename(), " ");
    ASSERT_EQ(sdbusplus::message::object_path("/_2F").filename(), "/");
    ASSERT_EQ(sdbusplus::message::object_path("/bios_active").filename(),
              "bios_active");
}

TEST(MessageTypes, ObjectPathParent)
{
    ASSERT_EQ(sdbusplus::message::object_path("/abc/def").parent_path(),
              sdbusplus::message::object_path("/abc"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc/").parent_path(),
              sdbusplus::message::object_path("/abc"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc").parent_path(),
              sdbusplus::message::object_path("/"));
    ASSERT_EQ(sdbusplus::message::object_path("/").parent_path(),
              sdbusplus::message::object_path("/"));
}

TEST(MessageTypes, ObjectPathOperatorSlash)
{
    ASSERT_EQ(sdbusplus::message::object_path("/") / "abc",
              sdbusplus::message::object_path("/_61bc"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc") / "def",
              sdbusplus::message::object_path("/abc/_64ef"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc") / "-",
              sdbusplus::message::object_path("/abc/_2d"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc") / " ",
              sdbusplus::message::object_path("/abc/_20"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc") / "/",
              sdbusplus::message::object_path("/abc/_2f"));
    ASSERT_EQ(sdbusplus::message::object_path("/abc") / "ab_cd",
              sdbusplus::message::object_path("/abc/_61b_5fcd"));

    // Test the std::string overload.  This is largely just for coverage
    ASSERT_EQ(sdbusplus::message::object_path("/") / std::string("abc"),
              sdbusplus::message::object_path("/_61bc"));
}

TEST(MessageTypes, ObjectPathOperatorSlashEqual)
{
    sdbusplus::message::object_path path("/");
    path /= "abc";
    ASSERT_EQ(path, sdbusplus::message::object_path("/_61bc"));

    sdbusplus::message::object_path path2("/");
    path2 /= std::string("def");
    ASSERT_EQ(path2, sdbusplus::message::object_path("/_64ef"));
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
