#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

const std::string demoServiceName = "demo.service";
const std::string demoObjectPath = "/xyz/demo";
const std::string demoInterfaceName = "xyz.demo";
const std::string propertyGrettingName = "Greetings";
const std::string propertyGoodbyesName = "Goodbyes";

class Application
{
  public:
    Application(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
                sdbusplus::asio::object_server& objServer) :
        ioc_(ioc), bus_(bus), objServer_(objServer)
    {
        demo_ = objServer_.add_unique_interface(
            demoObjectPath, demoInterfaceName,
            [this](sdbusplus::asio::dbus_interface& demo) {
                demo.register_property_r<std::string>(
                    propertyGrettingName, sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return greetings_; });

                demo.register_property_rw<std::string>(
                    propertyGoodbyesName,
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newPropertyValue, const auto&) {
                        goodbyes_ = newPropertyValue;
                        return true;
                    },
                    [this](const auto&) { return goodbyes_; });
            });
    }

    uint32_t fatalErrors() const
    {
        return fatalErrors_;
    }

    auto getFailed()
    {
        return [this](boost::system::error_code error) {
            std::cerr << "Error: getProperty failed " << error << "\n";
            ++fatalErrors_;
        };
    }

    void asyncReadPropertyWithIncorrectType()
    {
        sdbusplus::asio::getProperty<uint32_t>(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            propertyGrettingName,
            [this](boost::system::error_code ec, uint32_t) {
                if (ec)
                {
                    std::cout
                        << "As expected failed to getProperty with wrong type: "
                        << ec << "\n";
                    return;
                }

                std::cerr << "Error: it was expected to fail getProperty due "
                             "to wrong type\n";
                ++fatalErrors_;
            });
    }

    void asyncReadProperties()
    {
        sdbusplus::asio::getProperty<std::string>(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            propertyGrettingName,
            [this](boost::system::error_code ec, std::string value) {
                if (ec)
                {
                    getFailed();
                    return;
                }
                std::cout << "Greetings value is: " << value << "\n";
            });

        sdbusplus::asio::getProperty<std::string>(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            propertyGoodbyesName,
            [this](boost::system::error_code ec, std::string value) {
                if (ec)
                {
                    getFailed();
                    return;
                }
                std::cout << "Goodbyes value is: " << value << "\n";
            });
    }

    void asyncChangeProperty()
    {
        sdbusplus::asio::setProperty(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            propertyGrettingName, "Hi, hey, hello",
            [this](const boost::system::error_code& ec) {
                if (ec)
                {
                    std::cout
                        << "As expected, failed to set greetings property: "
                        << ec << "\n";
                    return;
                }

                std::cout
                    << "Error: it was expected to fail to change greetings\n";
                ++fatalErrors_;
            });

        sdbusplus::asio::setProperty(
            bus_, demoServiceName, demoObjectPath, demoInterfaceName,
            propertyGoodbyesName, "Bye bye",
            [this](const boost::system::error_code& ec) {
                if (ec)
                {
                    std::cout
                        << "Error: it supposed to be ok to change goodbyes "
                           "property: "
                        << ec << "\n";
                    ++fatalErrors_;
                    return;
                }
                std::cout << "Changed goodbyes property as expected\n";
                boost::asio::post(ioc_, [this] { asyncReadProperties(); });
            });
    }

    void syncChangeGoodbyes(std::string_view value)
    {
        goodbyes_ = value;
        demo_->signal_property(propertyGoodbyesName);
    }

  private:
    boost::asio::io_context& ioc_;
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

    Application app(ioc, *bus, *objServer);

    app.syncChangeGoodbyes("Good bye");

    boost::asio::post(ioc,
                      [&app] { app.asyncReadPropertyWithIncorrectType(); });
    boost::asio::post(ioc, [&app] { app.asyncReadProperties(); });
    boost::asio::post(ioc, [&app] { app.asyncChangeProperty(); });

    ioc.run();

    std::cout << "Fatal errors count: " << app.fatalErrors() << "\n";

    return app.fatalErrors();
}
