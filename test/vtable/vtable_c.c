#include <systemd/sd-bus.h>

const sd_bus_vtable example2[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD((const char *)1, (const char *)2, (const char *)3,
                  (sd_bus_message_handler_t)4, 0),
    SD_BUS_SIGNAL((const char *)5, (const char *)6, 0),
    SD_BUS_PROPERTY((const char *)7, (const char *)8, (sd_bus_property_get_t)9,
                    0, SD_BUS_VTABLE_PROPERTY_CONST),
    SD_BUS_WRITABLE_PROPERTY((const char *)10, (const char *)11,
                             (sd_bus_property_get_t)12,
                             (sd_bus_property_set_t)13, 0, 0),
    SD_BUS_PROPERTY((const char *)14, (const char *)15, NULL, 16, 0),
    SD_BUS_VTABLE_END,
};

const size_t example2_size = sizeof(example2);
