#pragma once

#ifndef BOOST_COROUTINES_NO_DEPRECATION_WARNING
// users should define this if they directly include boost/asio/spawn.hpp,
// but by defining it here, warnings won't cause problems with a compile
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#endif
#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/read.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <any>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace sdbusplus
{
namespace asio
{

enum class SetPropertyReturnValue
{
    fail = 0,
    valueUpdated,
    sameValueUpdated,
};

class property_callback
{
  public:
    std::string name;
    std::function<int(message_t&)> on_get;
    std::function<SetPropertyReturnValue(message_t&)> on_set_message;
    std::function<SetPropertyReturnValue(const std::any&)> on_set_value;
    const char* signature;
    decltype(vtable_t::flags) flags;
};

class method_callback
{
  public:
    std::string name;
    std::function<int(message_t&)> call;
    const char* arg_signature;
    const char* return_signature;
};

class signal
{
  public:
    std::string name;
    const char* signature;
};

template <typename T>
inline const bool FirstArgIsYield_v =
    std::is_same_v<utility::get_first_arg_t<utility::decay_tuple_t<
                       boost::callable_traits::args_t<T>>>,
                   boost::asio::yield_context>;

template <typename T>
inline const bool FirstArgIsMessage_v =
    std::is_same_v<utility::get_first_arg_t<utility::decay_tuple_t<
                       boost::callable_traits::args_t<T>>>,
                   message_t>;

template <typename T>
inline const bool SecondArgIsMessage_v = std::is_same_v<
    utility::get_first_arg_t<utility::strip_first_arg_t<
        utility::decay_tuple_t<boost::callable_traits::args_t<T>>>>,
    message_t>;

template <typename T>
static constexpr bool callbackWantsMessage = FirstArgIsMessage_v<T> ||
                                             SecondArgIsMessage_v<T>;

namespace details
{
// small helper class to count the number of non-dbus arguments
// to a registered dbus function (like message_t or yield_context)
// so the registered signature can omit them
template <typename FirstArg, typename... Rest>
struct NonDbusArgsCount;

template <>
struct NonDbusArgsCount<std::tuple<>>
{
    constexpr static std::size_t size()
    {
        return 0;
    }
};
template <typename FirstArg, typename... OtherArgs>
struct NonDbusArgsCount<std::tuple<FirstArg, OtherArgs...>>
{
    constexpr static std::size_t size()
    {
        if constexpr (std::is_same_v<FirstArg, message_t> ||
                      std::is_same_v<FirstArg, boost::asio::yield_context>)
        {
            return 1 + NonDbusArgsCount<std::tuple<OtherArgs...>>::size();
        }
        else
        {
            return NonDbusArgsCount<std::tuple<OtherArgs...>>::size();
        }
    }
};

template <typename PropertyType>
PropertyType nop_get_value(const PropertyType& value)
{
    return value;
}

template <typename PropertyType>
int nop_set_value(const PropertyType& req, PropertyType& old)
{
    old = req;
    return 1;
}

} // namespace details

template <typename InputArgs, typename Callback>
void callFunction(message_t& m, InputArgs& inputArgs, Callback&& callback)
{
    using ResultType = boost::callable_traits::return_type_t<Callback>;
    if constexpr (std::is_void_v<ResultType>)
    {
        std::apply(callback, inputArgs);
    }
    else
    {
        auto r = std::apply(callback, inputArgs);
        m.append(r);
    }
}

template <typename CallbackType>
class callback_method_instance
{
  private:
    using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
    using InputTupleType = utility::decay_tuple_t<CallbackSignature>;

  public:
    CallbackType func_;

    int operator()(message_t& m)
    {
        using DbusTupleType = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<InputTupleType>::size(), InputTupleType>;

        DbusTupleType dbusArgs;
        if (!utility::read_into_tuple(dbusArgs, m))
        {
            return -EINVAL;
        }
        auto ret = m.new_method_return();
        if constexpr (callbackWantsMessage<CallbackType>)
        {
            InputTupleType inputArgs =
                std::tuple_cat(std::forward_as_tuple(std::move(m)), dbusArgs);
            callFunction(ret, inputArgs, func_);
        }
        else
        {
            callFunction(ret, dbusArgs, func_);
        }
        ret.method_return();
        return 1;
    }
};

template <typename CallbackType>
class coroutine_method_instance
{
  public:
    using self_t = coroutine_method_instance<CallbackType>;
    boost::asio::io_context& io_;
    CallbackType func_;

    int operator()(message_t& m)
    {
        // make a copy of m to move into the coroutine
        message_t b{m};

        // spawn off a new coroutine to handle the method call
        boost::asio::spawn(io_, std::bind_front(&self_t::after_spawn, this, b));
        return 1;
    }

  private:
    void after_spawn(message_t b, boost::asio::yield_context yield)
    {
        using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
        using InputTupleType = utility::decay_tuple_t<CallbackSignature>;
        using DbusTupleType = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<InputTupleType>::size(), InputTupleType>;
        DbusTupleType dbusArgs;
        try
        {
            utility::read_into_tuple(dbusArgs, b);
        }
        catch (const exception::SdBusError& e)
        {
            auto ret = b.new_method_errno(e.get_errno(), e.get_error());
            ret.method_return();
            return;
        }

        try
        {
            auto ret = b.new_method_return();
            if constexpr (callbackWantsMessage<CallbackType>)
            {
                InputTupleType inputArgs = std::tuple_cat(
                    std::forward_as_tuple(std::move(yield)),
                    std::forward_as_tuple(std::move(b)), dbusArgs);
                callFunction(ret, inputArgs, func_);
            }
            else
            {
                InputTupleType inputArgs = std::tuple_cat(
                    std::forward_as_tuple(std::move(yield)), dbusArgs);
                callFunction(ret, inputArgs, func_);
            }
            ret.method_return();
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            // Catch D-Bus error explicitly called by method handler
            message_t err = b.new_method_errno(e.get_errno(), e.get_error());
            err.method_return();
        }
        catch (const sdbusplus::exception_t& e)
        {
            message_t err = b.new_method_error(e);
            err.method_return();
        }
        catch (...)
        {
            message_t err = b.new_method_errno(-EIO);
            err.method_return();
        }
    }
};

template <typename PropertyType, typename CallbackType>
struct callback_get_instance
{
    int operator()(message_t& m)
    {
        *value_ = func_(*value_);
        m.append(*value_);
        return 1;
    }

    std::shared_ptr<PropertyType> value_;
    CallbackType func_;
};

template <typename PropertyType>
struct callback_set_message_instance
{
    SetPropertyReturnValue operator()(message_t& m)
    {
        PropertyType input;
        m.read(input);
        PropertyType oldValue = *value_;
        if (!func_(input, *value_))
        {
            return SetPropertyReturnValue::fail;
        }
        if (oldValue == *value_)
        {
            return SetPropertyReturnValue::sameValueUpdated;
        }
        return SetPropertyReturnValue::valueUpdated;
    }

    std::shared_ptr<PropertyType> value_;
    std::function<bool(const PropertyType&, PropertyType&)> func_;
};

template <typename PropertyType>
struct callback_set_value_instance
{
    SetPropertyReturnValue operator()(const std::any& value)
    {
        const PropertyType& newValue = std::any_cast<PropertyType>(value);
        PropertyType oldValue = *value_;
        if (func_(newValue, *value_) == false)
        {
            return SetPropertyReturnValue::fail;
        }
        if (oldValue == *value_)
        {
            return SetPropertyReturnValue::sameValueUpdated;
        }
        return SetPropertyReturnValue::valueUpdated;
    }

    std::shared_ptr<PropertyType> value_;
    std::function<bool(const PropertyType&, PropertyType&)> func_;
};

enum class PropertyPermission
{
    readOnly,
    readWrite
};

class dbus_interface
{
  public:
    dbus_interface(std::shared_ptr<sdbusplus::asio::connection> conn,
                   const std::string& path, const std::string& name) :
        conn_(conn),
        path_(path), name_(name)

    {}
    ~dbus_interface()
    {
        conn_->emit_interfaces_removed(path_.c_str(),
                                       std::vector<std::string>{name_});
    }

    template <typename PropertyType, typename CallbackTypeGet>
    bool register_property_r(const std::string& name,
                             const PropertyType& property,
                             decltype(vtable_t::flags) flags,
                             CallbackTypeGet&& getFunction)
    {
        // can only register once
        if (is_initialized())
        {
            return false;
        }
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto propertyPtr = std::make_shared<PropertyType>(property);

        property_callbacks_.emplace_back(
            name,
            callback_get_instance<PropertyType, CallbackTypeGet>(
                propertyPtr, std::move(getFunction)),
            callback_set_message_instance<PropertyType>(
                propertyPtr, details::nop_set_value<PropertyType>),
            callback_set_value_instance<PropertyType>(
                propertyPtr, details::nop_set_value<PropertyType>),
            type.data(), flags);

        return true;
    }

    template <typename PropertyType, typename CallbackTypeGet>
    bool register_property_r(const std::string& name,
                             decltype(vtable_t::flags) flags,
                             CallbackTypeGet&& getFunction)
    {
        return register_property_r(name, PropertyType{}, flags,
                                   std::forward<CallbackTypeGet>(getFunction));
    }

    template <typename PropertyType, typename CallbackTypeSet,
              typename CallbackTypeGet>
    bool register_property_rw(const std::string& name,
                              const PropertyType& property,
                              decltype(vtable_t::flags) flags,
                              CallbackTypeSet&& setFunction,
                              CallbackTypeGet&& getFunction)
    {
        // can only register once
        if (is_initialized())
        {
            return false;
        }
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto propertyPtr = std::make_shared<PropertyType>(property);

        property_callbacks_.emplace_back(
            name,
            callback_get_instance<PropertyType, CallbackTypeGet>(
                propertyPtr, std::move(getFunction)),
            callback_set_message_instance<PropertyType>(propertyPtr,
                                                        std::move(setFunction)),
            callback_set_value_instance<PropertyType>(propertyPtr,
                                                      std::move(setFunction)),

            type.data(), flags);

        return true;
    }

    template <typename PropertyType, typename CallbackTypeSet,
              typename CallbackTypeGet>
    bool register_property_rw(const std::string& name,
                              decltype(vtable_t::flags) flags,
                              CallbackTypeSet&& setFunction,
                              CallbackTypeGet&& getFunction)
    {
        return register_property_rw(name, PropertyType{}, flags,
                                    std::forward<CallbackTypeSet>(setFunction),
                                    std::forward<CallbackTypeGet>(getFunction));
    }

    // default getter and setter
    template <typename PropertyType>
    bool register_property(
        const std::string& name, const PropertyType& property,
        PropertyPermission access = PropertyPermission::readOnly)
    {
        if (access == PropertyPermission::readOnly)
        {
            return register_property_r(name, property,
                                       vtable::property_::emits_change,
                                       details::nop_get_value<PropertyType>);
        }
        else
        {
            return register_property_rw(name, property,
                                        vtable::property_::emits_change,
                                        details::nop_set_value<PropertyType>,
                                        details::nop_get_value<PropertyType>);
        }
    }

    // custom setter, sets take an input property and respond with an int status
    template <typename PropertyType, typename CallbackTypeSet>
    bool register_property(const std::string& name,
                           const PropertyType& property,
                           CallbackTypeSet&& setFunction)
    {
        return register_property_rw(name, property,
                                    vtable::property_::emits_change,
                                    std::forward<CallbackTypeSet>(setFunction),
                                    details::nop_get_value<PropertyType>);
    }

    // custom getter and setter, gets take an input of void and respond with a
    // property. property is only passed for type deduction
    template <typename PropertyType, typename CallbackTypeSet,
              typename CallbackTypeGet>
    bool register_property(const std::string& name,
                           const PropertyType& property,
                           CallbackTypeSet&& setFunction,
                           CallbackTypeGet&& getFunction)
    {
        return register_property_rw(name, property,
                                    vtable::property_::emits_change,
                                    std::forward<CallbackTypeSet>(setFunction),
                                    std::forward<CallbackTypeGet>(getFunction));
    }

    template <typename PropertyType, bool changesOnly = false>
    bool set_property(const std::string& name, const PropertyType& value)
    {
        if (!is_initialized())
        {
            return false;
        }
        auto func = std::find_if(
            property_callbacks_.begin(), property_callbacks_.end(),
            [&name](const auto& element) { return element.name == name; });
        if (func != property_callbacks_.end())
        {
            SetPropertyReturnValue status = func->on_set_value(value);
            if ((status == SetPropertyReturnValue::valueUpdated) ||
                (status == SetPropertyReturnValue::sameValueUpdated))
            {
                if (status != SetPropertyReturnValue::sameValueUpdated)
                {
                    signal_property(name);
                    return true;
                }
                if constexpr (!changesOnly)
                {
                    return true;
                }
            }
        }
        return false;
    }

    template <typename... SignalSignature>
    bool register_signal(const std::string& name)
    {
        if (is_initialized())
        {
            return false;
        }
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
        {
            return false;
        }

        static constexpr auto signature = utility::tuple_to_array(
            message::types::type_id<SignalSignature...>());

        signals_.emplace_back(name, signature);
        return true;
    }

    template <typename CallbackType>
    bool register_method(const std::string& name, CallbackType&& handler)
    {
        using ActualSignature = boost::callable_traits::args_t<CallbackType>;
        using CallbackSignature = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<ActualSignature>::size(),
            ActualSignature>;
        using InputTupleType = utility::decay_tuple_t<CallbackSignature>;
        using ResultType = boost::callable_traits::return_type_t<CallbackType>;

        if (is_initialized())
        {
            return false;
        }
        static const auto argType = utility::strip_ends(
            utility::tuple_to_array(message::types::type_id<InputTupleType>()));
        static const auto resultType =
            utility::tuple_to_array(message::types::type_id<ResultType>());

        std::function<int(message_t&)> func;
        if constexpr (FirstArgIsYield_v<CallbackType>)
        {
            func = coroutine_method_instance<CallbackType>(
                conn_->get_io_context(), std::move(handler));
        }
        else
        {
            func = callback_method_instance<CallbackType>(std::move(handler));
        }
        method_callbacks_.emplace_back(name, std::move(func), argType.data(),
                                       resultType.data());

        return true;
    }

    static int get_handler(sd_bus* /*bus*/, const char* /*path*/,
                           const char* /*interface*/, const char* /*property*/,
                           sd_bus_message* reply, void* userdata,
                           sd_bus_error* error)
    {
        property_callback** func = static_cast<property_callback**>(userdata);
        auto mesg = message_t(reply);
#ifdef __EXCEPTIONS
        try
        {
#endif
            return (*func)->on_get(mesg);
#ifdef __EXCEPTIONS
        }

        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
        catch (...)
        {
            // hit default error below
        }
#endif
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS,
                                      nullptr);
    }

    static int set_handler(sd_bus* /*bus*/, const char* /*path*/,
                           const char* /*interface*/, const char* /*property*/,
                           sd_bus_message* value, void* userdata,
                           sd_bus_error* error)
    {
        property_callback** func = static_cast<property_callback**>(userdata);

        auto mesg = message_t(value);
#ifdef __EXCEPTIONS
        try
        {
#endif
            SetPropertyReturnValue status = (*func)->on_set_message(mesg);
            if ((status == SetPropertyReturnValue::valueUpdated) ||
                (status == SetPropertyReturnValue::sameValueUpdated))
            {
                if (status != SetPropertyReturnValue::sameValueUpdated)
                {
                    // data->signal_property(property);
                }
                // There shouldn't be any other callbacks that want to
                // handle the message so just return a positive integer.
                return 1;
            }
#ifdef __EXCEPTIONS
        }

        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
        catch (...)
        {
            // hit default error below
        }
#endif
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS,
                                      nullptr);
    }

    static int method_handler(sd_bus_message* m, void* userdata,
                              sd_bus_error* error)
    {
        method_callback** func = static_cast<method_callback**>(userdata);
        auto mesg = message_t(m);
#ifdef __EXCEPTIONS
        try
        {
#endif
            int status = (*func)->call(mesg);
            if (status == 1)
            {
                return status;
            }
#ifdef __EXCEPTIONS
        }

        catch (const sdbusplus::exception_t& e)
        {
            return sd_bus_error_set(error, e.name(), e.description());
        }
        catch (...)
        {
            // hit default error below
        }
#endif
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS,
                                      nullptr);
    }

    /** @brief Create a new signal message.
     *
     *  @param[in] member - The signal name to create.
     */
    auto new_signal(const char* member)
    {
        if (is_initialized())
        {
            return message_t(nullptr);
        }
        return interface_->new_signal(member);
    }

    bool initialize(const bool skipPropertyChangedSignal = false)
    {
        // can only register once
        if (is_initialized())
        {
            return false;
        }
        vtable_.reserve(2 + property_callbacks_.size() +
                        method_callbacks_.size() + signals_.size());
        vtable_.emplace_back(vtable::start());
        pointers.push_back(this);

        property_callbacks_.shrink_to_fit();
        for (auto& element : property_callbacks_)
        {
            pointers.push_back(&element);
            size_t pointer_off = (pointers.size() - 1) * sizeof(void*);
            vtable_.emplace_back(vtable::property_o(
                element.name.c_str(), element.signature, get_handler,
                set_handler, pointer_off, element.flags));
        }

        method_callbacks_.shrink_to_fit();
        for (auto& element : method_callbacks_)
        {
            pointers.push_back(&element);
            size_t pointer_off = (pointers.size() - 1) * sizeof(void*);
            vtable_.emplace_back(vtable::method_o(
                element.name.c_str(), element.arg_signature,
                element.return_signature, method_handler, pointer_off));
        }

        signals_.shrink_to_fit();
        for (const auto& element : signals_)
        {
            vtable_.emplace_back(
                vtable::signal(element.name.c_str(), element.signature));
        }

        vtable_.emplace_back(vtable::end());
        vtable_.shrink_to_fit();

        interface_.emplace(static_cast<sdbusplus::bus_t&>(*conn_),
                           path_.c_str(), name_.c_str(),
                           static_cast<const sd_bus_vtable*>(&vtable_[0]),
                           pointers.data());
        conn_->emit_interfaces_added(path_.c_str(),
                                     std::vector<std::string>{name_});
        if (!skipPropertyChangedSignal)
        {
            for (const auto& element : property_callbacks_)
            {
                signal_property(element.name);
            }
        }
        return true;
    }

    bool is_initialized()
    {
        return interface_.has_value();
    }

    bool signal_property(const std::string& name)
    {
        if (!is_initialized())
        {
            return false;
        }
        interface_->property_changed(name.c_str());
        return true;
    }

    std::string get_object_path(void)
    {
        return path_;
    }

    std::string get_interface_name(void)
    {
        return name_;
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> conn_;
    std::string path_;
    std::string name_;

    std::vector<signal> signals_;
    std::vector<property_callback> property_callbacks_;
    std::vector<method_callback> method_callbacks_;

    std::vector<void*> pointers;

    std::vector<sd_bus_vtable> vtable_;
    std::optional<sdbusplus::server::interface_t> interface_;
};

