<%
m_name = method.snake_case
m_tag = method.snake_case + "_t"
m_param = method.parameters_as_list()
m_pmove = method.parameters_as_list(lambda p: f"std::move({p.camelCase})")
m_pargs = method.parameters_as_list(lambda p: method.parameter(interface, p))
m_ptypes = method.parameter_types_as_list(interface)
m_return = method.cpp_return_type(interface)
i_name = interface.classname
m_param_count = len(method.parameters)
m_return_count = len(method.returns)
%>\
    static int _callback_m_${m_name}(sd_bus_message* msg, void* context,
                                     sd_bus_error* error [[maybe_unused]])
        requires (server_details::has_method<
                            ${m_tag}, Instance\
% if m_param_count:
, ${m_ptypes}\
% endif
>)
    {
        auto self = static_cast<${i_name}*>(context);
        auto self_i = static_cast<Instance*>(self);

        try
        {
            auto m = sdbusplus::message_t{msg};
% if m_param_count == 1:
            auto ${m_param} = m.unpack<${m_ptypes}>();
% elif m_param_count:
            auto [${m_param}] = m.unpack<${m_ptypes}>();
% endif

            constexpr auto has_method_msg =
                server_details::has_method_msg<
                    ${m_tag}, Instance\
% if m_param_count:
, ${m_ptypes}\
% endif
>;

            if constexpr (has_method_msg)
            {
                constexpr auto is_async = std::is_same_v<
                    boost::asio::awaitable<${m_return}>,
                    decltype(self_i->method_call(${m_tag}{}, m\
% if m_param_count:
,
                                ${m_pmove}\
%endif
))>;

                if constexpr (!is_async)
                {
                    auto r = m.new_method_return();
% if m_return_count == 0:
                    \
% elif m_return_count == 1:
                    r.append(\
% else:
                    std::apply(
                        [&](auto&&... v) { (r.append(std::move(v)), ...); },
                        \
% endif
self_i->method_call(${m_tag}{}, m\
% if m_param_count:
,
                            ${m_pmove}\
%endif
% if m_return_count != 0:
)\
% endif
);
                    r.method_return();
                }
                else
                {
                    auto fn = [](auto self, auto self_i,
                                 sdbusplus::message_t m\
% if m_param_count:
,
                                 ${m_pargs}\
% endif
)
                            -> boost::asio::awaitable<void>
                    {
                        try
                        {

                            auto r = m.new_method_return();
% if m_return_count == 0:
                            \
% elif m_return_count == 1:
                            r.append(\
% else:
                            std::apply(
                                [&](auto&&... v) { (r.append(std::move(v)), ...); },
                                \
% endif
co_await self_i->method_call(
                                ${m_tag}{}, m\
% if m_param_count:
, ${m_pmove}\
% endif
% if m_return_count != 0:
)\
% endif
);

                            r.method_return();
                            co_return;
                        }
% for e in method.errors:
                        catch(const ${interface.errorNamespacedClass(e)}& e)
                        {
                            m.new_method_error(e).method_return();
                            co_return;
                        }
                        % endfor
                        catch(const std::exception&)
                        {
                            self->_context().get_bus().set_current_exception(
                                std::current_exception());
                            co_return;
                        }
                    };

                    boost::asio::co_spawn(
                        self->_context().ctx,
                        std::move(fn(self, self_i, m\
% if m_param_count:
, ${m_pmove}\
% endif
)),
                        boost::asio::detached);
                }
            }
            else
            {
                constexpr auto is_async [[maybe_unused]] = std::is_same_v<
                    boost::asio::awaitable<${m_return}>,
                    decltype(self_i->method_call(${m_tag}{}\
% if m_param_count:
,
                                ${m_pmove}\
% endif:
))>;

                if constexpr (!is_async)
                {
                    auto r = m.new_method_return();
% if m_return_count == 0:
                    \
% elif m_return_count == 1:
                    r.append(\
% else:
                    std::apply(
                        [&](auto&&... v) { (r.append(std::move(v)), ...); },
                        \
% endif
self_i->method_call(${m_tag}{}\
% if m_param_count:
,
                            ${m_pmove}\
%endif
% if m_return_count != 0:
)\
% endif
);
                    r.method_return();
                }
                else
                {
                    auto fn = [](auto self, auto self_i,
                                 sdbusplus::message_t m\
% if m_param_count:
,
                                 ${m_pargs}\
% endif
)
                            -> boost::asio::awaitable<void>
                    {
                        try
                        {

                            auto r = m.new_method_return();
% if m_return_count == 0:
                            \
% elif m_return_count == 1:
                            r.append(\
% else:
                            std::apply(
                                [&](auto&&... v) { (r.append(std::move(v)), ...); },
                                \
% endif
co_await self_i->method_call(
                                ${m_tag}{}\
% if m_param_count:
, ${m_pmove}\
% endif
% if m_return_count != 0:
)\
% endif
);

                            r.method_return();
                            co_return;
                        }
% for e in method.errors:
                        catch(const ${interface.errorNamespacedClass(e)}& e)
                        {
                            m.new_method_error(e).method_return();
                            co_return;
                        }
                        % endfor
                        catch(const std::exception&)
                        {
                            self->_context().get_bus().set_current_exception(
                                std::current_exception());
                            co_return;
                        }
                    };

                    boost::asio::co_spawn(
                        self->_context().ctx,
                        std::move(fn(self, self_i, m\
% if m_param_count:
, ${m_pmove}\
% endif
)),
                        boost::asio::detached);
                }
            }
        }
% for e in method.errors:
        catch(const ${interface.errorNamespacedClass(e)}& e)
        {
            return e.set_error(error);
        }
% endfor
        catch(const std::exception&)
        {
            self->_context().get_bus().set_current_exception(
                std::current_exception());
            return -EINVAL;
        }

        return 1;
    }