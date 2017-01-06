#include <cassert>
#include <sdbusplus/message.hpp>

int main()
{
    std::string s1 = sdbusplus::message::object_path("/asdf/");
    sdbusplus::message::object_path p = std::move(s1);

    std::string s2 = sdbusplus::message::signature("iii");
    sdbusplus::message::signature sig = s2;

    return 0;
}
