// Unit tests for sdbusplus::async::signal_action handling in
// sdbusplus::async::server::server (include/sdbusplus/async/server.hpp),
// mirroring test/server/object.cpp for the synchronous object API.
#include <sys/eventfd.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/server.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <memory>

#include <gtest/gtest.h>

using sdbusplus::async::signal_action;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::StrEq;

// Generated-like interface bases for server_t tests (FakeIface,
// BareEmitIface) plus emission-less BareIface and non-conforming
// NoBlockIface for the fold/block_emit_added contract probes.
template <typename Instance, typename Server>
struct FakeIface
{
    using properties_t = std::nullopt_t;
    using signal_action = sdbusplus::async::signal_action;

    friend Server;

    explicit FakeIface(const char* path,
                       signal_action act = signal_action::default_behavior) :
        iface_path(path),
        iface(*_mock_bus_ptr, path, "fake.Iface", _vtable, this)
    {
        check_action(act);
    }

    explicit FakeIface(const sdbusplus::object_path& path,
                       signal_action act = signal_action::default_behavior) :
        FakeIface(path.str.c_str(), act)
    {}

    FakeIface(const char* path, std::nullopt_t,
              signal_action act = signal_action::default_behavior) :
        FakeIface(path, act)
    {}
    FakeIface(const sdbusplus::object_path& path, std::nullopt_t,
              signal_action act = signal_action::default_behavior) :
        FakeIface(path, act)
    {}

    ~FakeIface()
    {
        emit_removed();
        if (signalstate == signal_action::emit_object_added)
        {
            _mock_bus_ptr->emit_object_removed(iface_path.c_str());
        }
    }

    sdbusplus::async::context& _context()
    {
        return *_mock_ctx_ptr;
    }

    void emit_object_added()
    {
        if (signalstate == signal_action::defer_emit && !added)
        {
            _mock_bus_ptr->emit_object_added(iface_path.c_str());
            signalstate = signal_action::emit_object_added;
        }
    }

    void emit_added()
    {
        if (!added && signalstate == signal_action::defer_emit)
        {
            _context().emit_interfaces_added(iface_path.c_str(),
                                             {"fake.Iface"});
            added = true;
            signalstate = signal_action::emit_no_signals;
        }
    }

    /** Whether this interface already sent 'interfaces-added'. */
    bool interfaces_added() const
    {
        return added;
    }

  private:
    /** Block manual emit_added(); server-only via friend Server. */
    void block_emit_added()
    {
        if (signalstate == signal_action::defer_emit)
        {
            signalstate = signal_action::emit_no_signals;
        }
    }

  public:
    void emit_removed()
    {
        if (added)
        {
            FakeIface<Instance, Server>::_mock_ctx_ptr->emit_interfaces_removed(
                iface_path.c_str(), {"fake.Iface2"});
            added = false;
        }
    }

    void check_action(signal_action act)
    {
        switch (act)
        {
            case signal_action::emit_object_added:
                if (!added)
                {
                    FakeIface<Instance, Server>::_mock_bus_ptr
                        ->emit_object_added(iface_path.c_str());
                    signalstate = signal_action::emit_object_added;
                }
                break;
            case signal_action::emit_interface_added:
                signalstate = signal_action::defer_emit;
                emit_added();
                break;
            case signal_action::defer_emit:
            case signal_action::default_behavior:
                signalstate = signal_action::defer_emit;
                break;
            case signal_action::emit_no_signals:
                signalstate = signal_action::emit_no_signals;
                break;
        }
    }

    std::string iface_path;
    signal_action signalstate = signal_action::defer_emit;
    bool added = false;
    sdbusplus::server::interface_t iface;

    // Server-only block_emit_added(); see friend Server above.
    static sdbusplus::bus_t* _mock_bus_ptr;
    static sdbusplus::async::context* _mock_ctx_ptr;

    friend Server;

