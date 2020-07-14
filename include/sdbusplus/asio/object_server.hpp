#pragma once

#ifndef BOOST_COROUTINES_NO_DEPRECATION_WARNING
// users should define this if they directly include boost/asio/spawn.hpp,
// but by defining it here, warnings won't cause problems with a compile
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#endif

#include <boost/any.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/read.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <list>
#include <optional>
#include <regex>
#include <set>

namespace sdbusplus
{
namespace asio
{

constexpr const char* PropertyNamePattern = "[a-zA-Z0-9_]+";
constexpr const char* PathPattern = "(/[a-zA-Z0-9_]+)+";
constexpr const char* InterfaceNamePattern = "[a-zA-Z0-9_]+([.][a-zA-Z0-9_]+)+";

class callback
{
  public:
    virtual ~callback() = default;
    virtual int call(message::message& m) = 0;
};

enum class SetPropertyReturnValue : size_t
{
    fail = 0,
    valueUpdated,
    sameValueUpdated,
};

class callback_set
{
  public:
    virtual ~callback_set() = default;
    virtual SetPropertyReturnValue call(message::message& m) = 0;
    virtual SetPropertyReturnValue set(const boost::any& value) = 0;
};

template <typename T>
using FirstArgIsYield =
    std::is_same<typename utility::get_first_arg<typename utility::decay_tuple<
                     boost::callable_traits::args_t<T>>::type>::type,
                 boost::asio::yield_context>;

template <typename T>
using FirstArgIsMessage =
    std::is_same<typename utility::get_first_arg<typename utility::decay_tuple<
                     boost::callable_traits::args_t<T>>::type>::type,
                 message::message>;

template <typename T>
using SecondArgIsMessage = std::is_same<
    typename utility::get_first_arg<
        typename utility::strip_first_arg<typename utility::decay_tuple<
            boost::callable_traits::args_t<T>>::type>::type>::type,
    message::message>;
template <typename T>
static constexpr bool callbackYields = FirstArgIsYield<T>::value;
template <typename T>
static constexpr bool callbackWantsMessage = (FirstArgIsMessage<T>::value ||
                                              SecondArgIsMessage<T>::value);

#ifdef __cpp_if_constexpr
namespace details
{
// small helper class to count the number of non-dbus arguments
// to a registered dbus function (like message::message or yield_context)
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
        if constexpr (std::is_same<FirstArg, message::message>::value ||
                      std::is_same<FirstArg, boost::asio::yield_context>::value)
        {
            return 1 + NonDbusArgsCount<std::tuple<OtherArgs...>>::size();
        }
        else
        {
            return NonDbusArgsCount<std::tuple<OtherArgs...>>::size();
        }
    }
};
} // namespace details
#endif // __cpp_if_constexpr

template <typename CallbackType>
class callback_method_instance : public callback
{
  public:
    callback_method_instance(CallbackType&& func) : func_(std::move(func))
    {}
    int call(message::message& m) override
    {
        return expandCall(m);
    }

  private:
    using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
    using InputTupleType =
        typename utility::decay_tuple<CallbackSignature>::type;
    using ResultType = boost::callable_traits::return_type_t<CallbackType>;
    CallbackType func_;
    template <typename T>
    std::enable_if_t<!std::is_void<T>::value, void>
        callFunction(message::message& m, InputTupleType& inputArgs)
    {
        ResultType r = std::apply(func_, inputArgs);
        m.append(r);
    }
    template <typename T>
    std::enable_if_t<std::is_void<T>::value, void>
        callFunction(message::message& m, InputTupleType& inputArgs)
    {
        std::apply(func_, inputArgs);
    }
#ifdef __cpp_if_constexpr
    // optional message-first-argument callback
    int expandCall(message::message& m)
    {
        using DbusTupleType = typename utility::strip_first_n_args<
            details::NonDbusArgsCount<InputTupleType>::size(),
            InputTupleType>::type;

        DbusTupleType dbusArgs;
        if (!utility::read_into_tuple(dbusArgs, m))
        {
            return -EINVAL;
        }
        auto ret = m.new_method_return();
        std::optional<InputTupleType> inputArgs;
        if constexpr (callbackWantsMessage<CallbackType>)
        {
            inputArgs.emplace(
                std::tuple_cat(std::forward_as_tuple(std::move(m)), dbusArgs));
        }
        else
        {
            inputArgs.emplace(dbusArgs);
        }
        callFunction<ResultType>(ret, *inputArgs);
        ret.method_return();
        return 1;
    };
#else
    // normal dbus-types-only callback
    int expandCall(message::message& m)
    {
        InputTupleType inputArgs;
        if (!utility::read_into_tuple(inputArgs, m))
        {
            return -EINVAL;
        }

        auto ret = m.new_method_return();
        callFunction<ResultType>(ret, inputArgs);
        ret.method_return();
        return 1;
    };
#endif
};

