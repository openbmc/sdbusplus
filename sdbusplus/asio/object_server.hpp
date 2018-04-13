#pragma once

#include <list>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/message/read.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <boost/any.hpp>
#include <boost/container/flat_map.hpp>

namespace sdbusplus
{
namespace asio
{

constexpr const char *PropertyNameAllowedCharacters =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
class callback
{
  public:
    virtual int call(message::message &m) = 0;
};
class callback_set
{
  public:
    virtual int call(message::message &m) = 0;
    virtual int set(const boost::any &value) = 0;
};

template <typename CallbackType>
class callback_method_instance : public callback
{
  public:
    callback_method_instance(CallbackType &&func) : func_(std::move(func))
    {
    }
    int call(message::message &m) override
    {
        InputTupleType inputArgs;
        if (unpack_into_tuple(inputArgs, m) == false)
        {
            return -EINVAL;
        }

        auto ret = m.new_method_return();
        callFunction<ResultType>(ret, inputArgs);
        ret.method_return();
        return 1;
    };

  private:
    using traits = function_traits<CallbackType>;
    using InputTupleType = typename traits::decayed_arg_types;
    using ResultType = typename traits::result_type;
    CallbackType func_;
    template <typename T>
    std::enable_if_t<!std::is_void<T>::value, void>
        callFunction(message::message &m, InputTupleType &inputArgs)
    {
        ResultType r = sdbusplus::apply(func_, inputArgs);
        m.append(r);
    }
    template <typename T>
    std::enable_if_t<std::is_void<T>::value, void>
        callFunction(message::message &m, InputTupleType &inputArgs)
    {
        sdbusplus::apply(func_, inputArgs);
    }
};

template <typename PropertyType, typename CallbackType>
class callback_get_instance : public callback
{
  public:
    callback_get_instance(const std::shared_ptr<PropertyType> &value,
                          CallbackType &&func) :
        value_(value),
        func_(std::move(func))
    {
    }
    int call(message::message &m) override
    {
        ResultType r = func_(*value_);
        m.append(r);
        return 1;
    }

  private:
    using traits = function_traits<CallbackType>;
    using ResultType = typename traits::result_type;
    std::shared_ptr<PropertyType> value_;
    CallbackType func_;
};

template <typename PropertyType, typename CallbackType>
class callback_set_instance : public callback_set
{
  public:
    callback_set_instance(const std::shared_ptr<PropertyType> &value,
                          CallbackType &&func) :
        value_(value),
        func_(std::move(func))
    {
    }
    int call(message::message &m) override
    {
        PropertyType input;
        m.read(input);

        return func_(input, *value_);
    }
    int set(const boost::any &value)
    {
        return func_(boost::any_cast<PropertyType>(value), *value_);
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
                   const std::string &path, const std::string &name) :
        conn_(conn),
        path_(path), name_(name)

    {
        vtable_.emplace_back(vtable::start());
    }

    // default getter and setter
    template <typename PropertyType>
    bool register_property(
        const std::string &name, const PropertyType &property,
        PropertyPermission access = PropertyPermission::readOnly)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (name.find_first_not_of(PropertyNameAllowedCharacters) !=
            std::string::npos)
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto nameItr = propertyNames_.emplace(propertyNames_.end(), name);
        auto propertyPtr = std::make_shared<PropertyType>(property);

        callbacksGet_[name] = std::make_unique<callback_get_instance<
            PropertyType, std::function<PropertyType(const PropertyType &)>>>(
            propertyPtr, [](const PropertyType &value) { return value; });

        if (access == PropertyPermission::readOnly)
        {
            vtable_.emplace_back(
                vtable::property(nameItr->c_str(), type.data(), get_handler,
                                 vtable::property_::emits_change));
        }
        else
        {
            callbacksSet_[name] = std::make_unique<callback_set_instance<
                PropertyType,
                std::function<int(const PropertyType &, PropertyType &)>>>(
                propertyPtr, [](const PropertyType &req, PropertyType &old) {
                    old = req;
                    return 1;
                });
            vtable_.emplace_back(
                vtable::property(nameItr->c_str(), type.data(), get_handler,
                                 set_handler, vtable::property_::emits_change));
        }
        return true;
    }

    // custom setter, sets take an input property and respond with an int status
    template <typename PropertyType, typename CallbackType>
    bool register_property(const std::string &name,
                           const PropertyType &property,
                           CallbackType &&setFunction)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (name.find_first_not_of(PropertyNameAllowedCharacters) !=
            std::string::npos)
        {
            return false;
        }
        static const auto type =
            utility::tuple_to_array(message::types::type_id<PropertyType>());

