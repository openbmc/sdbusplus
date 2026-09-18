// Exercise signal_action with REAL generated aserver types on a mocked bus:
// direct details inheritance, direct outer-wrapper inheritance, and
// single/multi-interface server_t.
#include "server/Test/aserver.hpp"
#include "server/Test2/aserver.hpp"

#include <sys/eventfd.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/server.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <memory>

#include <gtest/gtest.h>

using sdbusplus::async::signal_action;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::StrEq;

struct Instance
{};

// details::<Iface> hosted directly (needs friend server_context_friend,
// like server_t).
class MiniServer :
    public sdbusplus::async::context_ref,
    public sdbusplus::aserver::server::details::Test<MiniServer, MiniServer>
{
    friend sdbusplus::async::server::details::server_context_friend;

  public:
    using Self = MiniServer;
    using Base = sdbusplus::aserver::server::details::Test<Self, Self>;

    explicit MiniServer(sdbusplus::async::context& ctx, const char* path,
                        signal_action act = signal_action::default_behavior) :
        context_ref(ctx), Base(path, act)
    {}
};

// Direct outer-wrapper inheritance (calculator-aserver.cpp pattern).
class Direct : public sdbusplus::aserver::server::Test<Direct>
{
  public:
    explicit Direct(sdbusplus::async::context& ctx, const char* path,
                    signal_action act = signal_action::default_behavior) :
        sdbusplus::aserver::server::Test<Direct>(ctx, path, act)
    {}
};

// Single- and multi-interface server_t over generated types.
using Single =
    sdbusplus::async::server_t<Instance, sdbusplus::aserver::server::Test>;

using Multi =
    sdbusplus::async::server_t<Instance, sdbusplus::aserver::server::Test,
                               sdbusplus::aserver::server::Test2>;

class AServerSignalAction : public ::testing::Test
{
  protected:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus_t bus = sdbusplus::get_mocked_new(&sdbusMock);

    static constexpr auto objPath = "/xyz/openbmc_project/sdbusplus/test";
    static constexpr auto busName = "xyz.openbmc_project.sdbusplus.test.Async";

    int eventFd = -1;

    void SetUp() override
    {
        eventFd = eventfd(0, EFD_NONBLOCK);
        ASSERT_GE(eventFd, 0);
        EXPECT_CALL(sdbusMock, sd_bus_get_fd(_)).WillOnce(Return(eventFd));
        EXPECT_CALL(sdbusMock, sd_bus_request_name(_, _, _)).Times(AnyNumber());
    }

    void TearDown() override
    {
        if (eventFd >= 0)
        {
            close(eventFd);
        }
    }

    std::unique_ptr<sdbusplus::async::context> makeCtx()
    {
        return std::make_unique<sdbusplus::async::context>(std::move(bus));
    }
};

TEST_F(AServerSignalAction, StandaloneDetailsEmitObjectAdded)
{
    // A details::<Iface> used directly (not through server_t or the outer
    // wrapper) handles the action itself: object-added on construction and
    // object-removed on destruction.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        MiniServer m(*ctx, objPath, signal_action::emit_object_added);
    }
}

TEST_F(AServerSignalAction, StandaloneDetailsEmitObjectThenInterfaceRefused)
{
    // emit_added() after object-added is refused: no dual added signals.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        MiniServer m(*ctx, objPath, signal_action::emit_object_added);
        m.emit_added();
    }
}
TEST_F(AServerSignalAction, StandaloneDetailsEmitInterfaceAdded)
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
        MiniServer m(*ctx, objPath, signal_action::emit_interface_added);
    }
}

TEST_F(AServerSignalAction, StandaloneDetailsDefaultDefersEmit)
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
        MiniServer m(*ctx, objPath);
    }
}

TEST_F(AServerSignalAction, StandaloneDetailsDefaultThenManualEmit)
{
    // Default is defer_emit: manual emit pairs with object_removed.
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto m = std::make_unique<MiniServer>(*ctx, objPath);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    m->emit_object_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    m.reset();
}

TEST_F(AServerSignalAction, DirectEmitInterfaceAdded)
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
        Direct d(*ctx, objPath, signal_action::emit_interface_added);
    }
}

TEST_F(AServerSignalAction, DirectDefaultDefersEmit)
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
        Direct d(*ctx, objPath);
    }
}

TEST_F(AServerSignalAction, SingleEmitObjectAdded)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        Single s(*ctx, objPath, signal_action::emit_object_added);
    }
}

TEST_F(AServerSignalAction, MultiEmitInterfaceAdded)
{
    // Each of the two generated interfaces self-emits; no object-level
    // signals, so no object-level remove on destruction either.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(2);

    {
        auto ctx = makeCtx();
        ctx->request_name(busName);
        Multi m(*ctx, objPath, signal_action::emit_interface_added);
    }
}

TEST_F(AServerSignalAction, MultiDeferThenEmit)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto m = std::make_unique<Multi>(*ctx, objPath, signal_action::defer_emit);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    m->emit_object_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    m.reset();
}

TEST_F(AServerSignalAction, ServerObjectThenInterfaceRefused)
{
    // Interface emit after server object_added is refused.
    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto m = std::make_unique<Multi>(*ctx, objPath, signal_action::defer_emit);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    m->emit_object_added();

    // Refused: no interfaces signals after the object-level added.
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_removed_strv(_, _, _))
        .Times(0);
    static_cast<sdbusplus::aserver::server::details::Test<Instance, Multi>*>(
        m.get())
        ->emit_added();

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    m.reset();
}

TEST_F(AServerSignalAction, MultiDeferWithoutManualEmitStaysSilent)
{
    // No added implies no removed.
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
        Multi m(*ctx, objPath, signal_action::defer_emit);
    }
}

TEST_F(AServerSignalAction, MultiDeferThenEmitInterfacesAdded)
{
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_interfaces_added_strv(_, _, _)).Times(0);

    auto ctx = makeCtx();
    ctx->request_name(busName);
    auto m = std::make_unique<Multi>(*ctx, objPath, signal_action::defer_emit);

    // One InterfacesAdded per contained interface.
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    m->emit_interfaces_added();

    // InterfacesRemoved only, no ObjectRemoved.
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(2);
    m.reset();
}
