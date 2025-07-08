#pragma once

#include <systemd/sd-bus.h>
#include <systemd/sd-daemon.h>

#include <chrono>

// ABC for sdbus implementation.
namespace sdbusplus
{

// Defined by systemd taking uint64_t usec params
using SdBusDuration =
    std::chrono::duration<uint64_t, std::chrono::microseconds::period>;

// A wrapper for interfacing or testing sd-bus.  This will hold methods for
// buses, messages, etc.
class SdBusInterface
{
  public:
    virtual ~SdBusInterface() = default;

    virtual int sd_bus_add_object_manager(sd_bus* bus, sd_bus_slot** slot,
                                          const char* path) = 0;

    virtual int sd_bus_add_object_vtable(
        sd_bus* bus, sd_bus_slot** slot, const char* path,
        const char* interface, const sd_bus_vtable* vtable, void* userdata) = 0;

    virtual int sd_bus_add_match(
        sd_bus* bus, sd_bus_slot** slot, const char* path,
        sd_bus_message_handler_t callback, void* userdata) = 0;

    virtual int sd_bus_attach_event(sd_bus* bus, sd_event* e, int priority) = 0;

    virtual int sd_bus_call(sd_bus* bus, sd_bus_message* m, uint64_t usec,
                            sd_bus_error* ret_error,
                            sd_bus_message** reply) = 0;

    virtual int sd_bus_call_async(
        sd_bus* bus, sd_bus_slot** slot, sd_bus_message* m,
        sd_bus_message_handler_t callback, void* userdata, uint64_t usec) = 0;

    virtual int sd_bus_detach_event(sd_bus* bus) = 0;

    virtual int sd_bus_emit_interfaces_added_strv(sd_bus* bus, const char* path,
                                                  char** interfaces) = 0;
    virtual int sd_bus_emit_interfaces_removed_strv(
        sd_bus* bus, const char* path, char** interfaces) = 0;
    virtual int sd_bus_emit_object_added(sd_bus* bus, const char* path) = 0;
    virtual int sd_bus_emit_object_removed(sd_bus* bus, const char* path) = 0;
    virtual int sd_bus_emit_properties_changed_strv(
        sd_bus* bus, const char* path, const char* interface,
        const char** names) = 0;

    virtual int sd_bus_error_set(sd_bus_error* e, const char* name,
                                 const char* message) = 0;
    virtual int sd_bus_error_set_const(sd_bus_error* e, const char* name,
                                       const char* message) = 0;
    virtual int sd_bus_error_get_errno(const sd_bus_error* e) = 0;
    virtual int sd_bus_error_set_errno(sd_bus_error* e, int error) = 0;
    virtual int sd_bus_error_is_set(const sd_bus_error* e) = 0;
    virtual void sd_bus_error_free(sd_bus_error* e) = 0;

    virtual sd_event* sd_bus_get_event(sd_bus* bus) = 0;
    virtual int sd_bus_get_fd(sd_bus* bus) = 0;
    virtual int sd_bus_get_events(sd_bus* bus) = 0;
    virtual int sd_bus_get_timeout(sd_bus* bus, uint64_t* timeout_usec) = 0;
    virtual int sd_bus_get_unique_name(sd_bus* bus, const char** unique) = 0;

    virtual int sd_bus_list_names(sd_bus* bus, char*** acquired,
                                  char*** activatable) = 0;

    // https://github.com/systemd/systemd/blob/master/src/systemd/sd-bus.h
    virtual int sd_bus_message_append_basic(sd_bus_message* message, char type,
                                            const void* value) = 0;

    virtual int sd_bus_message_append_string_iovec(
        sd_bus_message* message, const struct iovec* iov, int iovcnt) = 0;

    virtual int sd_bus_message_at_end(sd_bus_message* m, int complete) = 0;

    virtual int sd_bus_message_close_container(sd_bus_message* m) = 0;

    virtual int sd_bus_message_enter_container(sd_bus_message* m, char type,
                                               const char* contents) = 0;

    virtual int sd_bus_message_exit_container(sd_bus_message* m) = 0;

