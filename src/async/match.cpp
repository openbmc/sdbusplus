#include <sdbusplus/async/match.hpp>

namespace sdbusplus::async
{

slot_t match::makeMatch(context& ctx, const std::string_view& pattern)
{
    // C-style callback to redirect into this::handle_match.
    static auto match_cb =
        [](message::msgp_t msg, void* ctx, sd_bus_error*) noexcept {
            static_cast<match*>(ctx)->handle_match(message_t{msg});
            return 0;
        };

    sd_bus_slot* s;
    auto r =
        sd_bus_add_match(get_busp(ctx), &s, pattern.data(), match_cb, this);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_add_match (async::match)");
    }

    return slot_t{s, &sdbus_impl};
}

match::match(context& ctx, const std::string_view& pattern) :
    slot(makeMatch(ctx, pattern))
{}

match::~match()
{
    match_ns::match_completion* c = nullptr;

    {
        std::lock_guard l{lock};
        c = std::exchange(complete, nullptr);
    }

    if (c)
    {
        c->stop();
    }
}

void match_ns::match_completion::start() noexcept
{
    // Set ourselves as the awaiting Receiver and see if there is a message
    // to immediately complete on.

    std::unique_lock lock{m.lock};

    if (std::exchange(m.complete, this) != nullptr)
    {
        // We do not support two awaiters; throw exception.  Since we are in
        // a noexcept context this will std::terminate anyhow, which is
        // approximately the same as 'assert' but with better information.
        try
        {
            throw std::logic_error(
                "match_completion started with another await already pending!");
        }
        catch (...)
        {
            std::terminate();
        }
    }

    m.handle_completion(std::move(lock));
}

void match::handle_match(message_t&& msg) noexcept
{
    // Insert the message into the queue and see if there is a pair ready for
    // completion (Receiver + message).
    std::unique_lock l{lock};
    queue.emplace(std::move(msg));
    handle_completion(std::move(l));
}

void match::handle_completion(std::unique_lock<std::mutex>&& l) noexcept
{
    auto lock = std::move(l);

    // If there is no match_completion, there is no awaiting Receiver.
    // If the queue is empty, there is no message waiting, so the waiting
    // Receiver isn't complete.
    if ((complete == nullptr) || queue.empty())
    {
        return;
    }

    // Get the waiting completion and message.
    auto c = std::exchange(complete, nullptr);
    auto msg = std::move(queue.front());
    queue.pop();

    // Unlock before calling complete because the completed task may run and
    // attempt to complete on the next event (and thus deadlock).
    lock.unlock();

    // Signal completion.
    c->complete(std::move(msg));
}

} // namespace sdbusplus::async
