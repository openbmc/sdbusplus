<%
p_name = property.snake_case;
p_tag = property.snake_case + "_t"
%>\
    auto ${p_name}() const
        requires server_details::has_get_property_nomsg<${p_tag}, Instance>
    {
        return static_cast<const Instance*>(this)->get_property(${p_tag}{});
    }
    auto ${p_name}(sdbusplus::message_t& m) const
        requires server_details::has_get_property_msg<${p_tag}, Instance>
    {
        return static_cast<const Instance*>(this)->get_property(${p_tag}{}, m);
    }
    auto ${p_name}() const noexcept
        requires (!server_details::has_get_property<${p_tag}, Instance>)
    {
        static_assert(
            !server_details::has_get_property_missing_const<${p_tag},
                                                            Instance>,
            "Missing const on get_property(${p_tag})?");
        return ${p_name}_;
    }
