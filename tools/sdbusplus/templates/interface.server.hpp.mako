#pragma once
#include <limits>
#include <map>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <systemd/sd-bus.h>

% for h in interface.cpp_includes():
#include <${h}>
% endfor
#include <${interface.headerFile()}>

namespace sdbusplus::server::${interface.cppNamespace()}
{

class ${interface.classname} :
    public sdbusplus::common::${interface.cppNamespacedClass()}
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
            _sdbusplus_bus(bus) {}

    % if interface.properties:
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

        /** @return the bus instance */
        bus_t& get_bus()
        {
            return  _sdbusplus_bus;
        }

    protected:
        /** @return the current sdbus sender */
        std::string get_current_sender()
        {
            const char* sender = nullptr;
            if (_msg != nullptr) {
                sender = sd_bus_message_get_sender(_msg);
            }
            if (sender == nullptr) {
                throw exception::UnpackPropertyError(
                    "sender",
                    UnpackErrorReason::missingProperty);
            }
            return sender;
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
        bus_t&  _sdbusplus_bus;
        sd_bus_message* _msg;

    % for p in interface.properties:
        ${p.cppTypeParam(interface.name)} _${p.camelCase}${p.default_value(interface.name)};
    % endfor
};

} // namespace sdbusplus::server::${interface.cppNamespace()}

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespace()} {

using sdbusplus::server::${interface.cppNamespacedClass()};
    % if interface.enums:
using sdbusplus::common::${interface.cppNamespace()}::convertForMessage;
    % endif

} // namespace sdbusplus::${interface.old_cppNamespace()}
#endif
