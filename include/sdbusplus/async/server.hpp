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

/* Determine if a type has a get_property call. */
template <typename Tag, typename Instance>
concept has_get_property_nomsg =
    requires(const Instance& i) { i.get_property(Tag{}); };

/* Determine if a type has a get property call that requries a msg. */
template <typename Tag, typename Instance>
concept has_get_property_msg = requires(
    const Instance& i, sdbusplus::message_t& m) { i.get_property(Tag{}, m); };

/* Determine if a type has any get_property call. */
template <typename Tag, typename Instance>
concept has_get_property = has_get_property_nomsg<Tag, Instance> ||
                           has_get_property_msg<Tag, Instance>;

/* Determine if a type is missing the 'const' on get-property calls. */
template <typename Tag, typename Instance>
concept has_get_property_missing_const =
    !has_get_property<Tag, Instance> &&
    (
        requires(Instance& i) { i.get_property(Tag{}); } ||
        requires(Instance& i, sdbusplus::message_t& m) {
            i.get_property(Tag{}, m);
        });

/* Determine if a type has a set_property call. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property_nomsg =
    requires(const Instance& i, Arg&& a) {
        i.set_property(Tag{}, std::forward<Arg>(a));
    };

/* Determine if a type has a set property call that requries a msg. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property_msg =
    requires(const Instance& i, sdbusplus::message_t& m, Arg&& a) {
        i.set_property(Tag{}, m, std::forward<Arg>(a));
    };

/* Determine if a type has any set_property call. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property = has_set_property_nomsg<Tag, Instance, Arg> ||
                           has_set_property_msg<Tag, Instance, Arg>;

} // namespace server::details

} // namespace sdbusplus::async
