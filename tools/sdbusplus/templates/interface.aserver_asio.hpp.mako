#pragma once
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <sdbusplus/asio/aserver.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/server/transaction.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <type_traits>

% for h in interface.cpp_includes():
#include <${h}>
% endfor
#include <${interface.headerFile()}>

namespace sdbusplus::asio::aserver::${interface.cppNamespace()}
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
        sdbusplus::asio::async::server_t<Instance, details::${interface.classname}>,
        details::${interface.classname}<Instance, Server>>
{
    template <typename... Args>
    ${interface.classname}(Args&&... args) :
        std::conditional_t<
            std::is_void_v<Server>,
            sdbusplus::asio::async::server_t<Instance, details::${interface.classname}>,
            details::${interface.classname}<Instance, Server>>(
            std::forward<Args>(args)...)
    {}
};

namespace details
{

namespace server_details = sdbusplus::asio::async::server::details;

template <typename Instance, typename Server>
class ${interface.classname} :
    public sdbusplus::common::${interface.cppNamespacedClass()},
    protected server_details::server_context_friend
{
  public:
    explicit ${interface.classname}(sdbusplus::bus::bus& bus, const char* path) :
        _${interface.joinedName("_", "interface")}(
            bus, path, interface, _vtable, this)
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

    /* Method tags. */
% for m in interface.methods:
${m.render(loader, "method.aserver.tag.hpp.mako", method=m, interface=interface)}\
% endfor

% for p in interface.properties:
${p.render(loader, "property.aserver.get.hpp.mako", property=p, interface=interface)}
% endfor
% for p in interface.properties:
${p.render(loader, "property.aserver.set.hpp.mako", property=p, interface=interface)}
% endfor

  protected:
% for p in interface.properties:
    ${p.cppTypeParam(interface.name)} ${p.snake_case}_${p.default_value(interface.name)};
% endfor

  private:
    /** Wrapper to provide get_bus() compatibility */
    struct context_wrapper
    {
        boost::asio::io_context& ctx;
        
        struct bus_wrapper
        {
            void set_current_exception(std::exception_ptr) {}
        };
        
        bus_wrapper get_bus() { return bus_wrapper{}; }
    };

    /** @return the async context */
    context_wrapper _context()
    {
        return context_wrapper{server_details::server_context_friend::context<
            Server, ${interface.classname}>(this)};
    }

    sdbusplus::server::interface_t
        _${interface.joinedName("_", "interface")};

% for p in interface.properties:
${p.render(loader, "property.aserver.typeid.hpp.mako", property=p, interface=interface)}\
% endfor
% for m in interface.methods:
${m.render(loader, "method.aserver.typeid.hpp.mako", method=m, interface=interface)}\
% endfor
% for s in interface.signals:
${s.render(loader, "signal.aserver.typeid.hpp.mako", signal=s, interface=interface)}\
% endfor

% for p in interface.properties:
${p.render(loader, "property.aserver.callback.hpp.mako", property=p, interface=interface)}
% endfor

% for m in interface.methods:
${m.render(loader, "method.aserver_asio.callback.hpp.mako", method=m, interface=interface)}
% endfor

    static constexpr sdbusplus::vtable_t _vtable[] = {
        vtable::start(),

% for p in interface.properties:
${p.render(loader, "property.aserver.vtable.hpp.mako", property=p, interface=interface)}\
% endfor
% for m in interface.methods:
        vtable::method("${m.CamelCase}",
                       _method_typeid_p_${m.snake_case}.data(),
                       _method_typeid_r_${m.snake_case}.data(),
                       _callback_m_${m.snake_case}),

% endfor
% for s in interface.signals:
${s.render(loader, "signal.aserver.vtable.hpp.mako", signal=s, interface=interface)}\
% endfor

        vtable::end(),
    };
};

} // namespace details
} // namespace sdbusplus::asio::aserver::${interface.cppNamespace()}