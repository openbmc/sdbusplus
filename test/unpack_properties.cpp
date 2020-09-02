#include <boost/container/flat_map.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <gmock/gmock.h>

namespace sdbusplus
{

using VariantType = std::variant<std::string, uint32_t, float, double>;
using ContainerTypes =
    testing::Types<std::vector<std::pair<std::string, VariantType>>,
                   boost::container::flat_map<std::string, VariantType>,
                   std::map<std::string, VariantType>>;

template <typename Container>
struct UnpackPropertiesTest : public testing::Test
{
    void SetUp() override
    {
        using namespace std::string_literals;

        data.insert(data.end(),
                    std::make_pair("Key-1"s, VariantType("string"s)));
        data.insert(data.end(), std::make_pair("Key-2"s, VariantType(42.f)));
        data.insert(data.end(), std::make_pair("Key-3"s, VariantType(15.)));
    }

    Container data;
};

TYPED_TEST_SUITE(UnpackPropertiesTest, ContainerTypes);

TYPED_TEST(UnpackPropertiesTest,
           unpacksWithNoErrorWhenKeyIsPresentAndTypeMatches)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    ASSERT_THAT(unpackProperties(this->data, "Key-1", val1, "Key-2", val2,
                                 "Key-3", val3),
                Eq(std::nullopt));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackChangesOriginalDataWhenPassedAsNonConstReference)
{
    using namespace testing;

    std::string val1, val2;

    ASSERT_THAT(unpackProperties(this->data, "Key-1", val1), Eq(std::nullopt));
    ASSERT_THAT(unpackProperties(this->data, "Key-1", val2), Eq(std::nullopt));

    ASSERT_THAT(val1, Not(Eq(val2)));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackDoesntChangeOriginalDataWhenPassesAsConstReference)
{
    using namespace testing;

    std::string val1, val2;

    ASSERT_THAT(unpackProperties(Const(this->data), "Key-1", val1),
                Eq(std::nullopt));
    ASSERT_THAT(unpackProperties(Const(this->data), "Key-1", val2),
                Eq(std::nullopt));

    ASSERT_THAT(val1, Eq(val2));
}

TYPED_TEST(UnpackPropertiesTest, silentlyDiscardsDuplicatedKeyInData)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    this->data.insert(this->data.end(),
                      std::make_pair("Key-1"s, VariantType("string2"s)));

    auto error = unpackProperties(this->data, "Key-1", val1, "Key-2", val2,
                                  "Key-3", val3);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

TYPED_TEST(UnpackPropertiesTest, returnsErrorWhenKeyIsMissing)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    std::optional<UnpackPropertyError> error = unpackProperties(
        this->data, "Key-1", val1, "Key-4", val2, "Key-3", val3);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonMissingProperty));
    ASSERT_THAT(error->propertyName, Eq("Key-4"));
}

TYPED_TEST(UnpackPropertiesTest, returnsErrorWhenTypeDoesntMatch)
{
    using namespace testing;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    std::optional<UnpackPropertyError> error = unpackProperties(
        this->data, "Key-1", val1, "Key-2", val2, "Key-3", val3);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-2"));
}

TYPED_TEST(UnpackPropertiesTest,
           returnsErrorWhenKeyInParametersAppearsMultipleTimes)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto error = unpackProperties(this->data, "Key-1", val1, "Key-2", val2,
                                  "Key-3", val3, "Key-1", val1);

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason, Eq(UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-1"));
}

} // namespace sdbusplus
