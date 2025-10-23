#pragma once

#include <unistd.h>

#include <sdbusplus/async.hpp>

#include <filesystem>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

class FdioTimedTest : public ::testing::Test
{
  protected:
    enum class testWriteOperation
    {
        writeSync,
        writeAsync,
        writeSkip
    };

    fs::path path;

    FdioTimedTest();

    ~FdioTimedTest() noexcept override;

    auto writeToFile() -> sdbusplus::async::task<>;

    auto testFdTimedEvents(bool& ran, testWriteOperation writeOperation,
                           int testIterations) -> sdbusplus::async::task<>;

    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;
    std::optional<sdbusplus::async::context> ctx{std::in_place};

  private:
    int fd = -1;
    int wd = -1;
};
