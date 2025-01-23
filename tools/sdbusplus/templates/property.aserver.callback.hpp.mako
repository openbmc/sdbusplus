<%
p_name = property.snake_case
p_tag = property.snake_case + "_t"
p_type = property.cppTypeParam(interface.name)
i_name = interface.classname
%>\
    static int _callback_get_${p_name}(
        sd_bus*, const char*, const char*, const char*,
        sd_bus_message* reply, void* context,
        sd_bus_error* error [[maybe_unused]])
    {
        auto self = static_cast<${i_name}*>(context);

        try
        {
            auto m = sdbusplus::message_t{reply};

            // Set up the transaction.
            sdbusplus::server::transaction::set_id(m);

            // Get property value and add to message.
            if constexpr (server_details::has_get_property_msg<${p_tag},
                                                               Instance>)
            {
                auto v = self->${p_name}(m);
                static_assert(std::is_convertible_v<decltype(v), ${p_type}>,
                              "Property doesn't convert to '${p_type}'.");
                m.append<${p_type}>(std::move(v));
            }
            else
            {
                auto v = self->${p_name}();
                static_assert(std::is_convertible_v<decltype(v), ${p_type}>,
                              "Property doesn't convert to '${p_type}'.");
                m.append<${p_type}>(std::move(v));
            }
        }
        % for e in property.errors:
        catch (const ${interface.errorNamespacedClass(e)}& e)
        {
            return e.set_error(error);
        }
        % endfor
        catch (const std::exception&)
        {
            self->_context().get_bus().set_current_exception(
                std::current_exception());
            return -EINVAL;
        }

        return 1;
    }

% if 'const' not in property.flags and 'readonly' not in property.flags:
    static int _callback_set_${p_name}(
        sd_bus*, const char*, const char*, const char*,
        sd_bus_message* value[[maybe_unused]], void* context [[maybe_unused]],
        sd_bus_error* error [[maybe_unused]])
    {
        auto self = static_cast<${i_name}*>(context);

        try
        {
            auto m = sdbusplus::message_t{value};

            // Set up the transaction.
            sdbusplus::server::transaction::set_id(m);

            auto new_value = m.unpack<${p_type}>();

            // Get property value and add to message.
            if constexpr (server_details::has_set_property_msg<
                              ${p_tag}, Instance, ${p_type}>)
            {
                self->${p_name}(m, std::move(new_value));
            }
            else
            {
                self->${p_name}(std::move(new_value));
            }
        }
        % for e in property.errors:
        catch (const ${interface.errorNamespacedClass(e)}& e)
        {
            return e.set_error(error);
        }
        % endfor
        catch (const std::exception&)
        {
            self->_context().get_bus().set_current_exception(
                std::current_exception());
            return -EINVAL;
        }

        return 1;
    }
% endif
