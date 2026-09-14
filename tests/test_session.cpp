// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"
#include "peregrine/session.hpp"

using namespace peregrine;

TEST_CASE(SessionSuite, SessionContainerOperations) {
    Session s;
    EXPECT_TRUE(s.empty());
    EXPECT_FALSE(s.modified);
    EXPECT_FALSE(s.accessed);

    s["user_id"] = "1001";
    s["role"] = "member";

    EXPECT_TRUE(s.modified);
    EXPECT_TRUE(s.accessed);
    EXPECT_EQ(s.size(), static_cast<size_t>(2));
    EXPECT_EQ(s.get("user_id"), "1001");
    EXPECT_EQ(s.get_int("user_id"), 1001);
    EXPECT_EQ(s.get("non_existent", "fallback"), "fallback");

    EXPECT_TRUE(s.has("role"));
    EXPECT_FALSE(s.has("permissions"));

    std::string popped = s.pop("role");
    EXPECT_EQ(popped, "member");
    EXPECT_FALSE(s.has("role"));

    s.clear();
    EXPECT_TRUE(s.empty());
}

TEST_CASE(SessionSuite, SignAndVerifySessionToken) {
    std::string secret = "test_cryptographic_secret_key_2026";
    std::map<std::string, std::string> session_data;
    session_data["user_id"] = "777";
    session_data["username"] = "harshad";
    session_data["is_admin"] = "1";

    std::string token = sign_session(secret, session_data);
    EXPECT_FALSE(token.empty());
    // Format: payload.timestamp.signature (2 dots)
    EXPECT_TRUE(token.find('.') != std::string::npos);

    std::map<std::string, std::string> loaded;
    bool success = verify_and_load_session(secret, token, 3600, loaded);
    EXPECT_TRUE(success);
    EXPECT_EQ(loaded["user_id"], "777");
    EXPECT_EQ(loaded["username"], "harshad");
    EXPECT_EQ(loaded["is_admin"], "1");
}

TEST_CASE(SessionSuite, TamperedSessionRejection) {
    std::string secret = "test_cryptographic_secret_key_2026";
    std::map<std::string, std::string> session_data;
    session_data["user_id"] = "123";

    std::string token = sign_session(secret, session_data);

    // 1. Altered payload: replace part of token
    std::string tampered_token = token;
    tampered_token[5] = (tampered_token[5] == 'a') ? 'b' : 'a';

    std::map<std::string, std::string> loaded;
    EXPECT_FALSE(verify_and_load_session(secret, tampered_token, 3600, loaded));

    // 2. Wrong secret key
    EXPECT_FALSE(verify_and_load_session("wrong_secret_key", token, 3600, loaded));

    // 3. Expired token (crafted with an old timestamp from the past)
    std::string past_ts = "1000000000"; // Timestamp in year 2001
    std::string payload = base64url_encode("{\"user_id\":\"123\"}");
    std::string signing_input = payload + "." + past_ts;
    std::string eff_key = hmac_sha256(secret, "cookie-session");
    std::string sig = base64url_encode(hmac_sha256(eff_key, signing_input));
    std::string expired_token = signing_input + "." + sig;

    EXPECT_FALSE(verify_and_load_session(secret, expired_token, 3600, loaded));
}
