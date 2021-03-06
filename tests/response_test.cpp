// Created by moisrex on 2/4/20.

#include "../core/include/webpp/http/response.hpp"

#include "../core/include/webpp/http/body.hpp"

#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <type_traits>

using namespace webpp;

using res_t  = basic_response<std_traits>;
using body_t = response_body<std_traits>;

// TEST(Body, Text) {
//    body_t b = "Testing";
//    EXPECT_EQ(b.str(), "Testing");
//    EXPECT_TRUE(b == "Testing");
//
//    std::string str = "hello";
//    b               = str;
//
//    EXPECT_EQ(b, "hello");
//
//    std::string_view sth = "nice";
//    b                    = sth;
//    EXPECT_EQ(b, "nice");
//
//    b = std::string("cool");
//    EXPECT_EQ(b, "cool");
//
//    body_t bt;
//    {
//        std::string      _str = "testing";
//        std::string_view test = _str;
//        bt.replace_string_view(test);
//        EXPECT_EQ(bt.str(), test);
//        _str = "";
//    }
//    EXPECT_NE(bt.str(), "testing") << "The test should be empty since it was a
//    "
//                                      "string_view and not a string";
//}
//
// TEST(Response, Type) {
//    constexpr auto return_callback = [] {
//        return res_t("Hello");
//    };
//    using ret_type       = ::std::invoke_result_t<decltype(return_callback)>;
//    constexpr bool one   = ::std::is_same_v<ret_type, res_t>;
//    constexpr bool two   = ::std::is_convertible_v<ret_type, res_t>;
//    constexpr bool three = ::std::is_convertible_v<res_t, res_t>;
//    constexpr bool four  = ::std::is_convertible_v<std::string, res_t>;
//    //    constexpr bool five = std::is_convertible_v<std::string_view,
//    //    response>;
//    EXPECT_TRUE(one);
//    EXPECT_TRUE(two);
//    EXPECT_TRUE(three);
//    EXPECT_TRUE(four);
//    //    EXPECT_TRUE(five);
//}
//
// TEST(Response, Init) {
//    auto res  = res_t();
//    auto res2 = res_t();
//
//    EXPECT_EQ(res, res2);
//
//    EXPECT_EQ(std::string(res.body.str("")), "");
//    res2 << "Code";
//    EXPECT_EQ(std::string(res2.body.str()), "Code");
//    res = res2;
//    EXPECT_EQ(std::string(res.body.str()), "Code");
//
//    EXPECT_EQ(res, res2);
//}
//
// TEST(Response, File) {
//    std::filesystem::path file = std::filesystem::temp_directory_path();
//    file.append("webpp_test_file");
//    std::ofstream handle{file};
//    handle << "Hello World";
//    handle.close();
//    EXPECT_EQ(res_t::res_t(file).body.str(), "Hello World");
//    std::filesystem::remove(file);
//}