#ifdef __cpp_if_constexpr
template <typename CallbackType>
class coroutine_method_instance : public callback
{
  public:
    coroutine_method_instance(boost::asio::io_context& io,
                              CallbackType&& func) :
        io_(io),
        func_(std::move(func))
    {}
    int call(message::message& m) override
    {
        // make a copy of m to move into the coroutine
        message::message b{m};
        // spawn off a new coroutine to handle the method call
        boost::asio::spawn(
            io_, [this, b = std::move(b)](boost::asio::yield_context yield) {
                message::message mcpy{std::move(b)};
                std::optional<message::message> err{};

                try
                {
                    expandCall(yield, mcpy);
                }
                catch (sdbusplus::exception::SdBusError& e)
                {
                    // Catch D-Bus error explicitly called by method handler
                    err = mcpy.new_method_errno(e.get_errno(), e.get_error());
                }
                catch (...)
                {
                    err = mcpy.new_method_errno(-EIO);
                }

                if (err)
                {
                    err->method_return();
                }
            });
        return 1;
    }

  private:
    using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
    using InputTupleType =
        typename utility::decay_tuple<CallbackSignature>::type;
    using ResultType = boost::callable_traits::return_type_t<CallbackType>;
    boost::asio::io_context& io_;
    CallbackType func_;
    template <typename T>
    std::enable_if_t<!std::is_void<T>::value, void>
        callFunction(message::message& m, InputTupleType& inputArgs)
    {
        ResultType r = std::apply(func_, inputArgs);
        m.append(r);
    }
    template <typename T>
    std::enable_if_t<std::is_void<T>::value, void>
        callFunction(message::message& m, InputTupleType& inputArgs)
    {
        std::apply(func_, inputArgs);
    }
    // co-routine body for call
    void expandCall(boost::asio::yield_context yield, message::message& m)
    {
        using DbusTupleType = typename utility::strip_first_n_args<
            details::NonDbusArgsCount<InputTupleType>::size(),
            InputTupleType>::type;
        DbusTupleType dbusArgs;
        try
        {
            utility::read_into_tuple(dbusArgs, m);
        }
        catch (const exception::SdBusError& e)
        {
            auto ret = m.new_method_errno(e.get_errno(), e.get_error());
            ret.method_return();
            return;
        }

        auto ret = m.new_method_return();
        std::optional<InputTupleType> inputArgs;
        if constexpr (callbackWantsMessage<CallbackType>)
        {
            inputArgs.emplace(
                std::tuple_cat(std::forward_as_tuple(std::move(yield)),
                               std::forward_as_tuple(std::move(m)), dbusArgs));
        }
        else
        {
            inputArgs.emplace(std::tuple_cat(
                std::forward_as_tuple(std::move(yield)), dbusArgs));
        }
        callFunction<ResultType>(ret, *inputArgs);
        ret.method_return();
    };
};
#endif // __cpp_if_constexpr

template <typename PropertyType, typename CallbackType>
class callback_get_instance : public callback
{
  public:
    callback_get_instance(const std::shared_ptr<PropertyType>& value,
                          CallbackType&& func) :
        value_(value),
        func_(std::move(func))
    {}
    int call(message::message& m) override
    {
        auto r = func_(*value_);
        m.append(r);
        return 1;
    }

  private:
    std::shared_ptr<PropertyType> value_;
    CallbackType func_;
};

