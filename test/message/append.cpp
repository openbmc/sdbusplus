#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <array>
#include <map>
#include <set>
#include <span>
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

using testing::Eq;
using testing::MatcherCast;
using testing::Pointee;
using testing::Return;
using testing::SafeMatcherCast;
using testing::StrEq;

MATCHER_P(iovec_equal, match_string, "")
{
    const char* start = std::bit_cast<char*>(arg->iov_base);
    return std::string(start, arg->iov_len) == match_string;
}

class AppendTest : public testing::Test
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

    template <typename T>
    void expect_basic(char type, T val)
    {
        EXPECT_CALL(mock, sd_bus_message_append_basic(
                              nullptr, type,
                              MatcherCast<const void*>(
                                  SafeMatcherCast<const T*>(Pointee(Eq(val))))))
            .WillOnce(Return(0));
    }

    void expect_basic_string(char type, const char* str)
    {
        EXPECT_CALL(mock, sd_bus_message_append_basic(
                              nullptr, type,
                              MatcherCast<const void*>(
                                  SafeMatcherCast<const char*>(StrEq(str)))))
            .WillOnce(Return(0));
    }
    void expect_basic_string_iovec(const char* str, size_t size)
    {
        std::string tmp = {str, size};
        EXPECT_CALL(mock, sd_bus_message_append_string_iovec(
                              nullptr, iovec_equal(tmp), 1))
            .WillOnce(Return(0));
    }

    void expect_open_container(char type, const char* contents)
    {
        EXPECT_CALL(
            mock, sd_bus_message_open_container(nullptr, type, StrEq(contents)))
            .WillOnce(Return(0));
    }

    void expect_close_container()
    {
        EXPECT_CALL(mock, sd_bus_message_close_container(nullptr))
            .WillOnce(Return(0));
    }

    static int on_array_append(sd_bus_message*, char, const void*, size_t)
    {
        return 0;
    }

    void expect_append_array(char type, size_t sz)
    {
        EXPECT_CALL(mock,
                    sd_bus_message_append_array(nullptr, type, testing::_, sz))
            .WillOnce(testing::Invoke(on_array_append));
    }
};

TEST_F(AppendTest, RValueInt)
{
    expect_basic<int>(SD_BUS_TYPE_INT32, 1);
    new_message().append(1);
}

TEST_F(AppendTest, LValueInt)
{
    const int a = 1;
    expect_basic<int>(SD_BUS_TYPE_INT32, a);
    new_message().append(a);
}

TEST_F(AppendTest, XValueInt)
{
    int a = 1;
    expect_basic<int>(SD_BUS_TYPE_INT32, a);
    new_message().append(std::move(a));
}

TEST_F(AppendTest, RValueBool)
{
    expect_basic<int>(SD_BUS_TYPE_BOOLEAN, true);
    new_message().append(true);
}

TEST_F(AppendTest, LValueBool)
{
    const bool a = false;
    expect_basic<int>(SD_BUS_TYPE_BOOLEAN, false);
    new_message().append(a);
}

TEST_F(AppendTest, XValueBool)
{
    bool a = false;
    expect_basic<int>(SD_BUS_TYPE_BOOLEAN, false);
    new_message().append(std::move(a));
}

TEST_F(AppendTest, RValueDouble)
{
    expect_basic<double>(SD_BUS_TYPE_DOUBLE, 1.1);
    new_message().append(1.1);
}

TEST_F(AppendTest, LValueDouble)
{
    const double a = 1.1;
    expect_basic<double>(SD_BUS_TYPE_DOUBLE, a);
    new_message().append(a);
}

TEST_F(AppendTest, XValueDouble)
{
    double a = 1.1;
    expect_basic<double>(SD_BUS_TYPE_DOUBLE, a);
    new_message().append(std::move(a));
}

TEST_F(AppendTest, RValueCString)
{
    expect_basic_string(SD_BUS_TYPE_STRING, "asdf");
    new_message().append("asdf");
}

TEST_F(AppendTest, LValueCString)
{
    const char* const s = "asdf";
    expect_basic_string(SD_BUS_TYPE_STRING, s);
    new_message().append(s);
}

TEST_F(AppendTest, XValueCString)
{
    const char* s = "asdf";
    expect_basic_string(SD_BUS_TYPE_STRING, s);
    new_message().append(std::move(s));
}

