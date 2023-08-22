#pragma once
#include <sdbusplus/async/server.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/server/transaction.hpp>

#include <type_traits>

% for h in interface.cpp_includes():
#include <${h}>
% endfor
#include <${interface.headerFile()}>

namespace sdbusplus::aserver::${interface.cppNamespace()}
{

namespace details
{
// forward declaration
template <typename Instance, typename Server>
class ${interface.classname};
} // namespace details

template <typename Instance, typename Server = void>
struct ${interface.classname} :
    public std::conditional_t<
        std::is_void_v<Server>,
        sdbusplus::async::server_t<Instance, details::${interface.classname}>,
        details::${interface.classname}<Instance, Server>>
{
    template <typename... Args>
    ${interface.classname}(Args&&... args) :
        std::conditional_t<
            std::is_void_v<Server>,
            sdbusplus::async::server_t<Instance, details::${interface.classname}>,
            details::${interface.classname}<Instance, Server>>(std::forward<Args>(args)...)
    {}
};

namespace details
{

namespace server_details = sdbusplus::async::server::details;

template <typename Instance, typename Server>
class ${interface.classname} :
    public sdbusplus::common::${interface.cppNamespacedClass()},
    protected server_details::server_context_friend
{
  public:
    explicit ${interface.classname}(const char* path) :
        _${interface.joinedName("_", "interface")}(
            _context(), path, interface, _vtable, this)
    {}

% for s in interface.signals:
${s.render(loader, "signal.aserver.emit.hpp.mako", signal=s, interface=interface)}
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

    /* Property access tags. */
% for p in interface.properties:
${p.render(loader, "property.aserver.tag.hpp.mako", property=p, interface=interface)}\
% endfor

% for p in interface.properties:
${p.render(loader, "property.aserver.get.hpp.mako", property=p, interface=interface)}
% endfor
% for p in interface.properties:
${p.render(loader, "property.aserver.set.hpp.mako", property=p, interface=interface)}
% endfor

  protected:
% for p in interface.properties:
${p.render(loader, "property.aserver.value.hpp.mako", property=p, interface=interface)}\
% endfor

  private:
    /** @return the async context */
    sdbusplus::async::context& _context()
    {
        return server_details::server_context_friend::context<Server>();
    }

    sdbusplus::server::interface_t
        _${interface.joinedName("_", "interface")};

% for p in interface.properties:
${p.render(loader, "property.aserver.typeid.hpp.mako", property=p, interface=interface)}\
% endfor
% for s in interface.signals:
${s.render(loader, "signal.aserver.typeid.hpp.mako", signal=s, interface=interface)}\
% endfor

% for p in interface.properties:
${p.render(loader, "property.aserver.callback.hpp.mako", property=p, interface=interface)}
% endfor

    static constexpr sdbusplus::vtable_t _vtable[] = {
        vtable::start(),

% for p in interface.properties:
${p.render(loader, "property.aserver.vtable.hpp.mako", property=p, interface=interface)}\
% endfor
% for s in interface.signals:
${s.render(loader, "signal.aserver.vtable.hpp.mako", signal=s, interface=interface)}\
% endfor

        vtable::end(),
    };
};

} // namespace details
} // namespace sdbusplus::aserver::${interface.cppNamespace()}
