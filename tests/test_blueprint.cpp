#include "test_framework.hpp"
#include "peregrine/blueprint.hpp"

using namespace peregrine;

TEST_CASE(BlueprintSuite, BlueprintPrefixAndRoutes) {
    Blueprint auth_bp("auth", "/auth/");
    EXPECT_EQ(auth_bp.name(), "auth");
    EXPECT_EQ(auth_bp.url_prefix(), "/auth"); // Strips trailing slash

    auth_bp.get("/login", [](Request&) { return Response::text("Login"); });
    auth_bp.post("/register", [](Request&) { return Response::text("Register"); });
    auth_bp.del("/session", [](Request&) { return Response::text("Logout"); });

    const auto& routes = auth_bp.routes();
    EXPECT_EQ(routes.size(), static_cast<size_t>(3));
    EXPECT_EQ(routes[0].method, "GET");
    EXPECT_EQ(routes[0].pattern, "/login");
    EXPECT_EQ(routes[1].method, "POST");
    EXPECT_EQ(routes[1].pattern, "/register");
    EXPECT_EQ(routes[2].method, "DELETE");
    EXPECT_EQ(routes[2].pattern, "/session");
}

TEST_CASE(BlueprintSuite, BlueprintHooksRegistration) {
    Blueprint api("api", "/api/v1");

    bool hook_registered = false;
    api.before_request([&hook_registered](Request&) {
        hook_registered = true;
    });

    api.after_request([](Request&, Response& res) {
        res.set_header("X-API-Version", "1.0");
    });

    EXPECT_EQ(api.before_hooks().size(), static_cast<size_t>(1));
    EXPECT_EQ(api.after_hooks().size(), static_cast<size_t>(1));
}

TEST_CASE(BlueprintSuite, BlueprintErrorHandlers) {
    Blueprint admin("admin", "/admin");
    admin.error_handler(403, [](Request&) {
        return Response::text("Admin Access Forbidden", 403);
    });

    const auto& err_handlers = admin.error_handlers();
    EXPECT_TRUE(err_handlers.find(403) != err_handlers.end());
    EXPECT_TRUE(err_handlers.find(404) == err_handlers.end());
}
