# Copyright (c) Benjamin Kietzman (github.com/bkietz)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

###############
# CMake options
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HUNTER_ENABLED "Enable hunter package pulling" OFF)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.18.64.tar.gz"
    SHA1 "baf9c8cc4f65306f0e442b5419967b4c4c04589a"
)

project(boost-dbus CXX)

###############
# C++ options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test)

###############
# import Boost
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
hunter_add_package(Boost)
find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

###############
# import D-Bus
hunter_add_package(dbus)
find_package(dbus REQUIRED) # Include functions provided by PkgConfig module.
include_directories(${DBUS_INCLUDE_DIRS})

hunter_add_package(pthread-stubs)
find_package (Threads)

##############
# Install
add_library(boost-dbus INTERFACE)
target_include_directories(boost-dbus INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${DBUS_INCLUDE_DIRS})
target_link_libraries(boost-dbus INTERFACE ${DBUS_LIBRARIES})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)

##############
# Tests
enable_testing()

add_executable(dbustests "test/avahi.cpp" "test/message.cpp" "test/error.cpp" "test/dbusPropertiesServer.cpp")

##############
# import GTest
hunter_add_package(GTest)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

if (${HUNTER_ENABLED})
    find_package(GMock CONFIG REQUIRED)
    target_link_libraries(dbustests GMock::main)
else()
    find_package(GMock REQUIRED)
    target_link_libraries(dbustests ${GTEST_BOTH_LIBRARIES} gmock)
endif()
target_link_libraries(dbustests ${CMAKE_THREAD_LIBS_INIT})
add_test(dbustests dbustests "--gtest_output=xml:${test_name}.xml")

target_link_libraries(dbustests boost-dbus)
