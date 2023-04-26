#pragma once

#include <sdbusplus/async/proxy.hpp>

namespace sdbusplus::async
{

namespace client
{

/** An aggregation class of sdbusplus::async::proxy-based client types.
 *
 *  The resulting class acts as a union of all Types from the template
 *  arguments.
 *
 *  Like a `proxy`, the class only becomes functional once the service and
 *  path are populated.
 */
template <bool S, bool P, bool Preserved, template <typename> typename... Types>
class client :
    public Types<sdbusplus::async::proxy_ns::proxy<S, P, false, Preserved>>...
{
  private:
    sdbusplus::async::context& ctx{};
    sdbusplus::async::proxy_ns::proxy<S, P, false, Preserved> proxy{};

  public:
    constexpr client() = delete;
    /* Delete default constructor if Service or Path have been provided. */
    explicit client(sdbusplus::async::context& ctx)
        requires(S || P)
    = delete;
    /* Default (empty) constructor only when Service and Path are missing. */
    explicit client(sdbusplus::async::context& ctx)
        requires(!S && !P)
        : Types<decltype(proxy)>(ctx, proxy)..., ctx(ctx)
    {}

    /* Conversion constructor for a non-empty (Service and/or Path) proxy. */
    explicit client(sdbusplus::async::context& ctx,
                    sdbusplus::async::proxy_ns::proxy<S, P, false, Preserved> p)
        requires(S || P)
        : Types<decltype(proxy)>(ctx, p)..., ctx(ctx), proxy(p)
    {}

    /* Convert a non-Service instance to a Service instance. */
    auto service(auto& s) const noexcept
        requires(!S)
    {
        return client<true, P, Preserved, Types...>(ctx, proxy.service(s));
    }

    /* Convert a non-Path instance to a Path instance. */
    auto path(auto& p) const noexcept
        requires(!P)
    {
        return client<S, true, Preserved, Types...>(ctx, proxy.path(p));
    }
};

} // namespace client

/** A non-Preserved client alias.
 *
 *  This holds Service/Path in string-views, which must exist longer than
 *  the lifetime of this client_t.
 */
template <template <typename> typename... Types>
using client_t = client::client<false, false, false, Types...>;
/** A Preserved client alias.
 *
 *  This holds Service/Path in strings, which thus have lifetimes that are
 *  the same as the client itself.
 */
template <template <typename> typename... Types>
using client_preserved_t = client::client<false, false, false, Types...>;

} // namespace sdbusplus::async
