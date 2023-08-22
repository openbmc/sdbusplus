#include <net/poettering/Calculator/aserver.hpp>
#include <sdbusplus/async.hpp>

class Calculator :
    public sdbusplus::aserver::net::poettering::Calculator<Calculator>
{
  public:
    explicit Calculator(sdbusplus::async::context& ctx) :
        sdbusplus::aserver::net::poettering::Calculator<Calculator>(
            ctx, "/net/poettering/calculator"),
        manager(ctx, "/")
    {
        ctx.spawn(startup());
    }

    auto get_property(last_result_t) const
    {
        return _last_result;
    }

    bool set_property(last_result_t, int64_t v)
    {
        if (v % 2 == 0)
        {
            std::swap(_last_result, v);
            return v != _last_result;
        }
        return false;
    }

    auto method_call(multiply_t, auto x, auto y)
    {
        auto r = x * y;
        last_result(r);
        return r;
    }

    auto method_call(divide_t, auto x, auto y)
        -> sdbusplus::async::task<divide_t::return_type>
    {
        auto r = x / y;
        last_result(r);
        co_return r;
    }

    auto method_call(clear_t) -> sdbusplus::async::task<>
    {
        last_result(0);
        co_return;
    }

  private:
    auto startup() -> sdbusplus::async::task<>
    {
        last_result(123);
        ctx.get_bus().request_name("net.poettering.Calculator");

        status(State::Error);

        while (!ctx.stop_requested())
        {
            using namespace std::literals;
            co_await sdbusplus::async::sleep_for(ctx, 10s);

            cleared(42);
        }
        co_return;
    }

    sdbusplus::server::manager_t manager;
};

int main()
{
    sdbusplus::async::context ctx;
    [[maybe_unused]] Calculator c(ctx);

    ctx.run();

    return 0;
}