    virtual sd_bus* sd_bus_message_get_bus(sd_bus_message* m) = 0;
    virtual int sd_bus_message_get_type(sd_bus_message* m, uint8_t* type) = 0;
    virtual int sd_bus_message_get_cookie(sd_bus_message* m,
                                          uint64_t* cookie) = 0;
    virtual int sd_bus_message_get_reply_cookie(sd_bus_message* m,
                                                uint64_t* cookie) = 0;
    virtual const char* sd_bus_message_get_destination(sd_bus_message* m) = 0;
    virtual const char* sd_bus_message_get_interface(sd_bus_message* m) = 0;
    virtual const char* sd_bus_message_get_member(sd_bus_message* m) = 0;
    virtual const char* sd_bus_message_get_path(sd_bus_message* m) = 0;
    virtual const char* sd_bus_message_get_sender(sd_bus_message* m) = 0;
    virtual const char* sd_bus_message_get_signature(sd_bus_message* m,
                                                     int complete) = 0;
    virtual int sd_bus_message_get_errno(sd_bus_message* m) = 0;
    virtual const sd_bus_error* sd_bus_message_get_error(sd_bus_message* m) = 0;

    virtual int sd_bus_message_is_method_call(
        sd_bus_message* m, const char* interface, const char* member) = 0;
    virtual int sd_bus_message_is_method_error(sd_bus_message* m,
                                               const char* name) = 0;
    virtual int sd_bus_message_is_signal(
        sd_bus_message* m, const char* interface, const char* member) = 0;

    virtual int sd_bus_message_new_method_call(
        sd_bus* bus, sd_bus_message** m, const char* destination,
        const char* path, const char* interface, const char* member) = 0;

    virtual int sd_bus_message_new_method_return(sd_bus_message* call,
                                                 sd_bus_message** m) = 0;

    virtual int sd_bus_message_new_method_error(
        sd_bus_message* call, sd_bus_message** m, sd_bus_error* e) = 0;

    virtual int sd_bus_message_new_method_errno(sd_bus_message* call,
                                                sd_bus_message** m, int error,
                                                const sd_bus_error* p) = 0;

    virtual int sd_bus_message_new_signal(
        sd_bus* bus, sd_bus_message** m, const char* path,
        const char* interface, const char* member) = 0;

    virtual int sd_bus_message_open_container(sd_bus_message* m, char type,
                                              const char* contents) = 0;

    virtual int sd_bus_message_read_basic(sd_bus_message* m, char type,
                                          void* p) = 0;

    virtual sd_bus_message* sd_bus_message_ref(sd_bus_message* m) = 0;

    virtual int sd_bus_message_skip(sd_bus_message* m, const char* types) = 0;

    virtual int sd_bus_message_verify_type(sd_bus_message* m, char type,
                                           const char* contents) = 0;

    virtual int sd_bus_slot_set_destroy_callback(sd_bus_slot* slot,
                                                 sd_bus_destroy_t callback) = 0;

    virtual void* sd_bus_slot_set_userdata(sd_bus_slot* slot,
                                           void* userdata) = 0;

    virtual int sd_bus_process(sd_bus* bus, sd_bus_message** r) = 0;

    virtual sd_bus* sd_bus_ref(sd_bus* bus) = 0;

    virtual int sd_bus_request_name(sd_bus* bus, const char* name,
                                    uint64_t flags) = 0;

    virtual int sd_bus_send(sd_bus* bus, sd_bus_message* m,
                            uint64_t* cookie) = 0;

    virtual sd_bus* sd_bus_unref(sd_bus* bus) = 0;
    virtual sd_bus_slot* sd_bus_slot_unref(sd_bus_slot* slot) = 0;
    virtual sd_bus* sd_bus_flush_close_unref(sd_bus* bus) = 0;

    virtual int sd_bus_flush(sd_bus* bus) = 0;
    virtual void sd_bus_close(sd_bus* bus) = 0;
    virtual int sd_bus_is_open(sd_bus* bus) = 0;

    virtual int sd_bus_wait(sd_bus* bus, uint64_t timeout_usec) = 0;

    virtual int sd_notify(int unset_environment, const char* state) = 0;

    virtual int sd_watchdog_enabled(int unset_environment, uint64_t* usec) = 0;

