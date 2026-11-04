#include <sdbusplus/unpack_properties.hpp>
#include <xyz/openbmc_project/sdbusplus/Error/event.hpp>

#include <gtest/gtest.h>

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
    struct UnpackError
    {
        UnpackError(sdbusplus::UnpackErrorReason r, const std::string& p) :
            reason(r), property(p)
        {}
        sdbusplus::UnpackErrorReason reason;
        std::string property;
    };

    template <typename... Args>
    std::optional<UnpackError> operator()(Args&&... args) const
    {
        std::optional<UnpackError> error;
        unpackPropertiesNoThrow(
            [&error](sdbusplus::UnpackErrorReason reason,
                     const std::string& property) {
                error.emplace(reason, property);
            },
            std::forward<Args>(args)...);
        return error;
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
    TestingTypes<ThrowingUnpack,
                 std::vector<std::pair<std::string, VariantType>>>>;

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
    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2",
                                            val2, "Key-3", val3));

    ASSERT_EQ(val1, "string");
    ASSERT_FLOAT_EQ(val2, 42.f);
    ASSERT_DOUBLE_EQ(val3, 15.);
}

TYPED_TEST(UnpackPropertiesTest,
           unpackDoesntChangeOriginalDataWhenPassedAsNonConstReference)
{
    std::string val1, val2;

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1));
    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val2));

    ASSERT_EQ(val1, "string");
    ASSERT_EQ(val2, "string");
}

TYPED_TEST(UnpackPropertiesTest, doesntReportMissingPropertyForOptional)
{
    std::optional<std::string> val1;
    std::optional<std::string> val4;

    EXPECT_FALSE(
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-4", val4));

    ASSERT_EQ(val1, "string");
    ASSERT_EQ(val4, std::nullopt);
}

TYPED_TEST(UnpackPropertiesTest, setPresentPointersOnSuccess)
{
    const std::string* val1 = nullptr;
    const float* val2 = nullptr;
    const double* val3 = nullptr;
    const std::string* val4 = nullptr;

    EXPECT_FALSE(
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2", val2,
                                   "Key-3", val3, "Key-4", val4));

    ASSERT_TRUE(val1 && val2 && val3);
    ASSERT_TRUE(!val4);

    ASSERT_EQ(*val1, "string");
    ASSERT_FLOAT_EQ(*val2, 42.f);
    ASSERT_DOUBLE_EQ(*val3, 15.);
}

template <typename Params>
struct UnpackPropertiesThrowingTest : public UnpackPropertiesTest<Params>
{};

using ContainerTypesThrowing = testing::Types<TestingTypes<
    ThrowingUnpack, std::vector<std::pair<std::string, VariantType>>>>;

TYPED_TEST_SUITE(UnpackPropertiesThrowingTest, ContainerTypesThrowing);

TYPED_TEST(UnpackPropertiesThrowingTest, throwsErrorWhenKeyIsMissing)
{
    using UnpackError = ::sdbusplus::error::xyz::openbmc_project::sdbusplus::
        Error::UnpackPropertyError;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto error = captureException<UnpackError>([&] {
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-4", val2,
                                   "Key-3", val3);
    });
    ASSERT_TRUE(error.has_value());
    if (!error.has_value())
        return; // structural guard just for the analyzer
    ASSERT_EQ(error->reason, "missingProperty");
    ASSERT_EQ(error->propertyName, "Key-4");
}

TYPED_TEST(UnpackPropertiesThrowingTest, throwsErrorWhenTypeDoesntMatch)
{
    using UnpackError = ::sdbusplus::error::xyz::openbmc_project::sdbusplus::
        Error::UnpackPropertyError;

    std::string val1;
    std::string val2;
    double val3 = 0.;

    auto error = captureException<UnpackError>([&] {
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2", val2,
                                   "Key-3", val3);
    });
    ASSERT_TRUE(error.has_value());
    if (!error.has_value())
        return; // structural guard just for the analyzer
    ASSERT_EQ(error->reason, "wrongType");
    ASSERT_EQ(error->propertyName, "Key-2");
}

TYPED_TEST(UnpackPropertiesThrowingTest, throwsErrorWhenOptionalTypeDoesntMatch)
{
    using UnpackError = ::sdbusplus::error::xyz::openbmc_project::sdbusplus::
        Error::UnpackPropertyError;

    std::optional<std::string> val1;
    std::optional<std::string> val2;

    auto error = captureException<UnpackError>([&] {
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2", val2);
    });
    ASSERT_TRUE(error.has_value());
    if (!error.has_value())
        return; // structural guard just for the analyzer
    ASSERT_EQ(error->reason, "wrongType");
    ASSERT_EQ(error->propertyName, "Key-2");
}

template <typename Params>
struct UnpackPropertiesNonThrowingTest : public UnpackPropertiesTest<Params>
{};

using ContainerTypesNonThrowing = testing::Types<TestingTypes<
    NonThrowingUnpack, std::vector<std::pair<std::string, VariantType>>>>;

TYPED_TEST_SUITE(UnpackPropertiesNonThrowingTest, ContainerTypesNonThrowing);

TYPED_TEST(UnpackPropertiesNonThrowingTest, ErrorWhenKeyIsMissing)
{
    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    auto badProperty = this->unpackPropertiesCall(this->data, "Key-1", val1,
                                                  "Key-4", val2, "Key-3", val3);
    ASSERT_TRUE(badProperty.has_value());
    if (!badProperty.has_value())
        return; // structural guard just for the analyzer
    EXPECT_EQ(badProperty->reason, UnpackErrorReason::missingProperty);
    EXPECT_EQ(badProperty->property, "Key-4");
}

TYPED_TEST(UnpackPropertiesNonThrowingTest, ErrorWhenTypeDoesntMatch)
{
    std::string val1;
    std::string val2;
    double val3 = 0.;

    auto badProperty = this->unpackPropertiesCall(this->data, "Key-1", val1,
                                                  "Key-2", val2, "Key-3", val3);
    ASSERT_TRUE(badProperty.has_value());
    if (!badProperty.has_value())
        return; // structural guard just for the analyzer
    EXPECT_EQ(badProperty->reason, UnpackErrorReason::wrongType);
    EXPECT_EQ(badProperty->property, "Key-2");
}

TYPED_TEST(UnpackPropertiesNonThrowingTest, ErrorWhenOptionalTypeDoesntMatch)
{
    std::optional<std::string> val1;
    std::optional<std::string> val2;

    auto badProperty =
        this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2", val2);
    ASSERT_TRUE(badProperty.has_value());
    if (!badProperty.has_value())
        return; // structural guard just for the analyzer
    EXPECT_EQ(badProperty->reason, UnpackErrorReason::wrongType);
    EXPECT_EQ(badProperty->property, "Key-2");
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
    using namespace std::string_literals;

    std::string val1;
    float val2 = 0.f;
    double val3 = 0.;

    this->data.insert(this->data.end(),
                      std::make_pair("Key-1"s, VariantType("string2"s)));

    EXPECT_FALSE(this->unpackPropertiesCall(this->data, "Key-1", val1, "Key-2",
                                            val2, "Key-3", val3));

    ASSERT_EQ(val1, "string");
    ASSERT_FLOAT_EQ(val2, 42.f);
    ASSERT_DOUBLE_EQ(val3, 15.);
}

} // namespace sdbusplus
