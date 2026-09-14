#include "test_framework.hpp"
#include "peregrine/common.hpp"

using namespace peregrine;

TEST_CASE(CommonSuite, StringCaseConversions) {
    EXPECT_EQ(to_lower("HELLO World!"), "hello world!");
    EXPECT_EQ(to_upper("hello World!"), "HELLO WORLD!");
    EXPECT_EQ(to_lower(""), "");
    EXPECT_EQ(to_upper("123_abc"), "123_ABC");
}

TEST_CASE(CommonSuite, StringTrim) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("\t\r\n test string \n\r\t"), "test string");
    EXPECT_EQ(trim(""), "");
    EXPECT_EQ(trim("    "), "");
    EXPECT_EQ(trim("no_spaces"), "no_spaces");
}

TEST_CASE(CommonSuite, StringSplit) {
    auto parts = split("a,b,c", ',');
    EXPECT_EQ(parts.size(), static_cast<size_t>(3));
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");

    auto empty_parts = split("", ',');
    EXPECT_EQ(empty_parts.size(), static_cast<size_t>(0));

    auto single = split("single", ',');
    EXPECT_EQ(single.size(), static_cast<size_t>(1));
    EXPECT_EQ(single[0], "single");
}

TEST_CASE(CommonSuite, StringStartsAndEndsWith) {
    std::string s = "peregrine_framework_test";
    EXPECT_TRUE(starts_with(s, "peregrine"));
    EXPECT_FALSE(starts_with(s, "framework"));
    EXPECT_TRUE(ends_with(s, "_test"));
    EXPECT_FALSE(ends_with(s, "framework"));
    EXPECT_TRUE(starts_with(s, ""));
    EXPECT_TRUE(ends_with(s, ""));
}

TEST_CASE(CommonSuite, StringReplaceAll) {
    EXPECT_EQ(replace_all("foo bar foo baz", "foo", "qux"), "qux bar qux baz");
    EXPECT_EQ(replace_all("no match here", "xyz", "abc"), "no match here");
    EXPECT_EQ(replace_all("aaa", "a", "aa"), "aaaaaa");
    EXPECT_EQ(replace_all("test", "", "abc"), "test");
}

TEST_CASE(CommonSuite, UrlEncodingAndDecoding) {
    std::string original = "Hello World! @#$&*()=+;:,?";
    std::string encoded = url_encode(original);
    std::string decoded = url_decode(encoded);
    EXPECT_EQ(decoded, original);

    EXPECT_EQ(url_decode("hello+world"), "hello world");
    EXPECT_EQ(url_decode("test%20case"), "test case");
    EXPECT_EQ(url_decode("a%2Fb%2Fc"), "a/b/c");
}

TEST_CASE(CommonSuite, ParseUrlEncodedBody) {
    std::string body = "name=Alice+Smith&age=30&city=San%20Francisco&empty_val=&flag";
    auto params = parse_urlencoded(body);

    EXPECT_EQ(params["name"], "Alice Smith");
    EXPECT_EQ(params["age"], "30");
    EXPECT_EQ(params["city"], "San Francisco");
    EXPECT_EQ(params["empty_val"], "");
    EXPECT_EQ(params["flag"], "");
}

TEST_CASE(CommonSuite, Base64UrlRoundTrip) {
    std::string text = "Peregrine C++ Web Framework 2026! ~!@#$%^&*()_+";
    std::string b64 = base64url_encode(text);
    // Base64URL should never contain '+', '/', or '='
    EXPECT_TRUE(b64.find('+') == std::string::npos);
    EXPECT_TRUE(b64.find('/') == std::string::npos);
    EXPECT_TRUE(b64.find('=') == std::string::npos);

    std::string recovered = base64url_decode(b64);
    EXPECT_EQ(recovered, text);
}

TEST_CASE(CommonSuite, ConstantTimeEqual) {
    std::string s1 = "super_secure_hash_value_12345";
    std::string s2 = "super_secure_hash_value_12345";
    std::string s3 = "super_secure_hash_value_12346";
    std::string s4 = "short";

    EXPECT_TRUE(constant_time_equal(s1, s2));
    EXPECT_FALSE(constant_time_equal(s1, s3));
    EXPECT_FALSE(constant_time_equal(s1, s4));
}

TEST_CASE(CommonSuite, RandomHexGenerator) {
    std::string hex16 = random_hex(16);
    std::string hex32 = random_hex(32);

    EXPECT_EQ(hex16.size(), static_cast<size_t>(32)); // 16 bytes = 32 hex chars
    EXPECT_EQ(hex32.size(), static_cast<size_t>(64)); // 32 bytes = 64 hex chars
    EXPECT_NE(hex16, random_hex(16)); // Randomness check
}
