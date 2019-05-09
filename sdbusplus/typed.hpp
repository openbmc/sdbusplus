#pragma once
#include <any>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/types.hpp>
#include <utility>
#include <variant>

namespace sdbusplus
{
namespace typed
{

template <typename... Ts>
struct Types
{
};

namespace detail
{

template <typename Req, typename Rm>
constexpr bool validRm(Types<Req>, Types<Rm> rm)
{
    return message::types::type_id<Req>() == message::types::type_id<Rm>();
}

template <typename... ReqSet, typename Possible>
constexpr bool hasType(Types<ReqSet...>, Types<Possible>)
{
    return (... || validRm(Types<ReqSet>(), Types<Possible>()));
}

template <typename... ReqSet, typename... PossibleSet>
constexpr bool validRm(Types<std::variant<ReqSet...>>,
                       Types<std::variant<PossibleSet...>>)
{
    return (... && hasType(Types<ReqSet...>(), Types<PossibleSet>()));
}

template <typename... PossibleSet>
constexpr bool validRm(Types<std::any>, Types<std::variant<PossibleSet...>>)
{
    return true;
}

template <typename... Expects>
constexpr auto nextAppend(Types<Expects...> reqs, Types<>)
{
    return reqs;
}

template <typename Expect, typename... Expects, typename Append,
          typename... Appends>
constexpr auto nextAppend(Types<Expect, Expects...>, Types<Append, Appends...>)
{
    static_assert(validRm(Types<Expect>(), Types<Append>()));
    return nextAppend(Types<Expects...>(), Types<Appends...>());
}

template <typename... Expects>
constexpr auto nextRead(Types<Expects...> reqs, Types<>)
{
    return reqs;
}

template <typename Expect, typename... Expects, typename Read,
          typename... Reads>
constexpr auto nextRead(Types<Expect, Expects...>, Types<Read, Reads...>)
{
    static_assert(validRm(Types<Read>(), Types<Expect>()));
    return nextRead(Types<Expects...>(), Types<Reads...>());
}

template <typename Expects, typename Appends>
struct NextAppend
{
    using type = decltype(nextAppend(Expects(), Appends()));
};

template <typename Expects, typename Reads>
struct NextRead
{
    using type = decltype(nextRead(Expects(), Reads()));
};

} // namespace detail

class Sender
{
  public:
    Sender(message::message&& m) : m(std::move(m))
    {
    }
    Sender(const Sender&) = delete;
    Sender(Sender&&) = default;
    Sender& operator=(const Sender&) = delete;
    Sender& operator=(Sender&&) = default;
    virtual ~Sender() = default;

    virtual void send() &&
    {
        m.method_return();
        message::message(std::move(m)); // Drop our message reference
    }

  protected:
    message::message m;
};

class MethodError : public Sender
{
  public:
    MethodError(message::message&& m) : Sender(std::move(m))
    {
    }
};

template <typename Reqs, typename Rets>
class MethodCall
{
  public:
    MethodCall(message::message&& m) : m(std::move(m))
    {
    }
    MethodCall(const MethodCall&) = delete;
    MethodCall(MethodCall&&) = default;
    MethodCall& operator=(const MethodCall&) = delete;
    MethodCall& operator=(MethodCall&&) = default;
    virtual ~MethodCall() = default;

    template <typename... Appends>
    auto append(Appends&&... appends) &&
    {
        m.append(std::forward<Appends>(appends)...);
        using NextReqs =
            typename detail::NextAppend<Reqs, Types<Appends...>>::type;
        return MethodCall<NextReqs, Rets>(std::move(m));
    }

  private:
    message::message m;
};

template <typename Rets>
class MethodCall<Types<>, Rets>
{
  public:
    MethodCall(message::message&& m) : m(std::move(m))
    {
    }
    MethodCall(const MethodCall&) = delete;
    MethodCall(MethodCall&&) = default;
    MethodCall& operator=(const MethodCall&) = delete;
    MethodCall& operator=(MethodCall&&) = default;
    virtual ~MethodCall() = default;

    template <typename... Args>
    auto call(Args&&... args) &&
    {
        auto reply = m.get_bus().call(m, std::forward<Args>(args)...);
        message::message(std::move(m)); // Drop our message reference
        return MethodCall<void, Rets>(std::move(reply));
    }

  private:
    message::message m;
};

template <typename Rets>
class MethodCall<void, Rets>
{
  public:
    MethodCall(message::message&& m) : m(std::move(m))
    {
    }
    MethodCall(const MethodCall&) = delete;
    MethodCall(MethodCall&&) = default;
    MethodCall& operator=(const MethodCall&) = delete;
    MethodCall& operator=(MethodCall&&) = default;
    virtual ~MethodCall() = default;

    template <typename... Reads>
    auto read(Reads&&... reads) &&
    {
        m.read(std::forward<Reads>(reads)...);
        using NextRets = typename detail::NextRead<Rets, Types<Reads...>>::type;
        return MethodCall<void, NextRets>(std::move(m));
    }

  private:
    message::message m;
};

template <>
class MethodCall<void, Types<>>
{
  public:
    MethodCall(message::message&& m) : m(std::move(m))
    {
    }
    MethodCall(const MethodCall&) = delete;
    MethodCall(MethodCall&&) = delete;
    MethodCall& operator=(const MethodCall&) = delete;
    MethodCall& operator=(MethodCall&&) = delete;
    virtual ~MethodCall() = default;

  private:
    message::message m;
};

template <typename Rets>
class MethodReturn
{
  public:
    MethodReturn(message::message&& m) : m(std::move(m))
    {
    }
    MethodReturn(const MethodReturn&) = delete;
    MethodReturn(MethodReturn&&) = default;
    MethodReturn& operator=(const MethodReturn&) = delete;
    MethodReturn& operator=(MethodReturn&&) = default;
    virtual ~MethodReturn() = default;

    template <typename... Appends>
    auto append(Appends&&... appends) &&
    {
        m.append(std::forward<Appends>(appends)...);
        using NextRets =
            typename detail::NextAppend<Rets, Types<Appends...>>::type;
        return MethodReturn<NextRets>(std::move(m));
    }

  private:
    message::message m;
};

template <>
class MethodReturn<Types<>> : public Sender
{
  public:
    MethodReturn(message::message&& m) : Sender(std::move(m))
    {
    }
};

template <typename Rets>
class MethodReply
{
  public:
    MethodReply(const message::message& req) : req(req)
    {
    }
    MethodReply(const MethodReply&) = delete;
    MethodReply(MethodReply&&) = default;
    MethodReply& operator=(const MethodReply&) = delete;
    MethodReply& operator=(MethodReply&&) = default;
    virtual ~MethodReply() = default;

    auto startReturn() &&
    {
        return MethodReturn<Rets>(req.new_method_return());
    }
    auto startError(int error) &&
    {
        return MethodError(req.new_method_errno(error));
    }

  private:
    message::message req;
};

} // namespace typed

} // namespace sdbusplus
