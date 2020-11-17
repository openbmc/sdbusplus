#pragma once

#include <${"/".join(interface.name.split('.')  + [ 'server.hpp' ])}>

#include "gmock/gmock.h"

using ::testing::An;

<%

    namespaces = interface.namespaces
    original_classname = interface.classname

    mock_classname = "Mock" + original_classname

%>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace server
{

class ${mock_classname} : public ${original_classname}
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
        ${mock_classname}() = delete;
        ${mock_classname}(const ${mock_classname}&) = delete;
        ${mock_classname}& operator=(const ${mock_classname}&) = delete;
        ${mock_classname}(${mock_classname}&&) = delete;
        ${mock_classname}& operator=(${mock_classname}&&) = delete;
        virtual ~${mock_classname}() = default;

        /** @brief Constructor to put object onto bus at a dbus path.
         *  @param[in] bus - Bus to attach to.
         *  @param[in] path - Path to attach at.
         */
        ${mock_classname}(bus::bus& bus, const char* path):
        ${original_classname}(bus, path) {
            /* 
             * By default, all setter/getters are delegated to the real object.
            */
            % for p in interface.properties:

            ON_CALL(*this, ${p.camelCase}()).WillByDefault([this]()
            { 
                return ${original_classname}::${p.camelCase}(); 
            });

            ON_CALL(*this, ${p.camelCase}
                (An<${p.cppTypeParam(interface.name)}>())).WillByDefault([this](
                    ${p.cppTypeParam(interface.name)} v)
            { 
                return ${original_classname}::${p.camelCase}(v);
            });

            % endfor
        };

    % for m in interface.methods:
        MOCK_METHOD((${m.cpp_return_type(interface)}),
                    ${ m.camelCase },
                    (${ m.get_parameters_str(interface) }),
                    (override));

    % endfor

    /**
    * mock properties
    */
    % for p in interface.properties:

    MOCK_METHOD((${p.cppTypeParam(interface.name)}),
                ${p.camelCase},
                (),
                (const, override));
    MOCK_METHOD((${p.cppTypeParam(interface.name)}),
                ${p.camelCase},
                (${p.cppTypeParam(interface.name)} value),
                (override));

    % endfor

};


} // namespace server
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor

} // namespace sdbusplus
