#include <net/poettering/Calculator/client.hpp>
#include <sdbusplus/async.hpp>

#include <iostream>

auto startup(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    constexpr auto service = "net.poettering.Calculator";
    constexpr auto path = "/net/poettering/calculator";

    auto c =
        sdbusplus::client::net::poettering::Calculator().service(service).path(
            path);

    // Alternatively, sdbusplus::async::client_t<Calculator, ...>() could have
    // been used to combine multiple interfaces into a single client-proxy.

    {
        auto _ = co_await c.call<int64_t>(ctx, "Multiply", int64_t(7),
                                          int64_t(6));
        std::cout << "Should be 42: " << _ << std::endl;
    }

    {
        auto _ = co_await c.get_property<int64_t>(ctx, "LastResult");
        std::cout << "Should be 42: " << _ << std::endl;
    }

    {
        co_await c.call<>(ctx, "Clear");
    }

    {
        auto _ = co_await c.get_property<int64_t>(ctx, "LastResult");
        std::cout << "Should be 0: " << _ << std::endl;
    }

    {
        co_await c.set_property<int64_t>(ctx, "LastResult", 1234);
        auto _ = co_await c.get_property<int64_t>(ctx, "LastResult");
        std::cout << "Should be 1234: " << _ << std::endl;
    }

    co_return;
}

int main()
{
    sdbusplus::async::context ctx;
    ctx.spawn(startup(ctx));
    ctx.spawn(
        sdbusplus::async::execution::just() |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();

    return 0;
}
