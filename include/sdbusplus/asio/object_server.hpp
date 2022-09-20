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
#include <list>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>

namespace sdbusplus
{
namespace asio
{

class callback
{
  public:
    virtual ~callback() = default;
    virtual int call(message_t& m) = 0;
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
    virtual std::tuple<SetPropertyReturnValue, int> call(message_t& m) = 0;
    virtual SetPropertyReturnValue set(const std::any& value) = 0;
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
static constexpr bool callbackYields = FirstArgIsYield_v<T>;
template <typename T>
static constexpr bool callbackWantsMessage = (FirstArgIsMessage_v<T> ||
                                              SecondArgIsMessage_v<T>);

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
} // namespace details

template <typename CallbackType>
class callback_method_instance : public callback
{
  public:
    callback_method_instance(CallbackType&& func) : func_(std::move(func)) {}
    int call(message_t& m) override
    {
        return expandCall(m);
    }

  private:
    using CallbackSignature = boost::callable_traits::args_t<CallbackType>;
    using InputTupleType = utility::decay_tuple_t<CallbackSignature>;
    using ResultType = boost::callable_traits::return_type_t<CallbackType>;
    CallbackType func_;

    void callFunction(message_t& m, InputTupleType& inputArgs)
    {
        if constexpr (std::is_void_v<ResultType>)
        {
            std::apply(func_, inputArgs);
        }
        else
        {
            auto r = std::apply(func_, inputArgs);
            m.append(r);
        }
    }

