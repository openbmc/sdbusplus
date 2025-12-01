###
### Emit 'header'
###
    % if ptype == 'header':
        /** @brief Implementation for ${ method.name }
         *  ${ method.description.strip() }
    % if len(method.parameters) != 0:
         *
        % for p in method.parameters:
         *  @param[in] ${p.camelCase} - ${p.description.strip()}
        % endfor
    % endif
    % if len(method.returns) != 0:
         *
        % for r in method.returns:
         *  @return ${r.camelCase}[${r.cppTypeParam(interface.name)}] \
- ${r.description.strip()}
        % endfor
    % endif
         */
        virtual ${method.cpp_return_type(interface)} ${ method.camelCase }(
            ${ method.get_parameters_str(interface) }) = 0;
###
### Emit 'callback-header'
###
    % elif ptype == 'callback-header':
        /** @brief sd-bus callback for ${ method.name }
         */
        static int _callback_${ method.CamelCase }(
            sd_bus_message*, void*, sd_bus_error*);
###
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
int ${interface.classname}::_callback_${ method.CamelCase }(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto o = static_cast<${interface.classname}*>(context);

    try
    {
        return sdbusplus::sdbuspp::method_callback\
    % if len(method.returns) > 1:
<true>\
    % endif
(
                msg, o->get_bus().getInterface(), error,
                std::function(
                    [=](${method.parameters_as_arg_list(interface)})
                    {
                        return o->${ method.camelCase }(
                                ${method.parameters_as_list()});
                    }
                ));
    }
    % for e in method.errors:
    catch(const ${interface.errorNamespacedClass(e)}& e)
    {
        return e.set_error(o->get_bus().getInterface(), error);
    }
    % endfor
    catch (const std::exception&)
    {
        o->get_bus().set_current_exception(std::current_exception());
        return 1;
    }
}

namespace details
{
namespace ${interface.classname}
{
static const auto _param_${ method.CamelCase } =
        message::types::type_id<
                ${ method.parameter_types_as_list(interface) }>();
static const auto _return_${ method.CamelCase } =
        message::types::type_id<
                ${ method.returns_as_list(interface, full=True) }>();
}
}
    % endif
