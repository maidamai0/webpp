#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <webpp/utils/charset.h>
#include <webpp/validators/validators.h>

using namespace webpp::is;
using namespace std;

TEST(ValidationsTest, EmptyFunction) {
    EXPECT_TRUE(empty(""));
    EXPECT_FALSE(empty("not empty"));
    string str = "";
    EXPECT_TRUE(empty(str));
    str = "not empty";
    EXPECT_FALSE(empty(str));
}

TEST(ValidationsTest, ContainsFunctions) {
    map<int, string> data;
    data[0] = "hello world";
    data[10] = "testing";

    EXPECT_TRUE(contains_key(data, 10));
    EXPECT_TRUE(contains_value(data, string("hello world")));
    EXPECT_FALSE(contains_key(data, 100));
    EXPECT_FALSE(contains_value(data, string("it's not gonna be there")));
}

TEST(ValidationsTest, TrimmedFunctions) {
    EXPECT_TRUE(trimmed("trimmed string"));
    EXPECT_TRUE(rtrimmed(" right trimmed"));
    EXPECT_TRUE(ltrimmed("left trimmed "));
}

TEST(ValidationsTest, IPv4Functions) {
    EXPECT_TRUE(ipv4("255.255.255.255"));
    EXPECT_FALSE(ipv4("256.1.1.1"));
    EXPECT_TRUE(ipv4("127.0.0.1"));
    EXPECT_TRUE(ipv4("0.0.0.0"));
    EXPECT_TRUE(ipv4("192.168.0.0"));
    EXPECT_FALSE(ipv4("192.168.1.256"));
    EXPECT_TRUE(ipv4("192.168.0.255"));

    EXPECT_TRUE(ipv4_prefix("192.168.1.2:24"));
    EXPECT_TRUE(ipv4_prefix("192.168.1.2:16"));
    EXPECT_TRUE(ipv4_prefix("192.168.1.2:0"));
    EXPECT_TRUE(ipv4_prefix("192.168.1.2/24"));
    EXPECT_FALSE(ipv4_prefix("false_ip/24"));
    EXPECT_FALSE(ipv4_prefix("192.168.1.3/40"));
    EXPECT_FALSE(ipv4_prefix("192.168.1.3/false_prefix"));
    EXPECT_TRUE(ipv4_prefix("192.168.1.2-24", webpp::charset_t<1>('-')));
}

TEST(ValidationTest, IPv6Functions) {
    auto valids = {"0102:0304:0506:0708:090a:0b0c:0d0e:0f00",
                   "0102:0304:0506:0708:090a:0B0C:0d0E:0F00",
                   "fd11::abcd:e0e0:d10e:0001",
                   "fd11:1234:5678:abcd::abcd:e0e0:d10e:1000",
                   "ff03::0b",
                   "::",
                   "64:ff9b::100.200.15.4",
                   "2001:db8::abc:def1:127.0.0.1"};

    auto invalids = {"2001:db8::a::b",
                     "2001:db8::abcd:efgh",
                     "1:2:3:4:5:6:7:8:9",
                     "2001:db8::abc:def12:1:2",
                     "64:ff9b::123.231.0.257",
                     "64:ff9b::1.22.33",
                     "64:ff9b::1.22.33.44.5",
                     ".",
                     ":.",
                     "::.",
                     ":f:0:0:c:0:f:f:."};

    for (auto const& item : valids) {
        EXPECT_TRUE(ipv6(item)) << item;
        std::string ip = item;
        std::string ip2 = item;
        std::string ip3 = item;
        ip.append("/64");
        EXPECT_TRUE(ipv6_prefix(ip));
        ip2.append("/something bad");
        EXPECT_FALSE(ipv6_prefix(ip2));
        ip3.append("-128");
        EXPECT_TRUE(ipv6_prefix(ip3, webpp::charset_t<1>('-')));
        EXPECT_TRUE(host("[" + ip + "]"));
    }

    for (auto const& item : invalids) {
        EXPECT_FALSE(ipv6(item)) << item;
        std::string ip = item;
        ip.append("/64");
        EXPECT_FALSE(ipv6_prefix(ip));
        EXPECT_FALSE(host("[" + ip + "]"));
    }
}

TEST(ValidationsTest, HostFunction) {
    auto valids = {
        "localhost",       "one.com", "example.notcom", "192.168.0.1",
        "255.255.255.255", "[::1]",   "127.0.0.1",
    };
    auto invalids = {"260.1.2.3", "&^%&^%$&^%&^%$&^%$#@%$#@@!~#!@"};

    for (auto const& item : valids) {
        EXPECT_TRUE(host(item));
    }
    for (auto const& item : invalids) {
        EXPECT_FALSE(host(item));
    }
}

TEST(ValidationsTest, EmailFunction) {
    EXPECT_TRUE(email("moisrex@gmail.com"))
        << "moisrex@gmail.com should be valid";
    EXPECT_TRUE(email("moisrex.test@gmail.com"))
        << "moisrex.test@gmail.com should be valid";
    EXPECT_FALSE(email("not an.email@123.com"))
        << "spaces are not allowd in emails";
}

TEST(ValidationsTest, NumberFunctions) {
    for (char i = '0'; i <= '9'; i++)
        EXPECT_TRUE(digit(i));
    for (char i = 'a'; i <= 'z'; i++)
        EXPECT_FALSE(digit('c'));

    EXPECT_TRUE(digit("123"));
    EXPECT_FALSE(digit("1.3"));

    EXPECT_TRUE(number("123"));
    EXPECT_TRUE(number("1.23"));
    EXPECT_TRUE(number("12.3333333333333351846162000"));
    EXPECT_TRUE(number("12.3333333333333351846162000"));
    EXPECT_TRUE(number(".3333333333333351846162000"));
    EXPECT_TRUE(number("0.3333333333333351846162000"));
    EXPECT_TRUE(number("0."));
    EXPECT_TRUE(number("0.0"));
    EXPECT_FALSE(number("0.0a"));
    EXPECT_TRUE(number("00"));
    EXPECT_FALSE(number("abc"));
    EXPECT_FALSE(number(" "));
    EXPECT_FALSE(number(""));
    EXPECT_FALSE(number('a'));
    EXPECT_TRUE(number('1'));
    EXPECT_TRUE(number('.'));
}