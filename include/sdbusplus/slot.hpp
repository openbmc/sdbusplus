#pragma once

#include <systemd/sd-bus.h>

#include <memory>

namespace sdbusplus
{

namespace slot
{

using slotp_t = sd_bus_slot*;
struct slot;

namespace details
{

/** @brief unique_ptr functor to release a slot reference. */
struct SlotDeleter
{
    void operator()(slotp_t ptr) const
    {
        sd_bus_slot_unref(ptr);
    }
};

using slot = std::unique_ptr<sd_bus_slot, SlotDeleter>;

struct slot_friend;

} // namespace details

/** @class slot
 *  @brief Provides C++ holder for sd_bus_slot instances.
 */
struct slot
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Default constructor, assigned as a nullptr.
     *         - Move operations.
     *         - Destructor.
     */
    slot(const slot&) = delete;
    slot& operator=(const slot&) = delete;
    slot(slot&&) = default;
    slot& operator=(slot&&) = default;
    ~slot() = default;

    /** @brief Conversion constructor for 'slotp_t'.
     *
     *  Takes ownership of the slot-pointer and releases it when done.
     */
    explicit slot(slotp_t&& s = nullptr) : _slot(s)
    {
        s = nullptr;
    }

    /** @brief Conversion from 'slotp_t'.
     *
     *  Takes ownership of the slot-pointer and releases it when done.
     */
    slot& operator=(slotp_t&& s)
    {
        _slot.reset(s);
        s = nullptr;

        return *this;
    }

    /** @brief Release ownership of the stored slot-pointer. */
    slotp_t release()
    {
        return _slot.release();
    }

    /** @brief Check if slot contains a real pointer. (non-nullptr). */
    explicit operator bool() const
    {
        return bool(_slot);
    }

    friend struct details::slot_friend;

  private:
    slotp_t get() noexcept
    {
        return _slot.get();
    }
    details::slot _slot;
};

namespace details
{

// Some sdbusplus classes need to be able to pass the underlying slot pointer
// along to sd_bus calls, but we don't want to make it available for everyone.
// Define a class which can be inherited explicitly (intended for internal users
// only) to get the underlying bus pointer.
struct slot_friend
{
    static slotp_t get_slotp(sdbusplus::slot::slot& s) noexcept
    {
        return s.get();
    }
};

} // namespace details

} // namespace slot

using slot_t = slot::slot;

} // namespace sdbusplus
