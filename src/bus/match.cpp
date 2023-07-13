#include <systemd/sd-bus.h>

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus::bus::match
{

static slot_t makeMatch(SdBusInterface* intf, sd_bus* bus, const char* _match,
                        sd_bus_message_handler_t handler, void* context)
{
    sd_bus_slot* slot;
    int r = intf->sd_bus_add_match(bus, &slot, _match, handler, context);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_match");
    }
    return slot_t{slot, intf};
}

match::match(sdbusplus::bus_t& bus, const char* _match,
             sd_bus_message_handler_t handler, void* context) :
    _slot(
        makeMatch(bus.getInterface(), get_busp(bus), _match, handler, context))
{}

// The callback is 'noexcept' because it is called from C code (sd-bus).
static int matchCallback(sd_bus_message* m, void* context,
                         sd_bus_error* /*e*/) noexcept
{
    auto c = static_cast<match::callback_t*>(context);
    message_t message{m};
    (*c)(message);
    return 0;
}

match::match(sdbusplus::bus_t& bus, const char* _match, callback_t callback) :
    _callback(std::make_unique<callback_t>(std::move(callback))),
    _slot(makeMatch(bus.getInterface(), get_busp(bus), _match, matchCallback,
                    _callback.get()))
{}

} // namespace sdbusplus::bus::match