    virtual int sd_bus_message_append_array(sd_bus_message* m, char type,
                                            const void* ptr, size_t size) = 0;
    virtual int sd_bus_message_read_array(sd_bus_message* m, char type,
                                          const void** ptr, size_t* size) = 0;
};

class SdBusImpl : public SdBusInterface
{
  public:
    SdBusImpl() = default;
    ~SdBusImpl() override = default;
    SdBusImpl(const SdBusImpl&) = default;
    SdBusImpl& operator=(const SdBusImpl&) = default;
    SdBusImpl(SdBusImpl&&) = default;
    SdBusImpl& operator=(SdBusImpl&&) = default;

    int sd_bus_add_object_manager(sd_bus* bus, sd_bus_slot** slot,
                                  const char* path) override
    {
        return ::sd_bus_add_object_manager(bus, slot, path);
    }

    int sd_bus_add_object_vtable(sd_bus* bus, sd_bus_slot** slot,
                                 const char* path, const char* interface,
                                 const sd_bus_vtable* vtable,
                                 void* userdata) override
    {
        return ::sd_bus_add_object_vtable(bus, slot, path, interface, vtable,
                                          userdata);
    }

    int sd_bus_add_match(sd_bus* bus, sd_bus_slot** slot, const char* path,
                         sd_bus_message_handler_t callback,
                         void* userdata) override
    {
        return ::sd_bus_add_match(bus, slot, path, callback, userdata);
    }

    int sd_bus_attach_event(sd_bus* bus, sd_event* e, int priority) override
    {
        return ::sd_bus_attach_event(bus, e, priority);
    }

    int sd_bus_call(sd_bus* bus, sd_bus_message* m, uint64_t usec,
                    sd_bus_error* ret_error, sd_bus_message** reply) override
    {
        return ::sd_bus_call(bus, m, usec, ret_error, reply);
    }

    int sd_bus_call_async(sd_bus* bus, sd_bus_slot** slot, sd_bus_message* m,
                          sd_bus_message_handler_t callback, void* userdata,
                          uint64_t usec) override
    {
        return ::sd_bus_call_async(bus, slot, m, callback, userdata, usec);
    }

    int sd_bus_detach_event(sd_bus* bus) override
    {
        return ::sd_bus_detach_event(bus);
    }

    int sd_bus_emit_interfaces_added_strv(sd_bus* bus, const char* path,
                                          char** interfaces) override
    {
        return ::sd_bus_emit_interfaces_added_strv(bus, path, interfaces);
    }

    int sd_bus_emit_interfaces_removed_strv(sd_bus* bus, const char* path,
                                            char** interfaces) override
    {
        return ::sd_bus_emit_interfaces_removed_strv(bus, path, interfaces);
    }

    int sd_bus_emit_object_added(sd_bus* bus, const char* path) override
    {
        return ::sd_bus_emit_object_added(bus, path);
    }

    int sd_bus_emit_object_removed(sd_bus* bus, const char* path) override
    {
        return ::sd_bus_emit_object_removed(bus, path);
    }

    int sd_bus_emit_properties_changed_strv(sd_bus* bus, const char* path,
                                            const char* interface,
                                            const char** names) override
    {
        return ::sd_bus_emit_properties_changed_strv(bus, path, interface,
                                                     const_cast<char**>(names));
        // The const_cast above may seem unsafe, but it safe.  sd_bus's manpage
        // shows a 'const char*' but the header does not.  I examined the code
        // and no modification of the strings is done.  I tried to change sdbus
        // directly but due to quirks of C you cannot implicitly convert a
        // 'char **' to a 'const char**', so changing the implementation causes
        // lots of compile failures due to an incompatible API change.
        //
        // Performing a const_cast allows us to avoid a memory allocation of
        // the contained strings in 'interface::property_changed'.
    }

    int sd_bus_error_set(sd_bus_error* e, const char* name,
                         const char* message) override
    {
        return ::sd_bus_error_set(e, name, message);
    }

    int sd_bus_error_set_const(sd_bus_error* e, const char* name,
                               const char* message) override
    {
        return ::sd_bus_error_set_const(e, name, message);
    }

    int sd_bus_error_get_errno(const sd_bus_error* e) override
    {
        return ::sd_bus_error_get_errno(e);
    }

    int sd_bus_error_set_errno(sd_bus_error* e, int error) override
    {
        return ::sd_bus_error_set_errno(e, error);
    }

    int sd_bus_error_is_set(const sd_bus_error* e) override
    {
        return ::sd_bus_error_is_set(e);
    }

