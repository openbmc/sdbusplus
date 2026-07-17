#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <sdbusplus/vtable.hpp>

#include <cassert>
#include <cerrno>
#include <concepts>
#include <flat_map>
#include <format>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace sdbusplus::async
{

namespace details
{

template <typename... Types>
const char* signature_of() noexcept
{
    static constexpr auto sig =
        utility::tuple_to_array(message::types::type_id<Types...>());
    return sig.data();
}

template <typename T>
struct method_traits
{};

template <typename R, typename C, typename... Args>
struct method_traits<task<R> (C::*)(Args...)>
{
    using return_t = R;
    using raw_args_t = std::tuple<Args...>;
    using args_t = utility::decay_tuple_t<raw_args_t>;
};

template <typename R, typename C, typename... Args>
struct method_traits<task<R> (C::*)(Args...) const> :
    method_traits<task<R> (C::*)(Args...)>
{};

template <typename R, typename C, typename... Args>
struct method_traits<task<R> (C::*)(Args...) noexcept> :
    method_traits<task<R> (C::*)(Args...)>
{};

template <typename R, typename C, typename... Args>
struct method_traits<task<R> (C::*)(Args...) const noexcept> :
    method_traits<task<R> (C::*)(Args...)>
{};

template <typename R, typename... Args>
struct method_traits<task<R> (*)(Args...)>
{
    using return_t = R;
    using raw_args_t = std::tuple<Args...>;
    using args_t = utility::decay_tuple_t<raw_args_t>;
};

template <typename T>
    requires requires { &T::operator(); }
struct method_traits<T> : method_traits<decltype(&T::operator())>
{};

template <typename F>
concept method_handler =
    requires { typename method_traits<std::decay_t<F>>::args_t; };

template <typename T>
inline constexpr bool is_tuple = false;
template <typename... Ts>
inline constexpr bool is_tuple<std::tuple<Ts...>> = true;

template <typename Tuple>
struct tuple_signature;
template <typename... Ts>
struct tuple_signature<std::tuple<Ts...>>
{
    static const char* value() noexcept
    {
        return signature_of<Ts...>();
    }
};

template <typename R>
struct return_signature
{
    static const char* value() noexcept
    {
        return signature_of<R>();
    }
};
template <>
struct return_signature<void>
{
    static const char* value() noexcept
    {
        return signature_of<>();
    }
};
template <typename... Ts>
struct return_signature<std::tuple<Ts...>>
{
    static const char* value() noexcept
    {
        return signature_of<Ts...>();
    }
};

struct property_base
{
    property_base(decltype(vtable_t::flags) flags, bool writable) :
        flags(flags), writable(writable)
    {}
    virtual ~property_base() = default;

    virtual void get(message_t& reply) = 0;
    virtual bool set(message_t& value) = 0;
    virtual const char* signature() const noexcept = 0;

    decltype(vtable_t::flags) flags;
    bool writable;
};

template <std::equality_comparable T>
struct property_instance : property_base
{
    property_instance(T initial, std::move_only_function<T(T)> on_set,
                      decltype(vtable_t::flags) flags) :
        property_base(flags, static_cast<bool>(on_set)),
        value(std::move(initial)), on_set(std::move(on_set))
    {}

    void get(message_t& reply) override
    {
        reply.append(value);
    }

    bool set(message_t& m) override
    {
        assert(on_set);
        auto next = on_set(m.unpack<T>());
        bool changed = (next != value);
        value = std::move(next);
        return changed;
    }

    const char* signature() const noexcept override
    {
        return signature_of<T>();
    }

    T value;
    std::move_only_function<T(T)> on_set;
};

struct method_base
{
    virtual ~method_base() = default;

    virtual void spawn_invoke(std::shared_ptr<method_base> self, context& ctx,
                              message_t msg) = 0;

    virtual const char* in_signature() const noexcept = 0;
    virtual const char* out_signature() const noexcept = 0;

    decltype(vtable_t::flags) flags = 0;
};

template <method_handler F>
struct method_instance : method_base
{
    using traits = method_traits<std::decay_t<F>>;
    using return_t = typename traits::return_t;
    using all_args_t = typename traits::args_t;

    static_assert(std::is_same_v<typename traits::raw_args_t, all_args_t>,
                  "method handler parameters must be taken by value");

    static constexpr bool wants_msg =
        std::is_same_v<utility::get_first_arg_t<all_args_t>, message_t>;
    using dbus_args_t =
        std::conditional_t<wants_msg, utility::strip_first_arg_t<all_args_t>,
                           all_args_t>;

    explicit method_instance(F f) : handler(std::move(f)) {}

    void spawn_invoke(std::shared_ptr<method_base> self, context& ctx,
                      message_t m) override
    {
        auto args = unpack_args(m, std::type_identity<dbus_args_t>{});

        auto on_error =
            execution::upon_error([self, m](std::exception_ptr e) mutable {
                try
                {
                    std::rethrow_exception(e);
                }
                catch (const sdbusplus::exception_t& ex)
                {
                    m.new_method_error(ex).method_return();
                }
                catch (const std::exception& ex)
                {
                    sd_bus_error err = SD_BUS_ERROR_NULL;
                    sd_bus_error_set(&err, SD_BUS_ERROR_FAILED, ex.what());
                    m.new_method_errno(-EINVAL, &err).method_return();
                    sd_bus_error_free(&err);
                }
                catch (...)
                {
                    m.new_method_errno(-EINVAL).method_return();
                }
            });

        if constexpr (std::is_void_v<return_t>)
        {
            ctx.spawn(invoke_handler(m, std::move(args)) |
                      execution::then([self, m]() mutable {
                          m.new_method_return().method_return();
                      }) |
                      std::move(on_error));
        }
        else if constexpr (is_tuple<return_t>)
        {
            ctx.spawn(
                invoke_handler(m, std::move(args)) |
                execution::then([self, m](return_t result) mutable {
                    auto r = m.new_method_return();
                    std::apply(
                        [&](auto&&... v) { (r.append(std::move(v)), ...); },
                        std::move(result));
                    r.method_return();
                }) |
                std::move(on_error));
        }
        else
        {
            ctx.spawn(invoke_handler(m, std::move(args)) |
                      execution::then([self, m](return_t result) mutable {
                          auto r = m.new_method_return();
                          r.append(std::move(result));
                          r.method_return();
                      }) |
                      std::move(on_error));
        }
    }

    const char* in_signature() const noexcept override
    {
        return tuple_signature<dbus_args_t>::value();
    }

    const char* out_signature() const noexcept override
    {
        return return_signature<return_t>::value();
    }

  private:
    auto invoke_handler(message_t& m, dbus_args_t args)
    {
        if constexpr (wants_msg)
        {
            sd_bus_message_rewind(m.get(), 1);
            return std::apply(
                handler, std::tuple_cat(std::make_tuple(m), std::move(args)));
        }
        else
        {
            return std::apply(handler, std::move(args));
        }
    }

    template <typename... Ts>
    static std::tuple<Ts...> unpack_args(message_t& m,
                                         std::type_identity<std::tuple<Ts...>>)
    {
        if constexpr (sizeof...(Ts) == 0)
        {
            return {};
        }
        else if constexpr (sizeof...(Ts) == 1)
        {
            return std::tuple<Ts...>{m.unpack<Ts...>()};
        }
        else
        {
            return m.unpack<Ts...>();
        }
    }

    F handler;
};

} // namespace details

enum class property_permission
{
    read_only,
    read_write
};

class dbus_interface : private context_ref
{
  public:
    dbus_interface() = delete;
    dbus_interface(const dbus_interface&) = delete;
    dbus_interface& operator=(const dbus_interface&) = delete;
    dbus_interface(dbus_interface&&) = delete;
    dbus_interface& operator=(dbus_interface&&) = delete;
    ~dbus_interface() = default;

    dbus_interface(context& ctx, std::string path, std::string name) :
        context_ref(ctx), _path(std::move(path)), _name(std::move(name))
    {}

    template <std::equality_comparable T>
    void register_property(
        std::string name, T initial,
        property_permission access = property_permission::read_only,
        decltype(vtable_t::flags) flags = vtable::property_::emits_change)
    {
        register_property_impl<T>(
            std::move(name), std::move(initial),
            access == property_permission::read_write
                ? std::move_only_function<T(T)>([](T v) { return v; })
                : std::move_only_function<T(T)>{},
            flags);
    }

    template <std::equality_comparable T>
    void register_property(
        std::string name, T initial, std::move_only_function<T(T)> on_set,
        decltype(vtable_t::flags) flags = vtable::property_::emits_change)
    {
        if (!on_set)
        {
            throw std::invalid_argument(std::format(
                "dbus_interface: null on_set hook for '{}'", name));
        }
        register_property_impl<T>(std::move(name), std::move(initial),
                                  std::move(on_set), flags);
    }

    template <details::method_handler F>
    void register_method(std::string name, F&& handler,
                         decltype(vtable_t::flags) flags = {})
    {
        assert_not_initialized();
        validate_member_name(name);
        auto holder =
            std::make_shared<details::method_instance<std::decay_t<F>>>(
                std::forward<F>(handler));
        holder->flags = flags;
        auto [_, inserted] =
            _methods.try_emplace(std::move(name), std::move(holder));
        if (!inserted)
        {
            throw std::invalid_argument(
                "dbus_interface: duplicate method name");
        }
    }

    template <typename... Args>
    void register_signal(std::string name, decltype(vtable_t::flags) flags = {})
    {
        assert_not_initialized();
        validate_member_name(name);
        auto [_, inserted] = _signals.try_emplace(
            std::move(name),
            std::pair{details::signature_of<Args...>(), flags});
        if (!inserted)
        {
            throw std::invalid_argument(
                "dbus_interface: duplicate signal name");
        }
    }

    void initialize();

    template <std::equality_comparable T>
    bool set_property(std::string_view name, T value)
    {
        auto it = _properties.find(name);
        if (it == _properties.end())
        {
            throw std::out_of_range(
                std::format("dbus_interface: unknown property '{}'", name));
        }
        auto* p =
            dynamic_cast<details::property_instance<T>*>(it->second.get());
        if (p == nullptr)
        {
            throw std::invalid_argument(std::format(
                "dbus_interface: property type mismatch for '{}'", name));
        }

        if (p->on_set)
        {
            value = p->on_set(std::move(value));
        }
        bool changed = (value != p->value);
        p->value = std::move(value);

        if (changed && initialized() &&
            (p->flags & (vtable::property_::emits_change |
                         vtable::property_::emits_invalidation)))
        {
            _interface->property_changed(it->first.c_str());
        }
        return changed;
    }

    template <std::equality_comparable T>
    T get_property(std::string_view name) const
    {
        auto it = _properties.find(name);
        if (it == _properties.end())
        {
            throw std::out_of_range(
                std::format("dbus_interface: unknown property '{}'", name));
        }
        const auto* p = dynamic_cast<const details::property_instance<T>*>(
            it->second.get());
        if (p == nullptr)
        {
            throw std::invalid_argument(std::format(
                "dbus_interface: property type mismatch for '{}'", name));
        }
        return p->value;
    }

    template <typename... Args>
    void emit_signal(std::string_view name, Args&&... args)
    {
        assert_initialized();
        auto it = _signals.find(name);
        if (it == _signals.end())
        {
            throw std::out_of_range(
                std::format("dbus_interface: unknown signal '{}'", name));
        }
        if (std::string_view{it->second.first} !=
            details::signature_of<std::decay_t<Args>...>())
        {
            throw std::invalid_argument(std::format(
                "dbus_interface: signal signature mismatch for '{}'", name));
        }

        auto m = _interface->new_signal(it->first.c_str());
        if constexpr (sizeof...(Args) > 0)
        {
            m.append(std::forward<Args>(args)...);
        }
        m.signal_send();
    }

    void emit_added()
    {
        assert_initialized();
        _interface->emit_added();
    }

    void emit_removed()
    {
        assert_initialized();
        _interface->emit_removed();
    }

    void property_changed(const std::string& name)
    {
        assert_initialized();
        _interface->property_changed(name.c_str());
    }

    bool initialized() const noexcept
    {
        return _interface.has_value();
    }

    const std::string& path() const noexcept
    {
        return _path;
    }

    const std::string& name() const noexcept
    {
        return _name;
    }

  private:
    template <std::equality_comparable T>
    void register_property_impl(std::string name, T initial,
                                std::move_only_function<T(T)> on_set,
                                decltype(vtable_t::flags) flags)
    {
        assert_not_initialized();
        validate_member_name(name);
        auto holder = std::make_unique<details::property_instance<T>>(
            std::move(initial), std::move(on_set), flags);
        auto [_, inserted] =
            _properties.try_emplace(std::move(name), std::move(holder));
        if (!inserted)
        {
            throw std::invalid_argument(
                "dbus_interface: duplicate property name");
        }
    }

    static void validate_member_name(const std::string& name)
    {
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
        {
            throw std::invalid_argument(std::format(
                "dbus_interface: invalid member name '{}'", name));
        }
    }

    void assert_not_initialized() const
    {
        if (initialized())
        {
            throw std::logic_error(
                "dbus_interface: member registration after initialize()");
        }
    }

    void assert_initialized() const
    {
        if (!initialized())
        {
            throw std::logic_error(
                "dbus_interface: operation requires initialize()");
        }
    }

    static int _callback_get(sd_bus*, const char*, const char*,
                             const char* property, sd_bus_message* reply,
                             void* userdata, sd_bus_error* error) noexcept;
    static int _callback_set(sd_bus*, const char*, const char*,
                             const char* property, sd_bus_message* value,
                             void* userdata, sd_bus_error* error) noexcept;
    static int _callback_method(sd_bus_message* msg, void* userdata,
                                sd_bus_error* error) noexcept;

    std::string _path;
    std::string _name;

    std::flat_map<std::string, std::unique_ptr<details::property_base>,
                  std::less<>>
        _properties;
    std::flat_map<std::string, std::shared_ptr<details::method_base>,
                  std::less<>>
        _methods;
    std::flat_map<std::string,
                  std::pair<const char*, decltype(vtable_t::flags)>,
                  std::less<>>
        _signals;

    std::vector<vtable_t> _vtable;

    std::optional<sdbusplus::server::interface_t> _interface;
};

class object_server : private context_ref
{
  public:
    object_server() = delete;
    object_server(const object_server&) = delete;
    object_server& operator=(const object_server&) = delete;
    object_server(object_server&&) = delete;
    object_server& operator=(object_server&&) = delete;
    ~object_server() = default;

    explicit object_server(context& ctx) : context_ref(ctx) {}

    dbus_interface& add_interface(std::string path, std::string name)
    {
        return _interfaces.emplace_back(ctx, std::move(path), std::move(name));
    }

    bool remove_interface(dbus_interface& iface)
    {
        return _interfaces.remove_if([&iface](const dbus_interface& i) {
            return &i == &iface;
        }) > 0;
    }

    void add_manager(const std::string& path)
    {
        _managers.emplace_back(ctx.get_bus(), path.c_str());
    }

  private:
    std::list<dbus_interface> _interfaces;
    std::list<sdbusplus::server::manager_t> _managers;
};

} // namespace sdbusplus::async