    static void set_mock_targets(sdbusplus::async::context& ctx)
    {
        _mock_bus_ptr = &ctx.get_bus();
        _mock_ctx_ptr = &ctx;
    }
    static constexpr sdbusplus::vtable_t _vtable[] = {
        sdbusplus::vtable::start(), sdbusplus::vtable::end()};
};

template <typename Instance, typename Server>
sdbusplus::bus_t* FakeIface<Instance, Server>::_mock_bus_ptr = nullptr;

template <typename Instance, typename Server>
sdbusplus::async::context* FakeIface<Instance, Server>::_mock_ctx_ptr = nullptr;

// No emit_added(): tolerated by the fold.  Ignores trailing action.
template <typename Instance, typename Server>
struct BareIface
{
    using properties_t = std::nullopt_t;

    explicit BareIface(const char*,
                       signal_action = signal_action::default_behavior)
    {}
    explicit BareIface(const sdbusplus::object_path&,
                       signal_action = signal_action::default_behavior)
    {}
    BareIface(const char*, std::nullopt_t,
              signal_action = signal_action::default_behavior)
    {}
    BareIface(const sdbusplus::object_path&, std::nullopt_t,
              signal_action = signal_action::default_behavior)
    {}
};

// Second base for multi-interface servers ("fake.Iface2").
template <typename Instance, typename Server>
struct BareEmitIface
{
    using properties_t = std::nullopt_t;

    friend Server;

    explicit BareEmitIface(
        const char* path, signal_action act = signal_action::default_behavior) :
        iface_path(path)
    {
        check_action(act);
    }
    explicit BareEmitIface(
        const sdbusplus::object_path& path,
        signal_action act = signal_action::default_behavior) :
        BareEmitIface(path.str.c_str(), act)
    {}
    BareEmitIface(const char* path, std::nullopt_t,
                  signal_action act = signal_action::default_behavior) :
        BareEmitIface(path, act)
    {}
    BareEmitIface(const sdbusplus::object_path& path, std::nullopt_t,
                  signal_action act = signal_action::default_behavior) :
        BareEmitIface(path, act)
    {}

    ~BareEmitIface()
    {
        emit_removed();
        if (signalstate == signal_action::emit_object_added)
        {
            FakeIface<Instance, Server>::_mock_bus_ptr->emit_object_removed(
                iface_path.c_str());
        }
    }

    void emit_added()
    {
        if (!added && signalstate == signal_action::defer_emit)
        {
            FakeIface<Instance, Server>::_mock_ctx_ptr->emit_interfaces_added(
                iface_path.c_str(), {"fake.Iface2"});
            added = true;
            signalstate = signal_action::emit_no_signals;
        }
    }

    /** Whether this interface already sent 'interfaces-added'. */
    bool interfaces_added() const
    {
        return added;
    }

  private:
    /** Block manual emit_added(); server-only via friend Server. */
    void block_emit_added()
    {
        if (signalstate == signal_action::defer_emit)
        {
            signalstate = signal_action::emit_no_signals;
        }
    }

  public:
    void emit_removed()
    {
        if (added)
        {
            FakeIface<Instance, Server>::_mock_ctx_ptr->emit_interfaces_removed(
                iface_path.c_str(), {"fake.Iface2"});
            added = false;
        }
    }

    void emit_object_added()
    {
        if (signalstate == signal_action::defer_emit && !added)
        {
            FakeIface<Instance, Server>::_mock_bus_ptr->emit_object_added(
                iface_path.c_str());
            signalstate = signal_action::emit_object_added;
        }
    }

    void check_action(signal_action act)
    {
        switch (act)
        {
            case signal_action::emit_object_added:
                if (!added)
                {
                    FakeIface<Instance, Server>::_mock_bus_ptr
                        ->emit_object_added(iface_path.c_str());
                    signalstate = signal_action::emit_object_added;
                }
                break;
            case signal_action::emit_interface_added:
                signalstate = signal_action::defer_emit;
                emit_added();
                break;
            case signal_action::defer_emit:
            case signal_action::default_behavior:
                signalstate = signal_action::defer_emit;
                break;
            case signal_action::emit_no_signals:
                signalstate = signal_action::emit_no_signals;
                break;
        }
    }