TEST_F(AppendTest, RValueString)
{
    expect_basic_string(SD_BUS_TYPE_STRING, "asdf");
    new_message().append(std::string{"asdf"});
}

TEST_F(AppendTest, LValueString)
{
    std::string s{"asdf"};
    expect_basic_string(SD_BUS_TYPE_STRING, s.c_str());
    new_message().append(s);
}

TEST_F(AppendTest, XValueString)
{
    std::string s{"asdf"};
    expect_basic_string(SD_BUS_TYPE_STRING, s.c_str());
    new_message().append(std::move(s));
}

TEST_F(AppendTest, LValueStringView)
{
    std::string_view s{"asdf"};
    expect_basic_string_iovec(s.data(), s.size());
    new_message().append(s);
}

TEST_F(AppendTest, RValueStringView)
{
    std::string_view s{"asdf"};
    expect_basic_string_iovec(s.data(), s.size());
    new_message().append(std::string_view{"asdf"});
}

TEST_F(AppendTest, ObjectPath)
{
    sdbusplus::message::object_path o{"/asdf"};
    expect_basic_string(SD_BUS_TYPE_OBJECT_PATH, o.str.c_str());
    new_message().append(o);
}

TEST_F(AppendTest, Signature)
{
    sdbusplus::message::signature g{"ii"};
    expect_basic_string(SD_BUS_TYPE_SIGNATURE, g.str.c_str());
    new_message().append(g);
}

TEST_F(AppendTest, CombinedBasic)
{
    const int c = 3;
    const std::string s1{"fdsa"};
    const char* const s2 = "asdf";

    {
        testing::InSequence seq;
        expect_basic<int>(SD_BUS_TYPE_INT32, 1);
        expect_basic<double>(SD_BUS_TYPE_DOUBLE, 2.2);
        expect_basic<int>(SD_BUS_TYPE_INT32, c);
        expect_basic_string(SD_BUS_TYPE_STRING, s1.c_str());
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, false);
        expect_basic_string(SD_BUS_TYPE_STRING, s2);
    }
    new_message().append(1, 2.2, c, s1, false, s2);
}

TEST_F(AppendTest, Array)
{
    const std::array<double, 4> a{1.1, 2.2, 3.3, 4.4};

    {
        expect_append_array(SD_BUS_TYPE_DOUBLE, a.size() * sizeof(double));
    }
    new_message().append(a);
}

TEST_F(AppendTest, Span)
{
    const std::array<double, 4> a{1.1, 2.2, 3.3, 4.4};
    auto s = std::span{a};

    {
        expect_append_array(SD_BUS_TYPE_DOUBLE, a.size() * sizeof(double));
    }
    new_message().append(s);
}

TEST_F(AppendTest, Vector)
{
    const std::vector<std::string> v{"a", "b", "c", "d"};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& i : v)
        {
            expect_basic_string(SD_BUS_TYPE_STRING, i.c_str());
        }
        expect_close_container();
    }
    new_message().append(v);
}

TEST_F(AppendTest, VectorIntegral)
{
    const std::vector<int32_t> v{1, 2, 3, 4};
    expect_append_array(SD_BUS_TYPE_INT32, v.size() * sizeof(int32_t));
    new_message().append(v);
}

TEST_F(AppendTest, VectorBoolean)
{
    const std::vector<bool> v{false, true, false, true};
    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "b");
        for (const auto& i : v)
        {
            expect_basic<bool>(SD_BUS_TYPE_BOOLEAN, (int)i);
        }
        expect_close_container();
    }
    new_message().append(v);
}

TEST_F(AppendTest, VectorNestIntegral)
{
    const std::vector<std::array<int32_t, 3>> v{
        {1, 2, 3}, {3, 4, 5}, {6, 7, 8}};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "ai");
        for (long unsigned int i = 0; i < v.size(); i++)
        {
            expect_append_array(SD_BUS_TYPE_INT32,
                                v[i].size() * sizeof(int32_t));
        }
        expect_close_container();
    }
    new_message().append(v);
}

TEST_F(AppendTest, Set)
{
    const std::set<std::string> s{"one", "two", "eight"};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& i : s)
        {
            expect_basic_string(SD_BUS_TYPE_STRING, i.c_str());
        }
        expect_close_container();
    }
    new_message().append(s);
}

