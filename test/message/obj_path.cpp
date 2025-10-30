#include "sdbusplus/message/obj_path.hpp"

#include <type_traits>

#include <gtest/gtest.h>

TEST(ObjPath, ObjectPathFilename)
{
    // compile-time object path does not assume to be encoded
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc/def">().filename(), "def");
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">().filename(), "abc");
    EXPECT_EQ(sdbusplus::message::obj_path<"/">().filename(), "");
    EXPECT_EQ(sdbusplus::message::obj_path<"/bios_active">().filename(),
              "bios_active");

    // TODO: check that filename should be constexpr
}

TEST(ObjPath, IsValidObjectPath)
{
    EXPECT_TRUE(sdbusplus::message::isValidObjectPath<"/">());
    EXPECT_TRUE(sdbusplus::message::isValidObjectPath<"/abc">());
    EXPECT_TRUE(sdbusplus::message::isValidObjectPath<"/abc/328">());

    EXPECT_FALSE(sdbusplus::message::isValidObjectPath<"">());
    EXPECT_FALSE(sdbusplus::message::isValidObjectPath<"abc">());
    EXPECT_FALSE(sdbusplus::message::isValidObjectPath<"/abc/">());
    EXPECT_FALSE(sdbusplus::message::isValidObjectPath<"/abc**">());
}

TEST(ObjPath, SafeByConstruction)
{
    // constexpr DBus path fails to compile if the path is not valid on DBus
    // Try to uncomment following lines

    // sdbusplus::message::obj_path<"/abc**">();

    // sdbusplus::message::obj_path<"">();

    // sdbusplus::message::obj_path<"/abc/">();
}

TEST(ObjPath, ObjectPathParent)
{
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc/def">().parent_path(),
              sdbusplus::message::obj_path<"/abc">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">().parent_path(),
              sdbusplus::message::obj_path<"/">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/">().parent_path(),
              sdbusplus::message::obj_path<"/">());

    // TODO: check that parent path is constexpr
}

// obj_path should constexpr concatenate to itself
TEST(ObjPath, ObjectPathOperatorSlashConstexprSelf_Simple1)
{
    constexpr auto v1 = sdbusplus::message::obj_path<"/">() /
                        sdbusplus::message::obj_path<"/abc">();

    constexpr auto v2 = sdbusplus::message::obj_path<"/abc">();

    EXPECT_EQ(v1, v2);

    static_assert(std::is_same_v<decltype(v1), decltype(v2)>);

    const bool b = std::is_same_v<decltype(v1), decltype(v2)>;
    EXPECT_TRUE(b);
}

// obj_path should constexpr concatenate to itself
TEST(ObjPath, ObjectPathOperatorSlashConstexprSelf_Simple2)
{
    constexpr auto v1 = sdbusplus::message::obj_path<"/abc">() /
                        sdbusplus::message::obj_path<"/def">();

    constexpr auto v2 = sdbusplus::message::obj_path<"/abc/def">();

    EXPECT_EQ(v1, v2);

    const bool b = std::is_same_v<decltype(v1), decltype(v2)>;
    EXPECT_TRUE(b);
}

// obj_path has the expected concatenation semantics as object_path when
// concatenating runtime values
TEST(ObjPath, ObjectPathOperatorSlash)
{
    EXPECT_EQ(sdbusplus::message::obj_path<"/">() / "abc",
              sdbusplus::message::obj_path<"/abc">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/">() / "abc",
              sdbusplus::message::obj_path<"/abc">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "def",
              sdbusplus::message::obj_path<"/abc/def">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "-",
              sdbusplus::message::obj_path<"/abc/_2d">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / " ",
              sdbusplus::message::obj_path<"/abc/_20">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "/",
              sdbusplus::message::obj_path<"/abc/_2f">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "ab_cd",
              sdbusplus::message::obj_path<"/abc/ab_cd">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "_ab_cd",
              sdbusplus::message::obj_path<"/abc/_5fab_5fcd">());
    EXPECT_EQ(sdbusplus::message::obj_path<"/abc">() / "ab-c_d",
              sdbusplus::message::obj_path<"/abc/_61b_2dc_5fd">());

    // Test the std::string overload.  This is largely just for coverage
    EXPECT_EQ(sdbusplus::message::obj_path<"/">() / std::string("abc"),
              sdbusplus::message::obj_path<"/abc">());
}
