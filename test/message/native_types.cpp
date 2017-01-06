#include <cassert>
#include <sdbusplus/message.hpp>
#include <vector>
#include <map>
#include <unordered_map>

int main()
{
    std::string s1 = sdbusplus::message::object_path("/asdf/");
    sdbusplus::message::object_path p = std::move(s1);

    std::string s2 = sdbusplus::message::signature("iii");
    sdbusplus::message::signature sig = s2;

    std::vector<sdbusplus::message::signature> v =
        { sdbusplus::message::signature("iii") };

    std::map<sdbusplus::message::signature, int> m =
        { { sdbusplus::message::signature("iii"), 1 } };

    std::unordered_map<sdbusplus::message::signature, int> u =
        { { sdbusplus::message::signature("iii"), 1 } };

    return 0;
}
