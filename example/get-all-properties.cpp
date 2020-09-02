#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/get_all_properties.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <iostream>

namespace xyz
{
namespace demo
{

const std::string path = "/xyz/demo";
const std::string name = "xyz.demo";
const std::string interface = "xyz.demo.interface";

} // namespace demo
} // namespace xyz

namespace name
{

const std::string greetings = "Greetings";
const std::string goodbyes = "Goodbyes";
const std::string value = "Value";

} // namespace name

class Application
{
  public:
    Application(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
                sdbusplus::asio::object_server& objServer) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer)
    {
        demo_ = objServer_.add_interface(xyz::demo::path, xyz::demo::interface);

        demo_->register_property_r(name::greetings, std::string(),
                                   sdbusplus::vtable::property_::const_,
                                   [this](const auto&) { return greetings_; });

        demo_->register_property_rw(
            name::goodbyes, std::string(),
            sdbusplus::vtable::property_::emits_change,
            [this](const auto& newPropertyValue, const auto&) {
                goodbyes_ = newPropertyValue;
                return 1;
            },
            [this](const auto&) { return goodbyes_; });

        demo_->register_property_r(
            name::value, uint32_t{42}, sdbusplus::vtable::property_::const_,
            [](const auto& value) -> uint32_t { return value; });

        demo_->initialize();
    }

    ~Application()
    {
        objServer_.remove_interface(demo_);
    }

    void asyncGetAllProperties()
    {
        sdbusplus::getAllProperties(
            bus_, xyz::demo::name, xyz::demo::path, xyz::demo::interface,
            [](boost::system::error_code ec) {
                std::cerr << "Something went wrong: " << ec << "\n";
            },
            [](std::vector<
                std::pair<std::string, std::variant<std::string, uint32_t>>>&
                   properties) {
                std::string greetings;
                std::string goodbyes;
                uint32_t value = 0u;
                if (auto error = sdbusplus::unpackProperties(
                        properties, name::greetings, greetings, name::goodbyes,
                        goodbyes, name::value, value))
                {
                    std::cerr
                        << "It should be ok, but it is not: " << error->what()
                        << "\n";
                }
                else
                {
                    std::cout << "value of greetings: " << greetings << "\n";
                    std::cout << "value of goodbyes: " << goodbyes << "\n";
                    std::cout << "value of value: " << value << "\n";
                }

                std::string unknownProperty;
                if (auto error = sdbusplus::unpackProperties(
                        properties, "UnknownPropertyName", unknownProperty))
                {
                    std::cerr << error->what() << " => "
                              << "As expected " << error->propertyName
                              << " is missing because " << error->reason
                              << "\n";
                }
                else
                {
                    std::cerr << "It is unexpected, it should fail because of "
                                 "missing property\n";
                }

                uint32_t notMatchingType;
                if (auto error = sdbusplus::unpackProperties(
                        properties, name::greetings, notMatchingType))
                {
                    std::cerr << error->what() << " => "
                              << "As expected " << error->propertyName
                              << " is missing because " << error->reason
                              << "\n";
                }
                else
                {
                    std::cerr << "It is unexpected, it should fail because of "
                                 "not matched type\n";
                }
            });
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::shared_ptr<sdbusplus::asio::dbus_interface> demo_;
    std::string greetings_ = "Hello";
    std::string goodbyes_ = "Bye";
};

int main(int, char**)
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait(
        [&ioc](const boost::system::error_code&, const int&) { ioc.stop(); });

    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    auto objServer = std::make_unique<sdbusplus::asio::object_server>(bus);

    bus->request_name(xyz::demo::name.c_str());

    Application app(ioc, *bus, *objServer);

    boost::asio::post(ioc, [&app] { app.asyncGetAllProperties(); });

    ioc.run();

    return 0;
}
