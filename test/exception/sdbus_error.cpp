#include <systemd/sd-bus.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

// Needed for constructor error testing
extern sdbusplus::SdBusImpl sdbus_impl;

namespace
{

using sdbusplus::exception::SdBusError;
using testing::_;
using testing::Return;

TEST(SdBusError, BasicErrno)
{
    const int errorVal = EBUSY;
    const std::string prefix = "BasicErrno";

    // Build the reference sd_bus_error
    sd_bus_error error = SD_BUS_ERROR_NULL;
    EXPECT_EQ(-errorVal, sd_bus_error_set_errno(&error, errorVal));
    EXPECT_TRUE(sd_bus_error_is_set(&error));

    // Build the SdBusError
    SdBusError err(errorVal, prefix.c_str());

    // Make sure inheritance is defined correctly
    sdbusplus::exception::exception& sdbusErr = err;
    SdBusError& errNew = err;
    EXPECT_EQ(errorVal, errNew.get_errno());
    EXPECT_EQ(std::string{error.name}, sdbusErr.name());
    EXPECT_EQ(std::string{error.message}, sdbusErr.description());
    std::exception& stdErr = sdbusErr;
    EXPECT_EQ(prefix + ": " + error.name + ": " + error.message, stdErr.what());

    sd_bus_error_free(&error);
}

TEST(SdBusError, EnomemErrno)
{
    // Make sure no exception is thrown on construction
    SdBusError err(ENOMEM, "EnomemErrno");
}

TEST(SdBusError, NotSetErrno)
{
    const int errorVal = EBUSY;

    sdbusplus::SdBusMock sdbus;
    EXPECT_CALL(sdbus, sd_bus_error_set_errno(_, errorVal))
        .Times(1)
        .WillOnce(Return(errorVal));
    EXPECT_CALL(sdbus, sd_bus_error_is_set(_)).Times(1).WillOnce(Return(false));
    EXPECT_THROW(SdBusError(errorVal, "NotSetErrno", &sdbus),
                 std::runtime_error);
}

TEST(SdBusError, Move)
{
    const int errorVal = EIO;
    const std::string prefix = "Move";

    // Build the reference sd_bus_error
    sd_bus_error error = SD_BUS_ERROR_NULL;
    EXPECT_EQ(-errorVal, sd_bus_error_set_errno(&error, errorVal));
    EXPECT_TRUE(sd_bus_error_is_set(&error));
    const std::string name{error.name};
    const std::string message{error.message};
    const std::string what = prefix + ": " + error.name + ": " + error.message;

    SdBusError errFinal(EBUSY, "Move2");
    // Nest to make sure RAII works for moves
    {
        // Build our first SdBusError
        SdBusError err(errorVal, prefix.c_str());

        EXPECT_EQ(errorVal, err.get_errno());
        EXPECT_EQ(name, err.name());
        EXPECT_EQ(message, err.description());
        EXPECT_EQ(what, err.what());

        // Move our SdBusError to a new one
        SdBusError errNew(std::move(err));

        // We are purposefully calling functions on a moved-from object,
        // so we need to suppress the clang warnings.
#ifndef __clang_analyzer__
        // Ensure the old object was cleaned up
        EXPECT_EQ(0, err.get_errno());
        EXPECT_EQ(nullptr, err.name());
        EXPECT_EQ(nullptr, err.description());
#endif

        // Ensure our new object has the same data but moved
        EXPECT_EQ(errorVal, errNew.get_errno());
        EXPECT_EQ(name, errNew.name());
        EXPECT_EQ(message, errNew.description());
        EXPECT_EQ(what, errNew.what());

        // Move our SdBusError using the operator=()
        errFinal = std::move(errNew);

        // We are purposefully calling functions on a moved-from object,
        // so we need to suppress the clang warnings.
#ifndef __clang_analyzer__
        // Ensure the old object was cleaned up
        EXPECT_EQ(0, errNew.get_errno());
        EXPECT_EQ(nullptr, errNew.name());
        EXPECT_EQ(nullptr, errNew.description());
#endif
    }

    // Ensure our new object has the same data but moved
    EXPECT_EQ(errorVal, errFinal.get_errno());
    EXPECT_EQ(name, errFinal.name());
    EXPECT_EQ(message, errFinal.description());
    EXPECT_EQ(what, errFinal.what());

    sd_bus_error_free(&error);
}

TEST(SdBusError, BasicError)
{
    const std::string name = "org.freedesktop.DBus.Error.Failed";
    const std::string description = "TestCase";
    const std::string prefix = "BasicError";

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_error_set(&error, name.c_str(), description.c_str());
    EXPECT_TRUE(sd_bus_error_is_set(&error));
    const char* nameBeforeMove = error.name;
    const int errorVal = sd_bus_error_get_errno(&error);
    SdBusError err(&error, prefix.c_str());

    // We expect a move not copy
    EXPECT_EQ(nameBeforeMove, err.name());

    // The SdBusError should have moved our error so it should be freeable
    EXPECT_FALSE(sd_bus_error_is_set(&error));
    sd_bus_error_free(&error);
    sd_bus_error_free(&error);

    EXPECT_EQ(errorVal, err.get_errno());
    EXPECT_EQ(name, err.name());
    EXPECT_EQ(description, err.description());
    EXPECT_EQ(prefix + ": " + name + ": " + description, err.what());
}

TEST(SdBusError, CatchBaseClassExceptions)
{
    /* test each class in the chain:
     * std::exception
     *   -> sdbusplus::exception::exception
     *     -> sdbusplus::exception::internal_exception
     *       -> sdbusplus::exception::SdBusError
     */
    EXPECT_THROW({ throw SdBusError(-EINVAL, "SdBusError"); }, SdBusError);
    EXPECT_THROW(
        { throw SdBusError(-EINVAL, "internal_exception"); },
        sdbusplus::exception::internal_exception);
    EXPECT_THROW(
        { throw SdBusError(-EINVAL, "exception"); },
        sdbusplus::exception::exception);
    EXPECT_THROW(
        { throw SdBusError(-EINVAL, "std::exception"); }, std::exception);
}

} // namespace

#ifdef __clang__
#pragma clang diagnostic pop
#endif
