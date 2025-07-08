#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>

#include "gmock/gmock.h"

namespace sdbusplus
{

class SdBusMock : public SdBusInterface
{
  public:
    virtual ~SdBusMock() override {}

    MOCK_METHOD(int, sd_bus_add_object_manager,
                (sd_bus*, sd_bus_slot**, const char*), (override));

    MOCK_METHOD(int, sd_bus_add_object_vtable,
                (sd_bus*, sd_bus_slot**, const char*, const char*,
                 const sd_bus_vtable*, void*),
                (override));

    MOCK_METHOD(int, sd_bus_add_match,
                (sd_bus*, sd_bus_slot**, const char*, sd_bus_message_handler_t,
                 void*),
                (override));
    MOCK_METHOD(int, sd_bus_attach_event, (sd_bus*, sd_event*, int),
                (override));
    MOCK_METHOD(int, sd_bus_call,
                (sd_bus*, sd_bus_message*, uint64_t, sd_bus_error*,
                 sd_bus_message**),
                (override));
    MOCK_METHOD(int, sd_bus_call_async,
                (sd_bus*, sd_bus_slot**, sd_bus_message*,
                 sd_bus_message_handler_t, void*, uint64_t),
                (override));
    MOCK_METHOD(int, sd_bus_detach_event, (sd_bus*), (override));

    MOCK_METHOD(int, sd_bus_emit_interfaces_added_strv,
                (sd_bus*, const char*, char**), (override));
    MOCK_METHOD(int, sd_bus_emit_interfaces_removed_strv,
                (sd_bus*, const char*, char**), (override));
    MOCK_METHOD(int, sd_bus_emit_object_added, (sd_bus*, const char*),
                (override));
    MOCK_METHOD(int, sd_bus_emit_object_removed, (sd_bus*, const char*),
                (override));
    MOCK_METHOD(int, sd_bus_emit_properties_changed_strv,
                (sd_bus*, const char*, const char*, const char**), (override));

    MOCK_METHOD(int, sd_bus_error_set,
                (sd_bus_error*, const char*, const char*), (override));
    MOCK_METHOD(int, sd_bus_error_set_const,
                (sd_bus_error*, const char*, const char*), (override));
    MOCK_METHOD(int, sd_bus_error_get_errno, (const sd_bus_error*), (override));
    MOCK_METHOD(int, sd_bus_error_set_errno, (sd_bus_error*, int), (override));
    MOCK_METHOD(int, sd_bus_error_is_set, (const sd_bus_error*), (override));
    MOCK_METHOD(void, sd_bus_error_free, (sd_bus_error*), (override));

    MOCK_METHOD(sd_event*, sd_bus_get_event, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_get_fd, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_get_events, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_get_timeout, (sd_bus*, uint64_t* timeout_usec),
                (override));
    MOCK_METHOD(int, sd_bus_get_unique_name, (sd_bus*, const char**),
                (override));

    MOCK_METHOD(int, sd_bus_list_names, (sd_bus*, char***, char***),
                (override));

    MOCK_METHOD(int, sd_bus_message_append_basic,
                (sd_bus_message*, char, const void*), (override));
    MOCK_METHOD(int, sd_bus_message_append_string_iovec,
                (sd_bus_message*, const struct iovec* iov, int iovcnt),
                (override));
    MOCK_METHOD(int, sd_bus_message_at_end, (sd_bus_message*, int), (override));
    MOCK_METHOD(int, sd_bus_message_close_container, (sd_bus_message*),
                (override));
    MOCK_METHOD(int, sd_bus_message_enter_container,
                (sd_bus_message*, char, const char*), (override));
    MOCK_METHOD(int, sd_bus_message_exit_container, (sd_bus_message*),
                (override));

    MOCK_METHOD(sd_bus*, sd_bus_message_get_bus, (sd_bus_message*), (override));
    MOCK_METHOD(int, sd_bus_message_get_type, (sd_bus_message*, uint8_t*),
                (override));
    MOCK_METHOD(int, sd_bus_message_get_cookie, (sd_bus_message*, uint64_t*),
                (override));
    MOCK_METHOD(int, sd_bus_message_get_reply_cookie,
                (sd_bus_message*, uint64_t*), (override));
    MOCK_METHOD(const char*, sd_bus_message_get_destination, (sd_bus_message*),
                (override));
    MOCK_METHOD(const char*, sd_bus_message_get_interface, (sd_bus_message*),
                (override));
    MOCK_METHOD(const char*, sd_bus_message_get_member, (sd_bus_message*),
                (override));
    MOCK_METHOD(const char*, sd_bus_message_get_path, (sd_bus_message*),
                (override));
    MOCK_METHOD(const char*, sd_bus_message_get_sender, (sd_bus_message*),
                (override));
    MOCK_METHOD(const char*, sd_bus_message_get_signature,
                (sd_bus_message*, int), (override));
    MOCK_METHOD(int, sd_bus_message_get_errno, (sd_bus_message*), (override));
    MOCK_METHOD(const sd_bus_error*, sd_bus_message_get_error,
                (sd_bus_message*), (override));