TEST_F(AppendTest, UnorderedSet)
{
    const std::unordered_set<std::string> s{"one", "two", "eight"};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "s");
        for (const auto& i : s)
        {
            expect_basic_string(SD_BUS_TYPE_STRING, i.c_str());
        }
        expect_close_container();
    }
    new_message().append(s);
}

TEST_F(AppendTest, Map)
{
    const std::map<int, std::string> m{
        {1, "a"},
        {2, "bc"},
        {3, "def"},
        {4, "ghij"},
    };

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "{is}");
        for (const auto& i : m)
        {
            expect_open_container(SD_BUS_TYPE_DICT_ENTRY, "is");
            expect_basic<int>(SD_BUS_TYPE_INT32, i.first);
            expect_basic_string(SD_BUS_TYPE_STRING, i.second.c_str());
            expect_close_container();
        }
        expect_close_container();
    }
    new_message().append(m);
}

TEST_F(AppendTest, UnorderedMap)
{
    const std::unordered_map<int, bool> m{
        {1, false},
        {2, true},
        {3, true},
        {4, false},
    };

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_ARRAY, "{ib}");
        for (const auto& i : m)
        {
            expect_open_container(SD_BUS_TYPE_DICT_ENTRY, "ib");
            expect_basic<int>(SD_BUS_TYPE_INT32, i.first);
            expect_basic<int>(SD_BUS_TYPE_BOOLEAN, i.second);
            expect_close_container();
        }
        expect_close_container();
    }
    new_message().append(m);
}

TEST_F(AppendTest, Tuple)
{
    const std::tuple<int, std::string, bool> t{5, "asdf", false};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_STRUCT, "isb");
        expect_basic<int>(SD_BUS_TYPE_INT32, std::get<0>(t));
        expect_basic_string(SD_BUS_TYPE_STRING, std::get<1>(t).c_str());
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, std::get<2>(t));
        expect_close_container();
    }
    new_message().append(t);
}

TEST_F(AppendTest, Variant)
{
    const bool b1 = false;
    const std::string s2{"asdf"};
    const std::variant<int, std::string, bool> v1{b1}, v2{s2};

    {
        testing::InSequence seq;
        expect_open_container(SD_BUS_TYPE_VARIANT, "b");
        expect_basic<int>(SD_BUS_TYPE_BOOLEAN, b1);
        expect_close_container();
        expect_open_container(SD_BUS_TYPE_VARIANT, "s");
        expect_basic_string(SD_BUS_TYPE_STRING, s2.c_str());
        expect_close_container();
    }
    new_message().append(v1, v2);
}

TEST_F(AppendTest, LargeCombo)
{
    std::vector<std::array<std::string, 3>> vas{{"a", "b", "c"},
                                                {"d", "", "e"}};
    std::map<std::string, std::variant<int, double>> msv = {
        {"a", 3.3}, {"b", 1}, {"c", 4.4}};

    {
        testing::InSequence seq;

        expect_open_container(SD_BUS_TYPE_ARRAY, "as");
        for (const auto& as : vas)
        {
            expect_open_container(SD_BUS_TYPE_ARRAY, "s");
            for (const auto& s : as)
            {
                expect_basic_string(SD_BUS_TYPE_STRING, s.c_str());
            }
            expect_close_container();
        }
        expect_close_container();

        expect_open_container(SD_BUS_TYPE_ARRAY, "{sv}");
        for (const auto& sv : msv)
        {
            expect_open_container(SD_BUS_TYPE_DICT_ENTRY, "sv");
            expect_basic_string(SD_BUS_TYPE_STRING, sv.first.c_str());
            if (std::holds_alternative<int>(sv.second))
            {
                expect_open_container(SD_BUS_TYPE_VARIANT, "i");
                expect_basic<int>(SD_BUS_TYPE_INT32, std::get<int>(sv.second));
                expect_close_container();
            }
            else
            {
                expect_open_container(SD_BUS_TYPE_VARIANT, "d");
                expect_basic<double>(SD_BUS_TYPE_DOUBLE,
                                     std::get<double>(sv.second));
                expect_close_container();
            }
            expect_close_container();
        }
        expect_close_container();
    }
    new_message().append(vas, msv);
}

} // namespace
