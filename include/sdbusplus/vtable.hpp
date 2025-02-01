#pragma once

#include <systemd/sd-bus.h>

namespace sdbusplus
{

namespace vtable
{
/** Alias typedef for sd_bus_vtable */
using vtable_t = sd_bus_vtable;

/** Create a SD_BUS_VTABLE_START entry. */
constexpr vtable_t start(decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_VTABLE_END entry. */
constexpr vtable_t end();

/** Create a SD_BUS_VTABLE_METHOD entry.
 *
 *  @param[in] member - Name of method.
 *  @param[in] signature - Signature of method.
 *  @param[in] result - Signature of result.
 *  @param[in] handler - Functor to call on method invocation.
 *  @param[in] flags - optional sdbusplus::vtable::method_ value.
 */
constexpr vtable_t method(const char* member, const char* signature,
                          const char* result, sd_bus_message_handler_t handler,
                          decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_VTABLE_METHOD_WITH_OFFSET entry.
 *
 *  @param[in] member - Name of method.
 *  @param[in] signature - Signature of method.
 *  @param[in] result - Signature of result.
 *  @param[in] handler - Functor to call on method invocation.
 *  @param[in] offset - Offset.
 *  @param[in] flags - optional sdbusplus::vtable::method_ value.
 */
constexpr vtable_t method_o(const char* member, const char* signature,
                            const char* result,
                            sd_bus_message_handler_t handler, size_t offset,
                            decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_SIGNAL entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] flags - None supported.
 */
constexpr vtable_t signal(const char* member, const char* signature,
                          decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] get - Functor to call on property get.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property(const char* member, const char* signature,
                            sd_bus_property_get_t get,
                            decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_WRITABLE_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] get - Functor to call on property get.
 * @param[in] set - Functor to call on property set.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property(
    const char* member, const char* signature, sd_bus_property_get_t get,
    sd_bus_property_set_t set, decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] get - Functor to call on property get.
 * @param[in] offset - Offset within object for property.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property_o(const char* member, const char* signature,
                              sd_bus_property_get_t get, size_t offset,
                              decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_WRITABLE_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] get - Functor to call on property get.
 * @param[in] set - Functor to call on property set.
 * @param[in] offset - Offset within object for property.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property_o(const char* member, const char* signature,
                              sd_bus_property_get_t get,
                              sd_bus_property_set_t set, size_t offset,
                              decltype(vtable_t::flags) flags = 0);

namespace common_
{
constexpr auto deprecated = SD_BUS_VTABLE_DEPRECATED;
constexpr auto hidden = SD_BUS_VTABLE_HIDDEN;
constexpr auto unprivileged = SD_BUS_VTABLE_UNPRIVILEGED;
} // namespace common_

namespace method_
{
constexpr auto no_reply = SD_BUS_VTABLE_METHOD_NO_REPLY;
} // namespace method_

namespace property_
{
constexpr auto const_ = SD_BUS_VTABLE_PROPERTY_CONST;
constexpr auto emits_change = SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE;
constexpr auto emits_invalidation = SD_BUS_VTABLE_PROPERTY_EMITS_INVALIDATION;
constexpr auto explicit_ = SD_BUS_VTABLE_PROPERTY_EXPLICIT;
constexpr auto none = decltype(vtable_t::flags){};
} // namespace property_

constexpr vtable_t start(decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_VTABLE_START(flags);
}

constexpr vtable_t end()
{
    return vtable_t SD_BUS_VTABLE_END;
}

constexpr vtable_t method(const char* member, const char* signature,
                          const char* result, sd_bus_message_handler_t handler,
                          decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_METHOD(member, signature, result, handler, flags);
}

constexpr vtable_t method_o(const char* member, const char* signature,
                            const char* result,
                            sd_bus_message_handler_t handler, size_t offset,
                            decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_METHOD_WITH_OFFSET(member, signature, result,
                                              handler, offset, flags);
}

constexpr vtable_t signal(const char* member, const char* signature,
                          decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_SIGNAL(member, signature, flags);
}

constexpr vtable_t property(const char* member, const char* signature,
                            sd_bus_property_get_t get,
                            decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_PROPERTY(member, signature, get, 0, flags);
}

constexpr vtable_t property(
    const char* member, const char* signature, sd_bus_property_get_t get,
    sd_bus_property_set_t set, decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_WRITABLE_PROPERTY(member, signature, get, set, 0,
                                             flags);
}

constexpr vtable_t property_o(const char* member, const char* signature,
                              sd_bus_property_get_t get, size_t offset,
                              decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_PROPERTY(member, signature, get, offset, flags);
}

constexpr vtable_t property_o(
    const char* member, const char* signature, sd_bus_property_get_t get,
    sd_bus_property_set_t set, size_t offset, decltype(vtable_t::flags) flags)
{
    return vtable_t SD_BUS_WRITABLE_PROPERTY(member, signature, get, set,
                                             offset, flags);
}

} // namespace vtable

using vtable_t = vtable::vtable_t;

} // namespace sdbusplus