    std::string iface_path;
    signal_action signalstate = signal_action::defer_emit;
    bool added = false;
};

// try_block's static_assert needs this: emit_added() without
// block_emit_added().  Never instantiated; probed via SFINAE traits below.
template <typename Instance, typename Server>
struct NoBlockIface
{
    using properties_t = std::nullopt_t;

    explicit NoBlockIface(const char*,
                          signal_action = signal_action::default_behavior)
    {}
    explicit NoBlockIface(const sdbusplus::object_path&,
                          signal_action = signal_action::default_behavior)
    {}
    NoBlockIface(const char*, std::nullopt_t,
                 signal_action = signal_action::default_behavior)
    {}
    NoBlockIface(const sdbusplus::object_path&, std::nullopt_t,
                 signal_action = signal_action::default_behavior)
    {}

    void emit_added() {}
};

// Member probes (&T::member is a hard error when absent).
template <typename T, typename = void>
struct HasEmitAdded : std::false_type
{};
template <typename T>
struct HasEmitAdded<T, std::void_t<decltype(&T::emit_added)>> : std::true_type
{};
template <typename T, typename = void>
struct HasBlockEmitAdded : std::false_type
{};
template <typename T>
struct HasBlockEmitAdded<T, std::void_t<decltype(&T::block_emit_added)>> :
    std::true_type
{};

struct FakeInstance
{};

using FakeServer = sdbusplus::async::server_t<FakeInstance, FakeIface>;
using BareServer = sdbusplus::async::server_t<FakeInstance, BareIface>;
using TwoIfaceServer =
    sdbusplus::async::server_t<FakeInstance, FakeIface, BareEmitIface>;

class AsyncSignalAction : public ::testing::Test
{
  protected:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus_t bus = sdbusplus::get_mocked_new(&sdbusMock);

    static constexpr auto objPath = "/xyz/openbmc_project/sdbusplus/test";
    static constexpr auto busName = "xyz.openbmc_project.sdbusplus.test.Async";

    int eventFd = -1;

    void SetUp() override
    {
        FakeIface<FakeInstance, FakeServer>::_mock_bus_ptr = nullptr;
        FakeIface<FakeInstance, FakeServer>::_mock_ctx_ptr = nullptr;
        // Real eventfd for the context's event loop (AtMost(1): the
        // RogueBase test builds no context).
        eventFd = eventfd(0, EFD_NONBLOCK);
        ASSERT_GE(eventFd, 0);
        EXPECT_CALL(sdbusMock, sd_bus_get_fd(_))
            .Times(::testing::AtMost(1))
            .WillOnce(Return(eventFd));
        EXPECT_CALL(sdbusMock, sd_bus_request_name(_, _, _)).Times(AnyNumber());
    }

    void TearDown() override
    {
        if (eventFd >= 0)
        {
            close(eventFd);
        }
    }

    // Context owns the bus; register per-Server statics.
    std::unique_ptr<sdbusplus::async::context> makeCtx()
    {
        auto ctx = std::make_unique<sdbusplus::async::context>(std::move(bus));
        FakeIface<FakeInstance, FakeServer>::set_mock_targets(*ctx);
        FakeIface<FakeInstance, TwoIfaceServer>::set_mock_targets(*ctx);
        return ctx;
    }
};

TEST_F(AsyncSignalAction, DefaultIsDeferEmit)
{
    // Default: nothing automatic, silent destruction.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath);
    }
}

TEST_F(AsyncSignalAction, DefaultThenManualEmit)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto srv = std::make_unique<FakeServer>(*ctx, objPath);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    srv->emit_object_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    srv.reset();
}

