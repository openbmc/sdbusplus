#include "dbus.hpp"

#include <string.h>

using std::string;
using std::unique_ptr;

namespace ipmid
{
namespace
{
int HandleInternal(sd_bus_message* message, void* raw_context, void* ret_error)
{
    DBusHandlerContext* context = reinterpret_cast<DBusHandlerContext*>
                                  (raw_context);
    return context->handler->HandleMessage(context->dbus->dbus_message_operations(),
                                           message);
}
} // namespace

DBus::~DBus()
{
    bus_ops_->sd_bus_slot_unref(bus_slot_);
    bus_ops_->sd_bus_unref(bus_);
}

int DBus::Init()
{
    return bus_ops_->sd_bus_open_system(&bus_);
}

bool DBus::GetServiceMapping(const string& path, string* destination)
{
    char* c_dest = nullptr;
    if (bus_ops_->mapper_get_service(bus_, path.c_str(), &c_dest) < 0 ||
        c_dest == nullptr)
    {
        return false;
    }
    *destination = c_dest;
    free(c_dest);
    return true;
}

bool DBus::CallMethod(const DBusMemberInfo& info, const DBusInput& input,
                      DBusOutput* output)
{
    sd_bus_message* request = nullptr;
    if (bus_ops_->sd_bus_message_new_method_call(bus_,
            &request,
            info.destination.c_str(),
            info.path.c_str(),
            info.interface.c_str(),
            info.member.c_str()) < 0)
    {
        return false;
    }

    input.Compose(*message_ops_, request);

    DBusErrorUniquePtr error = bus_ops_->MakeError();
    sd_bus_message* response = nullptr;
    bool is_successful = true;
    VLOG(1) << "Calling D-Bus with "
            << "dest = " << info.destination << ", "
            << "path = " << info.path << ", "
            << "interface = " << info.interface << ", "
            << "member = " << info.member;
    int ret = bus_ops_->sd_bus_call(bus_, request, 0, error.get(), &response);
    if (ret < 0)
    {
        LOG(ERROR) << "Calling D-Bus failed: " << strerror(-ret) << ", "
                   << bus_ops_->DBusErrorToString(*error);
        bus_ops_->sd_bus_error_free(error.get());
        bus_ops_->sd_bus_message_unref(request);
        bus_ops_->sd_bus_message_unref(response);
        return false;
    }

    is_successful = output->Parse(*message_ops_, response);
    bus_ops_->sd_bus_error_free(error.get());
    bus_ops_->sd_bus_message_unref(request);
    bus_ops_->sd_bus_message_unref(response);
    return is_successful;
}

int DBus::ProcessMessage()
{
    VLOG(1) << "Trying to process message.";
    return bus_ops_->sd_bus_process(bus_, NULL);
}

int DBus::WaitForMessage()
{
    VLOG(1) << "Waiting for message.";
    return bus_ops_->sd_bus_wait(bus_, static_cast<uint64_t>(-1));
}

int DBus::RegisterHandler(const string& match, DBusHandler* handler)
{
    unique_ptr<DBusHandlerContext> context(new DBusHandlerContext);
    context->dbus = this;
    context->handler = handler;
    handlers_.push_back(std::move(context));
    return bus_ops_->sd_bus_add_match(bus_, &bus_slot_, match.c_str(),
                                      HandleInternal, handlers_.back().get());
}

} // namespace ipmid
