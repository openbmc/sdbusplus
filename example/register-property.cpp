#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
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

    template <class Handler>
    void async_get(Handler&& handler)
    {
        sdbusplus::asio::getProperty<T>(bus_, service_, path_, interface_,
                                        name_, std::forward<Handler>(handler));
    }

    template <class Handler>
    void async_set(const T& value, Handler&& handler)
    {
        sdbusplus::asio::setProperty(bus_, service_, path_, interface_, name_,
                                     value, std::forward<Handler>(handler));
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
        demo_ = objServer_.add_unique_interface(
            xyz::demo::path, xyz::demo::name,
            [this](sdbusplus::asio::dbus_interface& demo) {
                demo.register_property_r(
                    name::greetings, std::string(),
                    sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return greetings_; });

                demo.register_property_rw(
                    name::goodbyes, std::string(),
                    sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newPropertyValue, const auto&) {
                        goodbyes_ = newPropertyValue;
                        return 1;
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
        utils::Property<uint32_t> propertyWithWrongType{
            bus_, xyz::demo::name, xyz::demo::path, xyz::demo::name,
            name::greetings};

        propertyWithWrongType.async_get(
            [this](boost::system::error_code error, uint32_t) {
                if (error)
                {
                    std::cout
                        << "As expected failed to getProperty with wrong type: "
                        << error << "\n";
                    return;
                }

                std::cerr << "Error: it was expected to fail getProperty due "
                             "to wrong type\n";
                ++fatalErrors_;
            });
    }

    void asyncReadProperties()
    {
        propertyGreetings.async_get(
            [this](boost::system::error_code ec, std::string value) {
                if (ec)
                {
                    getFailed();
                    return;
                }
                std::cout << "Greetings value is: " << value << "\n";
            });

        propertyGoodbyes.async_get(
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
        propertyGreetings.async_set(
            "Hi, hey, hello", [this](const boost::system::error_code& error) {
                if (error)
                {
                    std::cout
                        << "As expected, failed to set greetings property: "
                        << error << "\n";
                    return;
                }

                std::cout
                    << "Error: it was expected to fail to change greetings\n";
                ++fatalErrors_;
            });

        propertyGoodbyes.async_set(
            "Bye bye", [this](const boost::system::error_code& error) {
                if (error)
                {
                    std::cout
                        << "Error: it supposed to be ok to change goodbyes "
                           "property: "
                        << error << "\n";
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
        demo_->signal_property(name::goodbyes);
    }

  private:
    boost::asio::io_context& ioc_;
    sdbusplus::asio::connection& bus_;
    sdbusplus::asio::object_server& objServer_;

    std::unique_ptr<sdbusplus::asio::dbus_interface> demo_;
    std::string greetings_ = "Hello";
    std::string goodbyes_ = "Bye";

    uint32_t fatalErrors_ = 0u;

    utils::Property<std::string> propertyGreetings{
        bus_, xyz::demo::name, xyz::demo::path, xyz::demo::name,
        name::greetings};
    utils::Property<std::string> propertyGoodbyes{
        bus_, xyz::demo::name, xyz::demo::path, xyz::demo::name,
        name::goodbyes};
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

    app.syncChangeGoodbyes("Good bye");

    boost::asio::post(ioc,
                      [&app] { app.asyncReadPropertyWithIncorrectType(); });
    boost::asio::post(ioc, [&app] { app.asyncReadProperties(); });
    boost::asio::post(ioc, [&app] { app.asyncChangeProperty(); });

    ioc.run();

    std::cout << "Fatal errors count: " << app.fatalErrors() << "\n";

    return app.fatalErrors();
}
