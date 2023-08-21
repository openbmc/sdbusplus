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

    auto get_property(owner_t) const
    {
        return "asdf";
    }

    auto get_property(last_result_t) const
    {
        return 42;
    }

  private:
    auto startup() -> sdbusplus::async::task<>
    {
        ctx.get_bus().request_name("net.poettering.Calculator");

        while (1)
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
