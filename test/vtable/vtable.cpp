#include <sdbusplus/vtable.hpp>

#include <gtest/gtest.h>

static const sdbusplus::vtable::vtable_t example[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method((const char*)1, (const char*)2, (const char*)3,
                              (sd_bus_message_handler_t)4),
    sdbusplus::vtable::signal((const char*)5, (const char*)6),
    sdbusplus::vtable::property((const char*)7, (const char*)8,
                                (sd_bus_property_get_t)9,
                                sdbusplus::vtable::property_::const_),
    sdbusplus::vtable::property((const char*)10, (const char*)11,
                                (sd_bus_property_get_t)12,
                                (sd_bus_property_set_t)13),
    sdbusplus::vtable::property_o((const char*)14, (const char*)15, 16),
    sdbusplus::vtable::end()};

extern const sd_bus_vtable example2[];
extern const size_t example2_size;

TEST(VtableTest, SameSize)
{
    ASSERT_EQ(sizeof(example), example2_size);
}

constexpr bool operator==(const sd_bus_vtable& t1, const sd_bus_vtable& t2)
{
    constexpr size_t sizeOfMethodWithoutNames =
        8 /* sizeof (type + flags) */ + sizeof(t1.x.method.member) +
        sizeof(t1.x.method.signature) + sizeof(t1.x.method.result) +
        sizeof(t1.x.method.handler) + sizeof(t1.x.method.offset);
    constexpr size_t sizeOfSignalWithoutNames = 8 /* sizeof (type + flags) */ +
                                                sizeof(t1.x.method.member) +
                                                sizeof(t1.x.method.signature);

    switch (t1.type)
    {
        case _SD_BUS_VTABLE_START:
        case _SD_BUS_VTABLE_END:
        case _SD_BUS_VTABLE_PROPERTY:
        case _SD_BUS_VTABLE_WRITABLE_PROPERTY:
            return memcmp(&t1, &t2, sizeof(t1)) == 0;
            break;
        case _SD_BUS_VTABLE_METHOD:
            // The sd_bus_vtable.x.method.names is a pointer to a const char*,
            // that is not guranteed to be the same address for the same string.
            return memcmp(&t1, &t2, sizeOfMethodWithoutNames) == 0 &&
                   strcmp(t1.x.method.names, t2.x.method.names) == 0;
            break;
        case _SD_BUS_VTABLE_SIGNAL:
            // The sd_bus_vtable.x.signal.names is a pointer to a const char*,
            // that is not guranteed to be the same address for the same string.
            return memcmp(&t1, &t2, sizeOfSignalWithoutNames) == 0 &&
                   strcmp(t1.x.signal.names, t2.x.signal.names) == 0;
            break;
    }
    return false;
}

TEST(VtableTest, SameContent)
{
    for (size_t i = 0; i < sizeof(example) / sizeof(example[0]); ++i)
    {
        EXPECT_EQ(example[i], example2[i]);
    }
}
