// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"
#include "peregrine/routing.hpp"

using namespace peregrine;

TEST_CASE(RoutingSuite, ExactPathMatching) {
    Router router;
    router.add("GET", "/", [](Request&) { return Response::text("Home"); });
    router.add("GET", "/about", [](Request&) { return Response::text("About"); });
    router.add("POST", "/submit", [](Request&) { return Response::text("Submitted"); });

    std::map<std::string, std::string> params;
    bool method_mismatch = false;
    std::vector<std::string> allowed;

    const Route* r1 = router.match("GET", "/", params, method_mismatch, allowed);
    EXPECT_TRUE(r1 != nullptr);
    EXPECT_FALSE(method_mismatch);

    const Route* r2 = router.match("GET", "/about", params, method_mismatch, allowed);
    EXPECT_TRUE(r2 != nullptr);

    const Route* r3 = router.match("POST", "/submit", params, method_mismatch, allowed);
    EXPECT_TRUE(r3 != nullptr);

    const Route* r4 = router.match("GET", "/non_existent", params, method_mismatch, allowed);
    EXPECT_TRUE(r4 == nullptr);
    EXPECT_FALSE(method_mismatch);
}

TEST_CASE(RoutingSuite, PathParameterExtraction) {
    Router router;
    router.add("GET", "/users/<int:user_id>", [](Request&) { return Response::text("User"); });
    router.add("GET", "/posts/<string:slug>", [](Request&) { return Response::text("Post"); });
    router.add("GET", "/files/<path:file_path>", [](Request&) { return Response::text("File"); });

    std::map<std::string, std::string> params;
    bool method_mismatch = false;
    std::vector<std::string> allowed;

    // Test <int:user_id>
    const Route* r_int = router.match("GET", "/users/12345", params, method_mismatch, allowed);
    EXPECT_TRUE(r_int != nullptr);
    EXPECT_EQ(params["user_id"], "12345");

    // Test int mismatch (non-digits should not match)
    params.clear();
    const Route* r_int_fail = router.match("GET", "/users/not_a_number", params, method_mismatch, allowed);
    EXPECT_TRUE(r_int_fail == nullptr);

    // Test <string:slug>
    params.clear();
    const Route* r_str = router.match("GET", "/posts/hello-world-2026", params, method_mismatch, allowed);
    EXPECT_TRUE(r_str != nullptr);
    EXPECT_EQ(params["slug"], "hello-world-2026");

    // Test <path:file_path>
    params.clear();
    const Route* r_path = router.match("GET", "/files/assets/images/logo.png", params, method_mismatch, allowed);
    EXPECT_TRUE(r_path != nullptr);
    EXPECT_EQ(params["file_path"], "assets/images/logo.png");
}

TEST_CASE(RoutingSuite, MethodMismatchAndAllowedVerbs) {
    Router router;
    router.add("GET", "/api/item", [](Request&) { return Response::text("Get Item"); });
    router.add("PUT", "/api/item", [](Request&) { return Response::text("Put Item"); });
    router.add("DELETE", "/api/item", [](Request&) { return Response::text("Delete Item"); });

    std::map<std::string, std::string> params;
    bool method_mismatch = false;
    std::vector<std::string> allowed;

    // Requesting with POST should trigger method_mismatch and return GET, PUT, DELETE
    const Route* r = router.match("POST", "/api/item", params, method_mismatch, allowed);
    EXPECT_TRUE(r == nullptr);
    EXPECT_TRUE(method_mismatch);
    EXPECT_EQ(allowed.size(), static_cast<size_t>(3));

    EXPECT_TRUE(std::find(allowed.begin(), allowed.end(), "GET") != allowed.end());
    EXPECT_TRUE(std::find(allowed.begin(), allowed.end(), "PUT") != allowed.end());
    EXPECT_TRUE(std::find(allowed.begin(), allowed.end(), "DELETE") != allowed.end());
}
