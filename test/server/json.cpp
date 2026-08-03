#include <nlohmann/json.hpp>
#include <server/Test/event.hpp>
#include <server/Test2/common.hpp>

#include <string>
#include <type_traits>

#include <gtest/gtest.h>

using EnumOne = sdbusplus::common::server::Test::EnumOne;
using EnumTwo = sdbusplus::common::server::Test::EnumTwo;

TEST(JsonEnum, EnumOneRoundTrip)
{
    EnumOne e = EnumOne::OneA;
    nlohmann::json j = e;
    EXPECT_EQ(j, "server.Test.EnumOne.OneA");
    EXPECT_EQ(j.get<EnumOne>(), e);
}

TEST(JsonEnum, EnumTwoRoundTrip)
{
    EnumTwo e = EnumTwo::TwoB;
    nlohmann::json j = e;
    EXPECT_EQ(j, "server.Test.EnumTwo.TwoB");
    EXPECT_EQ(j.get<EnumTwo>(), e);
}

TEST(JsonEnum, EnumToJsonToString)
{
    nlohmann::json j = EnumOne::OneB;
    EXPECT_EQ(j.dump(), "\"server.Test.EnumOne.OneB\"");
}

TEST(JsonEnum, InvalidEnumStringThrows)
{
    nlohmann::json j = "server.Test.EnumOne.NoSuchValue";
    EXPECT_THROW(j.get<EnumOne>(), sdbusplus::exception::InvalidEnumString);
}

TEST(JsonEvent, EnumAndObjectPathMetadataRoundTrip)
{
    using Error = sdbusplus::error::server::Test::EnumObjectPathError;

    Error e(Error::metadata_t<"ENUM_VALUE">{"ENUM_VALUE"}, EnumOne::OneA,
            Error::metadata_t<"OBJECT_PATH">{"OBJECT_PATH"},
            sdbusplus::object_path("/a/b/"));

    auto j = e.to_json();
    const auto& self = j.at(Error::errName);
    EXPECT_EQ(self.at("ENUM_VALUE"), "server.Test.EnumOne.OneA");
    EXPECT_EQ(self.at("OBJECT_PATH"), "/a/b/");

    Error e2(j, std::source_location::current());
    EXPECT_EQ(e2.enumValue, EnumOne::OneA);
    EXPECT_EQ(e2.objectPath, sdbusplus::object_path("/a/b/"));
}

TEST(JsonProperties, Test2PropertiesRoundTrip)
{
    using Props = sdbusplus::common::server::Test2::properties_t;

    Props p;
    p.new_value = 42;
    p.other_value = 7;

    nlohmann::json j = p;
    EXPECT_EQ(j.at("NewValue"), 42);
    EXPECT_EQ(j.at("OtherValue"), 7);

    auto round = j.get<Props>();
    EXPECT_EQ(round.new_value, 42);
    EXPECT_EQ(round.other_value, 7);
}

static_assert(
    std::is_same_v<
        sdbusplus::common::server::Test2::properties_t::interface_type,
        sdbusplus::common::server::Test2>,
    "properties_t::interface_type must name the owning interface class");

TEST(JsonProperties, Test2InterfaceType)
{
    using Props = sdbusplus::common::server::Test2::properties_t;
    EXPECT_EQ(Props::interface_type::interface, "server.Test2");
}
