#pragma once

#include <sdbusplus/timer.hpp>

#include <gtest/gtest.h>

using sdbusplus::Timer;

class TimerTest : public ::testing::Test
{
  public:
    // systemd event handler
    sd_event* events = nullptr;

    // Need this so that events can be initialized.
    int rc;

    // Source of event
    sd_event_source* eventSource = nullptr;

    // Add a Timer Object
    Timer timer;

    // Gets called as part of each TEST_F construction
    TimerTest() : rc(sd_event_default(&events)), timer(events)
    {
        // Check for successful creation of
        // event handler and timer object.
        EXPECT_GE(rc, 0);
    }

    // Gets called as part of each TEST_F destruction
    ~TimerTest() override
    {
        events = sd_event_unref(events);
    }
};

class TimerTestCallBack : public ::testing::Test
{
  public:
    // systemd event handler
    sd_event* events;

    // Need this so that events can be initialized.
    int rc;

    // Source of event
    sd_event_source* eventSource = nullptr;

    // Add a Timer Object
    std::unique_ptr<Timer> timer = nullptr;

    // Indicates optional call back fun was called
    bool callBackDone = false;

    void callBack()
    {
        callBackDone = true;
    }

    // Gets called as part of each TEST_F construction
    TimerTestCallBack() : rc(sd_event_default(&events))

    {
        // Check for successful creation of
        // event handler and timer object.
        EXPECT_GE(rc, 0);

        std::function<void()> func(
            std::bind(&TimerTestCallBack::callBack, this));
        timer = std::make_unique<Timer>(events, func);
    }

    // Gets called as part of each TEST_F destruction
    ~TimerTestCallBack() override
    {
        events = sd_event_unref(events);
    }
};
