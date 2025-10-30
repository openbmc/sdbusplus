#include "sdbusplus/async.hpp"
#include "server/Test/aserver.hpp"

#include <sdbusplus/async/context.hpp>

#include <gtest/gtest.h>

class A : public sdbusplus::aserver::server::Test<A>
{};

// Test that we can construct with an object path
TEST(AServerConstructWithObjectPath, NoProps)
{
    sdbusplus::async::context ctx;

    sdbusplus::message::object_path path("/");

    sdbusplus::aserver::server::Test<A> t2(ctx, path);
}

// Test that we can construct with an object path (the overload with properties
// passed in)
TEST(AServerConstructWithObjectPath, WithProps)
{
    sdbusplus::async::context ctx;

    auto x =
        std::variant<A::EnumOne, std::string, A::EnumTwo>(A::EnumOne::OneA);

    sdbusplus::common::server::Test::properties_t props{
        0, 0, 0, 0, 0, 0, 0, std::string("/my/path"), 1.0, 1.0, 1.0, 1.0, x};

    sdbusplus::message::object_path path("/");

    sdbusplus::aserver::server::Test<A> t2(ctx, path, props);
}
