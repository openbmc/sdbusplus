#pragma once

#include <cstdlib>

#if defined(__has_include)
#if __has_include(<valgrind/valgrind.h>)
#include <valgrind/valgrind.h>
#define SDBUSPLUS_TEST_HAVE_VALGRIND_H 1
#endif
#endif

inline bool isValgrind()
{
#ifdef SDBUSPLUS_TEST_HAVE_VALGRIND_H
    return RUNNING_ON_VALGRIND;
#else
    static const bool rc = std::getenv("VALGRIND_OPTS") != nullptr ||
                           std::getenv("VALGRIND_LIB") != nullptr;
    return rc;
#endif
}
