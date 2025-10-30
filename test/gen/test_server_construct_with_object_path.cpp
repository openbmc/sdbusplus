#include "sdbusplus/server.hpp"
#include "server/Test/server.hpp"

#include <gtest/gtest.h>

TEST(ServerConstructWithObjectPath, NoProps)
{
    auto bus = sdbusplus::bus::new_default();

    sdbusplus::message::object_path path("/");

    sdbusplus::server::server::Test t(bus, path);
}

TEST(ServerConstructWithObjectPath, WithProps)
{
    auto bus = sdbusplus::bus::new_default();

    const std::map<std::string,
                   sdbusplus::common::server::Test::PropertiesVariant>
        props = {{"SomeValue", 1}};

    sdbusplus::message::object_path path("/");

    sdbusplus::server::server::Test t(bus, path, props);
}