template <typename PropertyType, typename CallbackType>
class callback_set_instance : public callback_set
{
  public:
    callback_set_instance(const std::shared_ptr<PropertyType>& value,
                          CallbackType&& func) :
        value_(value),
        func_(std::move(func))
    {}
    SetPropertyReturnValue call(message::message& m) override
    {
        PropertyType input;
        m.read(input);

        auto oldValue = *value_;
        if (func_(input, *value_))
        {
            if (oldValue == *value_)
            {
                return SetPropertyReturnValue::sameValueUpdated;
            }
            return SetPropertyReturnValue::valueUpdated;
        }
        return SetPropertyReturnValue::fail;
    }
    SetPropertyReturnValue set(const boost::any& value) override
    {
        auto oldValue = *value_;
        if (func_(boost::any_cast<PropertyType>(value), *value_))
        {
            if (oldValue == *value_)
            {
                return SetPropertyReturnValue::sameValueUpdated;
            }
            return SetPropertyReturnValue::valueUpdated;
        }
        return SetPropertyReturnValue::fail;
    }

  private:
    std::shared_ptr<PropertyType> value_;
    CallbackType func_;
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

    {
        vtable_.emplace_back(vtable::start());
    }
    ~dbus_interface()
    {
        conn_->emit_interfaces_removed(path_.c_str(),
                                       std::vector<std::string>{name_});
    }

    // default getter and setter
    template <typename PropertyType>
    bool register_property(
        const std::string& name, const PropertyType& property,
        PropertyPermission access = PropertyPermission::readOnly)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (!std::regex_match(name, std::regex(PropertyNamePattern)))
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto nameItr = propertyNames_.emplace(propertyNames_.end(), name);
        auto propertyPtr = std::make_shared<PropertyType>(property);

        callbacksGet_[name] = std::make_unique<callback_get_instance<
            PropertyType, std::function<PropertyType(const PropertyType&)>>>(
            propertyPtr, [](const PropertyType& value) { return value; });
        callbacksSet_[name] = std::make_unique<callback_set_instance<
            PropertyType,
            std::function<int(const PropertyType&, PropertyType&)>>>(
            propertyPtr, [](const PropertyType& req, PropertyType& old) {
                old = req;
                return 1;
            });