    void sd_bus_error_free(sd_bus_error* e) override
    {
        return ::sd_bus_error_free(e);
    }

    sd_event* sd_bus_get_event(sd_bus* bus) override
    {
        return ::sd_bus_get_event(bus);
    }

    int sd_bus_get_fd(sd_bus* bus) override
    {
        return ::sd_bus_get_fd(bus);
    }

    int sd_bus_get_events(sd_bus* bus) override
    {
        return ::sd_bus_get_events(bus);
    }

    int sd_bus_get_timeout(sd_bus* bus, uint64_t* timeout_usec) override
    {
        return ::sd_bus_get_timeout(bus, timeout_usec);
    }

    int sd_bus_get_unique_name(sd_bus* bus, const char** unique) override
    {
        return ::sd_bus_get_unique_name(bus, unique);
    }

    int sd_bus_list_names(sd_bus* bus, char*** acquired,
                          char*** activatable) override
    {
        return ::sd_bus_list_names(bus, acquired, activatable);
    }

    int sd_bus_message_append_basic(sd_bus_message* message, char type,
                                    const void* value) override
    {
        return ::sd_bus_message_append_basic(message, type, value);
    }

    int sd_bus_message_append_string_iovec(
        sd_bus_message* message, const struct iovec* iov, int iovcnt) override
    {
        return ::sd_bus_message_append_string_iovec(message, iov, iovcnt);
    }

    int sd_bus_message_at_end(sd_bus_message* m, int complete) override
    {
        return ::sd_bus_message_at_end(m, complete);
    }

    int sd_bus_message_close_container(sd_bus_message* m) override
    {
        return ::sd_bus_message_close_container(m);
    }

    int sd_bus_message_enter_container(sd_bus_message* m, char type,
                                       const char* contents) override
    {
        return ::sd_bus_message_enter_container(m, type, contents);
    }

    int sd_bus_message_exit_container(sd_bus_message* m) override
    {
        return ::sd_bus_message_exit_container(m);
    }

    sd_bus* sd_bus_message_get_bus(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_bus(m);
    }

    int sd_bus_message_get_type(sd_bus_message* m, uint8_t* type) override
    {
        return ::sd_bus_message_get_type(m, type);
    }

    int sd_bus_message_get_cookie(sd_bus_message* m, uint64_t* cookie) override
    {
        return ::sd_bus_message_get_cookie(m, cookie);
    }

    int sd_bus_message_get_reply_cookie(sd_bus_message* m,
                                        uint64_t* cookie) override
    {
        return ::sd_bus_message_get_reply_cookie(m, cookie);
    }

    const char* sd_bus_message_get_destination(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_destination(m);
    }

    const char* sd_bus_message_get_interface(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_interface(m);
    }

    const char* sd_bus_message_get_member(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_member(m);
    }

    const char* sd_bus_message_get_path(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_path(m);
    }

    const char* sd_bus_message_get_sender(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_sender(m);
    }

    const char* sd_bus_message_get_signature(sd_bus_message* m,
                                             int complete) override
    {
        return ::sd_bus_message_get_signature(m, complete);
    }

    int sd_bus_message_get_errno(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_errno(m);
    }

    const sd_bus_error* sd_bus_message_get_error(sd_bus_message* m) override
    {
        return ::sd_bus_message_get_error(m);
    }

    int sd_bus_message_is_method_call(sd_bus_message* m, const char* interface,
                                      const char* member) override
    {
        return ::sd_bus_message_is_method_call(m, interface, member);
    }

    int sd_bus_message_is_method_error(sd_bus_message* m,
                                       const char* name) override
    {
        return ::sd_bus_message_is_method_error(m, name);
    }

    int sd_bus_message_is_signal(sd_bus_message* m, const char* interface,
                                 const char* member) override
    {
        return ::sd_bus_message_is_signal(m, interface, member);
    }

    int sd_bus_message_new_method_call(
        sd_bus* bus, sd_bus_message** m, const char* destination,
        const char* path, const char* interface, const char* member) override
    {
        return ::sd_bus_message_new_method_call(bus, m, destination, path,
                                                interface, member);
    }

    int sd_bus_message_new_method_return(sd_bus_message* call,
                                         sd_bus_message** m) override
    {
        return ::sd_bus_message_new_method_return(call, m);
    }

