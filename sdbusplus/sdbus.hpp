#pragma once

#include <systemd/sd-bus.h>

// ABC for sdbus implementation.
namespace sdbusplus
{

// A wrapper for interfacing or testing sd-bus.  This will hold methods for
// buses, messages, etc.
class SdBusInterface
{
  public:
    virtual ~SdBusInterface() = default;

    // https://github.com/systemd/systemd/blob/master/src/systemd/sd-bus.h
    virtual sd_bus_message *sd_bus_message_ref(sd_bus_message *m) = 0;

    virtual int sd_bus_message_append_basic(sd_bus_message *message, char type,
                                            const void *value) = 0;

    virtual int sd_bus_message_at_end(sd_bus_message *m, int complete) = 0;

    virtual int sd_bus_message_close_container(sd_bus_message *m) = 0;

    virtual int sd_bus_message_enter_container(sd_bus_message *m, char type,
                                               const char *contents) = 0;

    virtual int sd_bus_message_exit_container(sd_bus_message *m) = 0;

    virtual sd_bus *sd_bus_message_get_bus(sd_bus_message *m) = 0;
    virtual int sd_bus_message_get_cookie(sd_bus_message *m,
                                          uint64_t *cookie) = 0;
    virtual const char *sd_bus_message_get_destination(sd_bus_message *m) = 0;
    virtual const char *sd_bus_message_get_interface(sd_bus_message *m) = 0;
    virtual const char *sd_bus_message_get_member(sd_bus_message *m) = 0;
    virtual const char *sd_bus_message_get_path(sd_bus_message *m) = 0;
    virtual const char *sd_bus_message_get_sender(sd_bus_message *m) = 0;
    virtual const char *sd_bus_message_get_signature(sd_bus_message *m,
                                                     int complete) = 0;

    virtual int sd_bus_message_is_method_call(sd_bus_message *m,
                                              const char *interface,
                                              const char *member) = 0;
    virtual int sd_bus_message_is_method_error(sd_bus_message *m,
                                               const char *name) = 0;
    virtual int sd_bus_message_is_signal(sd_bus_message *m,
                                         const char *interface,
                                         const char *member) = 0;

    virtual int sd_bus_message_new_method_return(sd_bus_message *call,
                                                 sd_bus_message **m) = 0;

    virtual int sd_bus_message_read_basic(sd_bus_message *m, char type,
                                          void *p) = 0;

    virtual int sd_bus_message_skip(sd_bus_message *m, const char *types) = 0;

    virtual int sd_bus_message_verify_type(sd_bus_message *m, char type,
                                           const char *contents) = 0;

    virtual int sd_bus_send(sd_bus *bus, sd_bus_message *m,
                            uint64_t *cookie) = 0;

    virtual int sd_bus_message_open_container(sd_bus_message *m, char type,
                                              const char *contents) = 0;
};

class SdBusImpl : public SdBusInterface
{
  public:
    SdBusImpl() = default;
    ~SdBusImpl() = default;
    SdBusImpl(const SdBusImpl &) = default;
    SdBusImpl &operator=(const SdBusImpl &) = default;
    SdBusImpl(SdBusImpl &&) = default;
    SdBusImpl &operator=(SdBusImpl &&) = default;

    sd_bus_message *sd_bus_message_ref(sd_bus_message *m) override
    {
        return ::sd_bus_message_ref(m);
    }

    int sd_bus_message_append_basic(sd_bus_message *message, char type,
                                    const void *value) override
    {
        return ::sd_bus_message_append_basic(message, type, value);
    }

    int sd_bus_message_at_end(sd_bus_message *m, int complete) override
    {
        return ::sd_bus_message_at_end(m, complete);
    }

    int sd_bus_message_close_container(sd_bus_message *m) override
    {
        return ::sd_bus_message_close_container(m);
    }

    int sd_bus_message_enter_container(sd_bus_message *m, char type,
                                       const char *contents) override
    {
        return ::sd_bus_message_enter_container(m, type, contents);
    }

    int sd_bus_message_exit_container(sd_bus_message *m) override
    {
        return ::sd_bus_message_exit_container(m);
    }

    sd_bus *sd_bus_message_get_bus(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_bus(m);
    }

    int sd_bus_message_get_cookie(sd_bus_message *m, uint64_t *cookie) override
    {
        return ::sd_bus_message_get_cookie(m, cookie);
    }

    const char *sd_bus_message_get_destination(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_destination(m);
    }

    const char *sd_bus_message_get_interface(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_interface(m);
    }

    const char *sd_bus_message_get_member(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_member(m);
    }

    const char *sd_bus_message_get_path(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_path(m);
    }

    const char *sd_bus_message_get_sender(sd_bus_message *m) override
    {
        return ::sd_bus_message_get_sender(m);
    }

    const char *sd_bus_message_get_signature(sd_bus_message *m,
                                             int complete) override
    {
        return ::sd_bus_message_get_signature(m, complete);
    }

    int sd_bus_message_is_method_call(sd_bus_message *m, const char *interface,
                                      const char *member) override
    {
        return ::sd_bus_message_is_method_call(m, interface, member);
    }

    int sd_bus_message_is_method_error(sd_bus_message *m,
                                       const char *name) override
    {
        return ::sd_bus_message_is_method_error(m, name);
    }

    int sd_bus_message_is_signal(sd_bus_message *m, const char *interface,
                                 const char *member) override
    {
        return ::sd_bus_message_is_signal(m, interface, member);
    }

    int sd_bus_message_new_method_return(sd_bus_message *call,
                                         sd_bus_message **m) override
    {
        return ::sd_bus_message_new_method_return(call, m);
    }

    int sd_bus_message_read_basic(sd_bus_message *m, char type,
                                  void *p) override
    {
        return ::sd_bus_message_read_basic(m, type, p);
    }

    int sd_bus_message_skip(sd_bus_message *m, const char *types) override
    {
        return ::sd_bus_message_skip(m, types);
    }

    int sd_bus_message_verify_type(sd_bus_message *m, char type,
                                   const char *contents) override
    {
        return ::sd_bus_message_verify_type(m, type, contents);
    }

    int sd_bus_send(sd_bus *bus, sd_bus_message *m, uint64_t *cookie) override
    {
        return ::sd_bus_send(bus, m, cookie);
    }

    int sd_bus_message_open_container(sd_bus_message *m, char type,
                                      const char *contents) override
    {
        return ::sd_bus_message_open_container(m, type, contents);
    }
};

} // namespace sdbusplus
