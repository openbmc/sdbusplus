#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

namespace xyz
{
namespace demo
{

const std::string path = "/xyz/demo";
const std::string name = "xyz.demo";

} // namespace demo
} // namespace xyz

namespace name
{

const std::string greetings = "Greetings";
const std::string goodbyes = "Goodbyes";

} // namespace name

namespace utils
{

template <class T>
class Property
{
  public:
    Property(sdbusplus::asio::connection& bus, std::string_view service,
             std::string_view path, std::string_view interface,
             std::string_view name) :
        bus_(bus),
        service_(service), path_(path), interface_(interface), name_(name)
    {}

    template <class F>
    void async_get(F&& callback)
    {
        bus_.async_method_call(
            [callback =
                 std::move(callback)](const boost::system::error_code& error,
                                      const std::variant<T>& valueVariant) {
                if (error)
                {
                    callback(std::nullopt);
                    return;
                }

                if (auto value = std::get_if<T>(&valueVariant))
                {
                    callback(*value);
                    return;
                }

                callback(std::nullopt);
            },
            service_, path_, "org.freedesktop.DBus.Properties", "Get",
            interface_, name_);
    }

    template <class F>
    void async_set(const T& value, F&& callback)
    {
        bus_.async_method_call(
            [callback = std::move(callback)](
                const boost::system::error_code& error) { callback(error); },
            service_, path_, "org.freedesktop.DBus.Properties", "Set",
            interface_, name_, value);
    }

  private:
    sdbusplus::asio::connection& bus_;
    std::string service_;
    std::string path_;
    std::string interface_;
    std::string name_;
};

} // namespace utils

class Application
{
  public:
    Application(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
                sdbusplus::asio::object_server& objServer) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer)
    {
        demo_ = objServer_.add_interface(xyz::demo::path, xyz::demo::name);

        demo_->register_property_r(
            name::greetings, std::string(),
            sdbusplus::vtable::property_::const_,
            [this](const auto& propertyValue) { return greetings_; });

        demo_->register_property_rw(
            name::goodbyes, std::string(),
            sdbusplus::vtable::property_::emits_change,
            [this](const auto& newPropertyValue, const auto& oldPropertyValue) {
                goodbyes_ = newPropertyValue;
                return 1;
            },
            [this](const auto& propertyValue) { return goodbyes_; });

        demo_->initialize();
    }

    ~Application()
    {
        objServer_.remove_interface(demo_);
    }

    void asyncReadProperties()
    {
        propertyGreetings.async_get([](std::optional<std::string> value) {
            std::cout << "Greetings value is: "
                      << value.value_or("std::nullopt") << "\n";
        });

        propertyGoodbyes.async_get([](std::optional<std::string> value) {
            std::cout << "Goodbyes value is: " << value.value_or("std::nullopt")
                      << "\n";
        });
    }

    void asyncChangeProperty()
    {
        propertyGreetings.async_set(
            "Hi, hey, hello", [](const boost::system::error_code& error) {
                if (error)
                {
                    std::cout
                        << "As expected, failed to set greetings property\n";
                }
            });

        propertyGoodbyes.async_set(
            "Bye bye", [this](const boost::system::error_code& error) {
                if (!error)
                {
                    std::cout << "Changed goodbyes property as expected\n";
                }
                boost::asio::post(ioc_, [this] { asyncReadProperties(); });
            });
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::shared_ptr<sdbusplus::asio::dbus_interface> demo_;
    std::string greetings_ = "Hello";
    std::string goodbyes_ = "Bye";

    utils::Property<std::string> propertyGreetings{
        bus_, xyz::demo::name, xyz::demo::path, xyz::demo::name,
        name::greetings};
    utils::Property<std::string> propertyGoodbyes{
        bus_, xyz::demo::name, xyz::demo::path, xyz::demo::name,
        name::goodbyes};
};

int main(int argc, char** argv)
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait([&ioc](const boost::system::error_code& e,
                              const int& signal) { ioc.stop(); });

    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    auto objServer = std::make_unique<sdbusplus::asio::object_server>(bus);

    bus->request_name(xyz::demo::name.c_str());

    Application app(ioc, *bus, *objServer);

    boost::asio::post(ioc, [&app] { app.asyncReadProperties(); });
    boost::asio::post(ioc, [&app] { app.asyncChangeProperty(); });

    ioc.run();

    return 0;
}
