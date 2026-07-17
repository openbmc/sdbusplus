#include <sdbusplus/async/object_server.hpp>

#include <sdbusplus/server/transaction.hpp>

#include <cerrno>

namespace sdbusplus::async
{

void dbus_interface::initialize()
{
    assert_not_initialized();

    // A previous initialize() may have thrown after populating the vtable.
    _vtable.clear();
    _vtable.reserve(2 + _properties.size() + _methods.size() + _signals.size());
    _vtable.emplace_back(vtable::start());
    for (const auto& [pname, p] : _properties)
    {
        if (p->writable)
        {
            _vtable.emplace_back(
                vtable::property(pname.c_str(), p->signature(), _callback_get,
                                 _callback_set, p->flags));
        }
        else
        {
            _vtable.emplace_back(vtable::property(pname.c_str(), p->signature(),
                                                  _callback_get, p->flags));
        }
    }
    for (const auto& [mname, m] : _methods)
    {
        _vtable.emplace_back(
            vtable::method(mname.c_str(), m->in_signature(), m->out_signature(),
                           _callback_method, m->flags));
    }
    for (const auto& [sname, sig] : _signals)
    {
        _vtable.emplace_back(
            vtable::signal(sname.c_str(), sig.first, sig.second));
    }
    _vtable.emplace_back(vtable::end());

    _interface.emplace(ctx.get_bus(), _path.c_str(), _name.c_str(),
                       _vtable.data(), this);
}

int dbus_interface::_callback_get(sd_bus*, const char*, const char*,
                                  const char* property, sd_bus_message* reply,
                                  void* userdata, sd_bus_error* error) noexcept
{
    auto* self = static_cast<dbus_interface*>(userdata);
    try
    {
        auto m = message_t{reply};
        sdbusplus::server::transaction::set_id(m);

        auto it = self->_properties.find(std::string_view{property});
        if (it == self->_properties.end())
        {
            return -EINVAL;
        }
        it->second->get(m);
    }
    catch (const sdbusplus::exception_t& e)
    {
        return e.set_error(error);
    }
    catch (const std::exception& e)
    {
        return sd_bus_error_set(error, SD_BUS_ERROR_FAILED, e.what());
    }
    catch (...)
    {
        return -EINVAL;
    }
    return 1;
}

int dbus_interface::_callback_set(sd_bus*, const char*, const char*,
                                  const char* property, sd_bus_message* value,
                                  void* userdata, sd_bus_error* error) noexcept
{
    auto* self = static_cast<dbus_interface*>(userdata);
    try
    {
        auto m = message_t{value};
        sdbusplus::server::transaction::set_id(m);

        auto it = self->_properties.find(std::string_view{property});
        if (it == self->_properties.end())
        {
            return -EINVAL;
        }

        bool changed = it->second->set(m);
        if (changed &&
            (it->second->flags & (vtable::property_::emits_change |
                                  vtable::property_::emits_invalidation)))
        {
            self->_interface->property_changed(it->first.c_str());
        }
    }
    catch (const sdbusplus::exception_t& e)
    {
        return e.set_error(error);
    }
    catch (const std::exception& e)
    {
        return sd_bus_error_set(error, SD_BUS_ERROR_FAILED, e.what());
    }
    catch (...)
    {
        return -EINVAL;
    }
    return 1;
}

int dbus_interface::_callback_method(sd_bus_message* msg, void* userdata,
                                     sd_bus_error* error) noexcept
{
    auto* self = static_cast<dbus_interface*>(userdata);
    try
    {
        auto m = message_t{msg};

        const char* member = sd_bus_message_get_member(msg);
        auto it = self->_methods.find(std::string_view{member ? member : ""});
        if (it == self->_methods.end())
        {
            return -EINVAL;
        }

        auto& holder = it->second;
        holder->spawn_invoke(holder, self->ctx, std::move(m));
    }
    catch (const sdbusplus::exception_t& e)
    {
        return e.set_error(error);
    }
    catch (const std::exception& e)
    {
        return sd_bus_error_set(error, SD_BUS_ERROR_FAILED, e.what());
    }
    catch (...)
    {
        return -EINVAL;
    }
    return 1;
}

} // namespace sdbusplus::async
