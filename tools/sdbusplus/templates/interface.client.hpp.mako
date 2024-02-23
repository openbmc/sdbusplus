#pragma once
#include <sdbusplus/async/client.hpp>
#include <type_traits>

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
