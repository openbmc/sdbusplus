#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/vtable.hpp>

#include <new>
#include <string>

namespace sdbusplus::async
{

/** Control how ObjectManager/Interfaces signals are emitted when a server
 *  object is created.
 */
enum class signal_action
{
    /** Emit 'object-added' on construction (and 'object-removed' on
     *  destruction). */
    emit_object_added,
    /** Emit 'interfaces-added' for each contained interface on
     *  construction. */
    emit_interface_added,
    /** No automatic added signal.  The added signal can be emitted manually
     *  via emit_object_added() or emit_interfaces_added(); destruction only
     *  emits the matching 'removed' signal for whichever 'added' was sent,
     *  and stays silent if nothing was ever emitted. */
    defer_emit,
    /** No signals are emitted. */
    emit_no_signals,
    /** Default behavior.  This is a distinct value (currently equivalent to
     *  defer_emit) so that the default can be changed or deprecated in the
     * future. */
    default_behavior,
};

namespace server
{

namespace details
{
struct server_context_friend;

/** Map a server-level action to the action passed to contained bases.
 *
 *  Bases stay silent unless the server defers (partial init) or folds an
 *  interface-added over them.  An explicit emit_no_signals, or an
 *  object-level emission owned by the server, locks bases so a later manual
 *  emit_added() is refused.
 */
constexpr sdbusplus::async::signal_action base_action(
    sdbusplus::async::signal_action act)
{
    using sdbusplus::async::signal_action;
    if (act == signal_action::defer_emit ||
        act == signal_action::emit_interface_added ||
        act == signal_action::default_behavior)
    {
        return signal_action::defer_emit;
    }
    return signal_action::emit_no_signals;
}
} // namespace details

template <typename Instance, template <typename, typename> typename... Types>
class server :
    public sdbusplus::async::context_ref,
    public Types<Instance, server<Instance, Types...>>...
{
  public:
    using Self = server<Instance, Types...>;
    friend details::server_context_friend;

    using signal_action = sdbusplus::async::signal_action;

    server() = delete;

    explicit server(sdbusplus::async::context& ctx, const char* path,
                    signal_action act = signal_action::default_behavior) :
        context_ref(ctx),
        Types<Instance, Self>(path, details::base_action(act))...,
        sdbusplus_async_server_path_(path)
    {
        check_signal_action(act);
    }
    explicit server(sdbusplus::async::context& ctx,
                    const sdbusplus::object_path& path,
                    signal_action act = signal_action::default_behavior) :
        server(ctx, path.str.c_str(), act)
    {}

    // This constructor accepting one properties_t per interface:
    explicit server(sdbusplus::async::context& ctx, const char* path,
                    typename Types<Instance, Self>::properties_t... propValues,
                    signal_action act = signal_action::default_behavior) :
        context_ref(ctx),
        Types<Instance, Self>(path, propValues, details::base_action(act))...,
        sdbusplus_async_server_path_(path)
    {
        check_signal_action(act);
    }
    explicit server(sdbusplus::async::context& ctx,
                    const sdbusplus::object_path& path,
                    typename Types<Instance, Self>::properties_t... propValues,
                    signal_action act = signal_action::default_behavior) :
        server(ctx, path.str.c_str(), propValues..., act)
    {}

    ~server()
    {
        // Only pair an 'object-added' with its 'object-removed'.  A
        // deferred server that never emitted anything manual stays silent.
        if (sdbusplus_async_server_signalstate_ ==
            signal_action::emit_object_added)
        {
            ctx.emit_object_removed(sdbusplus_async_server_path_.c_str());
        }
    }

    /** Emit the 'object-added' signal, if not already sent. */
    void emit_object_added()
    {
        if (sdbusplus_async_server_signalstate_ == signal_action::defer_emit &&
            !any_interfaces_added())
        {
            ctx.emit_object_added(sdbusplus_async_server_path_.c_str());
            sdbusplus_async_server_signalstate_ =
                signal_action::emit_object_added;
            maybe_block_iface_added();
        }
    }

    /** Emit the 'interfaces-added' signals, if not already sent. */
    void emit_interfaces_added()
    {
        if (sdbusplus_async_server_signalstate_ == signal_action::defer_emit)
        {
            // Suppress the object-level remove first: the per-interface
            // emit calls set the per-interface state so the corresponding
            // emit_removed() happens automatically on destruction.  Never
            // also emit at the object level, even if the fold below throws
            // partway.
            sdbusplus_async_server_signalstate_ =
                signal_action::emit_no_signals;
            maybe_emit_iface_added();
        }
    }

  private:
    // These member names are purposefully chosen as long and, hopefully,
    // unique.  Since a server is composed via multiple-inheritance, all
    // members need to have unique names to ensure there is no ambiguity.
    std::string sdbusplus_async_server_path_;
    signal_action sdbusplus_async_server_signalstate_ =
        signal_action::defer_emit;

    /** Call emit_added on all the composed types. */
    void maybe_emit_iface_added()
    {
        (try_emit<Types<Instance, Self>>(), ...);
    }

    /** Call emit_added for an individual composed type. */
    template <class T>
    void try_emit()
    {
        // If T is an interface class with emit_added, call it.
        if constexpr (requires(T& t) { t.emit_added(); })
        {
            static_cast<T*>(this)->emit_added();
        }
    }

    /** True if any contained interface already sent 'interfaces-added'. */
    bool any_interfaces_added()
    {
        return (try_added<Types<Instance, Self>>() || ... || false);
    }

    /** Query added-state for an individual composed type. */
    template <class T>
    bool try_added()
    {
        // Bases that track per-interface emission expose interfaces_added().
        if constexpr (requires(const T& t) { t.interfaces_added(); })
        {
            return static_cast<const T*>(this)->interfaces_added();
        }
        // Bases without tracking (e.g. hand-written test doubles without
        // the query) are conservatively treated as not added.
        return false;
    }

    /** Block contained bases so a later manual emit_added() is refused.
     *
     *  Every composed base must cooperate: a base that neither tracks
     *  emission (interfaces_added) nor blocks (block_emit_added) would
     *  silently escape the server's object/interface pairing guarantee.
     */
    void maybe_block_iface_added()
    {
        (try_block<Types<Instance, Self>>(), ...);
    }

    /** Block an individual composed type. */
    template <class T>
    void try_block()
    {
        static_assert(
            !requires(T& t) { t.emit_added(); } ||
                requires(T& t) { t.block_emit_added(); },
            "Composed interface type has emit_added() but no block_emit_added(); the server cannot enforce object/interface signal pairing.");
        if constexpr (requires(T& t) { t.block_emit_added(); })
        {
            static_cast<T*>(this)->block_emit_added();
        }
    }

    void check_signal_action(signal_action act)
    {
        switch (act)
        {
            case signal_action::emit_object_added:
                if (!any_interfaces_added())
                {
                    ctx.emit_object_added(sdbusplus_async_server_path_.c_str());
                    sdbusplus_async_server_signalstate_ =
                        signal_action::emit_object_added;
                    maybe_block_iface_added();
                }
                break;

            case signal_action::emit_interface_added:
                // Suppress the object-level remove first, as in
                // emit_interfaces_added().
                sdbusplus_async_server_signalstate_ =
                    signal_action::emit_no_signals;
                maybe_emit_iface_added();
                break;

            case signal_action::defer_emit:
            case signal_action::default_behavior:
                // Resolve the placeholder default to the current behavior.
                sdbusplus_async_server_signalstate_ = signal_action::defer_emit;
                break;

            case signal_action::emit_no_signals:
                sdbusplus_async_server_signalstate_ =
                    signal_action::emit_no_signals;
                break;
        }
    }
};

} // namespace server

template <typename Instance, template <typename, typename> typename... Types>
using server_t = server::server<Instance, Types...>;

namespace server::details
{
/* Indirect so that the generated Types can access the server_t's context.
 *
 * If P2893 gets into C++26 we could eliminate this because we can set all
 * the Types as friends directly.
 */
struct server_context_friend
{
    template <typename Client, typename Self>
    static sdbusplus::async::context& context(Self* self)
    {
        return std::launder(static_cast<Client*>(self))->ctx;
    }
};

/* Determine if a type has a get_property call. */
template <typename Tag, typename Instance>
concept has_get_property_nomsg =
    requires(const Instance& i) { i.get_property(Tag{}); };

/* Determine if a type has a get property call that requires a msg. */
template <typename Tag, typename Instance>
concept has_get_property_msg =
    requires(const Instance& i, sdbusplus::message_t& m) {
        i.get_property(Tag{}, m);
    };

/* Determine if a type has any get_property call. */
template <typename Tag, typename Instance>
concept has_get_property = has_get_property_nomsg<Tag, Instance> ||
                           has_get_property_msg<Tag, Instance>;

/* Determine if a type is missing the 'const' on get-property calls. */
template <typename Tag, typename Instance>
concept has_get_property_missing_const =
    !has_get_property<Tag, Instance> &&
    (
        requires(Instance& i) { i.get_property(Tag{}); } ||
        requires(Instance& i, sdbusplus::message_t& m) {
            i.get_property(Tag{}, m);
        });

/* Determine if a type has a set_property call. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property_nomsg =
    requires(Instance& i, Arg&& a) {
        i.set_property(Tag{}, std::forward<Arg>(a));
    };

/* Determine if a type has a set property call that requires a msg. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property_msg =
    requires(Instance& i, sdbusplus::message_t& m, Arg&& a) {
        i.set_property(Tag{}, m, std::forward<Arg>(a));
    };

/* Determine if a type has any set_property call. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property = has_set_property_nomsg<Tag, Instance, Arg> ||
                           has_set_property_msg<Tag, Instance, Arg>;

/* Determine if a type has a method call. */
template <typename Tag, typename Instance, typename... Args>
concept has_method_nomsg = requires(Instance& i, Args&&... a) {
                               i.method_call(Tag{}, std::forward<Args>(a)...);
                           };

/* Determine if a type has a method call that requires a msg. */
template <typename Tag, typename Instance, typename... Args>
concept has_method_msg =
    requires(Instance& i, sdbusplus::message_t& m, Args&&... a) {
        i.method_call(Tag{}, m, std::forward<Args>(a)...);
    };

/* Determine if a type has any method call. */
template <typename Tag, typename Instance, typename... Args>
concept has_method = has_method_nomsg<Tag, Instance, Args...> ||
                     has_method_msg<Tag, Instance, Args...>;

} // namespace server::details

} // namespace sdbusplus::async