        if (access == PropertyPermission::readOnly)
        {
            vtable_.emplace_back(
                vtable::property(nameItr->c_str(), type.data(), get_handler,
                                 vtable::property_::emits_change));
        }
        else
        {
            vtable_.emplace_back(
                vtable::property(nameItr->c_str(), type.data(), get_handler,
                                 set_handler, vtable::property_::emits_change));
        }
        return true;
    }

    // custom setter, sets take an input property and respond with an int status
    template <typename PropertyType, typename CallbackType>
    bool register_property(const std::string& name,
                           const PropertyType& property,
                           CallbackType&& setFunction)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (!std::regex_match(name, std::regex(PropertyNamePattern)))
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto nameItr = propertyNames_.emplace(propertyNames_.end(), name);
        auto propertyPtr = std::make_shared<PropertyType>(property);

        callbacksGet_[name] = std::make_unique<callback_get_instance<
            PropertyType, std::function<PropertyType(const PropertyType&)>>>(
            propertyPtr, [](const PropertyType& value) { return value; });

        callbacksSet_[name] =
            std::make_unique<callback_set_instance<PropertyType, CallbackType>>(
                propertyPtr, std::move(setFunction));
        vtable_.emplace_back(vtable::property(nameItr->c_str(), type.data(),
                                              get_handler, set_handler,
                                              vtable::property_::emits_change));

        return true;
    }

    // custom getter and setter, gets take an input of void and respond with a
    // property. property is only passed for type deduction
    template <typename PropertyType, typename CallbackType,
              typename CallbackTypeGet>
    bool register_property(const std::string& name,
                           const PropertyType& property,
                           CallbackType&& setFunction,
                           CallbackTypeGet&& getFunction)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (!std::regex_match(name, std::regex(PropertyNamePattern)))
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto nameItr = propertyNames_.emplace(propertyNames_.end(), name);
        auto propertyPtr = std::make_shared<PropertyType>(property);

        callbacksGet_[name] = std::make_unique<
            callback_get_instance<PropertyType, CallbackTypeGet>>(
            propertyPtr, std::move(getFunction));
        callbacksSet_[name] =
            std::make_unique<callback_set_instance<PropertyType, CallbackType>>(
                propertyPtr, std::move(setFunction));

        vtable_.emplace_back(vtable::property(nameItr->c_str(), type.data(),
                                              get_handler, set_handler,
                                              vtable::property_::emits_change));

        return true;
    }
    template <typename PropertyType, bool changesOnly = false>
    bool set_property(const std::string& name, const PropertyType& value)
    {
        if (!initialized_)
        {
            return false;
        }
        auto func = callbacksSet_.find(name);
        if (func != callbacksSet_.end())
        {
            SetPropertyReturnValue status = func->second->set(value);
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
        if (initialized_)
        {
            return false;
        }
        if (!std::regex_match(name, std::regex(PropertyNamePattern)))
        {
            return false;
        }

        static constexpr auto signature = utility::tuple_to_array(
            message::types::type_id<SignalSignature...>());

        auto [itr, inserted] = signalNames_.insert(name);
        if (!inserted)
        {
            return false;
        }

        vtable_.emplace_back(vtable::signal(itr->c_str(), signature.data()));
        return true;
    }

#ifdef __cpp_if_constexpr
    template <typename CallbackType>
    bool register_method(const std::string& name, CallbackType&& handler)
    {
        using ActualSignature = boost::callable_traits::args_t<CallbackType>;
        using CallbackSignature = typename utility::strip_first_n_args<
            details::NonDbusArgsCount<ActualSignature>::size(),
            ActualSignature>::type;
        using InputTupleType =
            typename utility::decay_tuple<CallbackSignature>::type;
        using ResultType = boost::callable_traits::return_type_t<CallbackType>;

        if (initialized_)
        {
            return false;
        }
        static const auto argType = utility::strip_ends(
            utility::tuple_to_array(message::types::type_id<InputTupleType>()));
        static const auto resultType =
            utility::tuple_to_array(message::types::type_id<ResultType>());

        auto nameItr = methodNames_.emplace(methodNames_.end(), name);

        if constexpr (callbackYields<CallbackType>)
        {
            callbacksMethod_[name] =
                std::make_unique<coroutine_method_instance<CallbackType>>(
                    conn_->get_io_context(), std::move(handler));
        }
        else
        {
            callbacksMethod_[name] =
                std::make_unique<callback_method_instance<CallbackType>>(
                    std::move(handler));
        }

        vtable_.emplace_back(vtable::method(nameItr->c_str(), argType.data(),
                                            resultType.data(), method_handler));
        return true;
    }
#else  // __cpp_if_constexpr not available
       // without __cpp_if_constexpr, no support for message or yield in
       // callback
    template <typename CallbackType>
    bool register_method(const std::string& name, CallbackType&& handler)
    {
        using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
        using InputTupleType =
            typename utility::decay_tuple<CallbackSignature>::type;
        using ResultType = boost::callable_traits::return_type_t<CallbackType>;

        if (initialized_)
        {
            return false;
        }
        static const auto argType = utility::strip_ends(
            utility::tuple_to_array(message::types::type_id<InputTupleType>()));
        static const auto resultType =
            utility::tuple_to_array(message::types::type_id<ResultType>());

        auto nameItr = methodNames_.emplace(methodNames_.end(), name);

        callbacksMethod_[name] =
            std::make_unique<callback_method_instance<CallbackType>>(
                std::move(handler));

        vtable_.emplace_back(vtable::method(nameItr->c_str(), argType.data(),
                                            resultType.data(), method_handler));
        return true;
    }
#endif // __cpp_if_constexpr

    static int get_handler(sd_bus* /*bus*/, const char* /*path*/,
                           const char* /*interface*/, const char* property,
                           sd_bus_message* reply, void* userdata,
                           sd_bus_error* error)
    {
        dbus_interface* data = static_cast<dbus_interface*>(userdata);
        auto func = data->callbacksGet_.find(property);
        auto mesg = message::message(reply);
        if (func != data->callbacksGet_.end())
        {
#ifdef __EXCEPTIONS
            try
            {
#endif
                return func->second->call(mesg);
#ifdef __EXCEPTIONS
            }

            catch (sdbusplus::exception_t& e)
            {
                return sd_bus_error_set(error, e.name(), e.description());
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
    }

    static int set_handler(sd_bus* /*bus*/, const char* /*path*/,
                           const char* /*interface*/, const char* property,
                           sd_bus_message* value, void* userdata,
                           sd_bus_error* error)
    {
        dbus_interface* data = static_cast<dbus_interface*>(userdata);
        auto func = data->callbacksSet_.find(property);
        auto mesg = message::message(value);
        if (func != data->callbacksSet_.end())
        {
#ifdef __EXCEPTIONS
            try
            {
#endif
                SetPropertyReturnValue status = func->second->call(mesg);
                if ((status == SetPropertyReturnValue::valueUpdated) ||
                    (status == SetPropertyReturnValue::sameValueUpdated))
                {
                    if (status != SetPropertyReturnValue::sameValueUpdated)
                    {
                        data->signal_property(property);
                    }
                    return true;
                }
                return false;
#ifdef __EXCEPTIONS
            }

            catch (sdbusplus::exception_t& e)
            {
                return sd_bus_error_set(error, e.name(), e.description());
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
    }

    static int method_handler(sd_bus_message* m, void* userdata,
                              sd_bus_error* error)
    {
        dbus_interface* data = static_cast<dbus_interface*>(userdata);
        auto mesg = message::message(m);
        auto func = data->callbacksMethod_.find(mesg.get_member());
        if (func != data->callbacksMethod_.end())
        {
#ifdef __EXCEPTIONS
            try
            {
#endif
                int status = func->second->call(mesg);
                if (status == 1)
                {
                    return status;
                }
#ifdef __EXCEPTIONS
            }

            catch (sdbusplus::exception_t& e)
            {
                return sd_bus_error_set(error, e.name(), e.description());
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
    }

    /** @brief Create a new signal message.
     *
     *  @param[in] member - The signal name to create.
     */
    auto new_signal(const char* member)
    {
        if (!initialized_)
        {
            return message::message(nullptr);
        }
        return interface_->new_signal(member);
    }

    bool initialize(const bool skipPropertyChangedSignal = false)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        initialized_ = true;
        vtable_.emplace_back(vtable::end());

        interface_ = std::make_unique<sdbusplus::server::interface::interface>(
            static_cast<sdbusplus::bus::bus&>(*conn_), path_.c_str(),
            name_.c_str(), static_cast<const sd_bus_vtable*>(&vtable_[0]),
            this);
        conn_->emit_interfaces_added(path_.c_str(),
                                     std::vector<std::string>{name_});
        if (!skipPropertyChangedSignal)
        {
            for (const std::string& name : propertyNames_)
            {
                signal_property(name);
            }
        }
        return true;
    }

    bool is_initialized()
    {
        return initialized_;
    }

    bool signal_property(const std::string& name)
    {
        if (!initialized_)
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
    std::list<std::string> propertyNames_;
    std::list<std::string> methodNames_;
    std::set<std::string> signalNames_;
    boost::container::flat_map<std::string, std::unique_ptr<callback>>
        callbacksGet_;
    boost::container::flat_map<std::string, std::unique_ptr<callback_set>>
        callbacksSet_;
    boost::container::flat_map<std::string, std::unique_ptr<callback>>
        callbacksMethod_;
    std::vector<sd_bus_vtable> vtable_;
    std::unique_ptr<sdbusplus::server::interface::interface> interface_;

    bool initialized_ = false;
};

class object_server
{
  public:
    object_server(std::shared_ptr<sdbusplus::asio::connection>& conn,
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
        if (!std::regex_match(path, std::regex(PathPattern)) ||
            !std::regex_match(name, std::regex(InterfaceNamePattern)))
        {
            throw exception::SdBusError(EINVAL, "Invalid path or interface");
        }
        auto dbusIface = std::make_shared<dbus_interface>(conn_, path, name);
        interfaces_.emplace_back(dbusIface);
        return dbusIface;
    }

    void add_manager(const std::string& path)
    {
        managers_.emplace_back(
            std::make_unique<server::manager::manager>(server::manager::manager(
                static_cast<sdbusplus::bus::bus&>(*conn_), path.c_str())));
    }

    bool remove_interface(std::shared_ptr<dbus_interface>& iface)
    {
        auto findIface =
            std::find(interfaces_.begin(), interfaces_.end(), iface);
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
    std::vector<std::unique_ptr<server::manager::manager>> managers_;
};

} // namespace asio
} // namespace sdbusplus