    // optional message-first-argument callback
    int expandCall(message_t& m)
    {
        using DbusTupleType = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<InputTupleType>::size(), InputTupleType>;

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
        callFunction(ret, *inputArgs);
        ret.method_return();
        return 1;
    };
};

template <typename CallbackType>
class coroutine_method_instance : public callback
{
  public:
    coroutine_method_instance(boost::asio::io_context& io,
                              CallbackType&& func) :
        io_(io),
        func_(std::move(func))
    {}
    int call(message_t& m) override
    {
        // make a copy of m to move into the coroutine
        message_t b{m};
        // spawn off a new coroutine to handle the method call
        boost::asio::spawn(
            io_, [this, b = std::move(b)](boost::asio::yield_context yield) {
                message_t mcpy{std::move(b)};
                std::optional<message_t> err{};

                try
                {
                    expandCall(yield, mcpy);
                }
                catch (const sdbusplus::exception::SdBusError& e)
                {
                    // Catch D-Bus error explicitly called by method handler
                    err = mcpy.new_method_errno(e.get_errno(), e.get_error());
                }
                catch (const sdbusplus::exception_t& e)
                {
                    err = mcpy.new_method_error(e);
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
    using InputTupleType = utility::decay_tuple_t<CallbackSignature>;
    using ResultType = boost::callable_traits::return_type_t<CallbackType>;
    boost::asio::io_context& io_;
    CallbackType func_;

    void callFunction(message_t& m, InputTupleType& inputArgs)
    {
        if constexpr (std::is_void_v<ResultType>)
        {
            std::apply(func_, inputArgs);
        }
        else
        {
            auto r = std::apply(func_, inputArgs);
            m.append(r);
        }
    }

    // co-routine body for call
    void expandCall(boost::asio::yield_context yield, message_t& m)
    {
        using DbusTupleType = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<InputTupleType>::size(), InputTupleType>;
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
        callFunction(ret, *inputArgs);
        ret.method_return();
    };
};

template <typename PropertyType, typename CallbackType>
class callback_get_instance : public callback
{
  public:
    callback_get_instance(const std::shared_ptr<PropertyType>& value,
                          CallbackType&& func) :
        value_(value),
        func_(std::move(func))
    {}
    int call(message_t& m) override
    {
        *value_ = func_(*value_);
        m.append(*value_);
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
    std::tuple<SetPropertyReturnValue, int> call(message_t& m) override
    {
        PropertyType input;
        m.read(input);

        auto oldValue = *value_;
        auto ret = func_(input, *value_);

        if (ret < 0)
        {
            return {SetPropertyReturnValue::fail, ret};
        }
        if (oldValue == *value_)
        {
            return {SetPropertyReturnValue::sameValueUpdated, ret};
        }
        return {SetPropertyReturnValue::valueUpdated, ret};
    }
    SetPropertyReturnValue set(const std::any& value) override
    {
        auto oldValue = *value_;
        if (func_(std::any_cast<PropertyType>(value), *value_))
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

    template <typename PropertyType, typename CallbackTypeGet>
    bool register_property_r(const std::string& name,
                             const PropertyType& property,
                             decltype(vtable_t::flags) flags,
                             CallbackTypeGet&& getFunction)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
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
        callbacksSet_[name] = std::make_unique<callback_set_instance<
            PropertyType,
            std::function<int(const PropertyType&, PropertyType&)>>>(
            propertyPtr, [](const PropertyType& req, PropertyType& old) {
                old = req;
                return 1;
            });

        vtable_.emplace_back(vtable::property(nameItr->c_str(), type.data(),
                                              get_handler, flags));

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
        if (initialized_)
        {
            return false;
        }
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
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
        callbacksSet_[name] = std::make_unique<
            callback_set_instance<PropertyType, CallbackTypeSet>>(
            propertyPtr, std::move(setFunction));

        vtable_.emplace_back(vtable::property(nameItr->c_str(), type.data(),
                                              get_handler, set_handler, flags));

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
            return register_property_r(
                name, property, vtable::property_::emits_change,
                [](const PropertyType& value) { return value; });
        }
        else
        {
            return register_property_rw(
                name, property, vtable::property_::emits_change,
                [](const PropertyType& req, PropertyType& old) {
                    old = req;
                    return 1;
                },
                [](const PropertyType& value) { return value; });
        }
    }

    // custom setter, sets take an input property and respond with an int status
    template <typename PropertyType, typename CallbackTypeSet>
    bool register_property(const std::string& name,
                           const PropertyType& property,
                           CallbackTypeSet&& setFunction)
    {
        return register_property_rw(
            name, property, vtable::property_::emits_change,
            std::forward<CallbackTypeSet>(setFunction),
            [](const PropertyType& value) { return value; });
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
        if (sd_bus_member_name_is_valid(name.c_str()) != 1)
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

    template <typename CallbackType>
    bool register_method(const std::string& name, CallbackType&& handler)
    {
        using ActualSignature = boost::callable_traits::args_t<CallbackType>;
        using CallbackSignature = utility::strip_first_n_args_t<
            details::NonDbusArgsCount<ActualSignature>::size(),
            ActualSignature>;
        using InputTupleType = utility::decay_tuple_t<CallbackSignature>;
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

    static int get_handler(sd_bus* /*bus*/, const char* /*path*/,
                           const char* /*interface*/, const char* property,
                           sd_bus_message* reply, void* userdata,
                           sd_bus_error* error)
    {
        dbus_interface* data = static_cast<dbus_interface*>(userdata);
        auto func = data->callbacksGet_.find(property);
        auto mesg = message_t(reply);
        if (func != data->callbacksGet_.end())
        {
#ifdef __EXCEPTIONS
            try
            {
#endif
                return func->second->call(mesg);
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
        auto mesg = message_t(value);
        if (func != data->callbacksSet_.end())
        {
#ifdef __EXCEPTIONS
            try
            {
#endif
                auto [status, rc] = func->second->call(mesg);

                switch (status)
                {
                    case SetPropertyReturnValue::sameValueUpdated:
                    {
                        return true;
                    }
                    case SetPropertyReturnValue::valueUpdated:
                    {
                        data->signal_property(property);
                        return true;
                    }
                    case SetPropertyReturnValue::fail:
                    {
                        sd_bus_error_set_errno(error, rc);
                        return false;
                    }
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
        }
        return sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
    }

    static int method_handler(sd_bus_message* m, void* userdata,
                              sd_bus_error* error)
    {
        dbus_interface* data = static_cast<dbus_interface*>(userdata);
        auto mesg = message_t(m);
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

            catch (const sdbusplus::exception_t& e)
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
            return message_t(nullptr);
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

        interface_.emplace(static_cast<sdbusplus::bus_t&>(*conn_),
                           path_.c_str(), name_.c_str(),
                           static_cast<const sd_bus_vtable*>(&vtable_[0]),
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
    std::unordered_map<std::string, std::unique_ptr<callback>> callbacksGet_;
    std::unordered_map<std::string, std::unique_ptr<callback_set>>
        callbacksSet_;
    std::unordered_map<std::string, std::unique_ptr<callback>> callbacksMethod_;
    std::vector<sd_bus_vtable> vtable_;
    std::optional<sdbusplus::server::interface_t> interface_;

    bool initialized_ = false;
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
    std::vector<server::manager_t> managers_;
};

} // namespace asio
} // namespace sdbusplus
