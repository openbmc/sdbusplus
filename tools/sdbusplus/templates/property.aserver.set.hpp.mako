<%
p_name = property.snake_case;
p_tag = property.snake_case + "_t"
p_type = property.cppTypeParam(interface.name)
i_name = interface.joinedName("_", "interface")
%>\
    template <bool EmitSignal = true, typename Arg = ${p_type}>
    void ${p_name}(Arg&& new_value)
        requires server_details::has_set_property_nomsg<${p_tag}, Instance,
                                                        ${p_type}>
    {
        bool changed = static_cast<Instance*>(this)->set_property(
            ${p_tag}{}, std::forward<Arg>(new_value));

        if (changed && EmitSignal)
        {
            _${i_name}.property_changed("${property.name}");
        }
    }

    template <bool EmitSignal = true, typename Arg = ${p_type}>
    void ${p_name}(sdbusplus::message_t& m, Arg&& new_value)
        requires server_details::has_set_property_msg<${p_tag}, Instance,
                                                      ${p_type}>
    {
        bool changed = static_cast<Instance*>(this)->set_property(
            ${p_tag}{}, m, std::forward<Arg>(new_value));

        if (changed && EmitSignal)
        {
            _${i_name}.property_changed("${property.name}");
        }
    }

    template <bool EmitSignal = true, typename Arg = ${p_type}>
    void ${p_name}(Arg&& new_value)
        requires (!server_details::has_set_property<${p_tag}, Instance,
                                                    ${p_type}>)
    {
        static_assert(
            !server_details::has_get_property<${p_tag}, Instance>,
            "Cannot create default set-property for '${p_tag}' with get-property overload.");

        bool changed = (new_value != ${p_name}_);
        ${p_name}_ = std::forward<Arg>(new_value);

        if (changed && EmitSignal)
        {
            _${i_name}.property_changed("${property.name}");
        }
    }
