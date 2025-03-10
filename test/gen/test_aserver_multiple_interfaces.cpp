#include "server/Test/aserver.hpp"
#include "server/Test/client.hpp"
#include "server/Test2/aserver.hpp"
#include "server/Test2/client.hpp"
#include "server/Test3/aserver.hpp"

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/match.hpp>
#include <sdbusplus/async/timer.hpp>

class A
{};

auto constructInterfaces(sdbusplus::async::context& ctx, std::string& busName)
    -> sdbusplus::async::task<>
{
    auto x = std::variant<sdbusplus::common::server::Test::EnumOne, std::string,
                          sdbusplus::common::server::Test::EnumTwo>(
        sdbusplus::common::server::Test::EnumOne::OneA);

    sdbusplus::common::server::Test::properties_t props{
        832, 0, 0, 0, 0, 0, 0, std::string("/my/path"), 1.0, 1.0, 1.0, 1.0, x};

    sdbusplus::common::server::Test2::properties_t props2{4200, 2};

    sdbusplus::aserver::server::Test<A> a0(ctx, "/0");

    sdbusplus::aserver::server::Test<A> a1(ctx, "/1", props);

    // construct without properties
    sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test> a2(
        ctx, "/2");

    // construct Test with properties
    auto a3 = std::make_unique<
        sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test>>(
        ctx, "/3", props);

    // construct Test2 with properties
    sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test2> a4(
        ctx, "/4", props2);

    // does not compile, (wronge type of properties!)
    // sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test2> ax(ctx,
    // "/x", props);

    // construct with both interfaces, no properties
    sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test,
                               sdbusplus::aserver::server::Test2>
        a5(ctx, "/5");

    // construct with both interfaces and properties
    auto a6 = std::make_unique<
        sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test,
                                   sdbusplus::aserver::server::Test2>>(
        ctx, "/6", props, props2);

    // does not compile, (wrong order of properties!)
    // auto a7 = std::make_unique<
    //    sdbusplus::async::server_t<A, sdbusplus::aserver::server::Test,
    //                               sdbusplus::aserver::server::Test2>>(
    //    ctx, "/7", props2, props);

    auto a8 = std::make_unique<sdbusplus::async::server_t<
        A, sdbusplus::aserver::server::Test, sdbusplus::aserver::server::Test2,
        sdbusplus::aserver::server::Test3>>(ctx, "/8", props, props2,
                                            std::nullopt);

    auto client =
        sdbusplus::client::server::Test(ctx).service(busName).path("/3");

    assert(co_await client.some_value() == 832);

    auto client2 =
        sdbusplus::client::server::Test2(ctx).service(busName).path("/6");

    assert(co_await client2.new_value() == 4200);

    ctx.request_stop();

    co_return;
}

int main()
{
    sdbusplus::async::context ctx;

    std::srand(std::chrono::system_clock::now().time_since_epoch().count());
    std::string busName = "xyz.openbmc_project.TestingMultipleInterfaces" +
                          std::to_string(rand());

    ctx.request_name(busName.c_str());

    sdbusplus::server::manager_t manager(ctx.get_bus(), "/test");

    ctx.spawn(constructInterfaces(ctx, busName));

    ctx.run();

    return 0;
}
