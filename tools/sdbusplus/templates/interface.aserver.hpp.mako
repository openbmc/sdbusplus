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
    using signal_action = sdbusplus::async::signal_action;

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
    using signal_action = sdbusplus::async::signal_action;

    explicit ${interface.classname}(
        const char* path,
        signal_action act = signal_action::default_behavior) :
        _${interface.joinedName("_", "interface")}(
            _context(), path, interface, _vtable, this)
    {
        check_signal_action(act);
    }
    explicit ${interface.classname}(
        const sdbusplus::object_path& path,
        signal_action act = signal_action::default_behavior) :
        ${interface.classname}(path.str.c_str(), act)
    {}

    ${interface.classname}(
            const char* path,
            [[maybe_unused]] ${interface.classname}::properties_t props,
            signal_action act = signal_action::default_behavior)
        : ${interface.classname}(path, act)
    {
        % if interface.properties:
        properties = props;
        % endif
    }

    ${interface.classname}(
        const sdbusplus::object_path& path,
        [[maybe_unused]] ${interface.classname}::properties_t props,
        signal_action act = signal_action::default_behavior) :
        ${interface.classname}(path.str.c_str(), props, act)
    {}

    ~${interface.classname}()
    {
        // Emit the counterpart of whichever 'added' signal was sent:
        // interfaces-removed if emit_added() ran, object-removed if
        // emit_object_added() ran, and nothing otherwise.
        emit_removed();
        if (sdbusplus_async_iface_signalstate_ ==
            signal_action::emit_object_added)
        {
            _context().emit_object_removed(
                _${interface.joinedName("_", "interface")}.path().c_str());
        }
    }

    /** Emit the 'object-added' signal, if not already sent.
     *
     *  Refused if this interface already sent 'interfaces-added', since
     *  sending both an object-level and an interface-level added signal
     *  for the same object is never correct.
     */
    void emit_object_added()
    {
        if (sdbusplus_async_iface_signalstate_ == signal_action::defer_emit &&
            !sdbusplus_async_iface_added_)
        {
            _context().emit_object_added(
                _${interface.joinedName("_", "interface")}.path().c_str());
            sdbusplus_async_iface_signalstate_ =
                signal_action::emit_object_added;
        }
    }

% for s in interface.signals:
${s.render(loader, "signal.aserver.emit.hpp.mako", signal=s, interface=interface)}
% endfor

    /** Emit interface added. */
    void emit_added()
    {
        if (!sdbusplus_async_iface_added_ &&
            sdbusplus_async_iface_signalstate_ ==
                signal_action::defer_emit)
        {
            _context().emit_interfaces_added(
                _${interface.joinedName("_", "interface")}.path().c_str(),
                {interface});
            sdbusplus_async_iface_added_ = true;
            sdbusplus_async_iface_signalstate_ =
                signal_action::emit_no_signals;
        }
    }

    /** Query if 'interfaces-added' already sent. */
    bool interfaces_added() const
    {
        return sdbusplus_async_iface_added_;
    }

    /** Emit interface removed. */
    void emit_removed()
    {
        if (sdbusplus_async_iface_added_)
        {
            _context().emit_interfaces_removed(
                _${interface.joinedName("_", "interface")}.path().c_str(),
                {interface});
            sdbusplus_async_iface_added_ = false;
        }
    }

% for p in interface.properties:
${p.render(loader, "property.aserver.get.hpp.mako", property=p, interface=interface)}
% endfor
% for p in interface.properties:
${p.render(loader, "property.aserver.set.hpp.mako", property=p, interface=interface)}
% endfor

  protected:
% if interface.properties:
    properties_t properties{};
% endif

  private:
    /** @return the async context */
    sdbusplus::async::context& _context()
    {
        return server_details::server_context_friend::
            context<Server, ${interface.classname}>(this);
    }

    friend Server;

    // Long, unique member names to track signal state.
    sdbusplus::async::signal_action sdbusplus_async_iface_signalstate_ =
        sdbusplus::async::signal_action::defer_emit;
    bool sdbusplus_async_iface_added_ = false;

    /** Block manual emit_added().
     *
     *  Private: only the enclosing server may call this (friend Server
     *  above), after it sends 'object-added'.  The server owns the
     *  object-level emission (and its 'object-removed' pairing), so this
     *  silences the base entirely: a later manual emit_added() is refused
     *  and the destructor stays silent.
     */
    void block_emit_added()
    {
        if (sdbusplus_async_iface_signalstate_ == signal_action::defer_emit)
        {
            sdbusplus_async_iface_signalstate_ =
                signal_action::emit_no_signals;
        }
    }

    /** Check and run the action, mirroring sdbusplus::async::server. */
    void check_signal_action(signal_action act)
    {
        switch (act)
        {
            case signal_action::emit_object_added:
                if (!sdbusplus_async_iface_added_)
                {
                    _context().emit_object_added(
                        _${interface.joinedName("_", "interface")}.path()
                            .c_str());
                    sdbusplus_async_iface_signalstate_ =
                        signal_action::emit_object_added;
                }
                break;

            case signal_action::emit_interface_added:
                // Set up deferred state temporarily and then emit the
                // signal.  emit_added() transitions to emit_no_signals so
                // the destructor sends only 'interfaces-removed' (never
                // both removed signals).
                sdbusplus_async_iface_signalstate_ =
                    signal_action::defer_emit;
                emit_added();
                break;

            case signal_action::defer_emit:
            case signal_action::default_behavior:
                // Resolve the placeholder default to the current behavior.
                sdbusplus_async_iface_signalstate_ = signal_action::defer_emit;
                break;

            case signal_action::emit_no_signals:
                sdbusplus_async_iface_signalstate_ =
                    signal_action::emit_no_signals;
                break;
        }
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
${m.render(loader, "method.aserver.callback.hpp.mako", method=m, interface=interface)}\
% endfor

    static constexpr sdbusplus::vtable_t _vtable[] = {
        vtable::start(),

% for p in interface.properties:
${p.render(loader, "property.aserver.vtable.hpp.mako", property=p, interface=interface)}\
% endfor
% for m in interface.methods:
${m.render(loader, "method.aserver.vtable.hpp.mako", method=m, interface=interface)}\
% endfor
% for s in interface.signals:
${s.render(loader, "signal.aserver.vtable.hpp.mako", signal=s, interface=interface)}\
% endfor

        vtable::end(),
    };
};

} // namespace details
} // namespace sdbusplus::aserver::${interface.cppNamespace()}
