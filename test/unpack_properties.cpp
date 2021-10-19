#include <boost/container/flat_map.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <gmock/gmock.h>

namespace sdbusplus
{

struct ThrowingUnpack
{
    template <typename... Args>
    bool operator()(Args&&... args) const
    {
        unpackProperties(std::forward<Args>(args)...);
        return false;
    }
};

struct NonThrowingUnpack
{
    template <typename... Args>
    std::optional<std::string> operator()(Args&&... args) const
    {
        return unpackPropertiesNoThrow(std::forward<Args>(args)...);
    }
};

template <typename A, typename B>
struct TestingTypes
{
    using SystemUnderTest = A;
    using Container = B;
};

using VariantType = std::variant<std::string, uint32_t, float, double>;
using ContainerTypes = testing::Types<
    TestingTypes<NonThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>,
    TestingTypes<NonThrowingUnpack,
                 boost::container::flat_map<std::string, VariantType>>,
    TestingTypes<NonThrowingUnpack, std::map<std::string, VariantType>>,
    TestingTypes<ThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>,
    TestingTypes<ThrowingUnpack,
                 boost::container::flat_map<std::string, VariantType>>,
    TestingTypes<ThrowingUnpack, std::map<std::string, VariantType>>>;

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

template <typename Params>
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

    typename Params::Container data;
    typename Params::SystemUnderTest unpackPropertiesCall;
};

TYPED_TEST_SUITE(UnpackPropertiesTest, ContainerTypes);

TYPED_TEST(UnpackPropertiesTest, returnsValueWhenKeyIsPresentAndTypeMatches)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2",
                                            val2, "Key-3", val3));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackChangesOriginalDataWhenPassedAsNonConstReference)
{
    using namespace testing;

    std::string val1, val2;

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1));
    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val2));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, Not(Eq("string")));
}

TYPED_TEST(UnpackPropertiesTest,
           unpackDoesntChangeOriginalDataWhenPassesAsConstReference)
{
    using namespace testing;

    std::string val1, val2;

    EXPECT_FALSE(this->unpackPropertiesCall(Const(this->data), "Key-1", val1));
    EXPECT_FALSE(this->unpackPropertiesCall(Const(this->data), "Key-1", val2));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, Eq("string"));
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

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2",
                                            val2, "Key-3", val3, "Key-1",
                                            val4));

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

    EXPECT_FALSE(this->unpackPropertiesCall(Const(this->data), "Key-1", val1,
                                            "Key-2", val2, "Key-3", val3,
                                            "Key-1", val4));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
    ASSERT_THAT(val4, Eq("string"));
}

template <typename Params>
struct UnpackPropertiesThrowingTest : public UnpackPropertiesTest<Params>
{};

using ContainerTypesThrowing = testing::Types<
    TestingTypes<ThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>,
    TestingTypes<ThrowingUnpack,
                 boost::container::flat_map<std::string, VariantType>>,
    TestingTypes<ThrowingUnpack, std::map<std::string, VariantType>>>;

TYPED_TEST_SUITE(UnpackPropertiesThrowingTest, ContainerTypesThrowing);

TYPED_TEST(UnpackPropertiesThrowingTest, throwsErrorWhenKeyIsMissing)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto error = captureException<exception::UnpackPropertyError>([&] {
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-4", val2,
                                   "Key-3", val3);
    });

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason,
                Eq(exception::UnpackPropertyError::reasonMissingProperty));
    ASSERT_THAT(error->propertyName, Eq("Key-4"));
}

TYPED_TEST(UnpackPropertiesThrowingTest, throwsErrorWhenTypeDoesntMatch)
{
    using namespace testing;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    auto error = captureException<exception::UnpackPropertyError>([&] {
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2", val2,
                                   "Key-3", val3);
    });

    ASSERT_TRUE(error);
    ASSERT_THAT(error->reason,
                Eq(exception::UnpackPropertyError::reasonTypeNotMatched));
    ASSERT_THAT(error->propertyName, Eq("Key-2"));
}

template <typename Params>
struct UnpackPropertiesNonThrowingTest : public UnpackPropertiesTest<Params>
{};

using ContainerTypesNonThrowing = testing::Types<
    TestingTypes<NonThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>,
    TestingTypes<NonThrowingUnpack,
                 boost::container::flat_map<std::string, VariantType>>,
    TestingTypes<NonThrowingUnpack, std::map<std::string, VariantType>>>;

TYPED_TEST_SUITE(UnpackPropertiesNonThrowingTest, ContainerTypesNonThrowing);

TYPED_TEST(UnpackPropertiesNonThrowingTest, ErrorWhenKeyIsMissing)
{
    using namespace testing;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto badProperty = this->unpackPropertiesCall(this->data, "Key-1", val1,
                                                  "Key-4", val2, "Key-3", val3);

    ASSERT_TRUE(badProperty);
    ASSERT_THAT(*badProperty, Eq("Key-4"));
}

TYPED_TEST(UnpackPropertiesNonThrowingTest, ErrorWhenTypeDoesntMatch)
{
    using namespace testing;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    auto badProperty = this->unpackPropertiesCall(this->data, "Key-1", val1,
                                                  "Key-2", val2, "Key-3", val3);

    ASSERT_TRUE(badProperty);
    ASSERT_THAT(*badProperty, Eq("Key-2"));
}

template <typename Params>
struct UnpackPropertiesTest_ForVector : public UnpackPropertiesTest<Params>
{};

using ContainerTypesVector = testing::Types<
    TestingTypes<NonThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>,
    TestingTypes<ThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>>;

TYPED_TEST_SUITE(UnpackPropertiesTest_ForVector, ContainerTypesVector);

TYPED_TEST(UnpackPropertiesTest_ForVector, silentlyDiscardsDuplicatedKeyInData)
{
    using namespace testing;
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    this->data.insert(this->data.end(),
                      std::make_pair("Key-1"s, VariantType("string2"s)));

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2",
                                            val2, "Key-3", val3));

    ASSERT_THAT(val1, Eq("string"));
    ASSERT_THAT(val2, FloatEq(42.f));
    ASSERT_THAT(val3, DoubleEq(15.));
}

} // namespace sdbusplus
