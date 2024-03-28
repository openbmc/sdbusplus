#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <cerrno>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using testing::DoAll;
using testing::ElementsAre;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

ACTION_TEMPLATE(AssignReadVal, HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(val))
{
    *static_cast<T*>(arg2) = val;
}

class ReadTest : public testing::Test
{
  protected:
    testing::StrictMock<sdbusplus::SdBusMock> mock;

    void SetUp() override
    {
        EXPECT_CALL(mock, sd_bus_message_new_method_call(testing::_, testing::_,
                                                         nullptr, nullptr,
                                                         nullptr, nullptr))
            .WillRepeatedly(Return(0));
    }

    sdbusplus::message_t new_message()
    {
        return sdbusplus::get_mocked_new(&mock).new_method_call(
            nullptr, nullptr, nullptr, nullptr);
    }

    void expect_basic_error(char type, int ret)
    {
        EXPECT_CALL(mock, sd_bus_message_read_basic(nullptr, type, testing::_))
            .WillOnce(Return(ret));
    }

    template <typename T>
    void expect_basic(char type, T val)
    {
        EXPECT_CALL(mock, sd_bus_message_read_basic(nullptr, type, testing::_))
            .WillOnce(DoAll(AssignReadVal<T>(val), Return(0)));
    }

    template <typename T>
    static void read_array_callback(sd_bus_message*, char, const void** p,
                                    size_t* sz)
    {
        static const std::array<T, 3> arr{std::numeric_limits<T>::min(), 0,
                                          std::numeric_limits<T>::max()};

        *p = arr.data();
        *sz = arr.size() * sizeof(T);
    }

    template <typename T>
    void expect_read_array(char type)
    {
        EXPECT_CALL(mock, sd_bus_message_read_array(nullptr, type, testing::_,
                                                    testing::_))
            .WillOnce(
                DoAll(testing::Invoke(read_array_callback<T>), Return(0)));
    }

    void expect_verify_type(char type, const char* contents, int ret)
    {
        EXPECT_CALL(mock,
                    sd_bus_message_verify_type(nullptr, type, StrEq(contents)))
            .WillOnce(Return(ret));
    }

    void expect_at_end(bool complete, int ret)
    {
        EXPECT_CALL(mock, sd_bus_message_at_end(nullptr, complete))
            .WillOnce(Return(ret));
    }

    void expect_skip(const char* contents, int ret = 0)
    {
        EXPECT_CALL(mock, sd_bus_message_skip(nullptr, StrEq(contents)))
            .WillOnce(Return(ret));
    }

    void expect_enter_container(char type, const char* contents, int ret = 0)
    {
        EXPECT_CALL(mock, sd_bus_message_enter_container(nullptr, type,
                                                         StrEq(contents)))
            .WillOnce(Return(ret));
    }

    void expect_exit_container(int ret = 0)
    {
        EXPECT_CALL(mock, sd_bus_message_exit_container(nullptr))
            .WillOnce(Return(ret));
    }
};

TEST_F(ReadTest, Int)
{
    const int i = 1;
    expect_basic<int>(SD_BUS_TYPE_INT32, i);
    int ret;
    new_message().read(ret);
    EXPECT_EQ(i, ret);
}

TEST_F(ReadTest, Bool)
{
    const bool b = true;
    expect_basic<int>(SD_BUS_TYPE_BOOLEAN, b);
    bool ret;
    new_message().read(ret);
    EXPECT_EQ(b, ret);
}

TEST_F(ReadTest, Double)
{
    const double d = 1.1;
    expect_basic<double>(SD_BUS_TYPE_DOUBLE, d);
    double ret;
    new_message().read(ret);
    EXPECT_EQ(d, ret);
}

TEST_F(ReadTest, CString)
{
    const char* const s = "asdf";
    expect_basic<const char*>(SD_BUS_TYPE_STRING, s);
    const char* ret;
    new_message().read(ret);
    EXPECT_EQ(s, ret);
}

