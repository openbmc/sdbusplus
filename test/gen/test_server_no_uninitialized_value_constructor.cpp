
#include "sdbusplus/server.hpp"
#include "test/gen/server/Test/server.hpp"

#include <memory>
#include <regex>

#include <gtest/gtest.h>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    sdbusplus::server::server::Test t(bus, "/");

    assert(t.someValue() == 0);

    t.someValue(3);

    return 0;
}
