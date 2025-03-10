#include "server/Test/aserver.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/async/timer.hpp>

#include <gtest/gtest.h>

class A : public sdbusplus::aserver::server::Test<A>
{};

std::string busName = "xyz.openbmc_project.TODOmyrandom";

sdbusplus::async::task<void> waitForMatch(sdbusplus::async::context& ctx,
                                          sdbusplus::async::match& ifcAdded)
{
    auto x = co_await ifcAdded.next();

    ctx.request_stop();
}

sdbusplus::async::task<void> shouldEmitSignal(sdbusplus::async::context& ctx)
{
    auto x =
        std::variant<A::EnumOne, std::string, A::EnumTwo>(A::EnumOne::OneA);

    sdbusplus::common::server::Test::properties_t props{
        0, 0, 0, 0, 0, 0, 0, std::string("/my/path"), 1.0, 1.0, 1.0, 1.0, x};

    auto t2 = std::make_unique<sdbusplus::aserver::server::Test<A>>(
        ctx, "/test/something", props);

    co_await sdbusplus::async::sleep_for(ctx, std::chrono::seconds(1));

    co_return;
}

int main()
{
    sdbusplus::async::context ctx;

    ctx.request_name(busName.c_str());

    sdbusplus::server::manager_t manager(ctx.get_bus(), "/test");

    sdbusplus::async::match ifcAdded(
        ctx, sdbusplus::bus::match::rules::interfacesAdded() +
                 sdbusplus::bus::match::rules::sender(busName));

    ctx.spawn(waitForMatch(ctx, ifcAdded));
    ctx.spawn(shouldEmitSignal(ctx));

    ctx.run();

    return 0;
}
