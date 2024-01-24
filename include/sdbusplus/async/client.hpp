#pragma once

#include <sdbusplus/async/proxy.hpp>

namespace sdbusplus::async
{

namespace client
{

namespace details
{
struct client_context_friend;
}

/** An aggregation class of sdbusplus::async::proxy-based client types.
 *
 *  The resulting class acts as a union of all Types from the template
 *  arguments.
 *
 *  Like a `proxy`, the class only becomes functional once the service and
 *  path are populated.
 */
template <bool S, bool P, bool Preserved,
          template <typename, typename> typename... Types>
class client :
    public context_ref,
    public Types<client<S, P, Preserved, Types...>,
                 sdbusplus::async::proxy_ns::proxy<S, P, false, Preserved>>...
{
  public:
    using Self = client<S, P, Preserved, Types...>;
    using Proxy = sdbusplus::async::proxy_ns::proxy<S, P, false, Preserved>;
    friend details::client_context_friend;

  private:
    Proxy proxy{};

  public:
    constexpr client() = delete;
    /* Delete default constructor if Service or Path have been provided. */
    explicit client(sdbusplus::async::context& ctx)
        requires(S || P)
    = delete;
    /* Default (empty) constructor only when Service and Path are missing. */
    explicit client(sdbusplus::async::context& ctx)
        requires(!S && !P)
        : context_ref(ctx), Types<Self, Proxy>(Proxy{})...
    {}

    /* Conversion constructor for a non-empty (Service and/or Path) proxy. */
    explicit client(sdbusplus::async::context& ctx, Proxy p)
        requires(S || P)
        : context_ref(ctx), Types<Self, Proxy>(p)..., proxy(p)
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

    /* Convert client into a Preserved Proxy. */
    auto preserve() const noexcept
        requires(!Preserved)
    {
        return client<S, P, true, Types...>(ctx, proxy.preserve());
    }
};

} // namespace client

/** A non-Preserved client alias.
 *
 *  This holds Service/Path in string-views, which must exist longer than
 *  the lifetime of this client_t.
 */
template <template <typename, typename> typename... Types>
using client_t = client::client<false, false, false, Types...>;
/** A Preserved client alias.
 *
 *  This holds Service/Path in strings, which thus have lifetimes that are
 *  the same as the client itself.
 */
template <template <typename, typename> typename... Types>
using client_preserved_t = client::client<false, false, true, Types...>;

namespace client::details
{
/* Indirect so that the generated Types can access the client_t's context.
 *
 * If P2893 gets into C++26 we could eliminate this because we can set all
 * the Types as friends directly.
 */
struct client_context_friend
{
    template <typename Client, typename Self>
    static sdbusplus::async::context& context(Self* self)
    {
        return static_cast<Client*>(self)->ctx;
    }
};
} // namespace client::details

} // namespace sdbusplus::async
