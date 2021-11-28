#pragma once
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string_view>

namespace sdbusplus::async
{
namespace match
{
namespace rules
{
// Mirror bus::match::rules into async::match::rules.
using namespace sdbusplus::bus::match::rules;
} // namespace rules
namespace details
{
struct match_completion;
} // namespace details

/** A class that generates asynchronous dbus match Senders. */
class match : private bus::details::bus_friend
{
  public:
    match() = delete;
    match(const match&) = delete;
    match& operator=(const match&) = delete;
    match(match&&) = delete;
    match& operator=(match&&) = delete;
    ~match() = default;

    /** Construct the match using the `match` string on the bus managed by the
     *  context. */
    match(context_t& ctx, std::string_view match);

    /** Get the Sender for the next event.
     *
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    auto next() noexcept;

    friend struct details::match_completion;

  private:
    sdbusplus::slot_t slot{};

    std::mutex lock{};
    std::queue<sdbusplus::message_t> queue{};
    details::match_completion* complete = nullptr;

    /** Handle an incoming match event. */
    void handle_match(message_t&&) noexcept;

    /** Signal completion if there is an awaiting Sender.
     *
     *  This should be called only with `lock` held (which may be released as a
     *  side-effect).
     */
    void handle_completion(std::unique_lock<std::mutex>&) noexcept;
};

namespace details
{
// Virtual class to handle the match Sender completions.
struct match_completion
{
    virtual ~match_completion() = default;
    virtual void complete(message_t&&) noexcept = 0;

  protected:
    void associate(match&) noexcept;
};

// Implementation (templated based on Receiver) of match_completion.
template <execution::receiver_of<message_t> Receiver>
class match_operation : match_completion
{
  public:
    match_operation(match& m, Receiver r) : m(m), receiver(std::move(r))
    {}

    void complete(message_t&& msg) noexcept override final
    {
        execution::set_value(std::move(receiver), std::move(msg));
    }

    void start() noexcept
    {
        associate(m);
    }

  private:
    match& m;
    Receiver receiver;
};

// match Sender implementation.
class match_sender : public execution::sender_of<false, message_t>
{
  public:
    explicit match_sender(match& m) noexcept : m(m){};
    auto connect(execution::receiver_of<message_t> auto receiver) noexcept
    {
        return match_operation(m, std::move(receiver));
    }

  private:
    match& m;
};

}; // namespace details

auto match::next() noexcept
{
    return details::match_sender(*this);
}

} // namespace match

using match_t = match::match;

} // namespace sdbusplus::async
