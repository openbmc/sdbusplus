#pragma once

#include "gmock/gmock.h"

#include <string>
#include <vector>

#include <sdbusplus/base.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{
namespace bus
{

class BusMock : public BusAbc
{
 public:
  virtual ~BusMock() {}
  MOCK_METHOD0(release, busp_t());
  MOCK_METHOD1(wait, void(uint64_t));
  MOCK_METHOD0(process, message::message());
  MOCK_METHOD0(process_discard, void());
  MOCK_METHOD1(request_name, void(const char*));
  MOCK_METHOD4(new_method_call, message::message(const char*, const char*, const char*, const char*));
  MOCK_METHOD3(new_signal, message::message(const char*, const char*, const char*));
  MOCK_METHOD2(call, message::message(message::message&, uint64_t));
  MOCK_METHOD2(call_noreply, void(message::message&, uint64_t));
  MOCK_METHOD0(get_unique_name, std::string());
  MOCK_METHOD2(attach_event, void(sd_event*, int));
  MOCK_METHOD0(detach_event, void());
  MOCK_METHOD0(get_event, sd_event*());
  MOCK_METHOD2(emit_interfaces_added, void(const char*, const std::vector<std::string>&));
  MOCK_METHOD2(emit_interfaces_removed, void(const char*, const std::vector<std::string>&));
  MOCK_METHOD1(emit_object_added, void(const char*));
  MOCK_METHOD1(emit_object_removed, void(const char*));
  MOCK_METHOD0(list_names_acquired, std::vector<std::string>());
};

}  // namespace bus
}  // namespace sdbusplus

