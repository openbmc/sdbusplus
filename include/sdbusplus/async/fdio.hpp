#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/event.hpp>

namespace sdbusplus::async
{

namespace fdio_ns
{

/*
 * The file descriptor event
 * On start, registers the file descriptor with the event loop
 * On callback, completes the receiver
 */
template <execution::receiver R>
struct fdio_operation : public context_ref, details::context_friend
{
    fdio_operation() = delete;
    fdio_operation(fdio_operation&&) = delete;

    fdio_operation(context& ctx, int fd, R&& r) :
        context_ref(ctx), fd(fd), receiver(std::move(r))
    {}

    static int handler(sd_event_source*, int, uint32_t, void* data) noexcept
    {
        auto self = static_cast<fdio_operation<R>*>(data);
        execution::set_value(std::move(self->receiver));
        return -1;
    }

    friend auto tag_invoke(execution::start_t, fdio_operation& self) noexcept
    {
        try
        {
            self.source =
                self.event_loop().add_io(self.fd, EPOLLIN, handler, &self);
        }
        catch (...)
        {
            execution::set_error(std::move(self.receiver),
                                 std::current_exception());
        }
    }

  private:
    event_t& event_loop()
    {
        return get_event_loop(ctx);
    }

    int fd;
    event_source_t source;
    R receiver;
};

struct fdio_sender : public context_ref, details::context_friend
{
    using is_sender = void;
    fdio_sender() = delete;
    fdio_sender(context& ctx, int fd) noexcept : context_ref(ctx), fd(fd) {}

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const fdio_sender&, auto)
        -> execution::completion_signatures<
            execution::set_value_t(),
            execution::set_error_t(std::exception_ptr),
            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, fdio_sender&& self,
                           R&& r) -> fdio_operation<R>
    {
        return {self.ctx, self.fd, std::move(r)};
    }

  private:
    int fd;
};
} // namespace fdio_ns

auto async_fdio(context& ctx, int fd)
{
    return fdio_ns::fdio_sender{ctx, fd};
}

} // namespace sdbusplus::async
