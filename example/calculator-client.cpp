#include <net/poettering/Calculator/client.hpp>
#include <sdbusplus/async.hpp>

#include <iostream>

auto startup(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    constexpr auto service = "net.poettering.Calculator";
    constexpr auto path = "/net/poettering/calculator";

    auto c = sdbusplus::client::net::poettering::Calculator(ctx)
                 .service(service)
                 .path(path);

    // Alternatively, sdbusplus::async::client_t<Calculator, ...>() could have
    // been used to combine multiple interfaces into a single client-proxy.
    auto alternative_c [[maybe_unused]] =
        sdbusplus::async::client_t<
            sdbusplus::client::net::poettering::Calculator>(ctx)
            .service(service)
            .path(path);

    {
        // Call the Multiply method.
        auto _ = co_await c.multiply(7, 6);
        std::cout << "Should be 42: " << _ << std::endl;
    }

    {
        // Get the LastResult property.
        auto _ = co_await c.lastResult();
        std::cout << "Should be 42: " << _ << std::endl;
    }

    {
        // Call the Clear method.
        co_await c.clear();
    }

    {
        // Get the LastResult property.
        auto _ = co_await c.lastResult();
        std::cout << "Should be 0: " << _ << std::endl;
    }

    {
        // Set the LastResult property.
        co_await c.lastResult(1234);
        // Get the LastResult property.
        auto _ = co_await c.lastResult();
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
