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
constexpr vtable_t method(const char *member, const char *signature,
                          const char *result, sd_bus_message_handler_t handler,
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
constexpr vtable_t method_o(const char *member, const char *signature,
                            const char *result,
                            sd_bus_message_handler_t handler, size_t offset,
                            decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_SIGNAL entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] flags - None supported.
 */
constexpr vtable_t signal(const char *member, const char *signature,
                          decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] get - Functor to call on property get.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property(const char *member, const char *signature,
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
constexpr vtable_t property(const char *member, const char *signature,
                            sd_bus_property_get_t get,
                            sd_bus_property_set_t set,
                            decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] offset - Offset within object for property.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property_o(const char *member, const char *signature,
                              size_t offset,
                              decltype(vtable_t::flags) flags = 0);

/** Create a SD_BUS_WRITABLE_PROPERTY entry.
 *
 * @param[in] member - Name of signal.
 * @param[in] signature - Signature of signal.
 * @param[in] set - Functor to call on property set.
 * @param[in] offset - Offset within object for property.
 * @param[in] flags - optional sdbusplus::vtable::property_ value.
 */
constexpr vtable_t property_o(const char *member, const char *signature,
                              sd_bus_property_set_t set, size_t offset,
                              decltype(vtable_t::flags) flags = 0);

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
} // namespace property_

constexpr vtable_t start(decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_START;
    v.flags = flags;
    v.x.start = decltype(v.x.start){sizeof(vtable_t)};

    return v;
}

constexpr vtable_t end()
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_END;

    return v;
}

constexpr vtable_t method(const char *member, const char *signature,
                          const char *result, sd_bus_message_handler_t handler,
                          decltype(vtable_t::flags) flags)
{
    return method_o(member, signature, result, handler, 0, flags);
}

constexpr vtable_t method_o(const char *member, const char *signature,
                            const char *result,
                            sd_bus_message_handler_t handler, size_t offset,
                            decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_METHOD;
    v.flags = flags;
    v.x.method =
        decltype(v.x.method){member, signature, result, handler, offset};

    return v;
}

constexpr vtable_t signal(const char *member, const char *signature,
                          decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_SIGNAL;
    v.flags = flags;
    v.x.signal = decltype(v.x.signal){member, signature};

    return v;
}

constexpr vtable_t property(const char *member, const char *signature,
                            sd_bus_property_get_t get,
                            decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_PROPERTY;
    v.flags = flags;
    v.x.property = decltype(v.x.property){member, signature, get, nullptr, 0};

    return v;
}

constexpr vtable_t property(const char *member, const char *signature,
                            sd_bus_property_get_t get,
                            sd_bus_property_set_t set,
                            decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_WRITABLE_PROPERTY;
    v.flags = flags;
    v.x.property = decltype(v.x.property){member, signature, get, set, 0};

    return v;
}

constexpr vtable_t property_o(const char *member, const char *signature,
                              size_t offset, decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_PROPERTY;
    v.flags = flags;
    v.x.property =
        decltype(v.x.property){member, signature, nullptr, nullptr, offset};

    return v;
}

constexpr vtable_t property_o(const char *member, const char *signature,
                              sd_bus_property_set_t set, size_t offset,
                              decltype(vtable_t::flags) flags)
{
    vtable_t v{};
    v.type = _SD_BUS_VTABLE_WRITABLE_PROPERTY;
    v.flags = flags;
    v.x.property =
        decltype(v.x.property){member, signature, nullptr, set, offset};

    return v;
}

} // namespace vtable

} // namespace sdbusplus
