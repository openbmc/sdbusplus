#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/vtable.hpp>

namespace sdbusplus::async
{

namespace server
{

namespace details
{
struct server_context_friend;
}

template <typename Instance, template <typename, typename> typename... Types>
class server :
    public sdbusplus::async::context_ref,
    public Types<Instance, server<Instance, Types...>>...
{
  public:
    using Self = server<Instance, Types...>;
    friend details::server_context_friend;

    server() = delete;
    explicit server(sdbusplus::async::context& ctx, const char* path) :
        context_ref(ctx), Types<Instance, Self>(path)...
    {}
};

} // namespace server

template <typename Instance, template <typename, typename> typename... Types>
using server_t = server::server<Instance, Types...>;

namespace server::details
{
/* Indirect so that the generated Types can access the server_t's context.
 *
 * If P2893 gets into C++26 we could eliminate this because we can set all
 * the Types as friends directly.
 */
struct server_context_friend
{
    template <typename T>
    sdbusplus::async::context& context()
    {
        return static_cast<T*>(this)->ctx;
    }
};
} // namespace server::details

} // namespace sdbusplus::async