TEST_F(ReadTest, String)
{
    const char* const s = "fsda";
    expect_basic<const char*>(SD_BUS_TYPE_STRING, s);
    std::string ret;
    new_message().read(ret);
    // Pointer comparison here is intentional as we don't expect a copy
    EXPECT_EQ(s, ret);
}

TEST_F(ReadTest, ObjectPath)
{
    const char* const s = "/fsda";
    expect_basic<const char*>(SD_BUS_TYPE_OBJECT_PATH, s);
    sdbusplus::message::object_path ret;
    new_message().read(ret);
    EXPECT_EQ(s, ret.str);
}

TEST_F(ReadTest, Signature)
{
    const char* const s = "{ii}";
    expect_basic<const char*>(SD_BUS_TYPE_SIGNATURE, s);
    sdbusplus::message::signature ret;
    new_message().read(ret);
    EXPECT_EQ(s, ret.str);
}

TEST_F(ReadTest, UnixFd)
{
    const int fd = 42;
    expect_basic<int>(SD_BUS_TYPE_UNIX_FD, fd);
    sdbusplus::message::unix_fd ret;
    new_message().read(ret);
    EXPECT_EQ(fd, ret);
}

TEST_F(ReadTest, CombinedBasic)
{
    const double a = 2.2;
    const char* const b = "ijkd";
    const bool c = false;
    const int d = 18;

    {
        testing::InSequence seq;
        expect_basic<double>(SD_BUS_TYPE_DOUBLE, a);
        expect_basic<const char*>(SD_BUS_TYPE_STRING, b);
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, c);
        expect_basic<int>(SD_BUS_TYPE_INT32, d);
    }

    double ret_a;
    const char* ret_b;
    bool ret_c;
    int ret_d;
    new_message().read(ret_a, ret_b, ret_c, ret_d);
    EXPECT_EQ(a, ret_a);
    EXPECT_EQ(b, ret_b);
    EXPECT_EQ(c, ret_c);
    EXPECT_EQ(d, ret_d);
}

TEST_F(ReadTest, BasicError)
{
    expect_basic_error(SD_BUS_TYPE_INT32, -EINVAL);
    int ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, BasicStringError)
{
    expect_basic_error(SD_BUS_TYPE_STRING, -EINVAL);
    std::string ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, BasicStringWrapperError)
{
    expect_basic_error(SD_BUS_TYPE_SIGNATURE, -EINVAL);
    sdbusplus::message::signature ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, BasicBoolError)
{
    expect_basic_error(SD_BUS_TYPE_BOOLEAN, -EINVAL);
    bool ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, Vector)
{
    const std::vector<std::string> vs{"1", "2", "3", "4"};

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& i : vs)
        {
            expect_at_end(false, 0);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, i.c_str());
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::vector<std::string> ret_vs;
    new_message().read(ret_vs);
    EXPECT_EQ(vs, ret_vs);
}

TEST_F(ReadTest, VectorUnsignedIntegral8)
{
    expect_read_array<uint8_t>(SD_BUS_TYPE_BYTE);
    std::vector<uint8_t> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(0, 0, 255));
}

TEST_F(ReadTest, VectorIntegral32)
{
    expect_read_array<int32_t>(SD_BUS_TYPE_INT32);
    std::vector<int32_t> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(-2147483648, 0, 2147483647));
}

TEST_F(ReadTest, VectorIntegral64)
{
    expect_read_array<int64_t>(SD_BUS_TYPE_INT64);
    std::vector<int64_t> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(-9223372036854775807LL - 1, 0,
                                    9223372036854775807LL));
}

TEST_F(ReadTest, VectorUnsignedIntegral32)
{
    expect_read_array<uint32_t>(SD_BUS_TYPE_UINT32);
    std::vector<uint32_t> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(0, 0, 4294967295));
}

