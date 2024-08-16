#include <sdbusplus/async.hpp>

#include <iostream>
#include <string>
#include <variant>
#include <vector>

auto startup(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    // Create a proxy to the systemd manager object.
    constexpr auto systemd = sdbusplus::async::proxy()
                                 .service("org.freedesktop.systemd1")
                                 .path("/org/freedesktop/systemd1")
                                 .interface("org.freedesktop.systemd1.Manager");

    // Call ListUnitFiles method.
    using ret_type = std::vector<std::tuple<std::string, std::string>>;
    for (auto& [file, status] :
         co_await systemd.call<ret_type>(ctx, "ListUnitFiles"))
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
    for (auto& [property, value] :
         co_await systemd.get_all_properties<variant_type>(ctx))
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
    namespace rules = sdbusplus::bus::match::rules;
    auto match = sdbusplus::async::match(ctx, rules::nameOwnerChanged());

    // Listen for the signal 4 times...
    for (size_t i = 0; i < 4; ++i)
    {
        auto [service, old_name, new_name] =
            co_await match.next<std::string, std::string, std::string>();

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
