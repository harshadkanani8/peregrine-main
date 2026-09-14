#include "test_framework.hpp"
#include "peregrine/helpers.hpp"
#include <fstream>

using namespace peregrine;

TEST_CASE(HelpersSuite, ResponseBuilders) {
    Response r_text = make_response(std::string("Hello Text"), 200);
    EXPECT_EQ(r_text.status, 200);
    EXPECT_EQ(r_text.body, "Hello Text");
    EXPECT_EQ(r_text.headers["Content-Type"], "text/plain; charset=utf-8");

    Json j = Json::object();
    j["status"] = "success";
    Response r_json = jsonify(j, 201);
    EXPECT_EQ(r_json.status, 201);
    EXPECT_CONTAINS(r_json.body, "\"status\":\"success\"");
    EXPECT_EQ(r_json.headers["Content-Type"], "application/json");

    Response r_redir = redirect("/new_location", 301);
    EXPECT_EQ(r_redir.status, 301);
    EXPECT_EQ(r_redir.headers["Location"], "/new_location");
}

TEST_CASE(HelpersSuite, AbortThrowsHTTPException) {
    EXPECT_THROW(abort(404, "Page Not Found"), HTTPException);

    try {
        abort(403, "Access Forbidden");
    } catch (const HTTPException& ex) {
        EXPECT_EQ(ex.code, 403);
        EXPECT_EQ(ex.description, "Access Forbidden");
    }

    try {
        abort(500);
    } catch (const HTTPException& ex) {
        EXPECT_EQ(ex.code, 500);
        EXPECT_EQ(ex.description, "Internal Server Error");
    }
}

TEST_CASE(HelpersSuite, PathTraversalDefenseInReadStaticFile) {
    std::string content, mime;

    // 1. Directory traversal attempt with ..
    EXPECT_FALSE(read_static_file("dist", "../secret.txt", content, mime));
    EXPECT_FALSE(read_static_file("dist", "sub/../../secret.txt", content, mime));

    // 2. Null byte injection attempt
    std::string null_byte_path = "image.png";
    null_byte_path += '\0';
    null_byte_path += "../secret.txt";
    EXPECT_FALSE(read_static_file("dist", null_byte_path, content, mime));

    // 3. Valid file read
    std::string tmp_file = "test_valid_static.txt";
    std::ofstream out(tmp_file);
    out << "static content 123";
    out.close();

    EXPECT_TRUE(read_static_file(".", tmp_file, content, mime));
    EXPECT_EQ(content, "static content 123");
    EXPECT_EQ(mime, "text/plain; charset=utf-8");

    std::remove(tmp_file.c_str());
}