TEST_F(ReadTest, VectorUnsignedIntegral64)
{
    expect_read_array<uint64_t>(SD_BUS_TYPE_UINT64);
    std::vector<uint64_t> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(0, 0, 18446744073709551615ULL));
}

TEST_F(ReadTest, VectorDouble)
{
    expect_read_array<double>(SD_BUS_TYPE_DOUBLE);
    std::vector<double> ret_vi;
    new_message().read(ret_vi);
    EXPECT_THAT(ret_vi, ElementsAre(2.2250738585072014e-308, 0,
                                    1.7976931348623157e+308));
}

TEST_F(ReadTest, VectorEnterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s", -EINVAL);
    }

    std::vector<std::string> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, VectorIterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        expect_at_end(false, 0);
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "1");
        expect_at_end(false, -EINVAL);
    }

    std::vector<std::string> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, VectorExitError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        expect_at_end(false, 0);
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "1");
        expect_at_end(false, 0);
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "2");
        expect_at_end(false, 1);
        expect_exit_container(-EINVAL);
    }

    std::vector<std::string> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, Set)
{
    const std::set<std::string> ss{"one", "two", "eight"};

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& s : ss)
        {
            expect_at_end(false, 0);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, s.c_str());
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::set<std::string> ret_ss;
    new_message().read(ret_ss);
    EXPECT_EQ(ss, ret_ss);
}

TEST_F(ReadTest, UnorderedSet)
{
    const std::unordered_set<std::string> ss{"one", "two", "eight"};

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& s : ss)
        {
            expect_at_end(false, 0);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, s.c_str());
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::unordered_set<std::string> ret_ss;
    new_message().read(ret_ss);
    EXPECT_EQ(ss, ret_ss);
}

TEST_F(ReadTest, Map)
{
    const std::map<int, std::string> mis{
        {1, "a"},
        {2, "bc"},
        {3, "def"},
        {4, "ghij"},
    };

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{is}");
        for (const auto& is : mis)
        {
            expect_at_end(false, 0);
            expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "is");
            expect_basic<int>(SD_BUS_TYPE_INT32, is.first);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, is.second.c_str());
            expect_exit_container();
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::map<int, std::string> ret_mis;
    new_message().read(ret_mis);
    EXPECT_EQ(mis, ret_mis);
}

TEST_F(ReadTest, MapEnterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{si}", -EINVAL);
    }

    std::map<std::string, int> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, MapEntryEnterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{si}");
        expect_at_end(false, 0);
        expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "si", -EINVAL);
    }

    std::map<std::string, int> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, MapEntryExitError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{si}");
        expect_at_end(false, 0);
        expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "si");
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "ab");
        expect_basic<int>(SD_BUS_TYPE_INT32, 1);
        expect_exit_container(-EINVAL);
    }

    std::map<std::string, int> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, MapIterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{si}");
        expect_at_end(false, 0);
        expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "si");
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "ab");
        expect_basic<int>(SD_BUS_TYPE_INT32, 1);
        expect_exit_container();
        expect_at_end(false, -EINVAL);
    }

    std::map<std::string, int> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, MapExitError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{si}");
        expect_at_end(false, 0);
        expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "si");
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "ab");
        expect_basic<int>(SD_BUS_TYPE_INT32, 1);
        expect_exit_container();
        expect_at_end(false, 1);
        expect_exit_container(-EINVAL);
    }

    std::map<std::string, int> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, UnorderedMap)
{
    const std::unordered_map<int, std::string> mis{
        {1, "a"},
        {2, "bc"},
        {3, "def"},
        {4, "ghij"},
    };

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "{is}");
        for (const auto& is : mis)
        {
            expect_at_end(false, 0);
            expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "is");
            expect_basic<int>(SD_BUS_TYPE_INT32, is.first);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, is.second.c_str());
            expect_exit_container();
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::unordered_map<int, std::string> ret_mis;
    new_message().read(ret_mis);
    EXPECT_EQ(mis, ret_mis);
}

