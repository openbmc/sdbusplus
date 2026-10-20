#include "../valgrind.hpp"

#include <sys/epoll.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sdbusplus/event.hpp>

#include <chrono>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;

struct Event : public testing::Test
{
    sdbusplus::event_t ev{};
};

struct ReentrantSourceCallback
{
    explicit ReentrantSourceCallback(sdbusplus::event_source_t& source) :
        source(&source)
    {}

    sdbusplus::event_source_t* source;
    std::mutex mutex;
    std::condition_variable condition;
    bool callbackEntered = false;
    bool workerAttempting = false;
    bool called = false;

    static int run(sd_event_source*, int, uint32_t, void* data)
    {
        auto* self = static_cast<ReentrantSourceCallback*>(data);
        {
            std::unique_lock lock{self->mutex};
            self->callbackEntered = true;
            self->condition.notify_all();
            self->condition.wait(lock, [&]() {
                return self->workerAttempting;
            });
        }
        std::this_thread::sleep_for(50ms);
        self->source->set_enabled(false);
        self->called = true;
        return 0;
    }
};

int ignoreEvent(sd_event_source*, int, uint32_t, void*)
{
    return 0;
}

TEST_F(Event, TimeoutWorks)
{
    static constexpr auto timeout = 250ms;

    auto start = std::chrono::steady_clock::now();
    ev.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    const auto tolerance = isValgrind() ? 16 : 3;

    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * tolerance);
}

TEST_F(Event, Runnable)
{
    static constexpr auto timeout = 10s;

    std::jthread j{[&]() { ev.break_run(); }};

    auto start = std::chrono::steady_clock::now();
    ev.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start < timeout);
}

TEST_F(Event, ConditionSignals)
{
    struct run
    {
        static int _(sd_event_source*, int, uint32_t, void* data)
        {
            *static_cast<bool*>(data) = true;
            return 0;
        }
    };
    bool ran = false;

    auto c = ev.add_condition(run::_, &ran);
    std::jthread j{[&]() { c.signal(); }};

    ev.run_one();
    EXPECT_TRUE(ran);
    c.ack();
}

TEST_F(Event, Timer)
{
    static constexpr auto timeout = 50ms;

    struct handler
    {
        static int _(sd_event_source*, uint64_t, void* data)
        {
            *static_cast<bool*>(data) = true;
            return 0;
        }
    };
    bool ran = false;

    auto start = std::chrono::steady_clock::now();
    auto c = ev.add_oneshot_timer(handler::_, &ran, timeout);
    ev.run_one();
    auto stop = std::chrono::steady_clock::now();

    const auto tolerance = isValgrind() ? 16 : 3;

    EXPECT_TRUE(ran);
    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * tolerance);
}

TEST_F(Event, CallbackCanReenterEventLock)
{
    const auto child = fork();
    ASSERT_NE(child, -1);
    if (child == 0)
    {
        int fds[2];
        if (pipe(fds) != 0)
        {
            _exit(2);
        }

        sdbusplus::event_t childEvent{};
        auto source = childEvent.add_io(fds[0], EPOLLIN, ignoreEvent, nullptr);
        ReentrantSourceCallback callback{source};
        auto condition =
            childEvent.add_condition(ReentrantSourceCallback::run, &callback);
        std::thread worker{[&]() {
            {
                std::unique_lock lock{callback.mutex};
                callback.condition.wait(lock, [&]() {
                    return callback.callbackEntered;
                });
                callback.workerAttempting = true;
                callback.condition.notify_all();
            }
            source.set_enabled(false);
        }};
        condition.signal();
        childEvent.run_one(100ms);
        worker.join();
        _exit(callback.called ? 0 : 3);
    }

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto result = waitpid(child, &status, WNOHANG);
        if (result == child)
        {
            EXPECT_TRUE(WIFEXITED(status));
            EXPECT_EQ(WEXITSTATUS(status), 0);
            return;
        }
        ASSERT_NE(result, -1);
        std::this_thread::sleep_for(5ms);
    }

    kill(child, SIGKILL);
    waitpid(child, &status, 0);
    FAIL() << "Callback deadlocked while reentering the event lock";
}
