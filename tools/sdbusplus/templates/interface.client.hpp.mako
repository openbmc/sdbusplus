#pragma once
#include <sdbusplus/async/client.hpp>
#include <sdbusplus/async/execution.hpp>
#include <type_traits>
#include <variant>

% for h in interface.cpp_includes():
#include <${h}>
% endfor
#include <${interface.headerFile()}>

namespace sdbusplus::client::${interface.cppNamespace()}
{

namespace details
{
// forward declaration
template <typename Client, typename Proxy>
class ${interface.classname};
} // namespace details

/** Alias class so we can use the client in both a client_t aggregation
 *  and individually.
 *
 *  sdbusplus::async::client_t<${interface.classname}>() or
 *  ${interface.classname}() both construct an equivalent instance.
 */
template <typename Client = void, typename Proxy = void>
struct ${interface.classname} :
    public std::conditional_t<std::is_void_v<Client>,
                              sdbusplus::async::client_t<details::${interface.classname}>,
                              details::${interface.classname}<Client, Proxy>>
{
    template <typename... Args>
    ${interface.classname}(Args&&... args) :
        std::conditional_t<std::is_void_v<Client>,
                           sdbusplus::async::client_t<details::${interface.classname}>,
                           details::${interface.classname}<Client, Proxy>>(
            std::forward<Args>(args)...)
    {}
};

namespace details
{

template <typename Client, typename Proxy>
class ${interface.classname} :
    public sdbusplus::common::${interface.cppNamespacedClass()},
    private sdbusplus::async::client::details::client_context_friend
{
  public:
    struct properties_t
    {
        % for p in interface.properties:
        ${p.cppTypeParam(interface.name)} ${p.snake_case}${p.default_value(interface.name)};
        % endfor
    };

    friend Client;
    template <typename, typename>
    friend struct sdbusplus::client::${interface.cppNamespacedClass()};

    // Delete default constructor as these should only be constructed
    // indirectly through sdbusplus::async::client_t.
    ${interface.classname}() = delete;

    % for m in interface.methods:
${m.render(loader, "method.client.hpp.mako", method=m, interface=interface)}
    % endfor
    % for p in interface.properties:
${p.render(loader, "property.client.hpp.mako", property=p, interface=interface)}
    % endfor

    static properties_t
        unpack(const std::unordered_map<std::string, PropertiesVariant>& props)
    {
        properties_t result;
        for (const auto& [property, value] : props)
        {
            std::visit(
                [&](auto v) {
                    % for p in interface.properties:
                    if (property == "${p.name}")
                    {
                        if constexpr (std::is_same_v<std::decay_t<decltype(v)>,
                                                     ${p.cppTypeParam(interface.name)}>)
                        {
                            result.${p.snake_case} = v;
                            return;
                        }
                        else
                        {
                            throw exception::UnpackPropertyError(
                                property, UnpackErrorReason::wrongType);
                        }
                    }
                    % endfor
                },
                value);
        }
        return result;
    }

    template <typename V>
    auto managed_objects()
    {
        using result_t = std::unordered_map<sdbusplus::message::object_path,
                                            typename V::properties_t>;
        return proxy.template get_managed_objects<V>(context()) |
               sdbusplus::async::execution::then([](auto&& v) {
                   result_t result;
                   for (const auto& [path, properties] : v)
                   {
                       result.emplace(path.filename(), V::unpack(properties));
                   }
                   return result;
               });
    }

    auto properties()
    {
        return proxy.template get_all_properties<PropertiesVariant>(context()) |
               sdbusplus::async::execution::then([](auto&& v) {
                   return unpack(v);
               });
    }

  private:
    // Conversion constructor from proxy used by client_t.
    explicit constexpr ${interface.classname}(Proxy p) :
        proxy(p.interface(interface))
    {}

    sdbusplus::async::context& context()
    {
        return sdbusplus::async::client::details::client_context_friend::
            context<Client, ${interface.classname}>(this);
    }

    decltype(std::declval<Proxy>().interface(interface)) proxy = {};
};

} // namespace details

} // namespace sdbusplus::client::${interface.cppNamespace()}
