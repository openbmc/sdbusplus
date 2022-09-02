#include <sdbusplus/bus.hpp>
#include <server/Test/server.hpp>

#include <gtest/gtest.h>

using TestIf = sdbusplus::server::server::Test;

struct Object : public ::testing::Test
{
    sdbusplus::bus_t bus = sdbusplus::bus::new_default();
    sdbusplus::message_t msg = bus.new_method_call(
        "xyz.openbmc_project.sdbusplus.test.Object",
        "/xyz/openbmc_project/sdbusplus/test/object",
        "xyz.openbmc_project.sdbusplus.test.Object", "Unused");

    using variant_t =
        std::variant<TestIf::EnumOne, std::string, TestIf::EnumTwo>;

    template <typename V, typename T>
    void run_test(const T& value)
    {
        const V data = value;
        msg.append(data);
        ASSERT_EQ(0, sd_bus_message_seal(msg.get(), 0, 0));

        V new_data = {};
        msg.read(new_data);

        EXPECT_EQ(data, new_data);
    }

    template <typename V1, typename V2, typename T>
    void run_test_throw_bad_enum(const T& value)
    {
        const V1 data = value;
        msg.append(data);
        ASSERT_EQ(0, sd_bus_message_seal(msg.get(), 0, 0));

        V2 new_data = {};
        EXPECT_THROW(msg.read(new_data),
                     sdbusplus::exception::InvalidEnumString);
    }
};

static_assert(
    sdbusplus::message::details::has_convert_from_string_v<TestIf::EnumOne>,
    "EnumOne does not have convert_from_string!");
static_assert(
    sdbusplus::message::details::has_convert_from_string_v<TestIf::EnumTwo>,
    "EnumTwo does not have convert_from_string!");
static_assert(!sdbusplus::message::details::has_convert_from_string_v<size_t>,
              "size_t unexpectedly has a convert_from_string!");
static_assert(sdbusplus::message::details::has_convert_from_string_v<
                  TestIf::PropertiesVariant>,
              "TestIf::PropertiesVariant does not convert_from_string!");

TEST_F(Object, PlainEnumOne)
{
    run_test<TestIf::EnumOne>(TestIf::EnumOne::OneA);
}

TEST_F(Object, PlainEnumTwo)
{
    run_test<TestIf::EnumTwo>(TestIf::EnumTwo::TwoB);
}

TEST_F(Object, EnumOneAsEnumTwoThrows)
{
    run_test_throw_bad_enum<TestIf::EnumOne, TestIf::EnumTwo>(
        TestIf::EnumOne::OneA);
}

TEST_F(Object, EnumTwoAsEnumOneThrows)
{
    run_test_throw_bad_enum<TestIf::EnumTwo, TestIf::EnumOne>(
        TestIf::EnumTwo::TwoB);
}

TEST_F(Object, VariantAsString)
{
    run_test<variant_t>(std::string("Hello"));
}

TEST_F(Object, VariantAsEnumOne)
{
    run_test<variant_t>(TestIf::EnumOne::OneA);
}

TEST_F(Object, VariantAsEnumTwo)
{
    run_test<variant_t>(TestIf::EnumTwo::TwoB);
}
