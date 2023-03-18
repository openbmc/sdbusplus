#pragma once
#include <limits>
#include <map>
#include <optional>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>
#include <string>
#include <systemd/sd-bus.h>
#include <tuple>
% for m in interface.methods + interface.properties + interface.signals:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-hpp-includes') }\
% endfor
<%
    def setOfPropertyTypes():
        return set(p.cppTypeParam(interface.name) for p in
                   interface.properties);
%>

namespace sdbusplus::bindings::server::${interface.cppNamespace()}
{

class ${interface.classname}
{
    public:
        static constexpr auto interface =
                "${interface.name}";

        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
        ${interface.classname}() = delete;
        ${interface.classname}(const ${interface.classname}&) = delete;
        ${interface.classname}& operator=(const ${interface.classname}&) = delete;
        ${interface.classname}(${interface.classname}&&) = delete;
        ${interface.classname}& operator=(${interface.classname}&&) = delete;
        virtual ~${interface.classname}() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        ${interface.classname}(bus_t& bus, const char* path) :
            _${interface.joinedName("_", "interface")}(
                bus, path, interface, _vtable, this),
            _intf(bus.getInterface()) {}

    % for e in interface.enums:
        enum class ${e.name}
        {
        % for v in e.values:
            ${v.name},
        % endfor
        };
    % endfor
\
    % if interface.properties:
        using PropertiesVariant = sdbusplus::utility::dedup_variant_t<
                ${",\n                ".join(sorted(setOfPropertyTypes()))}>;
        /** @brief Constructor to initialize the object from a map of
         *         properties.
         *
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         *  @param[in] vals - Map of property name to value for initialization.
         */
        ${interface.classname}(bus_t& bus, const char* path,
                     const std::map<std::string, PropertiesVariant>& vals,
                     bool skipSignal = false) :
            ${interface.classname}(bus, path)
        {
            for (const auto& v : vals)
            {
                setPropertyByName(v.first, v.second, skipSignal);
            }
        }

    % endif
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor
\
    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor
\
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
         *
         *  @note Throws if string is not a valid mapping.
         */
        static ${e.name} convert${e.name}FromString(const std::string& s);

        /** @brief Convert a string to an appropriate enum value.
         *  @param[in] s - The string to convert in the form of
         *                 "${interface.name}.<value name>"
         *  @return - The enum value or std::nullopt
         */
        static std::optional<${e.name}> convertStringTo${e.name}(
                const std::string& s) noexcept;

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
            _${interface.joinedName("_", "interface")}.emit_added();
        }

        /** @brief Emit interface removed */
        void emit_removed()
        {
            _${interface.joinedName("_", "interface")}.emit_removed();
        }

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
        static const vtable_t _vtable[];
        sdbusplus::server::interface_t
                _${interface.joinedName("_", "interface")};
        sdbusplus::SdBusInterface* _intf;
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
/* Specialization of sdbusplus::server::convertForMessage
 * for enum-type ${interface.classname}::${e.name}.
 *
 * This converts from the enum to a constant c-string representing the enum.
 *
 * @param[in] e - Enum value to convert.
 * @return C-string representing the name for the enum value.
 */
inline std::string convertForMessage(${interface.classname}::${e.name} e)
{
    return ${interface.classname}::convert${e.name}ToString(e);
}
    % endfor

} // namespace sdbusplus::bindings::server::${interface.cppNamespace()}

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespace()} {

using sdbusplus::bindings::server::${interface.cppNamespace()}::${interface.classname};

} // namespace sdbusplus::${interface.old_cppNamespace()}
#endif

namespace sdbusplus::message::details
{
    % for e in interface.enums:
template <>
struct convert_from_string<
    bindings::server::${interface.cppNamespacedClass()}::${e.name}>
{
    static auto op(const std::string& value) noexcept
    {
        return bindings::server::
            ${interface.cppNamespacedClass()}::convertStringTo${e.name}(value);
    }
};

template <>
struct convert_to_string<
    bindings::server::${interface.cppNamespacedClass()}::${e.name}>
{
    static std::string op(
        bindings::server::${interface.cppNamespacedClass()}::${e.name} value)
    {
        return bindings::server::
            ${interface.cppNamespacedClass()}::convert${e.name}ToString(value);
    }
};
    % endfor
} // namespace sdbusplus::message::details
