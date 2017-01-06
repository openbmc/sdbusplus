#include <cassert>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>

template <typename ...Args>
auto dbus_string(Args&& ... args)
{
    return std::string(
               sdbusplus::utility::tuple_to_array(
                   sdbusplus::message::types::type_id<Args...>()).data());
}

int main()
{

    // Check a few simple types.
    assert(dbus_string(1) == "i");
    assert(dbus_string(1.0) == "d");

    // Check a multiple parameter call.
    assert(dbus_string(false, true) == "bb");
    assert(dbus_string(1, 2, 3, true, 1.0) == "iiibd");

    // Check an l-value and r-value reference.
    {
        std::string a = "a";
        std::string b = "b";
        const char* c = "c";

        assert(dbus_string(a, std::move(b), c) == "sss");
    }

    // Check object_path and signatures.
    assert(dbus_string(std::string("asdf"),
                       sdbusplus::message::object_path("/asdf"),
                       sdbusplus::message::signature("sss")) ==
           "sog");

    return 0;
}
