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
    EXPECT_EQ(dbus_string(1), "i");
}

TEST(MessageTypes, Double)
{
    EXPECT_EQ(dbus_string(1.0), "d");
}

TEST(MessageTypes, MultipleParameter)
{
    EXPECT_EQ(dbus_string(false, true), "bb");
    EXPECT_EQ(dbus_string(1, 2, 3, true, 1.0), "iiibd");
}

TEST(MessageTypes, StringReferences)
{
    std::string a = "a";
    std::string b = "b";
    const char* c = "c";

    EXPECT_EQ(dbus_string(a, std::move(b), c), "sss");
}

TEST(MessageTypes, ObjectPath)
{
    EXPECT_EQ(dbus_string(sdbusplus::message::object_path("/asdf")), "o");
}

TEST(MessageTypes, ObjectPathValidDefaultConstruct)
{
    const auto objPath1 = sdbusplus::message::object_path();

    // The root path '/' is a valid object path, the empty string is not.
    EXPECT_EQ(objPath1.str, "/");

    const sdbusplus::message::object_path objPath2 = {};
    EXPECT_EQ(objPath2.str, "/");
}

TEST(MessageTypes, ObjectPathFilename)
{
    EXPECT_EQ(sdbusplus::message::object_path("/abc/def").filename(), "def");
    EXPECT_EQ(sdbusplus::message::object_path("/abc/").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/abc").filename(), "abc");
    EXPECT_EQ(sdbusplus::message::object_path("/_61bc").filename(), "abc");
    EXPECT_EQ(sdbusplus::message::object_path("/").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path().filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("abc").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/_2d").filename(), "-");
    EXPECT_EQ(sdbusplus::message::object_path("/_20").filename(), " ");
    EXPECT_EQ(sdbusplus::message::object_path("/_2F").filename(), "/");
    EXPECT_EQ(sdbusplus::message::object_path("/_").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/_2").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/_2y").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/_y2").filename(), "");
    EXPECT_EQ(sdbusplus::message::object_path("/bios_active").filename(),
              "bios_active");
}

TEST(MessageTypes, ObjectPathParent)
{
    EXPECT_EQ(sdbusplus::message::object_path("/abc/def").parent_path(),
              sdbusplus::message::object_path("/abc"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc/").parent_path(),
              sdbusplus::message::object_path("/abc"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc").parent_path(),
              sdbusplus::message::object_path("/"));
    EXPECT_EQ(sdbusplus::message::object_path("/").parent_path(),
              sdbusplus::message::object_path("/"));
    EXPECT_EQ(sdbusplus::message::object_path().parent_path(),
              sdbusplus::message::object_path("/"));
    EXPECT_EQ(sdbusplus::message::object_path().parent_path(),
              sdbusplus::message::object_path());
}

TEST(MessageTypes, ObjectPathOperatorSlash)
{
    EXPECT_EQ(sdbusplus::message::object_path() / "abc",
              sdbusplus::message::object_path("/abc"));
    EXPECT_EQ(sdbusplus::message::object_path("/") / "abc",
              sdbusplus::message::object_path("/abc"));
    EXPECT_EQ(sdbusplus::message::object_path("/") / "abc",
              sdbusplus::message::object_path("/abc"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "def",
              sdbusplus::message::object_path("/abc/def"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "-",
              sdbusplus::message::object_path("/abc/_2d"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / " ",
              sdbusplus::message::object_path("/abc/_20"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "/",
              sdbusplus::message::object_path("/abc/_2f"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "ab_cd",
              sdbusplus::message::object_path("/abc/ab_cd"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "_ab_cd",
              sdbusplus::message::object_path("/abc/_5fab_5fcd"));
    EXPECT_EQ(sdbusplus::message::object_path("/abc") / "ab-c_d",
              sdbusplus::message::object_path("/abc/_61b_2dc_5fd"));

    // Test the std::string overload.  This is largely just for coverage
    EXPECT_EQ(sdbusplus::message::object_path("/") / std::string("abc"),
              sdbusplus::message::object_path("/abc"));
}

TEST(MessageTypes, ObjectPathOperatorSlashEqual)
{
    sdbusplus::message::object_path path("/");
    path /= "abc";
    EXPECT_EQ(path, sdbusplus::message::object_path("/abc"));

    sdbusplus::message::object_path path2("/");
    path2 /= std::string("d-ef");
    EXPECT_EQ(path2, sdbusplus::message::object_path("/_64_2def"));

    sdbusplus::message::object_path path3;
    path3 /= "abc";
    EXPECT_EQ(path3, sdbusplus::message::object_path("/abc"));
}

TEST(MessageTypes, Signature)
{
    EXPECT_EQ(dbus_string(sdbusplus::message::signature("sss")), "g");
}

TEST(MessageTypes, VectorOfString)
{
    std::vector<std::string> s = {"a", "b"};

    EXPECT_EQ(dbus_string(s), "as");
}

TEST(MessageTypes, SetOfString)
{
    std::set<std::string> s = {"a", "b"};

    EXPECT_EQ(dbus_string(s), "as");
}
