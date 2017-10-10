#include "dbus.hpp"

#include <memory>
#include "dbus-mocks.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "ipmid-test-utils.hpp"

using std::string;
using std::unique_ptr;
using ::testing::DoAll;
using ::testing::Exactly;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::WithArgs;
using ::testing::_;

namespace ipmid
{

class DBusTest : public ::testing::Test
{
    protected:
        DBusTest() :
            message_ops_(new NiceMock<MockDBusMessageOperations>),
            bus_ops_(new NiceMock<MockDBusBusOperations>),
            dbus_(unique_ptr<DBusMessageOperations>(message_ops_),
                  unique_ptr<DBusBusOperations>(bus_ops_)) {}

        MockDBusMessageOperations* message_ops_;
        MockDBusBusOperations* bus_ops_;
        DBus dbus_;
};

TEST_F(DBusTest, InitAndCleanUp)
{
    MockDBusMessageOperations* message_ops = new
    NiceMock<MockDBusMessageOperations>;
    MockDBusBusOperations* bus_ops = new NiceMock<MockDBusBusOperations>;
    sd_bus* bus = reinterpret_cast<sd_bus*>(646464868467486874);

    EXPECT_CALL(*bus_ops, sd_bus_open_system(_))
    .WillOnce(DoAll(SetArgPointee<0>(bus),
                    Return(0)));
    EXPECT_CALL(*bus_ops, sd_bus_unref(bus))
    .Times(Exactly(1));
    {
        DBus dbus(std::move(unique_ptr<DBusMessageOperations>(message_ops)),
                  std::move(unique_ptr<DBusBusOperations>(bus_ops)));
        EXPECT_EQ(dbus.Init(), 0);
    }

    message_ops = new NiceMock<MockDBusMessageOperations>;
    bus_ops = new NiceMock<MockDBusBusOperations>;
    EXPECT_CALL(*bus_ops, sd_bus_open_system(_))
    .WillOnce(Return(-1));
    EXPECT_CALL(*bus_ops, sd_bus_unref(nullptr))
    .Times(Exactly(1));
    {
        DBus dbus(std::move(unique_ptr<DBusMessageOperations>(message_ops)),
                  std::move(unique_ptr<DBusBusOperations>(bus_ops)));
        EXPECT_EQ(dbus.Init(), -1);
    }
}

TEST_F(DBusTest, RegisterHandlerSlotCleanUp)
{
    MockDBusMessageOperations* message_ops = new MockDBusMessageOperations;
    MockDBusBusOperations* bus_ops = new MockDBusBusOperations;
    sd_bus* bus = reinterpret_cast<sd_bus*>(5646874646464864);

    EXPECT_CALL(*bus_ops, sd_bus_open_system(_))
    .WillOnce(DoAll(SetArgPointee<0>(bus),
                    Return(0)));
    EXPECT_CALL(*bus_ops, sd_bus_unref(bus))
    .Times(Exactly(1));
    EXPECT_CALL(*bus_ops, sd_bus_slot_unref(nullptr))
    .Times(Exactly(1));
    {
        DBus dbus(std::move(unique_ptr<DBusMessageOperations>(message_ops)),
                  std::move(unique_ptr<DBusBusOperations>(bus_ops)));
        EXPECT_EQ(dbus.Init(), 0);
    }

    message_ops = new MockDBusMessageOperations;
    bus_ops = new MockDBusBusOperations;
    sd_bus_slot* bus_slot = reinterpret_cast<sd_bus_slot*>(123546468734378);
    const string match = "match";
    MockDBusHandler handler;
    EXPECT_CALL(*bus_ops, sd_bus_open_system(_))
    .WillOnce(DoAll(SetArgPointee<0>(bus),
                    Return(0)));
    EXPECT_CALL(*bus_ops, sd_bus_add_match(bus, _, match.c_str(), _, _))
    .WillOnce(DoAll(SetArgPointee<1>(bus_slot),
                    Return(0)));
    EXPECT_CALL(*bus_ops, sd_bus_unref(bus))
    .Times(Exactly(1));
    EXPECT_CALL(*bus_ops, sd_bus_slot_unref(bus_slot))
    .Times(Exactly(1));
    {
        DBus dbus(std::move(unique_ptr<DBusMessageOperations>(message_ops)),
                  std::move(unique_ptr<DBusBusOperations>(bus_ops)));
        EXPECT_EQ(dbus.Init(), 0);
        EXPECT_EQ(dbus.RegisterHandler(match, &handler), 0);
    }
}

TEST_F(DBusTest, RegisterHandler)
{
    const string match = "different match";
    MockDBusHandler handler;
    SdBusMessageHandlerT dbus_callback;
    void* context;
    EXPECT_CALL(*bus_ops_, sd_bus_add_match(_, _, match.c_str(), _, _))
    .WillOnce(WithArgs<3, 4>(
            Invoke([&dbus_callback, &context](SdBusMessageHandlerT dbus_callback_,
                                              void* context_) -> int
    {
        dbus_callback = dbus_callback_;
        context = context_;
        return 0;
    })));
    dbus_.RegisterHandler(match, &handler);

    sd_bus_message* message = reinterpret_cast<sd_bus_message*>(8791384615468);
    void* error = reinterpret_cast<void*>(1568445464164);
    EXPECT_CALL(handler, HandleMessage(Ref(*message_ops_), message))
    .Times(Exactly(1));
    dbus_callback(message, context, error);
}

TEST_F(DBusTest, CallMethodWhenNewMessageFails)
{
    MockDBusInput input;
    MockDBusOutput output;
    DBusMemberInfo info;
    info.destination = "dest";
    info.path = "path";
    info.interface = "interface";
    info.member = "member";

    EXPECT_CALL(*bus_ops_, sd_bus_message_new_method_call(
                _,
                _,
                info.destination.c_str(),
                info.path.c_str(),
                info.interface.c_str(),
                info.member.c_str()))
    .WillOnce(Return(-1));
    EXPECT_FALSE(dbus_.CallMethod(info, input, &output));
}

TEST_F(DBusTest, CallMethodWhenSdBusCallFails)
{
    MockDBusInput input;
    MockDBusOutput output;
    sd_bus_message* request = reinterpret_cast<sd_bus_message*>(6874619684);
    DBusMemberInfo info;
    info.destination = "new_dest";
    info.path = "new_path";
    info.interface = "new_interface";
    info.member = "new_member";

    {
        InSequence sequence;
        EXPECT_CALL(*bus_ops_, sd_bus_message_new_method_call(
                    _,
                    _,
                    info.destination.c_str(),
                    info.path.c_str(),
                    info.interface.c_str(),
                    info.member.c_str()))
        .WillOnce(DoAll(SetArgPointee<1>(request),
                        Return(0)));
        EXPECT_CALL(input, Compose(Ref(*message_ops_), request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_call(_, request, 0, _, _))
        .WillOnce(Return(-1));
        EXPECT_CALL(*bus_ops_, sd_bus_error_free(_))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(nullptr))
        .Times(Exactly(1));
        EXPECT_FALSE(dbus_.CallMethod(info, input, &output));
    }
}

TEST_F(DBusTest, CallMethodWhenParsingResponseFails)
{
    MockDBusInput input;
    MockDBusOutput output;
    sd_bus_message* request = reinterpret_cast<sd_bus_message*>(68435146436156);
    sd_bus_message* response = reinterpret_cast<sd_bus_message*>(4687461468754);
    DBusMemberInfo info;
    info.destination = "different dest";
    info.path = "different path";
    info.interface = "different interface";
    info.member = "different member";

    {
        InSequence sequence;
        EXPECT_CALL(*bus_ops_, sd_bus_message_new_method_call(
                    _,
                    _,
                    info.destination.c_str(),
                    info.path.c_str(),
                    info.interface.c_str(),
                    info.member.c_str()))
        .WillOnce(DoAll(SetArgPointee<1>(request),
                        Return(1)));
        EXPECT_CALL(input, Compose(Ref(*message_ops_), request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_call(_, request, 0, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(response),
                        Return(0)));
        EXPECT_CALL(output, Parse(Ref(*message_ops_), response))
        .WillOnce(Return(false));
        EXPECT_CALL(*bus_ops_, sd_bus_error_free(_))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(response))
        .Times(Exactly(1));
        EXPECT_FALSE(dbus_.CallMethod(info, input, &output));
    }
}

TEST_F(DBusTest, CallMethodSuccess)
{
    MockDBusInput input;
    MockDBusOutput output;
    sd_bus_message* request = reinterpret_cast<sd_bus_message*>(8641644367463);
    sd_bus_message* response = reinterpret_cast<sd_bus_message*>(68413574631);
    DBusMemberInfo info;
    info.destination = "different dest";
    info.path = "different path";
    info.interface = "different interface";
    info.member = "different member";

    {
        InSequence sequence;
        EXPECT_CALL(*bus_ops_, sd_bus_message_new_method_call(
                    _,
                    _,
                    info.destination.c_str(),
                    info.path.c_str(),
                    info.interface.c_str(),
                    info.member.c_str()))
        .WillOnce(DoAll(SetArgPointee<1>(request),
                        Return(1)));
        EXPECT_CALL(input, Compose(Ref(*message_ops_), request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_call(_, request, 0, _, _))
        .WillOnce(DoAll(SetArgPointee<4>(response),
                        Return(0)));
        EXPECT_CALL(output, Parse(Ref(*message_ops_), response))
        .WillOnce(Return(true));
        EXPECT_CALL(*bus_ops_, sd_bus_error_free(_))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(request))
        .Times(Exactly(1));
        EXPECT_CALL(*bus_ops_, sd_bus_message_unref(response))
        .Times(Exactly(1));
        EXPECT_TRUE(dbus_.CallMethod(info, input, &output));
    }
}

} // namespace ipmid
