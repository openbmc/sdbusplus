#include <sdbusplus/bus.hpp>
#include <sdbusplus/property.hpp>
#include <sdbusplus/timer.hpp>
#include <xyz/openbmc_project/Example/server.hpp>

#include <cinttypes>

using namespace sdbusplus;

using Example = server::xyz::openbmc_project::Example;
using ExampleBase = server::object_t<Example>;
struct ExampleServer : ExampleBase
{
    ExampleServer(bus_t& bus) : ExampleBase(bus, Example::instance_path) {}
};

int main()
{
    auto bus = bus::new_default();
    bus.request_name(Example::default_service);
    server::manager_t objManager(bus, "/");

    printf("Subscribing to ExampleInt\n");
    PropertyMatch exampleIntSubscription = subscribeToProperty<int64_t>(
        bus, Example::default_service, Example::instance_path,
        Example::interface, "ExampleInt",
        [](std::error_code ec, int64_t propVal) {
        if (ec)
        {
            printf("Client: Error: %d (%s)\n", ec.value(),
                   ec.message().c_str());
            return;
        }
        printf("Client: Got new ExampleInt value: %" PRId64 "\n", propVal);
    });

    sd_event* event;
    if (sd_event_default(&event) < 0)
    {
        return 1;
    }
    bus.attach_event(event, SD_EVENT_PRIORITY_NORMAL);

    ExampleServer example(bus);

    phosphor::Timer t1(event, [&] {
        auto oldValue = example.exampleInt();
        auto newValue = oldValue + 1;
        printf("Server: Updating ExampleInt to %" PRId64 "\n", newValue);
        example.exampleInt(newValue);
        if (newValue == 5)
        {
            printf("Canceling subscription\n");
            exampleIntSubscription = {};
        }
    });
    t1.start(std::chrono::seconds(1), true);

    sd_event_loop(event);

    return 0;
}
