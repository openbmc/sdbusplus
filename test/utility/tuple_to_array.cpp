#include <sdbusplus/utility/tuple_to_array.hpp>
#include <cassert>

int main()
{
    std::array<const char, 3> a{'a', 'b', 'c'};
    auto t = std::make_tuple('a', 'b', 'c');

    assert(a == sdbusplus::utility::tuple_to_array(std::move(t)));

    std::array<const int, 4> b{1, 2, 3, 4};
    auto t2 = std::make_tuple(1, 2, 3, 4);

    assert(b == sdbusplus::utility::tuple_to_array(std::move(t2)));

    return 0;
}
