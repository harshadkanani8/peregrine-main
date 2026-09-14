#include "test_framework.hpp"
#include "peregrine/json.hpp"

using namespace peregrine;

TEST_CASE(JsonSuite, ScalarTypesAndInspection) {
    Json j_null;
    EXPECT_TRUE(j_null.type() == Json::Type::Null);

    Json j_bool(true);
    EXPECT_TRUE(j_bool.is_bool());
    EXPECT_TRUE(j_bool.as_bool());

    Json j_int(42);
    EXPECT_TRUE(j_int.is_number());
    EXPECT_EQ(j_int.as_int(), 42);

    Json j_double(3.1415);
    EXPECT_TRUE(j_double.is_number());
    EXPECT_TRUE(j_double.as_number() > 3.14 && j_double.as_number() < 3.15);

    Json j_str("hello");
    EXPECT_TRUE(j_str.is_string());
    EXPECT_EQ(j_str.as_string(), "hello");
}

TEST_CASE(JsonSuite, ArrayOperations) {
    Json arr = Json::array();
    EXPECT_TRUE(arr.is_array());
    EXPECT_EQ(arr.size(), static_cast<size_t>(0));

    arr.push_back(10);
    arr.push_back("second");
    arr.push_back(true);

    EXPECT_EQ(arr.size(), static_cast<size_t>(3));
    EXPECT_EQ(arr[0].as_int(), 10);
    EXPECT_EQ(arr[1].as_string(), "second");
    EXPECT_TRUE(arr[2].as_bool());

    // Iterating array items
    const auto& items = arr.items();
    EXPECT_EQ(items.size(), static_cast<size_t>(3));
}

TEST_CASE(JsonSuite, ObjectOperations) {
    Json obj = Json::object();
    EXPECT_TRUE(obj.is_object());
    EXPECT_FALSE(obj.has("username"));

    obj["username"] = "harshad";
    obj["age"] = 28;
    obj["is_admin"] = true;

    EXPECT_TRUE(obj.has("username"));
    EXPECT_TRUE(obj.has("age"));
    EXPECT_TRUE(obj.has("is_admin"));
    EXPECT_FALSE(obj.has("unknown_key"));

    EXPECT_EQ(obj["username"].as_string(), "harshad");
    EXPECT_EQ(obj["age"].as_int(), 28);
    EXPECT_TRUE(obj["is_admin"].as_bool());

    EXPECT_EQ(obj.size(), static_cast<size_t>(3));
}

TEST_CASE(JsonSuite, NestedStructureAndDump) {
    Json root = Json::object();
    root["app"] = "Peregrine";
    root["version"] = 1.0;

    Json tags = Json::array();
    tags.push_back("web");
    tags.push_back("c++11");
    tags.push_back("fast");
    root["tags"] = tags;

    Json nested_user = Json::object();
    nested_user["id"] = 101;
    nested_user["name"] = "Alice";
    root["user"] = nested_user;

    std::string json_str = root.dump();
    EXPECT_CONTAINS(json_str, "\"app\":\"Peregrine\"");
    EXPECT_CONTAINS(json_str, "\"tags\":[\"web\",\"c++11\",\"fast\"]");
    EXPECT_CONTAINS(json_str, "\"id\":101");
}

TEST_CASE(JsonSuite, ParseScalars) {
    EXPECT_TRUE(Json::parse("null").is_null());
    EXPECT_TRUE(Json::parse("true").as_bool());
    EXPECT_FALSE(Json::parse("false").as_bool());
    EXPECT_EQ(Json::parse("12345").as_int(), 12345);
    EXPECT_EQ(Json::parse("-99").as_int(), -99);
    EXPECT_EQ(Json::parse("\"hello world\"").as_string(), "hello world");
}

TEST_CASE(JsonSuite, ParseComplexJson) {
    std::string raw = "{\n"
                      "  \"title\": \"C++ Framework\",\n"
                      "  \"port\": 5000,\n"
                      "  \"ssl\": false,\n"
                      "  \"endpoints\": [\"/api\", \"/auth\", \"/ws\"],\n"
                      "  \"owner\": {\n"
                      "    \"name\": \"Harshad\",\n"
                      "    \"id\": 7\n"
                      "  }\n"
                      "}";

    Json parsed = Json::parse(raw);
    EXPECT_TRUE(parsed.is_object());
    EXPECT_EQ(parsed["title"].as_string(), "C++ Framework");
    EXPECT_EQ(parsed["port"].as_int(), 5000);
    EXPECT_FALSE(parsed["ssl"].as_bool());

    EXPECT_TRUE(parsed["endpoints"].is_array());
    EXPECT_EQ(parsed["endpoints"].size(), static_cast<size_t>(3));
    EXPECT_EQ(parsed["endpoints"][0].as_string(), "/api");
    EXPECT_EQ(parsed["endpoints"][1].as_string(), "/auth");
    EXPECT_EQ(parsed["endpoints"][2].as_string(), "/ws");

    EXPECT_TRUE(parsed["owner"].is_object());
    EXPECT_EQ(parsed["owner"]["name"].as_string(), "Harshad");
    EXPECT_EQ(parsed["owner"]["id"].as_int(), 7);
}

TEST_CASE(JsonSuite, StringEscapesRoundTrip) {
    Json j = Json::object();
    j["quote"] = "He said \"Hello!\"\nNext line with \t tab.";
    std::string dumped = j.dump();

    Json recovered = Json::parse(dumped);
    EXPECT_EQ(recovered["quote"].as_string(), j["quote"].as_string());
}
