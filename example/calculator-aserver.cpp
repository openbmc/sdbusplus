#include <net/poettering/Calculator/aserver.hpp>
#include <sdbusplus/async.hpp>

#include <iostream>

class Calculator :
    public sdbusplus::aserver::net::poettering::Calculator<Calculator>
{
  public:
    explicit Calculator(sdbusplus::async::context& ctx, auto path) :
        sdbusplus::aserver::net::poettering::Calculator<Calculator>(ctx, path)
    {}

    auto method_call(multiply_t, auto x, auto y)
    {
        auto r = x * y;
        last_result(r);
        return r;
    }

    auto method_call(divide_t, auto x, auto y)
        -> sdbusplus::async::task<divide_t::return_type>
    {
        using sdbusplus::error::net::poettering::Calculator::DivisionByZero;
        if (y == 0)
        {
            status(State::Error);
            throw DivisionByZero();
        }

        auto r = x / y;
        last_result(r);
        co_return r;
    }

    auto method_call(clear_t) -> sdbusplus::async::task<>
    {
        auto v = last_result();
        last_result(0);
        cleared(v);
        co_return;
    }

    auto get_property(owner_t) const
    {
        std::cout << " get_property on owner\n";
        return owner_;
    }

    bool set_property(owner_t, auto owner)
    {
        std::cout << " set_property on owner\n";
        std::swap(owner_, owner);
        return owner_ == owner;
    }
};

int main()
{
    constexpr auto path = Calculator::instance_path;

    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t manager{ctx, path};

    Calculator c{ctx, path};

    ctx.spawn([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        ctx.request_name(Calculator::default_service);
        co_return;
    }(ctx));

    ctx.run();

    return 0;
}
