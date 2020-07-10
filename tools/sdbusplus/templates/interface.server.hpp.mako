#pragma once
#include <map>
#include <string>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <systemd/sd-bus.h>
#include <tuple>
#include <variant>
% for m in interface.methods + interface.properties + interface.signals:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-hpp-includes') }
% endfor
<%
    namespaces = interface.name.split('.')
    classname = namespaces.pop()

    def setOfPropertyTypes():
        return set(p.cppTypeParam(interface.name) for p in
                   interface.properties);

    def cppNamespace():
        return "::".join(namespaces) + "::server::" + classname
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
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
        ${classname}() = delete;
        ${classname}(const ${classname}&) = delete;
        ${classname}& operator=(const ${classname}&) = delete;
        ${classname}(${classname}&&) = delete;
        ${classname}& operator=(${classname}&&) = delete;
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

    % if interface.properties:
        using PropertiesVariant = std::variant<
                ${",\n                ".join(sorted(setOfPropertyTypes()))}>;

        /** @brief Constructor to initialize the object from a map of
         *         properties.
         *
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         *  @param[in] vals - Map of property name to value for initialization.
         */
        ${classname}(bus::bus& bus, const char* path,
                     const std::map<std::string, PropertiesVariant>& vals,
                     bool skipSignal = false);

    % endif
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor

    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor

    % for p in interface.properties:
        /** Get value of ${p.name} */
        virtual ${p.cppTypeParam(interface.name)} ${p.camelCase}() const;
        /** Set value of ${p.name} with option to skip sending signal */
        virtual ${p.cppTypeParam(interface.name)} \
${p.camelCase}(${p.cppTypeParam(interface.name)} value,
               bool skipSignal);
        /** Set value of ${p.name} */
        virtual ${p.cppTypeParam(interface.name)} \
${p.camelCase}(${p.cppTypeParam(interface.name)} value);
    % endfor

    % if interface.properties:
        /** @brief Sets a property by name.
         *  @param[in] _name - A string representation of the property name.
         *  @param[in] val - A variant containing the value to set.
         */
        void setPropertyByName(const std::string& _name,
                               const PropertiesVariant& val,
                               bool skipSignal = false);

        /** @brief Gets a property by name.
         *  @param[in] _name - A string representation of the property name.
         *  @return - A variant containing the value of the property.
         */
        PropertiesVariant getPropertyByName(const std::string& _name);

    % endif
    % for e in interface.enums:
        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "${interface.name}.<value name>"
         *  @return - The enum value.
         */
        static ${e.name} convert${e.name}FromString(const std::string& s);

        /** @brief Convert an enum value to a string.
         *  @param[in] e - The enum to convert to a string.
         *  @return - The string conversion in the form of
         *            "${interface.name}.<value name>"
         */
        static std::string convert${e.name}ToString(${e.name} e);
    % endfor

        /** @brief Emit interface added */
        void emit_added()
        {
            _${"_".join(interface.name.split('.'))}_interface.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _${"_".join(interface.name.split('.'))}_interface.emit_removed();
        }

        static constexpr auto interface = "${interface.name}";

    private:
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-header') }
    % endfor

    % for p in interface.properties:
        /** @brief sd-bus callback for get-property '${p.name}' */
        static int _callback_get_${p.name}(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        % if 'const' not in p.flags and 'readonly' not in p.flags:
        /** @brief sd-bus callback for set-property '${p.name}' */
        static int _callback_set_${p.name}(
            sd_bus*, const char*, const char*, const char*,
            sd_bus_message*, void*, sd_bus_error*);
        % endif

    % endfor

        static const vtable::vtable_t _vtable[];
        sdbusplus::server::interface::interface
                _${"_".join(interface.name.split('.'))}_interface;
        sdbusplus::SdBusInterface *_intf;

    % for p in interface.properties:
        % if p.defaultValue is not None:
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
inline std::string convertForMessage(${classname}::${e.name} e)
{
    return ${classname}::convert${e.name}ToString(e);
}
    % endfor

} // namespace server
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor

namespace message
{
namespace details
{
    % for e in interface.enums:
template <>
inline auto convert_from_string<${cppNamespace()}::${e.name}>(
        const std::string& value)
{
    return ${cppNamespace()}::convert${e.name}FromString(value);
}

template <>
inline std::string convert_to_string<${cppNamespace()}::${e.name}>(
        ${cppNamespace()}::${e.name} value)
{
    return ${cppNamespace()}::convert${e.name}ToString(value);
}
    % endfor
} // namespace details
} // namespace message
} // namespace sdbusplus
