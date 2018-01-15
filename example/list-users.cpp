#include <cstdint>
#include <iostream>
#include <sdbusplus/bus.hpp>

/** An example dbus client application.
 *  Calls org.freedesktop.login1's ListUsers interface to find all active
 *  users in the system and displays their username.
 */

int main()
{
    using namespace sdbusplus;

    auto b = bus::new_system();
    auto m =
        b.new_method_call("org.freedesktop.login1", "/org/freedesktop/login1",
                          "org.freedesktop.login1.Manager", "ListUsers");
    auto reply = b.call(m);

    std::vector<std::tuple<uint32_t, std::string, message::object_path>> users;
    reply.read(users);

    for (auto &user : users)
    {
        std::cout << std::get<std::string>(user) << "\n";
    }

    return 0;
}
