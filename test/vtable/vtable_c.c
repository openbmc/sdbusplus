#include <systemd/sd-bus.h>

#define UNUSED(x) (void)(x)

int test_handler(sd_bus_message* m, void* userdata, sd_bus_error* ret_error)
{
    UNUSED(m);
    UNUSED(userdata);
    UNUSED(ret_error);
    return 0;
}
int test_get(sd_bus* bus, const char* path, const char* interface,
             const char* property, sd_bus_message* reply, void* userdata,
             sd_bus_error* ret_error)
{
    UNUSED(bus);
    UNUSED(path);
    UNUSED(interface);
    UNUSED(property);
    UNUSED(reply);
    UNUSED(userdata);
    UNUSED(ret_error);
    return 0;
}
int test_set(sd_bus* bus, const char* path, const char* interface,
             const char* property, sd_bus_message* value, void* userdata,
             sd_bus_error* ret_error)
{
    UNUSED(bus);
    UNUSED(path);
    UNUSED(interface);
    UNUSED(property);
    UNUSED(value);
    UNUSED(userdata);
    UNUSED(ret_error);
    return 0;
}

typedef int (*sd_bus_message_handler_t)(sd_bus_message* m, void* userdata,
                                        sd_bus_error* ret_error);
const sd_bus_vtable example2[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("1", "2", "3", &test_handler, 0),
    SD_BUS_SIGNAL("5", "6", 0),
    SD_BUS_PROPERTY("7", "8", &test_get, 0, SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_WRITABLE_PROPERTY("10", "11", &test_get, &test_set, 0, 0),
    SD_BUS_PROPERTY("14", "15", &test_get, 16, 0),
    SD_BUS_VTABLE_END,
};

const size_t example2_size = sizeof(example2);
