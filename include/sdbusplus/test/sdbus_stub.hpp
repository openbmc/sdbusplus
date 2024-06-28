#pragma once

#include <boost/callable_traits.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <any>
#include <deque>
#include <tuple>
#include <type_traits>

class asioDbusStub
{
  public:
    asioDbusStub() = delete;
    asioDbusStub(boost::asio::io_context& ioCtx) : io(ioCtx), responses() {}
    ~asioDbusStub() = default;

    template <typename... T>
    void addResponse(T&&... r)
    {
        responses.emplace_back(std::make_tuple(std::forward<T>(r)...));
    }

    template <typename CompletionToken>
    inline auto async_send(CompletionToken&& token, uint64_t /* timeout */)
    {
#ifdef SDBUSPLUS_DISABLE_BOOST_COROUTINES
        constexpr bool is_yield = false;
#else
        constexpr bool is_yield =
            std::is_same_v<CompletionToken, boost::asio::yield_context>;
#endif
        boost::system::error_code ec{};
        if (responses.empty())
        {
            throw std::runtime_error("Insufficient messages in queue");
        }

        std::any r(std::move(responses.front()));
        responses.pop_front();

        if constexpr (is_yield)
        {
            return r;
        }
        else
        {
            boost::asio::post(io, [a{std::move(r)}, ec,
                                   token{std::forward<CompletionToken>(
                                       token)}]() mutable { token(ec, a); });
            return;
        }
    }

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call_timed(MessageHandler&& handler, const std::string&,
                                 const std::string&, const std::string&,
                                 const std::string&, uint64_t timeout,
                                 const InputArgs&...)
    {
        using FunctionTuple = boost::callable_traits::args_t<MessageHandler>;
        using FunctionTupleType =
            sdbusplus::utility::decay_tuple_t<FunctionTuple>;
        using UnpackType =
            sdbusplus::utility::strip_first_n_args_t<1, FunctionTupleType>;
        auto applyHandler = [handler = std::forward<MessageHandler>(handler)](
                                boost::system::error_code ec,
                                std::any& r) mutable {
            UnpackType responseData;
            if (!ec)
            {
                try
                {
                    responseData = std::any_cast<UnpackType>(r);
                }
                catch (const std::exception& e)
                {
                    // Set error code if not already set
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                }
            }
            // Note.  Callback is called whether or not the unpack was
            // successful to allow the user to implement their own handling
            auto response = std::tuple_cat(std::make_tuple(ec),
                                           std::move(responseData));
            std::apply(handler, response);
        };
        async_send(std::forward<decltype(applyHandler)>(applyHandler), timeout);
    }

    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&& handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        async_method_call_timed(std::forward<MessageHandler>(handler), service,
                                objpath, interf, method, 0, a...);
    }

  protected:
    boost::asio::io_context& io;
    std::deque<std::any> responses;
};

/* To use, the code to be tested should only access
 * the bus via the getSystemBus() returned value
 *
 * make a file like the following in the projects that use this
 */

/*

#ifdef UNIT_TESTING

#include <sdbusplus/test/sdbus_stub.hpp>

extern asioDbusStub* dbusStub;

static inline auto getSystemBus()
{
    return dbusStub;
}

#else  // ! UNIT_TESTING

static inline auto getSystemBus()
{
    return crow::connections::systemBus;
}

#endif // UNIT_TESTING

*/