TEST_F(ReadTest, Tuple)
{
    const std::tuple<int, std::string, bool> tisb{3, "hi", false};

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_STRUCT, "isb");
        expect_basic<int>(SD_BUS_TYPE_INT32, std::get<0>(tisb));
        expect_basic<const char*>(SD_BUS_TYPE_STRING,
                                  std::get<1>(tisb).c_str());
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, std::get<2>(tisb));
        expect_exit_container();
    }

    std::tuple<int, std::string, bool> ret_tisb;
    new_message().read(ret_tisb);
    EXPECT_EQ(tisb, ret_tisb);
}

TEST_F(ReadTest, TupleEnterError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_STRUCT, "bis", -EINVAL);
    }

    std::tuple<bool, int, std::string> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, TupleExitError)
{
    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_STRUCT, "bis");
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, false);
        expect_basic<int>(SD_BUS_TYPE_INT32, 1);
        expect_basic<const char*>(SD_BUS_TYPE_STRING, "ab");
        expect_exit_container(-EINVAL);
    }

    std::tuple<bool, int, std::string> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, Variant)
{
    const bool b1 = false;
    const std::string s2{"asdf"};
    const std::variant<int, std::string, bool> v1{b1}, v2{s2};

    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", false);
        expect_verify_type(SD_BUS_TYPE_VARIANT, "s", false);
        expect_verify_type(SD_BUS_TYPE_VARIANT, "b", true);
        expect_enter_container(SD_BUS_TYPE_VARIANT, "b");
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, b1);
        expect_exit_container();
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", false);
        expect_verify_type(SD_BUS_TYPE_VARIANT, "s", true);
        expect_enter_container(SD_BUS_TYPE_VARIANT, "s");
        expect_basic<const char*>(SD_BUS_TYPE_STRING, s2.c_str());
        expect_exit_container();
    }

    std::variant<int, std::string, bool> ret_v1, ret_v2;
    new_message().read(ret_v1, ret_v2);
    EXPECT_EQ(v1, ret_v1);
    EXPECT_EQ(v2, ret_v2);
}

TEST_F(ReadTest, VariantVerifyError)
{
    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", -EINVAL);
    }

    std::variant<int, bool> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, VariantSkipUnmatched)
{
    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", false);
        expect_verify_type(SD_BUS_TYPE_VARIANT, "b", false);
        expect_skip("v");
    }

    std::variant<int, bool> ret;
    new_message().read(ret);
}

TEST_F(ReadTest, VariantSkipError)
{
    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", false);
        expect_verify_type(SD_BUS_TYPE_VARIANT, "b", false);
        expect_skip("v", -EINVAL);
    }

    std::variant<int, bool> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, VariantEnterError)
{
    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", true);
        expect_enter_container(SD_BUS_TYPE_VARIANT, "i", -EINVAL);
    }

    std::variant<int, bool> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, VariantExitError)
{
    {
        testing::InSequence seq;
        expect_verify_type(SD_BUS_TYPE_VARIANT, "i", true);
        expect_enter_container(SD_BUS_TYPE_VARIANT, "i");
        expect_basic<int>(SD_BUS_TYPE_INT32, 10);
        expect_exit_container(-EINVAL);
    }

    std::variant<int, bool> ret;
    EXPECT_THROW(new_message().read(ret), sdbusplus::exception::SdBusError);
}

