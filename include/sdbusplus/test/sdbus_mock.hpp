#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>

#include "gmock/gmock.h"

namespace sdbusplus
{

class SdBusMock : public SdBusInterface
{
  public:
    virtual ~SdBusMock(){};

    MOCK_METHOD3(sd_bus_add_object_manager,
                 int(sd_bus*, sd_bus_slot**, const char*));
    MOCK_METHOD6(sd_bus_add_object_vtable,
                 int(sd_bus*, sd_bus_slot**, const char*, const char*,
                     const sd_bus_vtable*, void*));
    MOCK_METHOD3(sd_bus_attach_event, int(sd_bus*, sd_event*, int));
    MOCK_METHOD5(sd_bus_call, int(sd_bus*, sd_bus_message*, uint64_t,
                                  sd_bus_error*, sd_bus_message**));
    MOCK_METHOD1(sd_bus_detach_event, int(sd_bus*));

    MOCK_METHOD3(sd_bus_emit_interfaces_added_strv,
                 int(sd_bus*, const char*, char**));
    MOCK_METHOD3(sd_bus_emit_interfaces_removed_strv,
                 int(sd_bus*, const char*, char**));
    MOCK_METHOD2(sd_bus_emit_object_added, int(sd_bus*, const char*));
    MOCK_METHOD2(sd_bus_emit_object_removed, int(sd_bus*, const char*));
    MOCK_METHOD4(sd_bus_emit_properties_changed_strv,
                 int(sd_bus*, const char*, const char*, char**));

    MOCK_METHOD3(sd_bus_error_set,
                 int(sd_bus_error*, const char*, const char*));
    MOCK_METHOD3(sd_bus_error_set_const,
                 int(sd_bus_error*, const char*, const char*));
    MOCK_METHOD1(sd_bus_error_get_errno, int(const sd_bus_error*));
    MOCK_METHOD2(sd_bus_error_set_errno, int(sd_bus_error*, int));
    MOCK_METHOD1(sd_bus_error_is_set, int(const sd_bus_error*));
    MOCK_METHOD1(sd_bus_error_free, void(sd_bus_error*));

    MOCK_METHOD1(sd_bus_get_event, sd_event*(sd_bus*));
    MOCK_METHOD1(sd_bus_get_fd, int(sd_bus*));
    MOCK_METHOD2(sd_bus_get_unique_name, int(sd_bus*, const char**));

    MOCK_METHOD3(sd_bus_list_names, int(sd_bus*, char***, char***));

    MOCK_METHOD3(sd_bus_message_append_basic,
                 int(sd_bus_message*, char, const void*));
    MOCK_METHOD2(sd_bus_message_at_end, int(sd_bus_message*, int));
    MOCK_METHOD1(sd_bus_message_close_container, int(sd_bus_message*));
    MOCK_METHOD3(sd_bus_message_enter_container,
                 int(sd_bus_message*, char, const char*));
    MOCK_METHOD1(sd_bus_message_exit_container, int(sd_bus_message*));

    MOCK_METHOD1(sd_bus_message_get_bus, sd_bus*(sd_bus_message*));
    MOCK_METHOD2(sd_bus_message_get_type, int(sd_bus_message*, uint8_t*));
    MOCK_METHOD2(sd_bus_message_get_cookie, int(sd_bus_message*, uint64_t*));
    MOCK_METHOD2(sd_bus_message_get_reply_cookie,
                 int(sd_bus_message*, uint64_t*));
    MOCK_METHOD1(sd_bus_message_get_destination, const char*(sd_bus_message*));
    MOCK_METHOD1(sd_bus_message_get_interface, const char*(sd_bus_message*));
    MOCK_METHOD1(sd_bus_message_get_member, const char*(sd_bus_message*));
    MOCK_METHOD1(sd_bus_message_get_path, const char*(sd_bus_message*));
    MOCK_METHOD1(sd_bus_message_get_sender, const char*(sd_bus_message*));
    MOCK_METHOD2(sd_bus_message_get_signature,
                 const char*(sd_bus_message*, int));
    MOCK_METHOD1(sd_bus_message_get_errno, int(sd_bus_message*));
    MOCK_METHOD1(sd_bus_message_get_error,
                 const sd_bus_error*(sd_bus_message*));

    MOCK_METHOD3(sd_bus_message_is_method_call,
                 int(sd_bus_message*, const char*, const char*));
    MOCK_METHOD2(sd_bus_message_is_method_error,
                 int(sd_bus_message*, const char*));
    MOCK_METHOD3(sd_bus_message_is_signal,
                 int(sd_bus_message*, const char*, const char*));

    MOCK_METHOD6(sd_bus_message_new_method_call,
                 int(sd_bus*, sd_bus_message**, const char*, const char*,
                     const char*, const char*));

    MOCK_METHOD2(sd_bus_message_new_method_return,
                 int(sd_bus_message*, sd_bus_message**));

    MOCK_METHOD4(sd_bus_message_new_method_error,
                 int(sd_bus_message* call, sd_bus_message** m, const char* name,
                     const char* description));

    MOCK_METHOD4(sd_bus_message_new_method_errno,
                 int(sd_bus_message* call, sd_bus_message** m, int error,
                     const sd_bus_error* p));

    MOCK_METHOD5(sd_bus_message_new_signal,
                 int(sd_bus*, sd_bus_message**, const char*, const char*,
                     const char*));

    MOCK_METHOD3(sd_bus_message_open_container,
                 int(sd_bus_message*, char, const char*));

    MOCK_METHOD3(sd_bus_message_read_basic, int(sd_bus_message*, char, void*));
    MOCK_METHOD1(sd_bus_message_ref, sd_bus_message*(sd_bus_message*));

    MOCK_METHOD2(sd_bus_message_skip, int(sd_bus_message*, const char*));
    MOCK_METHOD3(sd_bus_message_verify_type,
                 int(sd_bus_message*, char, const char*));

    MOCK_METHOD2(sd_bus_process, int(sd_bus*, sd_bus_message**));
    MOCK_METHOD1(sd_bus_ref, sd_bus*(sd_bus*));
    MOCK_METHOD3(sd_bus_request_name, int(sd_bus*, const char*, uint64_t));
    MOCK_METHOD3(sd_bus_send, int(sd_bus*, sd_bus_message*, uint64_t*));
    MOCK_METHOD1(sd_bus_unref, sd_bus*(sd_bus*));
    MOCK_METHOD1(sd_bus_flush_close_unref, sd_bus*(sd_bus*));
    MOCK_METHOD1(sd_bus_flush, int(sd_bus*));
    MOCK_METHOD1(sd_bus_close, void(sd_bus*));
    MOCK_METHOD2(sd_bus_wait, int(sd_bus*, uint64_t));
};

inline bus::bus get_mocked_new(SdBusMock* sdbus)
{
    using ::testing::IsNull;
    using ::testing::Return;

    EXPECT_CALL(*sdbus, sd_bus_ref(IsNull())).WillRepeatedly(Return(nullptr));
    bus::bus bus_mock(nullptr, sdbus);

    return bus_mock;
}

} // namespace sdbusplus
