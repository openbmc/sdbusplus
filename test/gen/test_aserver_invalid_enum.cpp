#include <sdbusplus/async.hpp>
#include <server/TestWithMethod/aserver.hpp>

#include <algorithm>
#include <cstring>
#include <string>
#include <thread>

#include <gtest/gtest.h>

class A : public sdbusplus::aserver::server::TestWithMethod<A>
{
  public:
    using sdbusplus::aserver::server::TestWithMethod<A>::TestWithMethod;

    bool method_call(sdbusplus::common::server::TestWithMethod::update_value_t,
                     EnumOne)
    {
        ADD_FAILURE();
        return false;
    }
};

/** @brief Wait for the server to be ready on a given bus name. */
auto waitForService(sdbusplus::async::context& ctx, const std::string& name)
    -> sdbusplus::async::task<>
{
    // Create a method call with an invalid enum string.
    auto this_service = sdbusplus::async::proxy()
                            .service(name)
                            .path("/xyz/openbmc_project/test/aserver")
                            .interface("server.TestWithMethod");

    try
    {
        co_await this_service.call<bool>(ctx, "UpdateValue",
                                         "InvalidEnumValue");
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        // The generated callback should catch the InvalidEnumString,
        // call e.set_error(error), and return it to the client.
        if (std::strcmp(
                e.name(),
                "xyz.openbmc_project.sdbusplus.Error.InvalidEnumString") == 0)
        {
            ctx.request_stop();
            co_return;
        }
    }

    ctx.request_stop();
    ADD_FAILURE();
    co_return;
}

TEST(AServerInvalidEnum, CheckMethodCallThrows)
{
    sdbusplus::async::context ctx;

    auto bus_name = std::format("xyz.openbmc_project.TestingInvalidEnum_{}",
                                std::this_thread::get_id());

    ctx.request_name(bus_name.c_str());

    A server(ctx, "/xyz/openbmc_project/test/aserver");

    ctx.spawn(waitForService(ctx, bus_name));
    ctx.run();
}