        auto nameItr = propertyNames_.emplace(propertyNames_.end(), name);
        auto propertyPtr = std::make_shared<PropertyType>(property);

        callbacksGet_[name] = std::make_unique<callback_get_instance<
            PropertyType, std::function<PropertyType(const PropertyType &)>>>(
            propertyPtr, [](const PropertyType &value) { return value; });

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
    bool register_property(const std::string &name,
                           const PropertyType &property,
                           CallbackType &&setFunction,
                           CallbackTypeGet &&getFunction)
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        if (name.find_first_not_of(PropertyNameAllowedCharacters) !=
            std::string::npos)
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
    template <typename PropertyType>
    bool set_property(const std::string &name, const PropertyType &value)
    {
        if (!initialized_)
        {
            return false;
        }
        auto func = callbacksSet_.find(name);
        if (func != callbacksSet_.end())
        {
            if (func->second->set(value) != 1)
            {
                return false;
            }
            signal_property(name);
            return true;
        }
        return false;
    }

    template <typename CallbackType>
    bool register_method(const std::string &name, CallbackType &&handler)
    {
        using traits = function_traits<CallbackType>;
        using InputTupleType = typename traits::decayed_arg_types;
        using ResultType = typename traits::result_type;

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

    static int get_handler(sd_bus *bus, const char *path, const char *interface,
                           const char *property, sd_bus_message *reply,
                           void *userdata, sd_bus_error *error)
    {
        dbus_interface *data = static_cast<dbus_interface *>(userdata);
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

            catch (sdbusplus::exception_t &e)
            {
                sd_bus_error_set_const(error, e.name(), e.description());
                return -EINVAL;
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
        return -EINVAL;
    }

    static int set_handler(sd_bus *bus, const char *path, const char *interface,
                           const char *property, sd_bus_message *value,
                           void *userdata, sd_bus_error *error)
    {
        dbus_interface *data = static_cast<dbus_interface *>(userdata);
        auto func = data->callbacksSet_.find(property);
        auto mesg = message::message(value);
        if (func != data->callbacksSet_.end())
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

            catch (sdbusplus::exception_t &e)
            {
                sd_bus_error_set_const(error, e.name(), e.description());
                return -EINVAL;
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
        return -EINVAL;
    }

    static int method_handler(sd_bus_message *m, void *userdata,
                              sd_bus_error *error)
    {
        dbus_interface *data = static_cast<dbus_interface *>(userdata);
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

            catch (sdbusplus::exception_t &e)
            {
                sd_bus_error_set_const(error, e.name(), e.description());
                return -EINVAL;
            }
            catch (...)
            {
                // hit default error below
            }
#endif
        }
        sd_bus_error_set_const(error, SD_BUS_ERROR_INVALID_ARGS, NULL);
        return -EINVAL;
    }

    bool initialize()
    {
        // can only register once
        if (initialized_)
        {
            return false;
        }
        initialized_ = true;
        vtable_.emplace_back(vtable::end());

        interface_ = std::make_unique<sdbusplus::server::interface::interface>(
            static_cast<sdbusplus::bus::bus &>(*conn_), path_.c_str(),
            name_.c_str(), static_cast<const sd_bus_vtable *>(&vtable_[0]),
            this);

        for (const std::string &name : propertyNames_)
        {
            signal_property(name);
        }
        return true;
    }
    void signal_property(const std::string &name)
    {
        interface_->property_changed(name.c_str());
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
    boost::container::flat_map<std::string, std::unique_ptr<callback>>
        callbacksGet_;
    boost::container::flat_map<std::string, std::unique_ptr<callback_set>>
        callbacksSet_;
    boost::container::flat_map<std::string, std::unique_ptr<callback>>
        callbacksMethod_;
    std::unique_ptr<sdbusplus::server::interface::interface> interface_;
    std::vector<sd_bus_vtable> vtable_;

    bool initialized_ = false;
};

class object_server
{
  public:
    object_server(std::shared_ptr<sdbusplus::asio::connection> &conn) :
        conn_(conn)
    {
        auto root = add_interface("/", "");
        root->initialize();
        add_manager("/");
    }

    std::shared_ptr<dbus_interface> add_interface(const std::string &path,
                                                  const std::string &name)
    {

        auto dbusIface = std::make_shared<dbus_interface>(conn_, path, name);
        interfaces_.emplace_back(dbusIface);
        return dbusIface;
    }

    void add_manager(const std::string &path)
    {
        managers_.emplace_back(
            std::make_unique<server::manager::manager>(server::manager::manager(
                static_cast<sdbusplus::bus::bus &>(*conn_), path.c_str())));
    }

    bool remove_interface(std::shared_ptr<dbus_interface> &iface)
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
