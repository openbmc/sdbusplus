#include <systemd/sd-bus.h>

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus::bus::match
{

static sd_bus_slot* makeMatch(sd_bus* bus, stdplus::const_zstring match,
                              sd_bus_message_handler_t handler, void* context)
{
    sd_bus_slot* slot;
    int r = sd_bus_add_match(bus, &slot, match.c_str(), handler, context);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_match");
    }
    return slot;
}

match::match(sdbusplus::bus_t& bus, stdplus::const_zstring match,
             sd_bus_message_handler_t handler, void* context) :
    slot(makeMatch(get_busp(bus), match, handler, context))
{}

static int matchCallback(sd_bus_message* m, void* context, sd_bus_error* /*e*/)
{
    auto c = static_cast<match::callback_t*>(context);
    message_t message{m};
    (*c)(message);
    return 0;
}

match::match(sdbusplus::bus_t& bus, stdplus::const_zstring match,
             callback_t&& cb) :
    cb(std::make_unique<callback_t>(std::move(cb))),
    slot(makeMatch(get_busp(bus), match, matchCallback, this->cb.get()))
{}

} // namespace sdbusplus::bus::match
