#include <sdbusplus/vtable.hpp>

#include <gtest/gtest.h>

extern "C"
{
int test_handler(sd_bus_message* m, void* userdata, sd_bus_error* ret_error);
int test_get(sd_bus* bus, const char* path, const char* interface,
             const char* property, sd_bus_message* reply, void* userdata,
             sd_bus_error* ret_error);
int test_set(sd_bus* bus, const char* path, const char* interface,
             const char* property, sd_bus_message* value, void* userdata,
             sd_bus_error* ret_error);

extern const sd_bus_vtable example2[];
extern const size_t example2_size;
}

static const sdbusplus::vtable_t example[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method("1", "2", "3", &test_handler, 0),
    sdbusplus::vtable::signal("5", "6"),
    sdbusplus::vtable::property("7", "8", &test_get,
                                sdbusplus::vtable::property_::const_),
    sdbusplus::vtable::property("10", "11", &test_get, &test_set),
    sdbusplus::vtable::property_o("14", "15", &test_get, 16),
    sdbusplus::vtable::end()};

TEST(VtableTest, SameSize)
{
    ASSERT_EQ(sizeof(example), example2_size);
}

constexpr bool operator==(const sd_bus_vtable& t1, const sd_bus_vtable& t2)
{
    if (t1.type != t2.type || t1.flags != t2.flags)
    {
        return false;
    }

    switch (t1.type)
    {
        case _SD_BUS_VTABLE_START:
            return t1.x.start.element_size == t2.x.start.element_size &&
                   t1.x.start.features == t2.x.start.features &&
                   t1.x.start.vtable_format_reference ==
                       t2.x.start.vtable_format_reference;
        case _SD_BUS_VTABLE_END:
        {
            // The union x shall be all zeros for END
            constexpr uint8_t allZeors[sizeof(t1.x)] = {0};
            return memcmp(&t1.x, allZeors, sizeof(t1.x)) == 0 &&
                   memcmp(&t2.x, allZeors, sizeof(t2.x)) == 0;
        }
        case _SD_BUS_VTABLE_METHOD:
            return strcmp(t1.x.method.member, t2.x.method.member) == 0 &&
                   strcmp(t1.x.method.signature, t2.x.method.signature) == 0 &&
                   strcmp(t1.x.method.result, t2.x.method.result) == 0 &&
                   t1.x.method.handler == t2.x.method.handler &&
                   t1.x.method.offset == t2.x.method.offset &&
                   strcmp(t1.x.method.names, t2.x.method.names) == 0;
        case _SD_BUS_VTABLE_SIGNAL:
            return strcmp(t1.x.signal.member, t2.x.signal.member) == 0 &&
                   strcmp(t1.x.signal.signature, t2.x.signal.signature) == 0 &&
                   strcmp(t1.x.signal.names, t2.x.signal.names) == 0;
        case _SD_BUS_VTABLE_PROPERTY:
        case _SD_BUS_VTABLE_WRITABLE_PROPERTY:
            return strcmp(t1.x.property.member, t2.x.property.member) == 0 &&
                   strcmp(t1.x.property.signature, t2.x.property.signature) ==
                       0 &&
                   t1.x.property.get == t2.x.property.get &&
                   t1.x.property.set == t2.x.property.set &&
                   t1.x.property.offset == t2.x.property.offset;
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
