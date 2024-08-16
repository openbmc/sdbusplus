#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <iostream>

const std::string demoServiceName = "demo.service";
const std::string demoObjectPath = "/xyz/demo";
const std::string demoInterfaceName = "xyz.demo";
const std::string propertyGrettingName = "Greetings";
const std::string propertyGoodbyesName = "Goodbyes";
const std::string propertyValueName = "Value";

class Application
{
  public:
    Application(sdbusplus::asio::connection& bus,
                sdbusplus::asio::object_server& objServer) :
        bus_(bus), objServer_(objServer)
    {
        demo_ =
            objServer_.add_unique_interface(demoObjectPath, demoInterfaceName);

        demo_->register_property_r<std::string>(
            propertyGrettingName, sdbusplus::vtable::property_::const_,
            [this](const auto&) { return greetings_; });

        demo_->register_property_rw<std::string>(
            propertyGoodbyesName, sdbusplus::vtable::property_::emits_change,
            [this](const auto& newPropertyValue, const auto&) {
                goodbyes_ = newPropertyValue;
                return true;
            },
            [this](const auto&) { return goodbyes_; });

        demo_->register_property_r<uint32_t>(
            propertyValueName, sdbusplus::vtable::property_::const_,
            [](const auto& value) -> uint32_t { return value; });

        demo_->initialize();
    }

    uint32_t fatalErrors() const
    {
        return fatalErrors_;
    }

    auto logSystemErrorCode(boost::system::error_code ec)
    {
        std::cerr << "Error: " << ec << "\n";
        ++fatalErrors_;
    }

    void logException(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        ++fatalErrors_;
    }

    void logUnpackError(const sdbusplus::UnpackErrorReason reason,
                        const std::string& property)
    {
        std::cerr << "UnpackError: " << static_cast<int>(reason) << ", "
                  << property << "\n";
        ++fatalErrors_;
    }

    void logExpectedException(
        const sdbusplus::exception::UnpackPropertyError& error)
    {
        std::cout << "As expected " << error.what() << "\n";
    }

    void asyncGetAllPropertiesStringTypeOnly()
    {
        sdbusplus::asio::getAllProperties(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            [this](const boost::system::error_code ec,
                   const std::vector<std::pair<
                       std::string, std::variant<std::monostate, std::string>>>&
                       properties) -> void {
                if (ec)
                {
                    logSystemErrorCode(ec);
                    return;
                }
                {
                    const std::string* greetings = nullptr;
                    const std::string* goodbyes = nullptr;
                    const bool success = sdbusplus::unpackPropertiesNoThrow(
                        [this](const sdbusplus::UnpackErrorReason reason,
                               const std::string& property) {
                            logUnpackError(reason, property);
                        },
                        properties, propertyGrettingName, greetings,
                        propertyGoodbyesName, goodbyes);

                    if (success)
                    {
                        std::cout
                            << "value of greetings: " << *greetings << "\n";
                        std::cout << "value of goodbyes: " << *goodbyes << "\n";
                    }
                    else
                    {
                        ++fatalErrors_;
                    }
                }

                try
                {
                    std::string value;
                    sdbusplus::unpackProperties(properties, propertyValueName,
                                                value);

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
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            [this](const boost::system::error_code ec,
                   const std::vector<std::pair<
                       std::string,
                       std::variant<std::monostate, std::string, uint32_t>>>&
                       properties) -> void {
                if (ec)
                {
                    logSystemErrorCode(ec);
                    return;
                }
                try
                {
                    std::string greetings;
                    std::string goodbyes;
                    uint32_t value = 0u;
                    sdbusplus::unpackProperties(
                        properties, propertyGrettingName, greetings,
                        propertyGoodbyesName, goodbyes, propertyValueName,
                        value);

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
                    sdbusplus::unpackProperties(
                        properties, propertyGrettingName, notMatchingType);

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
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::unique_ptr<sdbusplus::asio::dbus_interface> demo_;
    std::string greetings_ = "Hello";
    std::string goodbyes_ = "Bye";

    uint32_t fatalErrors_ = 0u;
};

int main(int, char**)
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait([&ioc](const boost::system::error_code&, const int&) {
        ioc.stop();
    });

    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    auto objServer = std::make_unique<sdbusplus::asio::object_server>(bus);

    bus->request_name(demoServiceName.c_str());

    Application app(*bus, *objServer);

    boost::asio::post(ioc,
                      [&app] { app.asyncGetAllPropertiesStringTypeOnly(); });
    boost::asio::post(ioc, [&app] { app.asyncGetAllProperties(); });

    ioc.run();

    std::cout << "Fatal errors count: " << app.fatalErrors() << "\n";

    return app.fatalErrors();
}
