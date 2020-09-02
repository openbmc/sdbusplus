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

template <typename Exception, typename F>
std::optional<Exception> captureException(F&& code)
{
    try
    {
        code();
    }
    catch (const Exception& e)
    {
        return e;
    }

    return std::nullopt;
}

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

TYPED_TEST(UnpackPropertiesTest, returnsValueWhenKeyIsPresentAndTypeMatches)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    unpackProperties(this->data, "Key-1", val1, "Key-2", val2, "Key-3", val3);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackChangesOriginalDataWhenPassedAsNonConstReference)
{
    using namespace testing;

    std::string val1, val2;

    unpackProperties(this->data, "Key-1", val1);
    unpackProperties(this->data, "Key-1", val2);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, Not(Eq("string")));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackDoesntChangeOriginalDataWhenPassesAsConstReference)
{
    using namespace testing;

    std::string val1, val2;

    unpackProperties(Const(this->data), "Key-1", val1);
    unpackProperties(Const(this->data), "Key-1", val2);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, Eq("string"));
}

TYPED_TEST(UnpackPropertiesTest, throwsErrorWhenKeyIsMissing)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto error = captureException<exception::UnpackPropertyError>([&] {
        unpackProperties(this->data, "Key-1", val1, "Key-4", val2, "Key-3",
                         val3);
    });

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason,
                Eq(exception::UnpackPropertyError::reasonMissingProperty));
    ASSERT_THAT(error->propertyName, Eq("Key-4"));
}

TYPED_TEST(UnpackPropertiesTest, throwsErrorWhenTypeDoesntMatch)
{
    using namespace testing;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    auto error = captureException<exception::UnpackPropertyError>([&] {
        unpackProperties(this->data, "Key-1", val1, "Key-2", val2, "Key-3",
                         val3);
    });

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason,
                Eq(exception::UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-2"));
}

TYPED_TEST(UnpackPropertiesTest,
           returnsUndefinedValueForDuplicatedKeysWhenDataIsNonConstReference)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;
    std::string val4;

    unpackProperties(this->data, "Key-1", val1, "Key-2", val2, "Key-3", val3,
                     "Key-1", val4);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
    ASSERT_THAT(val4, Not(Eq("string")));
}

TYPED_TEST(UnpackPropertiesTest,
           returnsValueForDuplicatedKeysWhenDataIsConstReference)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;
    std::string val4;

    unpackProperties(Const(this->data), "Key-1", val1, "Key-2", val2, "Key-3",
                     val3, "Key-1", val4);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
    ASSERT_THAT(val4, Eq("string"));
}

struct UnpackPropertiesTest_ForVector :
    public UnpackPropertiesTest<
        std::vector<std::pair<std::string, VariantType>>>
{};

TEST_F(UnpackPropertiesTest_ForVector, silentlyDiscardsDuplicatedKeyInData)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    this->data.insert(this->data.end(),
                      std::make_pair("Key-1"s, VariantType("string2"s)));

    unpackProperties(this->data, "Key-1", val1, "Key-2", val2, "Key-3", val3);

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

} // namespace sdbusplus
