#include <sdbusplus/async/fdio.hpp>

namespace sdbusplus::async
{
fdio::fdio(context& ctx, int fd) : context_ref(ctx), fd(fd)
{
    static auto eventHandler =
        [](sd_event_source*, int, uint32_t, void* data) noexcept {
            static_cast<fdio*>(data)->handleEvent();
            return 0;
        };

    try
    {
        source = event_loop().add_io(fd, EPOLLIN, eventHandler, this);
    }
    catch (...)
    {
        throw std::runtime_error("Failed to add fd to event loop");
    }
}

fdio::~fdio() {}

void fdio::handleEvent() noexcept
{
    std::unique_lock l{lock};
    if (complete == nullptr)
    {
        l.unlock();
        return;
    }
    auto c = std::exchange(complete, nullptr);
    l.unlock();
    c->complete();
}

namespace fdio_ns
{
void fdio_completion::arm() noexcept
{
    // Set ourselves as the awaiting Receiver
    std::unique_lock l{fdioInstance.lock};

    if (std::exchange(fdioInstance.complete, this) != nullptr)
    {
        // We do not support two awaiters; throw exception. Since we are in
        // a noexcept context this will std::terminate anyhow, which is
        // approximately the same as 'assert' but with better information.
        try
        {
            throw std::logic_error(
                "fdio_completion started with another await already pending!");
        }
        catch (...)
        {
            std::terminate();
        }
    }

    l.unlock();
}

} // namespace fdio_ns

} // namespace sdbusplus::async