    MOCK_METHOD(int, sd_bus_message_is_method_call,
                (sd_bus_message*, const char*, const char*), (override));
    MOCK_METHOD(int, sd_bus_message_is_method_error,
                (sd_bus_message*, const char*), (override));
    MOCK_METHOD(int, sd_bus_message_is_signal,
                (sd_bus_message*, const char*, const char*), (override));

    MOCK_METHOD(int, sd_bus_message_new_method_call,
                (sd_bus*, sd_bus_message**, const char*, const char*,
                 const char*, const char*),
                (override));

    MOCK_METHOD(int, sd_bus_message_new_method_return,
                (sd_bus_message*, sd_bus_message**), (override));

    MOCK_METHOD(int, sd_bus_message_new_method_error,
                (sd_bus_message * call, sd_bus_message** m, sd_bus_error* e),
                (override));

    MOCK_METHOD(int, sd_bus_message_new_method_errno,
                (sd_bus_message * call, sd_bus_message** m, int error,
                 const sd_bus_error* p),
                (override));

    MOCK_METHOD(int, sd_bus_message_new_signal,
                (sd_bus*, sd_bus_message**, const char*, const char*,
                 const char*),
                (override));

    MOCK_METHOD(int, sd_bus_message_open_container,
                (sd_bus_message*, char, const char*), (override));

    MOCK_METHOD(int, sd_bus_message_read_basic, (sd_bus_message*, char, void*),
                (override));
    MOCK_METHOD(sd_bus_message*, sd_bus_message_ref, (sd_bus_message*),
                (override));

    MOCK_METHOD(int, sd_bus_message_skip, (sd_bus_message*, const char*),
                (override));
    MOCK_METHOD(int, sd_bus_message_verify_type,
                (sd_bus_message*, char, const char*), (override));

    MOCK_METHOD(int, sd_bus_slot_set_destroy_callback,
                (sd_bus_slot*, sd_bus_destroy_t), (override));
    MOCK_METHOD(void*, sd_bus_slot_set_userdata, (sd_bus_slot*, void*),
                (override));

    MOCK_METHOD(int, sd_bus_process, (sd_bus*, sd_bus_message**), (override));
    MOCK_METHOD(sd_bus*, sd_bus_ref, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_request_name, (sd_bus*, const char*, uint64_t),
                (override));
    MOCK_METHOD(int, sd_bus_send, (sd_bus*, sd_bus_message*, uint64_t*),
                (override));
    MOCK_METHOD(sd_bus*, sd_bus_unref, (sd_bus*), (override));
    MOCK_METHOD(sd_bus_slot*, sd_bus_slot_unref, (sd_bus_slot*), (override));
    MOCK_METHOD(sd_bus*, sd_bus_flush_close_unref, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_flush, (sd_bus*), (override));
    MOCK_METHOD(void, sd_bus_close, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_is_open, (sd_bus*), (override));
    MOCK_METHOD(int, sd_bus_wait, (sd_bus*, uint64_t), (override));

    MOCK_METHOD(int, sd_notify, (int, const char*), (override));

    MOCK_METHOD(int, sd_watchdog_enabled, (int, uint64_t* usec), (override));

    MOCK_METHOD(int, sd_bus_message_append_array,
                (sd_bus_message*, char, const void*, size_t), (override));
    MOCK_METHOD(int, sd_bus_message_read_array,
                (sd_bus_message*, char, const void**, size_t*), (override));

    SdBusMock()
    {
        auto slotcb = [](sd_bus*, sd_bus_slot** slot, auto&&...) {
            *slot = reinterpret_cast<sd_bus_slot*>(0xdefa);
            return 0;
        };
        ON_CALL(*this, sd_bus_add_object_manager).WillByDefault(slotcb);
        ON_CALL(*this, sd_bus_add_object_vtable).WillByDefault(slotcb);
        ON_CALL(*this, sd_bus_add_match).WillByDefault(slotcb);
        ON_CALL(*this, sd_bus_call_async).WillByDefault(slotcb);
    }
};

inline bus_t get_mocked_new(SdBusMock* sdbus)
{
    using ::testing::IsNull;
    using ::testing::Return;

    EXPECT_CALL(*sdbus, sd_bus_ref(IsNull())).WillRepeatedly(Return(nullptr));
    bus_t bus_mock(nullptr, sdbus);

    return bus_mock;
}

} // namespace sdbusplus
