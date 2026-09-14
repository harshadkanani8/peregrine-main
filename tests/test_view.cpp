// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"
#include "peregrine/view.hpp"

using namespace peregrine;

class UserAPI : public MethodView {
public:
    Response get(Request&) override {
        return Response::text("Fetched User");
    }
    Response post(Request&) override {
        return Response::text("Created User", 201);
    }
    Response delete_(Request&) override {
        return Response::text("Deleted User", 204);
    }
};

TEST_CASE(ViewSuite, MethodViewDispatching) {
    auto handler = MethodView::as_view<UserAPI>();

    Request req_get;
    req_get.method = "GET";
    Response res_get = handler(req_get);
    EXPECT_EQ(res_get.status, 200);
    EXPECT_EQ(res_get.body, "Fetched User");

    Request req_post;
    req_post.method = "POST";
    Response res_post = handler(req_post);
    EXPECT_EQ(res_post.status, 201);
    EXPECT_EQ(res_post.body, "Created User");

    Request req_del;
    req_del.method = "DELETE";
    Response res_del = handler(req_del);
    EXPECT_EQ(res_del.status, 204);

    // Unimplemented methods should return 405 Method Not Allowed
    Request req_put;
    req_put.method = "PUT";
    Response res_put = handler(req_put);
    EXPECT_EQ(res_put.status, 405);

    Request req_patch;
    req_patch.method = "PATCH";
    Response res_patch = handler(req_patch);
    EXPECT_EQ(res_patch.status, 405);
}
