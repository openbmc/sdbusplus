#include "sdbusplus/async.hpp"
#include "server/Test/aserver.hpp"

#include <sdbusplus/async/context.hpp>

#include <memory>
#include <regex>

#include <gtest/gtest.h>

class A : public sdbusplus::aserver::server::Test<A>
{};

int main()
{
    sdbusplus::async::context ctx;

    sdbusplus::aserver::server::Test<A> t2(ctx, "/");

    assert(t2.some_value() == 0);

    t2.some_value(4);

    return 0;
}