    int sd_bus_message_new_method_error(
        sd_bus_message* call, sd_bus_message** m, sd_bus_error* e) override
    {
        return ::sd_bus_message_new_method_error(call, m, e);
    }

    int sd_bus_message_new_method_errno(sd_bus_message* call,
                                        sd_bus_message** m, int error,
                                        const sd_bus_error* p) override
    {
        return ::sd_bus_message_new_method_errno(call, m, error, p);
    }

    int sd_bus_message_new_signal(sd_bus* bus, sd_bus_message** m,
                                  const char* path, const char* interface,
                                  const char* member) override
    {
        return ::sd_bus_message_new_signal(bus, m, path, interface, member);
    }

    int sd_bus_message_open_container(sd_bus_message* m, char type,
                                      const char* contents) override
    {
        return ::sd_bus_message_open_container(m, type, contents);
    }

    int sd_bus_message_read_basic(sd_bus_message* m, char type,
                                  void* p) override
    {
        return ::sd_bus_message_read_basic(m, type, p);
    }

    sd_bus_message* sd_bus_message_ref(sd_bus_message* m) override
    {
        return ::sd_bus_message_ref(m);
    }

    int sd_bus_message_skip(sd_bus_message* m, const char* types) override
    {
        return ::sd_bus_message_skip(m, types);
    }

    int sd_bus_message_verify_type(sd_bus_message* m, char type,
                                   const char* contents) override
    {
        return ::sd_bus_message_verify_type(m, type, contents);
    }

    int sd_bus_slot_set_destroy_callback(sd_bus_slot* slot,
                                         sd_bus_destroy_t callback) override
    {
        return ::sd_bus_slot_set_destroy_callback(slot, callback);
    }

    void* sd_bus_slot_set_userdata(sd_bus_slot* slot, void* userdata) override
    {
        return ::sd_bus_slot_set_userdata(slot, userdata);
    }

    int sd_bus_process(sd_bus* bus, sd_bus_message** r) override
    {
        return ::sd_bus_process(bus, r);
    }

    sd_bus* sd_bus_ref(sd_bus* bus) override
    {
        return ::sd_bus_ref(bus);
    }

    int sd_bus_request_name(sd_bus* bus, const char* name,
                            uint64_t flags) override
    {
        return ::sd_bus_request_name(bus, name, flags);
    }

    int sd_bus_send(sd_bus* bus, sd_bus_message* m, uint64_t* cookie) override
    {
        return ::sd_bus_send(bus, m, cookie);
    }

    sd_bus* sd_bus_unref(sd_bus* bus) override
    {
        return ::sd_bus_unref(bus);
    }

    sd_bus_slot* sd_bus_slot_unref(sd_bus_slot* slot) override
    {
        return ::sd_bus_slot_unref(slot);
    }

    sd_bus* sd_bus_flush_close_unref(sd_bus* bus) override
    {
        return ::sd_bus_flush_close_unref(bus);
    }

    int sd_bus_flush(sd_bus* bus) override
    {
        return ::sd_bus_flush(bus);
    }

    void sd_bus_close(sd_bus* bus) override
    {
        ::sd_bus_close(bus);
    }

    int sd_bus_is_open(sd_bus* bus) override
    {
        return ::sd_bus_is_open(bus);
    }

    int sd_bus_wait(sd_bus* bus, uint64_t timeout_usec) override
    {
        return ::sd_bus_wait(bus, timeout_usec);
    }

    int sd_notify(int unset_environment, const char* state) override
    {
        return ::sd_notify(unset_environment, state);
    }

    int sd_watchdog_enabled(int unset_environment, uint64_t* usec) override
    {
        return ::sd_watchdog_enabled(unset_environment, usec);
    }

    int sd_bus_message_append_array(sd_bus_message* m, char type,
                                    const void* ptr, size_t size) override
    {
        return ::sd_bus_message_append_array(m, type, ptr, size);
    }

    int sd_bus_message_read_array(sd_bus_message* m, char type,
                                  const void** ptr, size_t* size) override
    {
        return ::sd_bus_message_read_array(m, type, ptr, size);
    }
};

extern SdBusImpl sdbus_impl;

} // namespace sdbusplus
