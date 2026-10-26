#include <sdbusplus/bus.hpp>
#include <server/Aggregate/common.hpp>
#include <server/Test/common.hpp>

#include <gtest/gtest.h>

namespace
{

class PropertyVariantTest : public testing::Test
{
  protected:
    sdbusplus::bus_t bus = sdbusplus::bus::new_default();

    template <typename Source, typename Expected>
    void roundTrip(const Source& source, const Expected& expected)
    {
        auto message = bus.new_method_call("example.Service", "/example",
                                           "example.Interface", "Unused");
        message.append(std::variant<Source>{source});
        ASSERT_EQ(0, sd_bus_message_seal(message.get(), 1, 0));

        auto value = message.unpack<sdbusplus::PropertyVariant>();
        const auto* result = std::get_if<Expected>(&value);
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(*result, expected);
    }
};

TEST_F(PropertyVariantTest, NumericArray)
{
    const std::vector<uint64_t> numbers{1, 2, 3};
    roundTrip(numbers, numbers);
}

TEST_F(PropertyVariantTest, Associations)
{
    const std::vector<std::tuple<std::string, std::string, std::string>>
        associations{{"running", "ran_on", "/example/inventory"}};
    roundTrip(associations, associations);
}

TEST_F(PropertyVariantTest, EnumAsString)
{
    roundTrip(sdbusplus::common::server::Test::EnumOne::OneA,
              std::string("server.Test.EnumOne.OneA"));
}

TEST_F(PropertyVariantTest, EnumArrayAsStringArray)
{
    const std::vector<sdbusplus::common::server::Test::EnumOne> states{
        sdbusplus::common::server::Test::EnumOne::OneB};
    roundTrip(states, std::vector<std::string>{"server.Test.EnumOne.OneB"});
}

TEST_F(PropertyVariantTest, VariantProperty)
{
    const std::variant<sdbusplus::common::server::Test::EnumOne, std::string>
        state{sdbusplus::common::server::Test::EnumOne::OneA};
    roundTrip(state, std::variant<bool, std::string, uint32_t, uint64_t>{
                         "server.Test.EnumOne.OneA"});
}

TEST_F(PropertyVariantTest, MergedVariantProperty)
{
    const std::variant<uint64_t, bool> option{uint64_t{42}};
    roundTrip(option, std::variant<bool, std::string, uint32_t, uint64_t>{
                          uint64_t{42}});
}

TEST_F(PropertyVariantTest, ObjectManagerReply)
{
    using Objects = sdbusplus::common::server::Aggregate::
        get_managed_objects_t::return_type;
    const Objects objects{
        {sdbusplus::object_path("/example/device"),
         {{"example.Interface", {{"Numbers", std::vector<uint64_t>{4, 5}}}}}}};
    auto message = bus.new_method_call("example.Service", "/example",
                                       "example.Interface", "Unused");
    message.append(objects);
    ASSERT_STREQ(message.get_signature(), "a{oa{sa{sv}}}");
    ASSERT_EQ(0, sd_bus_message_seal(message.get(), 1, 0));
    EXPECT_EQ(message.unpack<Objects>(), objects);
}

} // namespace
