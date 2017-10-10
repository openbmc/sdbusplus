#ifndef DBUS_HPP_
#define DBUS_HPP_

#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <stdint.h>

extern "C" {
struct sd_bus_message;
struct sd_bus;
struct sd_bus_slot;

typedef struct sd_bus_message sd_bus_message;
typedef struct sd_bus sd_bus;
typedef struct sd_bus_slot sd_bus_slot;
} // extern "C"

namespace drpc
{

typedef int (* SdBusMessageHandlerT)(sd_bus_message* message,
                                     void* userdata,
                                     void* ret_error);
class DBusMessageOperations
{
    public:
        virtual ~DBusMessageOperations() {}

        virtual int sd_bus_message_append_basic(sd_bus_message* message,
                                                char type,
                                                const void* value) const = 0;

        virtual int sd_bus_message_append_array(sd_bus_message* message,
                                                char type,
                                                const void* array,
                                                size_t size) const = 0;

        virtual int sd_bus_message_read_basic(sd_bus_message* message,
                                              char type,
                                              void* value) const = 0;

        virtual int sd_bus_message_read_array(sd_bus_message* message,
                                              char type,
                                              const void** array,
                                              size_t* size) const = 0;

        virtual int sd_bus_message_enter_container(sd_bus_message* message,
                                                   char type,
                                                   const char* contents) const = 0;

        virtual const char* sd_bus_message_get_sender(
                sd_bus_message* message) const = 0;

        virtual const char* sd_bus_message_get_path(sd_bus_message* message) const = 0;
};

// sd_bus_error is a typedef'd annonymous struct which means it cannot be
// forward declared. Also, unique_ptr does not like void*'s so we have to use a
// stand in type and reinterpret cast it in the implementation.
struct DBusError;
struct DBusErrorDeleter
{
    void operator()(DBusError* error)
    {
        free(error);
    }
};
typedef std::unique_ptr<DBusError, DBusErrorDeleter> DBusErrorUniquePtr;

class DBusBusOperations
{
    public:
        virtual ~DBusBusOperations() {}

        virtual int sd_bus_open_system(sd_bus** bus) const = 0;

        virtual sd_bus* sd_bus_unref(sd_bus* bus) const = 0;

        virtual sd_bus_slot* sd_bus_slot_unref(sd_bus_slot* slot) const = 0;

        virtual int mapper_get_service(sd_bus* bus, const char* path,
                                       char** dest) const = 0;

        virtual int sd_bus_message_new_method_call(sd_bus* bus,
                sd_bus_message** message,
                const char* destination,
                const char* path,
                const char* interface,
                const char* member) const = 0;

        virtual int sd_bus_call(sd_bus* bus,
                                sd_bus_message* message,
                                uint64_t usec,
                                void* error,
                                sd_bus_message** reply) const = 0;

        virtual void sd_bus_error_free(void* error) const = 0;

        virtual sd_bus_message* sd_bus_message_unref(sd_bus_message* message) const = 0;

        virtual int sd_bus_add_match(sd_bus* bus,
                                     sd_bus_slot** slot,
                                     const char* match,
                                     SdBusMessageHandlerT callback,
                                     void* userdata) const = 0;

        virtual int sd_bus_process(sd_bus* bus, sd_bus_message** message) const = 0;

        virtual int sd_bus_wait(sd_bus* bus, uint64_t timeout_usec) const = 0;

        virtual DBusErrorUniquePtr MakeError() const = 0;

        virtual std::string DBusErrorToString(const DBusError& error) const = 0;
};

struct DBusMemberInfo
{
    std::string destination;
    std::string path;
    std::string interface;
    std::string member;
};

class DBusHandler
{
    public:
        virtual ~DBusHandler() {}

        virtual int HandleMessage(const DBusMessageOperations& ops,
                                  sd_bus_message* message) = 0;
};

class DBusInput
{
    public:
        virtual ~DBusInput() {}

        virtual void Compose(const DBusMessageOperations& ops,
                             sd_bus_message* message) const = 0;
};

class DBusOutput
{
    public:
        virtual ~DBusOutput() {}

        virtual bool Parse(const DBusMessageOperations& ops,
                           sd_bus_message* message) = 0;
};

class DBus;

struct DBusHandlerContext
{
    DBus* dbus;
    DBusHandler* handler;
};

class DBus
{
    public:
        DBus(std::unique_ptr<DBusMessageOperations>&& message_ops,
             std::unique_ptr<DBusBusOperations>&& bus_ops)
            : message_ops_(std::move(message_ops)), bus_ops_(std::move(bus_ops)) {}

        virtual ~DBus();

        // Initialization can fail.
        virtual int Init();

        virtual bool GetServiceMapping(const std::string& path, std::string* destination);

        virtual bool CallMethod(const DBusMemberInfo& info, const DBusInput& input,
                                DBusOutput* output);

        virtual int ProcessMessage();

        virtual int WaitForMessage();

        virtual int RegisterHandler(const std::string& match, DBusHandler* dbus_handler);

        virtual const DBusMessageOperations& dbus_message_operations()
        {
            return *message_ops_;
        }

        virtual sd_bus* mutable_sd_bus()
        {
            return bus_;
        }

        virtual sd_bus_slot* mutable_sd_bus_slot()
        {
            return bus_slot_;
        }

    private:
        std::unique_ptr<DBusMessageOperations> message_ops_;
        std::unique_ptr<DBusBusOperations> bus_ops_;
        std::vector<std::unique_ptr<DBusHandlerContext>> handlers_;
        sd_bus* bus_ = nullptr;
        sd_bus_slot* bus_slot_ = nullptr;
};

} // namespace drpc
#endif // DBUS_HPP_
