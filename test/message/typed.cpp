#include <any>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <sdbusplus/typed.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include <gmock/gmock.h>

namespace sdbusplus
{
namespace typed
{
namespace
{

using testing::DoAll;
using testing::Eq;
using testing::MatcherCast;
using testing::Pointee;
using testing::Return;
using testing::SafeMatcherCast;
using testing::SaveArgPointee;
using testing::SetArgPointee;
using testing::StrEq;

void expect_send(SdBusMock& mock)
{
    EXPECT_CALL(mock, sd_bus_message_get_bus(nullptr))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock, sd_bus_send(nullptr, nullptr, nullptr))
        .WillOnce(Return(0));
}

void expect_call(SdBusMock& mock)
{
    EXPECT_CALL(mock, sd_bus_message_get_bus(nullptr))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock, sd_bus_call(nullptr, nullptr, 0, testing::_, testing::_))
        .WillOnce(Return(0));
}

template <typename T>
void expect_append_basic(SdBusMock& mock, char type, T val)
{
    EXPECT_CALL(mock, sd_bus_message_append_basic(
                          nullptr, type,
                          MatcherCast<const void*>(
                              SafeMatcherCast<const T*>(Pointee(Eq(val))))))
        .WillOnce(Return(0));
}

ACTION_TEMPLATE(AssignReadVal, HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(val))
{
    *static_cast<T*>(arg2) = val;
}

template <typename T>
void expect_read_basic(SdBusMock& mock, char type, T val)
{
    EXPECT_CALL(mock, sd_bus_message_read_basic(nullptr, type, testing::_))
        .WillOnce(DoAll(AssignReadVal<T>(val), Return(0)));
}

TEST(MethodCall, MethodEmpty)
{
    testing::StrictMock<SdBusMock> mock;
    EXPECT_CALL(mock, sd_bus_ref(nullptr)).WillRepeatedly(Return(nullptr));
    {
        testing::InSequence seq;
        expect_call(mock);
    }
    MethodCall<Types<>, Types<>>(
        message::message(nullptr, &mock, std::false_type()))
        .call();
}

TEST(MethodCall, MethodWithParams)
{
    testing::StrictMock<SdBusMock> mock;
    EXPECT_CALL(mock, sd_bus_ref(nullptr)).WillRepeatedly(Return(nullptr));
    {
        testing::InSequence seq;
        expect_append_basic(mock, 'i', 1);
        expect_call(mock);
        expect_read_basic(mock, 'i', 2);
    }
    int v;
    MethodCall<Types<int>, Types<int>>(
        message::message(nullptr, &mock, std::false_type()))
        .append(1)
        .call()
        .read(v);
    EXPECT_EQ(2, v);
}

TEST(MethodCall, SignalEmpty)
{
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        expect_send(mock);
    }
    MethodCall<Types<>, void>(
        message::message(nullptr, &mock, std::false_type()))
        .send();
}

TEST(MethodCall, SignalWithParams)
{
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        expect_append_basic(mock, 'i', 1);
        expect_send(mock);
    }
    MethodCall<Types<int>, void>(
        message::message(nullptr, &mock, std::false_type()))
        .append(1)
        .send();
}

TEST(MethodReturn, Empty)
{
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        expect_send(mock);
    }
    MethodReturn<Types<>>(message::message(nullptr, &mock, std::false_type()))
        .send();
}

TEST(MethodReturn, ReplyUnsent)
{
    testing::StrictMock<SdBusMock> mock;
    MethodReturn<Types<int, int>> reply(
        message::message(nullptr, &mock, std::false_type()));
}

TEST(MethodReturn, ReplySent)
{
    constexpr int val1 = 1, val2 = 2;
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        expect_append_basic(mock, 'i', val1);
        expect_append_basic(mock, 'i', val2);
        expect_send(mock);
    }
    MethodReturn<Types<int, int>> reply(
        message::message(nullptr, &mock, std::false_type()));
    std::move(reply).append(val1).append(val2).send();
}

TEST(MethodReturn, Variant)
{
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        EXPECT_CALL(mock,
                    sd_bus_message_open_container(nullptr, 'v', StrEq("b")))
            .WillOnce(Return(0));
        expect_append_basic(mock, 'b', 1);
        EXPECT_CALL(mock, sd_bus_message_close_container(nullptr))
            .WillOnce(Return(0));
        expect_append_basic(mock, 'i', 1);
        expect_send(mock);
    }
    MethodReturn<Types<std::variant<std::string, bool>, int>> reply(
        message::message(nullptr, &mock, std::false_type()));
    std::move(reply).append(std::variant<bool>(true)).append(1).send();
}

TEST(MethodReturn, Any)
{
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        EXPECT_CALL(mock,
                    sd_bus_message_open_container(nullptr, 'v', StrEq("b")))
            .WillOnce(Return(0));
        expect_append_basic(mock, 'b', 1);
        EXPECT_CALL(mock, sd_bus_message_close_container(nullptr))
            .WillOnce(Return(0));
        expect_append_basic(mock, 'i', 1);
        expect_send(mock);
    }
    MethodReturn<Types<std::any, int>> reply(
        message::message(nullptr, &mock, std::false_type()));
    std::move(reply).append(std::variant<bool>(true)).append(1).send();
}

TEST(MethodReply, NewReturn)
{
    constexpr int v = 5;
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        EXPECT_CALL(mock, sd_bus_message_new_method_return(nullptr, testing::_))
            .WillOnce(DoAll(SetArgPointee<1>(nullptr), Return(0)));
        expect_append_basic(mock, 'i', v);
        expect_send(mock);
    }
    MethodReply<Types<int>>(message::message(nullptr, &mock, std::false_type()))
        .startReturn()
        .append(v)
        .send();
}

TEST(MethodReply, NewError)
{
    constexpr int error = 10;
    testing::StrictMock<SdBusMock> mock;
    {
        testing::InSequence seq;
        EXPECT_CALL(mock, sd_bus_message_new_method_errno(nullptr, testing::_,
                                                          error, nullptr))
            .WillOnce(DoAll(SetArgPointee<1>(nullptr), Return(0)));
        expect_send(mock);
    }
    MethodReply<Types<int>>(message::message(nullptr, &mock, std::false_type()))
        .startError(error)
        .send();
}

} // namespace
} // namespace typed
} // namespace sdbusplus
