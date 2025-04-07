
#include "sdbusplus/server.hpp"
#include "server/Test/server.hpp"

#include <cassert>
#include <memory>
#include <regex>

int main()
{
    auto bus = sdbusplus::bus::new_default();

    sdbusplus::server::server::Test t(bus, "/");

    assert(t.someValue() == 0);

    t.someValue(3);

    return 0;
}
