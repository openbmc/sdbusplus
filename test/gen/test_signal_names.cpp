#include "sdbusplus/asio/connection.hpp"
#include "sdbusplus/bus/match.hpp"
#include "server/Test2/common.hpp"

#include <print>

#include <gtest/gtest.h>

void cb(sdbusplus::message_t& msg)
{
    // We can access the signal name as a symbol.
    // The property name is constexpr.

    constexpr auto sigName =
        sdbusplus::common::server::Test2::signal_names::other_value_changed;

    // The property name can be used as part of error logs.

    std::println("signal {} received on path {}\n", sigName, msg.get_path());
}

int main()
{
    // If the signal is removed from the interface definition, it will cause a
    // build failure in applications still using that signal. That can work
    // even if the application is not (yet) using PDI-generated bindings for
    // it's DBus interactions.

    std::println(
        "using signal {} \n",
        sdbusplus::common::server::Test2::signal_names::other_value_changed);

    EXPECT_EQ(
        sdbusplus::common::server::Test2::signal_names::other_value_changed,
        "OtherValueChanged");

    boost::asio::io_context io;
    sdbusplus::asio::connection conn(io);

    std::string matchStr = std::format(
        "type='signal',member={}",
        sdbusplus::common::server::Test2::signal_names::other_value_changed);

    auto match = sdbusplus::bus::match_t(conn, matchStr, cb);

    return 0;
}
