#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/server.hpp>
    <%
        namespaces = interface.name.split('.')
        classname = namespaces.pop()
    %>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace server
{

class ${classname}
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
        ${classname}() = delete;
        ${classname}(const ${classname}&) = delete;
        ${classname}& operator=(const ${classname}&) = delete;
        ${classname}(${classname}&&) = default;
        ${classname}& operator=(${classname}&&) = default;
        virtual ~${classname}() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        ${classname}(bus::bus& bus, const char* path);

    % for e in interface.enums:
        enum class ${e.name}
        {
        % for v in e.values:
            ${v.name},
        % endfor
        };
    % endfor

    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor

    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor

    % for p in interface.properties:
        /** Get value of ${p.name} */
        virtual ${p.cppTypeParam(interface.name)} ${p.camelCase}() const;
        /** Set value of ${p.name} */
        virtual ${p.cppTypeParam(interface.name)} \
${p.camelCase}(${p.cppTypeParam(interface.name)} value);
    % endfor

    % for e in interface.enums:
    /** @brief Convert a string to an appropriate enum value.
     *  @param[in] s - The string to convert in the form of
     *                 "${interface.name}.<value name>"
     *  @return - The enum value.
     */
    static ${e.name} convert${e.name}FromString(std::string& s);
    % endfor

    private:
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-header') }
    % endfor

    % for p in interface.properties:
        /** @brief sd-bus callback for get-property '${p.name}' */
        static int _callback_get_${p.name}(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        /** @brief sd-bus callback for set-property '${p.name}' */
        static int _callback_set_${p.name}(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);

    % endfor

        static constexpr auto _interface = "${interface.name}";
        static const vtable::vtable_t _vtable[];
        sdbusplus::server::interface::interface
                _${"_".join(interface.name.split('.'))}_interface;

    % for p in interface.properties:
        % if p.defaultValue:
        ${p.cppTypeParam(interface.name)} _${p.camelCase} = \
            % if p.is_enum():
${p.cppTypeParam(interface.name)}::\
            % endif
${p.defaultValue};
        % else:
        ${p.cppTypeParam(interface.name)} _${p.camelCase}{};
        % endif
    % endfor

};

    % for e in interface.enums:
/* Specialization of sdbusplus::server::bindings::details::convertForMessage
 * for enum-type ${classname}::${e.name}.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
std::string convertForMessage(${classname}::${e.name} e);
    % endfor

} // namespace server
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