TEST_F(AsyncSignalAction, EmitObjectAdded)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath, signal_action::emit_object_added);
    }
}

TEST_F(AsyncSignalAction, EmitInterfaceAdded)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(1);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath, signal_action::emit_interface_added);
    }
}

TEST_F(AsyncSignalAction, DeferEmit)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto srv =
        std::make_unique<FakeServer>(*ctx, objPath, signal_action::defer_emit);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    srv->emit_object_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    srv.reset();
}

TEST_F(AsyncSignalAction, DeferEmitWithoutManualEmitStaysSilent)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath, signal_action::defer_emit);
    }
}

TEST_F(AsyncSignalAction, DeferThenEmitInterfacesAdded)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto srv =
        std::make_unique<FakeServer>(*ctx, objPath, signal_action::defer_emit);

    // Manual emit: 1x InterfacesAdded; destruction pairs InterfacesRemoved
    // with no ObjectRemoved.  Double-emit is a no-op.
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    srv->emit_interfaces_added();
    srv->emit_interfaces_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(1);
    srv.reset();
}

TEST_F(AsyncSignalAction, EmitNoSignals)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath, signal_action::emit_no_signals);
    }
}

TEST_F(AsyncSignalAction, EmitNoSignalsBlocksManualEmitAdded)
{
    // emit_no_signals refuses later manual emission.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv(*ctx, objPath, signal_action::emit_no_signals);
        srv.emit_interfaces_added();
    }
}
TEST_F(AsyncSignalAction, DeferPartialThenServerFoldEmitsRemainder)
{
    // One base emitted manually; the server fold emits only the other.
    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto srv = std::make_unique<TwoIfaceServer>(*ctx, objPath,
                                                signal_action::defer_emit);

    // Manually emit the first base only.
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(1);
    static_cast<FakeIface<FakeInstance, TwoIfaceServer>*>(srv.get())
        ->emit_added();

    // Fold emits the remaining interface; VerifyAndClear separates the two
    // emissions for counting.
    ::testing::Mock::VerifyAndClearExpectations(&sdbusMock);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(1);
    srv->emit_interfaces_added();

    // Destruction: 2x InterfacesRemoved, no ObjectRemoved.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(2);
    srv.reset();
}

TEST_F(AsyncSignalAction, EmitInterfaceAddedWithoutEmitInterfaceAdded)
{
    // try_emit skips bases without emit_added(); construction succeeds.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        BareServer srv(*ctx, objPath, signal_action::emit_interface_added);
    }
}

TEST_F(AsyncSignalAction, PropertiesAndObjectPathOverloads)
{
    // properties_t / object_path ctor overloads with explicit actions.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(2);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        FakeServer srv1(*ctx, objPath, std::nullopt,
                        signal_action::emit_object_added);
        FakeServer srv2(*ctx, sdbusplus::object_path(objPath), std::nullopt,
                        signal_action::emit_object_added);
        FakeServer srv3(*ctx, sdbusplus::object_path(objPath),
                        signal_action::emit_no_signals);
    }
}

TEST_F(AsyncSignalAction, RogueBaseWithoutBlockFailsToCompose)
{
    // Probes never instantiate the rogue composition (that would trip the
    // static_assert and break the build).
    static_assert(
        requires(FakeServer& s) { s.emit_object_added(); },
        "test harness sanity: server must have emit_object_added");
    constexpr bool rogue_can_emit =
        HasEmitAdded<NoBlockIface<FakeInstance, FakeServer>>::value;
    constexpr bool rogue_can_block =
        HasBlockEmitAdded<NoBlockIface<FakeInstance, FakeServer>>::value;
    static_assert(rogue_can_emit, "rogue type must have emit_added");
    static_assert(!rogue_can_block, "rogue type must lack block_emit_added");
    EXPECT_TRUE(rogue_can_emit);
    EXPECT_FALSE(rogue_can_block)
        << "NoBlockIface gained block_emit_added; delete the rogue type and "
           "this test";
}
