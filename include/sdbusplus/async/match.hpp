#pragma once
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/bus/match.hpp> // IWYU pragma: export
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string_view>

namespace sdbusplus::async
{
namespace match_ns
{
struct match_completion;
}

/** Generator of dbus match Senders.
 *
 *  This class registers a signal match pattern with the dbus and generates
 *  Senders using `next` to await the next matching signal.
 */
class match : private bus::details::bus_friend
{
  public:
    match() = delete;
    match(const match&) = delete;
    match& operator=(const match&) = delete;
    match(match&&) = delete;
    match& operator=(match&&) = delete;
    ~match();

    /** Construct the match using the `pattern` string on the bus managed by the
     *  context. */
    match(context& ctx, const std::string_view& pattern);

    /** Get the Sender for the next event (as message).
     *
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    auto next() noexcept;

    /** Get the Sender for the next event, which yields one of:
     *    void, Rs, tuple<Rs...>
     *
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    template <typename... Rs>
    auto next() noexcept;

    friend match_ns::match_completion;

  private:
    sdbusplus::slot_t slot;

    std::mutex lock{};
    std::queue<sdbusplus::message_t> queue{};
    match_ns::match_completion* complete = nullptr;

    /** Handle an incoming match event. */
    void handle_match(message_t&&) noexcept;

    /** Signal completion if there is an awaiting Receiver.
     *
     *  This must be called with `lock` held (and ownership transfers).
     */
    void handle_completion(std::unique_lock<std::mutex>&&) noexcept;

    slot_t makeMatch(context& ctx, const std::string_view& pattern);
};

namespace match_ns
{
// Virtual class to handle the match Receiver completions.
struct match_completion
{
    match_completion() = delete;
    match_completion(match_completion&&) = delete;

    explicit match_completion(match& m) : m(m) {};
    virtual ~match_completion() = default;

    friend match;

    void start() noexcept;

  private:
    virtual void complete(message_t&&) noexcept = 0;
    virtual void stop() noexcept = 0;

    match& m;
};

// Implementation (templated based on Receiver) of match_completion.
template <execution::receiver Receiver>
struct match_operation : match_completion
{
    match_operation(match& m, Receiver r) :
        match_completion(m), receiver(std::move(r))
    {}

  private:
    void complete(message_t&& msg) noexcept override final
    {
        execution::set_value(std::move(receiver), std::move(msg));
    }

    void stop() noexcept override final
    {
        execution::set_stopped(std::move(receiver));
    }

    Receiver receiver;
};

// match Sender implementation.
struct match_sender
{
    using sender_concept = execution::sender_t;

    match_sender() = delete;
    explicit match_sender(match& m) noexcept : m(m) {};

    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<execution::set_value_t(message_t),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    auto connect(R r)
        -> match_operation<R>
    {
        return {m, std::move(r)};
    }

  private:
    match& m;
};

}; // namespace match_ns

inline auto match::next() noexcept
{
    return match_ns::match_sender(*this);
}

template <typename... Rs>
auto match::next() noexcept
{
    return match_ns::match_sender(*this) |
           execution::then([](message_t&& m) { return m.unpack<Rs...>(); });
}

} // namespace sdbusplus::async
