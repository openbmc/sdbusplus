#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus.hpp>

#include <iostream>

class Application
{
  public:
    Application(boost::asio::io_context& ioc, sdbusplus::asio::connection& bus,
                sdbusplus::asio::object_server& objServer) :
        ioc_(ioc),
        bus_(bus), objServer_(objServer)
    {
        demo_ = objServer_.add_unique_interface(
            "/xyz/demo", "xyz.demo",
            [this](sdbusplus::asio::dbus_interface& demo) {
                demo.register_property_r(
                    "StringProperty", std::string{},
                    sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return "Text"; });

                demo.register_property_r("DoubleProperty", double{},
                                         sdbusplus::vtable::property_::const_,
                                         [this](const auto&) { return 42.7; });
            });
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::unique_ptr<sdbusplus::asio::dbus_interface> demo_;
};

int main(int, char**)
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait(
        [&ioc](const boost::system::error_code&, const int&) { ioc.stop(); });

    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    auto objServer = std::make_unique<sdbusplus::asio::object_server>(bus);

    bus->request_name("demo.service");

    Application app(ioc, *bus, *objServer);

    std::cout << "\n";

    boost::asio::post(ioc, [&bus] {
        bus->async_method_call(
            [](boost::system::error_code ec,
               std::vector<std::pair<std::string, std::variant<std::string>>>
                   properties) {
                std::cout << "variant<string>\n";

                if (ec)
                {
                    std::cout << "error: " << ec << "\n";
                    return;
                }

                for (const auto& [key, valueVariant] : properties)
                {
                    if (const std::string* value =
                            std::get_if<std::string>(&valueVariant))
                    {
                        std::cout << key << " = " << *value << "\n";
                    }
                    else
                    {
                        std::cout << key << " std::get_if failed\n";
                    }
                }

                std::cout << "\n";
            },
            "demo.service", "/xyz/demo", "org.freedesktop.DBus.Properties",
            "GetAll", "xyz.demo");
    });

    boost::asio::post(ioc, [&bus] {
        bus->async_method_call(
            [](boost::system::error_code ec,
               std::vector<std::pair<std::string,
                                     std::variant<std::monostate, std::string>>>
                   properties) {
                std::cout << "variant<monostate, string>\n";
                if (ec)
                {
                    std::cout << "error: " << ec << "\n";
                    return;
                }

                for (const auto& [key, valueVariant] : properties)
                {
                    if (const std::string* value =
                            std::get_if<std::string>(&valueVariant))
                    {
                        std::cout << key << " = " << *value << "\n";
                    }
                    else
                    {
                        std::cout << key << " std::get_if failed\n";
                    }
                }

                std::cout << "\n";
            },
            "demo.service", "/xyz/demo", "org.freedesktop.DBus.Properties",
            "GetAll", "xyz.demo");
    });

    boost::asio::post(ioc, [&bus] {
        bus->async_method_call(
            [](boost::system::error_code ec,
               std::vector<
                   std::pair<std::string, std::variant<std::string, double>>>
                   properties) {
                std::cout << "variant<string, double>\n";
                if (ec)
                {
                    std::cout << "error: " << ec << "\n";
                    return;
                }

                for (const auto& [key, valueVariant] : properties)
                {
                    if (const std::string* value =
                            std::get_if<std::string>(&valueVariant))
                    {
                        std::cout << key << " = " << *value << "\n";
                    }
                    else if (const double* value =
                                 std::get_if<double>(&valueVariant))
                    {
                        std::cout << key << " = " << *value << "\n";
                    }
                    else
                    {
                        std::cout << key << " std::get_if failed\n";
                    }
                }

                std::cout << "\n";
            },
            "demo.service", "/xyz/demo", "org.freedesktop.DBus.Properties",
            "GetAll", "xyz.demo");
    });

    ioc.run();

    return 0;
}
