#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/vtable.hpp>
namespace sdbusplus::asio::async
{

namespace server
{

namespace details
{
struct server_context_friend;
}

template <typename Instance, template <typename, typename> typename... Types>
class server : public Types<Instance, server<Instance, Types...>>...
{
  public:
    using Self = server<Instance, Types...>;
    friend details::server_context_friend;
    boost::asio::io_context& ctx;
    server() = delete;
    explicit server(bus_t& bus, boost::asio::io_context& ctx,
                    const char* path) :
        ctx(ctx), Types<Instance, Self>(bus, path)...
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
    template <typename Client, typename Self>
    static boost::asio::io_context& context(Self* self)
    {
        return static_cast<Client*>(self)->ctx;
    }
};

/* Determine if a type has a get_property call. */
template <typename Tag, typename Instance>
concept has_get_property_nomsg =
    requires(const Instance& i) { i.get_property(Tag{}); };

/* Determine if a type has a get property call that requires a msg. */
template <typename Tag, typename Instance>
concept has_get_property_msg =
    requires(const Instance& i, sdbusplus::message_t& m) {
        i.get_property(Tag{}, m);
    };

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
    requires(Instance& i, Arg&& a) {
        i.set_property(Tag{}, std::forward<Arg>(a));
    };

/* Determine if a type has a set property call that requires a msg. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property_msg =
    requires(Instance& i, sdbusplus::message_t& m, Arg&& a) {
        i.set_property(Tag{}, m, std::forward<Arg>(a));
    };

/* Determine if a type has any set_property call. */
template <typename Tag, typename Instance, typename Arg>
concept has_set_property = has_set_property_nomsg<Tag, Instance, Arg> ||
                           has_set_property_msg<Tag, Instance, Arg>;

/* Determine if a type has a method call. */
template <typename Tag, typename Instance, typename... Args>
concept has_method_nomsg = requires(Instance& i, Args&&... a) {
                               i.method_call(Tag{}, std::forward<Args>(a)...);
                           };

/* Determine if a type has a method call that requires a msg. */
template <typename Tag, typename Instance, typename... Args>
concept has_method_msg =
    requires(Instance& i, sdbusplus::message_t& m, Args&&... a) {
        i.method_call(Tag{}, m, std::forward<Args>(a)...);
    };

/* Determine if a type has any method call. */
template <typename Tag, typename Instance, typename... Args>
concept has_method = has_method_nomsg<Tag, Instance, Args...> ||
                     has_method_msg<Tag, Instance, Args...>;

} // namespace server::details

} // namespace sdbusplus::asio::async
