#ifndef DBUS_MOCKS_HPP_
#define DBUS_MOCKS_HPP_

#include "dbus.hpp"
#include "gmock/gmock.h"

namespace ipmid
{

class MockDBusMessageOperations : public DBusMessageOperations
{
    public:
        MOCK_CONST_METHOD3(sd_bus_message_append_basic,
                           int(sd_bus_message*, char, const void*));

        MOCK_CONST_METHOD4(sd_bus_message_append_array,
                           int(sd_bus_message*, char, const void*, size_t));

        MOCK_CONST_METHOD3(sd_bus_message_read_basic,
                           int(sd_bus_message*, char, void*));

        MOCK_CONST_METHOD4(sd_bus_message_read_array,
                           int(sd_bus_message*, char, const void**, size_t*));

        MOCK_CONST_METHOD1(sd_bus_message_get_sender, const char* (sd_bus_message*));

        MOCK_CONST_METHOD1(sd_bus_message_get_path, const char* (sd_bus_message*));

        MOCK_CONST_METHOD3(sd_bus_message_enter_container,
                           int(sd_bus_message*, char, const char*));
};

struct DBusError {};

class MockDBusBusOperations : public DBusBusOperations
{
    public:
        MOCK_CONST_METHOD1(sd_bus_open_system, int(sd_bus**));

        MOCK_CONST_METHOD1(sd_bus_unref, sd_bus * (sd_bus*));

        MOCK_CONST_METHOD1(sd_bus_slot_unref, sd_bus_slot * (sd_bus_slot*));

        MOCK_CONST_METHOD3(mapper_get_service, int(sd_bus*, const char*, char**));

        MOCK_CONST_METHOD6(sd_bus_message_new_method_call,
                           int(sd_bus*, sd_bus_message**, const char*, const char*, const char*,
                               const char*));

        MOCK_CONST_METHOD5(sd_bus_call,
                           int(sd_bus*, sd_bus_message*, uint64_t, void*, sd_bus_message**));

        MOCK_CONST_METHOD1(sd_bus_error_free, void(void*));

        MOCK_CONST_METHOD1(sd_bus_message_unref, sd_bus_message * (sd_bus_message*));

        MOCK_CONST_METHOD5(sd_bus_add_match,
                           int(sd_bus*, sd_bus_slot**, const char*,
                               SdBusMessageHandlerT, void*));

        MOCK_CONST_METHOD2(sd_bus_process, int(sd_bus*, sd_bus_message**));

        MOCK_CONST_METHOD2(sd_bus_wait, int(sd_bus*, uint64_t));

        DBusErrorUniquePtr MakeError() const override
        {
            return DBusErrorUniquePtr(reinterpret_cast<DBusError*>(malloc(sizeof(uint64_t))));
        }

        MOCK_CONST_METHOD1(DBusErrorToString, std::string(const DBusError&));
};

class MockDBusHandler : public DBusHandler
{
    public:
        MOCK_METHOD2(HandleMessage, int(const DBusMessageOperations&, sd_bus_message*));
};

class MockDBusInput : public DBusInput
{
    public:
        MOCK_CONST_METHOD2(Compose, void(const DBusMessageOperations&,
                                         sd_bus_message*));
};

class MockDBusOutput : public DBusOutput
{
    public:
        MOCK_METHOD2(Parse, bool(const DBusMessageOperations&, sd_bus_message*));
};

} // namespace ipmid
#endif // DBUS_MOCKS_HPP_
