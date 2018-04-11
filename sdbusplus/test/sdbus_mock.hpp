#pragma once

#include "gmock/gmock.h"

#include <iostream>

#include <sdbusplus/sdbus.hpp>

namespace sdbusplus {

class SdBusMock : public SdBusInterface
{
 public:
  virtual ~SdBusMock() {};

  MOCK_METHOD3(sd_bus_add_object_manager, int(sd_bus *, sd_bus_slot **, const char *));

  MOCK_METHOD1(sd_bus_message_ref, sd_bus_message*(sd_bus_message *));
  MOCK_METHOD3(sd_bus_message_append_basic, int(sd_bus_message *, char, const void *));
  MOCK_METHOD2(sd_bus_message_at_end, int(sd_bus_message *, int));
  MOCK_METHOD1(sd_bus_message_close_container, int(sd_bus_message *));
  MOCK_METHOD3(sd_bus_message_enter_container, int(sd_bus_message *, char, const char *));
  MOCK_METHOD1(sd_bus_message_exit_container, int(sd_bus_message *));

  MOCK_METHOD1(sd_bus_message_get_bus, sd_bus*(sd_bus_message *));
  MOCK_METHOD2(sd_bus_message_get_cookie, int(sd_bus_message *, uint64_t *));
  MOCK_METHOD1(sd_bus_message_get_destination, const char *(sd_bus_message *));
  MOCK_METHOD1(sd_bus_message_get_interface, const char *(sd_bus_message *));
  MOCK_METHOD1(sd_bus_message_get_member, const char *(sd_bus_message *));
  MOCK_METHOD1(sd_bus_message_get_path, const char *(sd_bus_message *));
  MOCK_METHOD1(sd_bus_message_get_sender, const char *(sd_bus_message *));
  MOCK_METHOD2(sd_bus_message_get_signature, const char *(sd_bus_message *, int));

  MOCK_METHOD3(sd_bus_message_is_method_call, int(sd_bus_message *, const char *, const char *));
  MOCK_METHOD2(sd_bus_message_is_method_error, int(sd_bus_message *, const char *));
  MOCK_METHOD3(sd_bus_message_is_signal, int(sd_bus_message *, const char *, const char *));

  MOCK_METHOD2(sd_bus_message_new_method_return, int(sd_bus_message *, sd_bus_message **));
  MOCK_METHOD3(sd_bus_message_read_basic, int(sd_bus_message *, char, void *));

  MOCK_METHOD2(sd_bus_message_skip, int(sd_bus_message *, const char *));
  MOCK_METHOD3(sd_bus_message_verify_type, int(sd_bus_message *, char, const char *));
  MOCK_METHOD3(sd_bus_send, int(sd_bus *, sd_bus_message *, uint64_t *));

  MOCK_METHOD3(sd_bus_message_open_container, int(sd_bus_message *, char, const char *));
};

}  // sdbusplus
