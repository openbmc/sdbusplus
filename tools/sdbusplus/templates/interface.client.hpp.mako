#pragma once
#include <sdbusplus/async/client.hpp>
#include <type_traits>

% for h in interface.cpp_includes():
#include <${h}>
% endfor
#include <${interface.headerFile("common")}>

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
{

constexpr auto interface =
    sdbusplus::common::${interface.cppNamespacedClass()}::interface;

} // namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
#endif

namespace sdbusplus::client::${interface.cppNamespace()}
{

namespace details
{

template <typename Proxy>
class ${interface.classname} :
    public sdbusplus::common::${interface.cppNamespacedClass()}
{
  public:
    template <bool S, bool P, bool Preserved,
              template <typename> typename... Types>
    friend class sdbusplus::async::client::client;
    // Delete default constructor as these should only be constructed
    // indirectly through sdbusplus::async::client_t.
    ${interface.classname}() = delete;

    // To be replaced by generators...
    template <typename... Rs, typename... Ss>
    auto call(sdbusplus::async::context& ctx, auto& method, Ss&&... ss) const
    {
        return proxy.template call<Rs...>(ctx, method, std::forward<Ss>(ss)...);
    }

    // To be replaced by generators...
    template <typename T>
    auto get_property(sdbusplus::async::context& ctx, auto& property) const
    {
        return proxy.template get_property<T>(ctx, property);
    }

    // To be replaced by generators...
    template <typename T>
    auto set_property(sdbusplus::async::context& ctx, auto& property,
                      T&& value) const
    {
        return proxy.template set_property<T>(ctx, property,
                                              std::forward<T>(value));
    }

  private:
    // Conversion constructor from proxy used by client_t.
    constexpr explicit ${interface.classname}(Proxy p) :
        proxy(p.interface(interface)) {}

    decltype(std::declval<Proxy>().interface(interface)) proxy = {};
};

} // namespace details

/** Alias class so we can use the client in both a client_t aggregation
 *  and individually.
 *
 *  sdbusplus::async::client_t<${interface.classname}>() or
 *  ${interface.classname}() both construct an equivalent instance.
 */
template <typename Proxy = void>
struct ${interface.classname} : public
    std::conditional_t<
        std::is_void_v<Proxy>,
        sdbusplus::async::client_t<details::${interface.classname}>,
        details::${interface.classname}<Proxy>>
{
    template <typename... Args>
    ${interface.classname}(Args&&... args) :
        std::conditional_t<std::is_void_v<Proxy>,
                           sdbusplus::async::client_t<details::${interface.classname}>,
                           details::${interface.classname}<Proxy>>(
            std::forward<Args>(args)...) {}
};

} // namespace sdbusplus::client::${interface.cppNamespacedClass()}
