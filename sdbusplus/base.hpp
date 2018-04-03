#pragma once

#include <climits>
#include <iostream>
#include <vector>
#include <string>

#include <systemd/sd-bus.h>

namespace sdbusplus
{

namespace message
{
class message;
}  // message

namespace bus
{
using busp_t = sd_bus*;

/** @class BusAbc
 *  @brief Provides an abstract-base-class for bus to allow for mock
 *  injection.
 */
class BusAbc
{
 public:
  virtual ~BusAbc() {}

  virtual busp_t release() = 0;
  virtual void wait(uint64_t timeout_us = ULLONG_MAX) = 0;
  virtual message::message process() = 0;
  virtual void process_discard() = 0;
  virtual void request_name(const char* service) = 0;
  virtual message::message new_method_call(
      const char* service, const char* objpath,
      const char* interf, const char* method) = 0;
  virtual message::message new_signal(
      const char* objpath, const char* interf, const char* member) = 0;
  virtual message::message call(message::message& m, uint64_t timeout_us = 0) = 0;
  virtual void call_noreply(message::message& m, uint64_t timeout_us = 0) = 0;
  virtual std::string get_unique_name() = 0;
  virtual void attach_event(sd_event* event, int priority) = 0;
  virtual void detach_event() = 0;
  virtual sd_event* get_event() = 0;
  virtual void emit_interfaces_added(
      const char* path, const std::vector<std::string>& ifaces) = 0;
  virtual void emit_interfaces_removed(
      const char* path, const std::vector<std::string>& ifaces) = 0;
  virtual void emit_object_added(const char* path) = 0;
  virtual void emit_object_removed(const char* path) = 0;
  virtual std::vector<std::string> list_names_acquired() = 0;
};

}  // bus

}  // sdbusplus
