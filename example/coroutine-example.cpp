#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/match.hpp>

#include <iostream>

auto watch_events(sdbusplus::async::context_t& ctx)
    -> sdbusplus::execution::task<void>
{
    using namespace sdbusplus::async::match;
    auto m = match(ctx, rules::nameOwnerChanged());

    while (auto msg = co_await m.next())
    {
        std::string service, old_name, new_name;
        msg.read(service, old_name, new_name);
        if (!new_name.empty())
        {
            std::cout << new_name << " owns " << service << std::endl;
        }
        else
        {
            std::cout << service << " released" << std::endl;
        }
    };

    co_return;
}

int main()
{
    sdbusplus::async::context_t ctx{};

    ctx.startup([&]() { return watch_events(ctx); });
    ctx.run();

    return 0;
}
