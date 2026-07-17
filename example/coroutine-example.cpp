#include <sdbusplus/async.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

/* Demonstrate the runtime object_server: stand up a dynamic interface with
 * properties, methods, and a signal, then call it back over the bus.
 *
 * The server outlives this coroutine (it is owned by startup) so the
 * published objects stay on the bus for the process lifetime. */
auto object_server_example(sdbusplus::async::context& ctx,
                           sdbusplus::async::object_server& server)
    -> sdbusplus::async::task<>
{
    constexpr auto service = "xyz.openbmc_project.CoroutineExample";
    constexpr auto path = "/xyz/openbmc_project/coroutine_example";
    constexpr auto interface = "xyz.openbmc_project.Example";

    // Register a dynamic interface and publish it.
    server.add_manager("/xyz/openbmc_project");

    auto& intf = server.add_interface(path, interface);

    // Read-only for external clients; the owner updates via set_property.
    intf.register_property<uint64_t>("Version", 1);

    // Externally writable; any written value is accepted as-is.
    intf.register_property<std::string>(
        "Mode", "auto", sdbusplus::async::property_permission::read_write);

    // Externally writable through a hook, which can transform the written
    // value or throw to reject it.
    intf.register_property<uint32_t>("Limit", 50, [](uint32_t requested) {
        return std::min(requested, 100u);
    });

    // Coroutine methods: one result, no result, multiple results.
    intf.register_method(
        "Add", [](int32_t a, int32_t b) -> sdbusplus::async::task<int32_t> {
            co_return a + b;
        });
    intf.register_method("Ping", []() -> sdbusplus::async::task<> {
        co_return;
    });
    intf.register_method(
        "MinMax",
        [](std::vector<int32_t> v)
            -> sdbusplus::async::task<std::tuple<int32_t, int32_t>> {
            auto [min, max] = std::ranges::minmax_element(v);
            co_return {*min, *max};
        });

    // Signal with one payload.
    intf.register_signal<uint64_t>("VersionChanged");

    intf.initialize();
    intf.emit_added();

    ctx.request_name(service);

    // Call our own interface over the bus.
    constexpr auto self =
        sdbusplus::async::proxy().service(service).path(path).interface(
            interface);

    auto sum = co_await self.call<int32_t>(ctx, "Add", int32_t{2}, int32_t{3});
    std::cout << "Add(2, 3) = " << sum << std::endl;

    co_await self.call<>(ctx, "Ping");

    auto [min, max] = co_await self.call<int32_t, int32_t>(
        ctx, "MinMax", std::vector<int32_t>{3, 1, 4, 1, 5});
    std::cout << "MinMax = " << min << ", " << max << std::endl;

    auto version = co_await self.get_property<uint64_t>(ctx, "Version");
    std::cout << "Version = " << version << std::endl;

    // Write properties over the bus; "Limit" goes through the hook.
    co_await self.set_property(ctx, "Mode", std::string{"manual"});
    std::cout << "Mode = " << intf.get_property<std::string>("Mode")
              << std::endl;

    co_await self.set_property(ctx, "Limit", uint32_t{500});
    std::cout << "Limit = " << intf.get_property<uint32_t>("Limit")
              << std::endl;

    // Update a property owner-side (emits PropertiesChanged) and fire a
    // signal.
    intf.set_property<uint64_t>("Version", 2);
    intf.emit_signal("VersionChanged", uint64_t{2});

    co_return;
}

auto startup(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    // Demonstrate serving a runtime-defined interface.  Claiming the bus
    // name can fail (e.g. no policy on the system bus); the rest of the
    // example does not depend on it.
    sdbusplus::async::object_server server{ctx};
    try
    {
        co_await object_server_example(ctx, server);
    }
    catch (const std::exception& e)
    {
        std::cout << "object_server example skipped: " << e.what() << std::endl;
    }

    // Create a proxy to the systemd manager object.
    constexpr auto systemd = sdbusplus::async::proxy()
                                 .service("org.freedesktop.systemd1")
                                 .path("/org/freedesktop/systemd1")
                                 .interface("org.freedesktop.systemd1.Manager");

    // Call ListUnitFiles method.
    using ret_type = std::vector<std::tuple<std::string, std::string>>;
    auto files = co_await systemd.call<ret_type>(ctx, "ListUnitFiles");
    for (auto& [file, status] : files)
    {
        std::cout << file << " " << status << std::endl;
    }

    // Get the Architecture property.
    std::cout << co_await systemd.get_property<std::string>(ctx, "Architecture")
              << std::endl;

    // Get all the properties.
    using variant_type =
        std::variant<bool, std::string, std::vector<std::string>, uint64_t,
                     int32_t, uint32_t, double>;
    auto properties = co_await systemd.get_all_properties<variant_type>(ctx);
    for (auto& [property, value] : properties)
    {
        std::cout
            << property << " is "
            << std::visit(
                   // Convert the variant member to a string for printing.
                   [](auto v) {
                       if constexpr (std::is_same_v<
                                         std::remove_cvref_t<decltype(v)>,
                                         std::vector<std::string>>)
                       {
                           return std::string{"Array"};
                       }
                       else if constexpr (std::is_same_v<
                                              std::remove_cvref_t<decltype(v)>,
                                              std::string>)
                       {
                           return v;
                       }
                       else
                       {
                           return std::to_string(v);
                       }
                   },
                   value)
            << std::endl;
    }

    // Try to set the Architecture property (which won't work).
    try
    {
        co_await systemd.set_property(ctx, "Architecture", "arm64");
    }
    catch (const std::exception& e)
    {
        std::cout << "Caught exception because you cannot set Architecture: "
                  << e.what() << std::endl;
    }

    // Create a match for the NameOwnerChanged signal.
    namespace rules = sdbusplus::match_rules;
    auto match = sdbusplus::async::match(ctx, rules::nameOwnerChanged());

    // Listen for the signal 4 times...
    for (size_t i = 0; i < 4; ++i)
    {
        auto nextResult =
            co_await match.next<std::string, std::string, std::string>();

        auto [service, old_name, new_name] = std::move(nextResult);

        if (!new_name.empty())
        {
            std::cout << new_name << " owns " << service << std::endl;
        }
        else
        {
            std::cout << service << " released" << std::endl;
        }
    }

    // We are all done, so shutdown the server.
    ctx.request_stop();

    co_return;
}

int main()
{
    sdbusplus::async::context ctx;
    ctx.spawn(startup(ctx));
    ctx.run();

    return 0;
}
