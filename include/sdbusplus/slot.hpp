#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/sdbus.hpp>

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
    explicit SlotDeleter(SdBusInterface* intf) : intf(intf) {}

    void operator()(slotp_t ptr) const
    {
        intf->sd_bus_slot_unref(ptr);
    }

    SdBusInterface* intf;
};

using slot = std::unique_ptr<sd_bus_slot, SlotDeleter>;

struct slot_friend;

} // namespace details

/** @class slot
 *  @brief Provides C++ holder for sd_bus_slot instances.
 */
struct slot
{
    /** @brief Empty (unused) slot */
    slot() : _slot(nullptr, details::SlotDeleter(&sdbus_impl)) {}
    explicit slot(std::nullptr_t) : slot() {}

    /** @brief Conversion constructor for 'slotp_t'.
     *
     *  Takes ownership of the slot-pointer and releases it when done.
     */
    slot(slotp_t s, sdbusplus::SdBusInterface* intf) :
        _slot(s, details::SlotDeleter(intf))
    {}

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