TEST_F(ReadTest, LargeCombo)
{
    const std::vector<std::set<std::string>> vas{
        {"a", "b", "c"},
        {"d", "", "e"},
    };
    const std::map<std::string, std::variant<int, double>> msv = {
        {"a", 3.3}, {"b", 1}, {"c", 4.4}};

    {
        testing::InSequence seq;

        expect_enter_container(SD_BUS_TYPE_ARRAY, "as");
        for (const auto& as : vas)
        {
            expect_at_end(false, 0);
            expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
            for (const auto& s : as)
            {
                expect_at_end(false, 0);
                expect_basic<const char*>(SD_BUS_TYPE_STRING, s.c_str());
            }
            expect_at_end(false, 1);
            expect_exit_container();
        }
        expect_at_end(false, 1);
        expect_exit_container();

        expect_enter_container(SD_BUS_TYPE_ARRAY, "{sv}");
        for (const auto& sv : msv)
        {
            expect_at_end(false, 0);
            expect_enter_container(SD_BUS_TYPE_DICT_ENTRY, "sv");
            expect_basic<const char*>(SD_BUS_TYPE_STRING, sv.first.c_str());
            if (std::holds_alternative<int>(sv.second))
            {
                expect_verify_type(SD_BUS_TYPE_VARIANT, "i", true);
                expect_enter_container(SD_BUS_TYPE_VARIANT, "i");
                expect_basic<int>(SD_BUS_TYPE_INT32, std::get<int>(sv.second));
                expect_exit_container();
            }
            else
            {
                expect_verify_type(SD_BUS_TYPE_VARIANT, "i", false);
                expect_verify_type(SD_BUS_TYPE_VARIANT, "d", true);
                expect_enter_container(SD_BUS_TYPE_VARIANT, "d");
                expect_basic<double>(SD_BUS_TYPE_DOUBLE,
                                     std::get<double>(sv.second));
                expect_exit_container();
            }
            expect_exit_container();
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    std::vector<std::set<std::string>> ret_vas;
    std::map<std::string, std::variant<int, double>> ret_msv;
    new_message().read(ret_vas, ret_msv);
    EXPECT_EQ(vas, ret_vas);
    EXPECT_EQ(msv, ret_msv);
}

// Unpack tests.
// Since unpack uses read, we're mostly just testing the compilation.
// Duplicate a few tests from Read using 'unpack'.

TEST_F(ReadTest, UnpackSingleVector)
{
    const std::vector<std::string> vs{"a", "b", "c", "d"};

    {
        testing::InSequence seq;
        expect_enter_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& i : vs)
        {
            expect_at_end(false, 0);
            expect_basic<const char*>(SD_BUS_TYPE_STRING, i.c_str());
        }
        expect_at_end(false, 1);
        expect_exit_container();
    }

    auto ret_vs = new_message().unpack<std::vector<std::string>>();
    EXPECT_EQ(vs, ret_vs);
}

TEST_F(ReadTest, UnpackMultiple)
{
    const std::tuple<int, std::string, bool> tisb{3, "hi", false};

    {
        testing::InSequence seq;
        expect_basic<int>(SD_BUS_TYPE_INT32, std::get<0>(tisb));
        expect_basic<const char*>(SD_BUS_TYPE_STRING,
                                  std::get<1>(tisb).c_str());
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, std::get<2>(tisb));
    }

    auto ret_tisb = new_message().unpack<int, std::string, bool>();
    EXPECT_EQ(tisb, ret_tisb);
}

TEST_F(ReadTest, UnpackStructuredBinding)
{
    const std::tuple<int, std::string, bool> tisb{3, "hi", false};

    {
        testing::InSequence seq;
        expect_basic<int>(SD_BUS_TYPE_INT32, std::get<0>(tisb));
        expect_basic<const char*>(SD_BUS_TYPE_STRING,
                                  std::get<1>(tisb).c_str());
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, std::get<2>(tisb));
    }

    auto [ret_ti, ret_ts,
          ret_tb] = new_message().unpack<int, std::string, bool>();
    EXPECT_EQ(tisb, std::make_tuple(ret_ti, ret_ts, ret_tb));
}

TEST_F(ReadTest, UnpackVoid)
{
    new_message().unpack<>();
    new_message().unpack<void>();
}

} // namespace
