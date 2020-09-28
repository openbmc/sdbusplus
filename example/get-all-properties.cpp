#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/get_all_properties.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
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

    uint32_t fatalErrors() const
    {
        return fatalErrors_;
    }

    auto logSystemErrorCode()
    {
        return [this](boost::system::error_code ec) {
            std::cerr << "Error: " << ec << "\n";
            ++fatalErrors_;
        };
    }

    void logException(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        ++fatalErrors_;
    }

    void logExpectedException(
        const sdbusplus::exception::UnpackPropertyError& error)
    {
        std::cout << "As expected " << error.what() << " => "
                  << error.propertyName << " is missing because "
                  << error.reason << "\n";
    }

    void asyncGetAllPropertiesStringTypeOnly()
    {
        sdbusplus::asio::getAllProperties(
            bus_, xyz::demo::name, xyz::demo::path, xyz::demo::interface,
            logSystemErrorCode(),
            [this](std::vector<std::pair<
                       std::string, std::variant<std::monostate, std::string>>>&
                       properties) {
                try
                {
                    std::string greetings;
                    std::string goodbyes;
                    sdbusplus::unpackProperties(properties, name::greetings,
                                                greetings, name::goodbyes,
                                                goodbyes);

                    std::cout << "value of greetings: " << greetings << "\n";
                    std::cout << "value of goodbyes: " << goodbyes << "\n";
                }
                catch (const sdbusplus::exception::UnpackPropertyError& error)
                {
                    logException(error);
                }

                try
                {
                    std::string value;
                    sdbusplus::unpackProperties(properties, name::value, value);

                    std::cerr << "Error: it should fail because of "
                                 "not matched type\n";
                    ++fatalErrors_;
                }
                catch (const sdbusplus::exception::UnpackPropertyError& error)
                {
                    logExpectedException(error);
                }
            });
    }

    void asyncGetAllProperties()
    {
        sdbusplus::asio::getAllProperties(
            bus_, xyz::demo::name, xyz::demo::path, xyz::demo::interface,
            logSystemErrorCode(),
            [this](
                std::vector<std::pair<std::string,
                                      std::variant<std::monostate, std::string,
                                                   uint32_t>>>& properties) {
                try
                {
                    std::string greetings;
                    std::string goodbyes;
                    uint32_t value = 0u;
                    sdbusplus::unpackProperties(properties, name::greetings,
                                                greetings, name::goodbyes,
                                                goodbyes, name::value, value);

                    std::cout << "value of greetings: " << greetings << "\n";
                    std::cout << "value of goodbyes: " << goodbyes << "\n";
                    std::cout << "value of value: " << value << "\n";
                }
                catch (const sdbusplus::exception::UnpackPropertyError& error)
                {
                    logException(error);
                }

                try
                {
                    std::string unknownProperty;
                    sdbusplus::unpackProperties(
                        properties, "UnknownPropertyName", unknownProperty);

                    std::cerr << "Error: it should fail because of "
                                 "missing property\n";
                    ++fatalErrors_;
                }
                catch (const sdbusplus::exception::UnpackPropertyError& error)
                {
                    logExpectedException(error);
                }

                try
                {
                    uint32_t notMatchingType;
                    sdbusplus::unpackProperties(properties, name::greetings,
                                                notMatchingType);

                    std::cerr << "Error: it should fail because of "
                                 "not matched type\n";
                    ++fatalErrors_;
                }
                catch (const sdbusplus::exception::UnpackPropertyError& error)
                {
                    logExpectedException(error);
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

    uint32_t fatalErrors_ = 0u;
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

    boost::asio::post(ioc,
                      [&app] { app.asyncGetAllPropertiesStringTypeOnly(); });
    boost::asio::post(ioc, [&app] { app.asyncGetAllProperties(); });

    ioc.run();

    std::cout << "Fatal errors count: " << app.fatalErrors() << "\n";

    return app.fatalErrors();
}
