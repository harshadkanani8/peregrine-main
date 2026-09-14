#include "test_framework.hpp"
#include "peregrine/csrf.hpp"

using namespace peregrine;

TEST_CASE(CsrfSuite, GetTokenGeneratesAndPersistsInSession) {
    Request req;
    std::string token1 = csrf::get_token(req);
    EXPECT_FALSE(token1.empty());
    EXPECT_EQ(token1.size(), static_cast<size_t>(64)); // 32 bytes hex = 64 chars

    // Calling it again on the same request should return the exact same token
    std::string token2 = csrf::get_token(req);
    EXPECT_EQ(token1, token2);
    EXPECT_EQ(req.session.get("_csrf_token"), token1);
}

TEST_CASE(CsrfSuite, VerifyTokenFromFormField) {
    Request req;
    std::string valid_token = csrf::get_token(req);

    // 1. Valid token in form
    req.form["csrf_token"] = valid_token;
    EXPECT_TRUE(csrf::verify_token(req));

    // 2. Invalid token in form
    req.form["csrf_token"] = "invalid_token_12345";
    EXPECT_FALSE(csrf::verify_token(req));
}

TEST_CASE(CsrfSuite, VerifyTokenFromHeaders) {
    Request req;
    std::string valid_token = csrf::get_token(req);

    // 1. Header: X-CSRF-Token
    req.headers["x-csrf-token"] = valid_token;
    EXPECT_TRUE(csrf::verify_token(req));

    // 2. Header: X-CSRFToken
    req.headers.clear();
    req.headers["x-csrftoken"] = valid_token;
    EXPECT_TRUE(csrf::verify_token(req));
}

TEST_CASE(CsrfSuite, VerifyTokenFromQueryString) {
    Request req;
    std::string valid_token = csrf::get_token(req);

    req.query["csrf_token"] = valid_token;
    EXPECT_TRUE(csrf::verify_token(req));
}

TEST_CASE(CsrfSuite, MissingTokenRejection) {
    Request req;
    // Session has no CSRF token
    EXPECT_FALSE(csrf::verify_token(req));

    // Session has token, but request carries no matching parameter
    csrf::get_token(req);
    EXPECT_FALSE(csrf::verify_token(req));
}
