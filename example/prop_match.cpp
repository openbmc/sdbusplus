#include <sdbusplus/bus.hpp>
#include <sdbusplus/property.hpp>
#include <sdbusplus/timer.hpp>
#include <xyz/openbmc_project/Example/server.hpp>

#include <iostream>
#include <thread>

using namespace sdbusplus;
using namespace std::chrono_literals;

using Example = server::xyz::openbmc_project::Example;
using ExampleBase = server::object_t<Example>;
struct ExampleServer : ExampleBase
{
    ExampleServer(bus_t& bus) : ExampleBase(bus, Example::instance_path) {}
};

int main()
{
    //
    // Server code - update a property value every second
    //

    std::jthread serverThread([] {
        auto serverBus = bus::new_default();
        serverBus.request_name(Example::default_service);
        server::manager_t objManager(serverBus, "/");

        std::this_thread::sleep_for(1s);

        ExampleServer example(serverBus);

        while (true)
        {
            auto oldValue = example.exampleInt();
            auto newValue = oldValue + 1;
            std::cout << "\nServer: Updating ExampleInt to " << newValue
                      << '\n';
            example.exampleInt(newValue);

            auto variant = example.exampleVariant();
            if (newValue % 2 == 0)
            {
                variant = "string";
            }
            else
            {
                variant = 0xdeadbeef;
            }
            std::visit(
                [](auto&& val) {
                std::cout << "Server: Updating ExampleVariant to " << val
                          << '\n';
            },
                variant);
            example.exampleVariant(variant);

            std::this_thread::sleep_for(2s);
        }
    });

    //
    // Monitor the value of ExampleInt until it goes over 5
    //

    auto clientBus = bus::new_default();
    std::cout << "Subscribing to ExampleInt\n";
    PropertyMatch intMatch;
    intMatch = subscribeToProperty<int64_t>(
        clientBus, Example::default_service, Example::instance_path,
        Example::interface, "ExampleInt",
        [&intMatch](std::error_code ec, int64_t propVal) {
        if (ec)
        {
            std::cout << "ExampleInt Client: Error: " << ec.value() << " ("
                      << ec.message() << ")\n";
            return;
        }
        std::cout << "ExampleInt Client: Got new value: " << propVal << '\n';
        if (propVal >= 5)
        {
            std::cout << "ExampleInt Client: Cancelling Subscription\n";
            intMatch = {};
        }
    });

    //
    // Monitor the value of ExampleVariant
    //

    std::cout << "Subscribing to ExampleVariant\n";
    PropertyMatch variantMatch;
    variantMatch = subscribeToProperty<std::variant<size_t, std::string>>(
        clientBus, Example::default_service, Example::instance_path,
        Example::interface, "ExampleVariant",
        [](std::error_code ec, std::variant<size_t, std::string> propVal) {
        if (ec)
        {
            std::cout << "ExampleVariant Client: Error: " << ec.message()
                      << '\n';
            return;
        }
        std::visit(
            [](auto&& arg) {
            std::cout << "ExampleVariant Client: Got new value: " << arg
                      << '\n';
        },
            propVal);
    });

    clientBus.process_loop();

    return 0;
}
