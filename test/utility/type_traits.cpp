#include <sdbusplus/utility/type_traits.hpp>

int main()
{
    using sdbusplus::utility::array_to_ptr_t;

    static_assert(std::is_same<char, array_to_ptr_t<char, char>>::value,
                  "array_to_ptr_t<char, char> != char");

    static_assert(std::is_same<char *, array_to_ptr_t<char, char *>>::value,
                  "array_to_ptr_t<char, char*> != char*");

    static_assert(std::is_same<char *, array_to_ptr_t<char, char[100]>>::value,
                  "array_to_ptr_t<char, char[100]> != char*");

    static_assert(
        std::is_same<char[100], array_to_ptr_t<int, char[100]>>::value,
        "array_to_ptr_t<int, char[100]> != char[100]");

    return 0;
}
