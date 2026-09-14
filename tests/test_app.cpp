// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"
#include "test_client.hpp"

using namespace peregrine;

TEST_CASE(AppSuite, BasicRoutingAndDispatch) {
    App app("test_app");

    app.get("/", [](Request&) {
        return Response::html("<h1>Welcome</h1>");
    });

    app.get("/users/<int:id>", [](Request& req) {
        std::string uid = req.path_params["id"];
        Json j = Json::object();
        j["id"] = std::stoi(uid);
        j["status"] = "active";
        return Response::json(j);
    });

    TestClient client(app);

    Response r1 = client.get("/");
    EXPECT_EQ(r1.status, 200);
    EXPECT_EQ(r1.body, "<h1>Welcome</h1>");

    Response r2 = client.get("/users/42");
    EXPECT_EQ(r2.status, 200);
    EXPECT_CONTAINS(r2.body, "\"id\":42");
    EXPECT_CONTAINS(r2.body, "\"status\":\"active\"");

    Response r3 = client.get("/not_found");
    EXPECT_EQ(r3.status, 404);
}

TEST_CASE(AppSuite, GlobalBeforeAndAfterMiddleware) {
    App app("test_app");

    app.before_request([](Request& req) {
        req.g["middleware_flag"] = "visited";
    });

    app.after_request([](Request&, Response& res) {
        res.set_header("X-Framework", "Peregrine");
    });

    app.get("/middleware_test", [](Request& req) {
        return Response::text("Flag: " + req.g["middleware_flag"]);
    });

    TestClient client(app);
    Response res = client.get("/middleware_test");
    EXPECT_EQ(res.status, 200);
    EXPECT_EQ(res.body, "Flag: visited");
    EXPECT_EQ(res.headers["X-Framework"], "Peregrine");
}

TEST_CASE(AppSuite, BlueprintRegistrationAndScopedHooks) {
    App app("test_app");

    Blueprint api("api", "/api/v1");

    api.before_request([](Request& req) {
        req.g["api_scoped"] = "true";
    });

    api.get("/status", [](Request& req) {
        Json j = Json::object();
        j["status"] = "online";
        j["scoped"] = req.g["api_scoped"];
        return Response::json(j);
    });

    app.register_blueprint(api);

    TestClient client(app);
    Response res = client.get("/api/v1/status");
    EXPECT_EQ(res.status, 200);
    EXPECT_CONTAINS(res.body, "\"status\":\"online\"");
    EXPECT_CONTAINS(res.body, "\"scoped\":\"true\"");
}

TEST_CASE(AppSuite, SessionAndCookiePersistenceAcrossRequests) {
    App app("test_app");
    app.secret_key = "secure_integration_test_key_12345";

    app.post("/login", [](Request& req) {
        req.session["user"] = req.form_get("username");
        req.session.permanent = true;
        return Response::text("Logged In");
    });

    app.get("/dashboard", [](Request& req) {
        if (!req.session.has("user")) {
            return Response::text("Unauthorized", 401);
        }
        return Response::text("Welcome, " + req.session["user"]);
    });

    TestClient client(app);

    // 1. Unauthenticated request
    Response r1 = client.get("/dashboard");
    EXPECT_EQ(r1.status, 401);

    // 2. Perform login
    Response r2 = client.post("/login", {{"username", "alice"}});
    EXPECT_EQ(r2.status, 200);
    EXPECT_TRUE(client.has_cookie("session"));

    // 3. Authenticated request (cookie automatically propagated by TestClient)
    Response r3 = client.get("/dashboard");
    EXPECT_EQ(r3.status, 200);
    EXPECT_EQ(r3.body, "Welcome, alice");
}

TEST_CASE(AppSuite, CustomErrorHandlers) {
    App app("test_app");

    app.error_handler(404, [](Request&) {
        return Response::html("<h1>Custom 404 Page</h1>", 404);
    });

    app.get("/error", [](Request&) -> Response {
        abort(404, "Not Found");
    });

    TestClient client(app);
    Response res = client.get("/error");
    EXPECT_EQ(res.status, 404);
    EXPECT_EQ(res.body, "<h1>Custom 404 Page</h1>");
}
