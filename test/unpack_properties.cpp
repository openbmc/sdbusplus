#include <sdbusplus/unpack_properties.hpp>

#include <gmock/gmock.h>

namespace sdbusplus
{

struct UnpackPropertiesTest : public testing::Test
{
    void SetUp() override
    {
        using namespace std::string_literals;

        data.emplace_back("Key-1"s, VariantType("string"s));
        data.emplace_back("Key-2"s, VariantType(42.f));
        data.emplace_back("Key-3"s, VariantType(15.));
    }

    using VariantType = std::variant<std::string, uint32_t, float, double>;
    std::vector<std::pair<std::string, VariantType>> emptyData;
    std::vector<std::pair<std::string, VariantType>> data;
};

TEST_F(UnpackPropertiesTest, unpacksWithNoErrorWhenKeyIsPresentAndTypeMatches)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    ASSERT_THAT(
        unpackProperties(data, "Key-1", val1, "Key-2", val2, "Key-3", val3),
        Eq(std::nullopt));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

TEST_F(UnpackPropertiesTest, returnsErrorWhenKeyIsMissing)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    std::optional<UnpackPropertyError> error =
        unpackProperties(data, "Key-1", val1, "Key-4", val2, "Key-3", val3);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonMissingProperty));
    ASSERT_THAT(error->propertyName, Eq("Key-4"));
}

TEST_F(UnpackPropertiesTest, returnsErrorWhenTypeDoesntMatch)
{
    using namespace testing;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    std::optional<UnpackPropertyError> error =
        unpackProperties(data, "Key-1", val1, "Key-2", val2, "Key-3", val3);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-2"));
}

TEST_F(UnpackPropertiesTest, returnsErrorWhenKeyInDataAppearsMultipleTimes)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    data.emplace_back("Key-1"s, VariantType("string2"s));

    auto error =
        unpackProperties(data, "Key-1", val1, "Key-2", val2, "Key-3", val3);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason,
                Eq(UnpackPropertyError::reasonPropertyAppearsMoreThanOnce));
    ASSERT_THAT(error->propertyName, Eq("Key-1"));
}

TEST_F(UnpackPropertiesTest,
       returnsErrorWhenKeyInParametersAppearsMultipleTimes)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto error = unpackProperties(data, "Key-1", val1, "Key-2", val2, "Key-3",
                                  val3, "Key-1", val1);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-1"));
}

} // namespace sdbusplus
