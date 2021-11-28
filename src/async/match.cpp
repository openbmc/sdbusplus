#include <sdbusplus/async/match.hpp>
#include <sdbusplus/exception.hpp>

namespace sdbusplus::async::match
{

match::match(context_t& ctx, std::string_view match)
{
    // C-style callback to redirect into this::handle_match.
    static auto match_cb = [](message::msgp_t msg, void* ctx,
                              sd_bus_error*) noexcept {
        static_cast<match_t*>(ctx)->handle_match(message_t{msg});
        return 0;
    };

    sd_bus_slot* s = nullptr;
    auto r = sd_bus_add_match(get_busp(ctx.get_bus()), &s, match.data(),
                              match_cb, this);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_add_match (async::match)");
    }

    slot = std::move(s);
}

void details::match_completion::associate(match& m) noexcept
{
    // Set ourselves and the awaiting Sender and see if there is a message
    // to immediately complete on.

    std::unique_lock lock{m.lock};

    assert(m.complete ==
           nullptr); // We should only have one task waiting for matches.
    m.complete = this;

    m.handle_completion(lock);
}

void match::handle_match(message_t&& msg) noexcept
{
    // Insert the message into the queue and see if there is a pair ready for
    // completion (Sender + message).
    std::unique_lock l{lock};
    queue.emplace(std::move(msg));
    handle_completion(l);
}

void match::handle_completion(std::unique_lock<std::mutex>& lock) noexcept
{
    // If there is no match_completion, there is no awaiting Sender.  Message
    // has already been queued.  If the queue is empty, there is no message
    // waiting, so there waiting Sender isn't complete.
    if ((complete == nullptr) || queue.empty())
    {
        return;
    }

    // Get the waiting completion and message.

    details::match_completion* c = nullptr;
    std::swap(complete, c);

    auto msg = std::move(queue.front());
    queue.pop();

    // Unlock before calling complete because the completed task may run and
    // attempt to complete on the next event (and thus deadlock).
    lock.unlock();
    c->complete(std::move(msg));
}

} // namespace sdbusplus::async::match
