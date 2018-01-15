#include <gtest/gtest.h>
#include <sdbusplus/vtable.hpp>

static const sdbusplus::vtable::vtable_t example[] = {
    sdbusplus::vtable::start(),
    sdbusplus::vtable::method((const char *)1, (const char *)2, (const char *)3,
                              (sd_bus_message_handler_t)4),
    sdbusplus::vtable::signal((const char *)5, (const char *)6),
    sdbusplus::vtable::property((const char *)7, (const char *)8,
                                (sd_bus_property_get_t)9,
                                sdbusplus::vtable::property_::const_),
    sdbusplus::vtable::property((const char *)10, (const char *)11,
                                (sd_bus_property_get_t)12,
                                (sd_bus_property_set_t)13),
    sdbusplus::vtable::property_o((const char *)14, (const char *)15, 16),
    sdbusplus::vtable::end()};

extern const sd_bus_vtable example2[];
extern const size_t example2_size;

TEST(VtableTest, SameSize)
{
    ASSERT_EQ(sizeof(example), example2_size);
}

TEST(VtableTest, SameContent)
{
    ASSERT_EQ(0, memcmp(example, example2, example2_size));
}