class object_server
{
  public:
    object_server(const std::shared_ptr<sdbusplus::asio::connection>& conn,
                  const bool skipManager = false) :
        conn_(conn)
    {
        if (!skipManager)
        {
            add_manager("/");
        }
    }

    std::shared_ptr<dbus_interface> add_interface(const std::string& path,
                                                  const std::string& name)
    {
        auto dbusIface = std::make_shared<dbus_interface>(conn_, path, name);
        interfaces_.emplace_back(dbusIface);
        return dbusIface;
    }

    std::unique_ptr<dbus_interface>
        add_unique_interface(const std::string& path, const std::string& name)
    {
        return std::make_unique<dbus_interface>(conn_, path, name);
    }

    /**
      @brief creates initialized dbus_interface
      @param path a string path to interface
      @param name a string name of the interface
      @param initializer a functor (void (dbus_interface&)) to be called before
             call to dbus_interface::initialize
      @return an unique_ptr to initialized dbus_interface
      */
    template <class Initializer>
    std::unique_ptr<dbus_interface>
        add_unique_interface(const std::string& path, const std::string& name,
                             Initializer&& initializer)
    {
        auto dbusIface = std::make_unique<dbus_interface>(conn_, path, name);
        initializer(*dbusIface);
        dbusIface->initialize();
        return dbusIface;
    }

    void add_manager(const std::string& path)
    {
        managers_.emplace_back(static_cast<sdbusplus::bus_t&>(*conn_),
                               path.c_str());
    }

    bool remove_interface(const std::shared_ptr<dbus_interface>& iface)
    {
        auto findIface = std::find(interfaces_.begin(), interfaces_.end(),
                                   iface);
        if (findIface != interfaces_.end())
        {
            interfaces_.erase(findIface);
            return true;
        }
        return false;
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> conn_;
    std::vector<std::shared_ptr<dbus_interface>> interfaces_;
    std::vector<server::manager_t> managers_;
};

} // namespace asio
} // namespace sdbusplus
