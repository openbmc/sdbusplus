#include <sdbusplus/async.hpp>
#include <sdbusplus/server/interface.hpp>

#include <chrono>
#include <iostream>

constexpr auto fooProperty = "Foo";
constexpr auto service = "xyz.openbmc_project.MatchIssueDemo";
constexpr auto path = "/xyz/openbmc_project/match_issue_demo";
constexpr auto interface = service;

std::optional<sdbusplus::async::match> match;
size_t stop_count = 0;

auto timer(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    using namespace std::literals;
    while (1)
    {
        // Picking a prime number so we see activity every few seconds
        // plus we get an indication that the match-await is getting properly
        // cancelled upon destruction of the match optional.
        if (stop_count % 101 == 1)
        {
            std::cout << "Stop count: " << stop_count << std::endl;
        }

        co_await sdbusplus::async::sleep_for(ctx, 10ms);

        match.emplace(ctx, sdbusplus::bus::match::rules::propertiesChanged(
                               path, interface));
        ctx.spawn([]() -> sdbusplus::async::task<> {
            while (1)
            {
                co_await match->next();
            }
        }() | sdbusplus::async::execution::upon_stopped([]() {
            stop_count++;
        }));
    };
}

struct Foo
{
    sdbusplus::async::context& ctx;
    sdbusplus::server::interface_t intf;
    std::string foo = "foo";

    explicit Foo(sdbusplus::async::context& ctx) :
        ctx(ctx), intf(ctx, path, interface, _vtable, this)
    {}

    static int callback_get_foo(sd_bus*, const char*, const char*, const char*,
                                sd_bus_message* reply, void* context,
                                sd_bus_error*)
    {
        auto self = static_cast<Foo*>(context);

        auto m = sdbusplus::message_t{reply};
        m.append(self->foo);

        return 1;
    }

    static int callback_set_foo(sd_bus*, const char*, const char*, const char*,
                                sd_bus_message* value, void* context,
                                sd_bus_error*)
    {
        auto self = static_cast<Foo*>(context);

        auto m = sdbusplus::message_t{value};

        self->foo = m.unpack<std::string>();
        self->intf.property_changed(fooProperty);

        return 1;
    }

    static constexpr sdbusplus::vtable_t _vtable[]{
        sdbusplus::vtable::start(),
        sdbusplus::vtable::property(fooProperty, "s", callback_get_foo,
                                    callback_set_foo),
        sdbusplus::vtable::end(),
    };
};

std::unique_ptr<Foo> foo;

auto setup(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    ctx.request_name(service);
    ctx.spawn(timer(ctx));
    foo = std::make_unique<Foo>(ctx);
    co_return;
}

int main()
{
    sdbusplus::async::context ctx;

    ctx.spawn(setup(ctx));
    ctx.run();

    return 0;
}
