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
    static int _callback_m_${m.snake_case}(sd_bus_message* msg, void* context,
                                     sd_bus_error* error [[maybe_unused]])
        requires (server_details::has_method<
                            ${m.snake_case}_t, Instance${", " if m.parameters else ""}${", ".join([p.cppTypeParam(interface.name) for p in m.parameters])}${">)" if m.parameters else ">)"}
    {
        auto self = static_cast<${interface.classname}*>(context);
        auto self_i = static_cast<Instance*>(self);

        try
        {
            auto m = sdbusplus::message_t{msg};
% if len(m.parameters) == 1:
            auto ${m.parameters[0].camelCase} =
                m.unpack<${m.parameters[0].cppTypeParam(interface.name)}>();
% elif m.parameters:
            auto ${"[" + ", ".join([p.camelCase for p in m.parameters]) + "]"} =
                m.unpack<${", ".join([p.cppTypeParam(interface.name) for p in m.parameters])}>();
% endif

            constexpr auto has_method_msg =
                server_details::has_method_msg<
                    ${m.snake_case}_t, Instance${", " if m.parameters else ""}${", ".join([p.cppTypeParam(interface.name) for p in m.parameters])}>;

            if constexpr (has_method_msg)
            {
                constexpr auto is_async = std::is_same_v<
                    boost::asio::awaitable<${"std::tuple<" + ", ".join([r.cppTypeParam(interface.name) for r in m.returns]) + ">" if len(m.returns) > 1 else (m.returns[0].cppTypeParam(interface.name) if m.returns else "void")}>,
                    decltype(self_i->method_call(
                        ${m.snake_case}_t{}, m${", " if m.parameters else ""}
                        ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}))>;

                if constexpr (!is_async)
                {
                    auto r = m.new_method_return();
% if m.returns:
                    r.append(self_i->method_call(${m.snake_case}_t{}, m${", " if m.parameters else ""}
                            ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}));
% else:
                    self_i->method_call(${m.snake_case}_t{}, m${", " if m.parameters else ""}
                            ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])});
% endif
                    r.method_return();
                }
                else
                {
                    auto fn =
                        [](auto self, auto self_i, sdbusplus::message_t m${", " if m.parameters else ""}
                           ${", ".join([p.cppTypeParam(interface.name) + " " + p.camelCase for p in m.parameters])}) -> boost::asio::awaitable<void> {
                        try
                        {
                            auto r = m.new_method_return();
% if m.returns:
                            r.append(co_await self_i->method_call(
                                ${m.snake_case}_t{}, m${", " if m.parameters else ""}
                                ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}));
% else:
                            co_await self_i->method_call(
                                ${m.snake_case}_t{}, m${", " if m.parameters else ""}
                                ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])});
% endif

                            r.method_return();
                            co_return;
                        }
% for e in m.errors:
                        catch(const ${interface.errorNamespacedClass(e)}& e)
                        {
                            m.new_method_error(e).method_return();
                            co_return;
                        }
% endfor
                        catch (const std::exception&)
                        {
                            self->_context().get_bus().set_current_exception(
                                std::current_exception());
                            co_return;
                        }
                    };

                    boost::asio::co_spawn(
                        self->_context().ctx,
                        std::bind(fn, self, self_i, m${", " if m.parameters else ""}
                                  ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}),
                        boost::asio::detached);
                }
            }
            else
            {
                constexpr auto is_async [[maybe_unused]] = std::is_same_v<
                    boost::asio::awaitable<${"std::tuple<" + ", ".join([r.cppTypeParam(interface.name) for r in m.returns]) + ">" if len(m.returns) > 1 else (m.returns[0].cppTypeParam(interface.name) if m.returns else "void")}>,
                    decltype(self_i->method_call(
                        ${m.snake_case}_t{}${", " if m.parameters else ""}
                        ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}))>;

                if constexpr (!is_async)
                {
                    auto r = m.new_method_return();
% if m.returns:
                    r.append(self_i->method_call(${m.snake_case}_t{}${", " if m.parameters else ""}
                            ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}));
% else:
                    self_i->method_call(${m.snake_case}_t{}${", " if m.parameters else ""}
                            ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])});
% endif
                    r.method_return();
                }
                else
                {
                    auto fn =
                        [](auto self, auto self_i, sdbusplus::message_t m${", " if m.parameters else ""}
                           ${", ".join([p.cppTypeParam(interface.name) + " " + p.camelCase for p in m.parameters])}) -> boost::asio::awaitable<void> {
                        try
                        {
                            auto r = m.new_method_return();
% if m.returns:
                            r.append(co_await self_i->method_call(
                                ${m.snake_case}_t{}${", " if m.parameters else ""}
                                ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}));
% else:
                            co_await self_i->method_call(
                                ${m.snake_case}_t{}${", " if m.parameters else ""}
                                ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])});
% endif

                            r.method_return();
                            co_return;
                        }
% for e in m.errors:
                        catch(const ${interface.errorNamespacedClass(e)}& e)
                        {
                            m.new_method_error(e).method_return();
                            co_return;
                        }
% endfor
                        catch (const std::exception&)
                        {
                            m.new_method_error(
                                 sdbusplus::xyz::openbmc_project::Common::
                                     Error::InternalFailure())
                                .method_return();
                            co_return;
                        }
                    };

                    boost::asio::co_spawn(
                        self->_context().ctx,
                        std::bind_front(fn, self, self_i, m${", " if m.parameters else ""}
                                        ${", ".join(["std::move(" + p.camelCase + ")" for p in m.parameters])}),
                        boost::asio::detached);
                }
            }
        }
% for e in m.errors:
        catch(const ${interface.errorNamespacedClass(e)}& e)
        {
            return e.set_error(error);
        }
% endfor
        catch (const std::exception& e)
        {
            return sd_bus_error_set(error, "std::exception", e.what());
        }

        return 1;
    }

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