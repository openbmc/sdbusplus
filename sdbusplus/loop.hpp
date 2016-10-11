#pragma once

#include <memory>
#include <systemd/sd-event.h>

namespace sdeventplus
{
namespace loop
{

using loopp_t = sd_event*;
class loop;

/** @brief Get an sd_event loop instance. */
loop new_loop();

namespace details
{

/** @brief unique_ptr functor to release a loop reference. */
struct LoopDeleter
{
    void operator()(sd_event* ptr) const
    {
        sd_event_unref(ptr);
    }
};

/* @brief Alias 'loop' to a unique_ptr type for auto-release. */
using loop = std::unique_ptr<sd_event, LoopDeleter>;

} // namespace details

/** @class loop
 *  @brief Provides C++ bindings to the sd_event_* class functions.
 */
struct loop
{
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
    loop() = delete;
    loop(const loop&) = delete;
    loop& operator=(const loop&) = delete;
    loop(loop&&) = default;
    loop& operator=(loop&&) = default;
    ~loop() = default;

    /** @brief Conversion constructor from 'loopp_t'.
     *
     *  Takes ownership of the loop-pointer and releases it when done.
     */
    explicit loop(loopp_t l) : _loop(l) {}

    /** @brief Release ownership of the stored loop-pointer. */
    loopp_t release() { return _loop.release(); }

    /** @brief Start an event loop.
     */
    void start()
    {
        sd_event_loop(_loop.get());
    }

    private:
        details::loop _loop;
};

loop new_loop()
{
    sd_event* l = nullptr;
    sd_event_new(&l);
    return loop(l);
}

} // namespace loop

} // namespace sdeventplus
