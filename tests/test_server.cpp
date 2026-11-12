// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "test_framework.hpp"
#include "peregrine/server.hpp"

using namespace peregrine;

TEST_CASE(ServerSuite, ParseSimpleGetRequest) {
    std::string raw = 
        "GET /index.html?search=peregrine&page=2 HTTP/1.1\r\n"
        "Host: localhost:5000\r\n"
        "User-Agent: TestRunner/1.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    Request req = detail::parse_request(raw);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.path, "/index.html");
    EXPECT_EQ(req.http_version, "HTTP/1.1");
    EXPECT_EQ(req.header("Host"), "localhost:5000");
    EXPECT_EQ(req.header("user-agent"), "TestRunner/1.0");
    EXPECT_EQ(req.arg("search"), "peregrine");
    EXPECT_EQ(req.arg("page"), "2");
}

TEST_CASE(ServerSuite, ParsePostFormAndCookies) {
    std::string raw = 
        "POST /login HTTP/1.1\r\n"
        "Host: localhost:5000\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Cookie: session=xyz987; theme=dark\r\n"
        "Content-Length: 29\r\n"
        "\r\n"
        "user=alice&password=secret123";

    Request req = detail::parse_request(raw);
    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/login");
    EXPECT_EQ(req.cookie("session"), "xyz987");
    EXPECT_EQ(req.cookie("theme"), "dark");
    EXPECT_EQ(req.form_get("user"), "alice");
    EXPECT_EQ(req.form_get("password"), "secret123");
}

TEST_CASE(ServerSuite, ParseMultipartFormData) {
    std::string boundary = "----WebKitFormBoundaryX9aY7bZ";
    std::string body = 
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"username\"\r\n\r\n"
        "john_doe\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Hello World Content\r\n"
        "--" + boundary + "--\r\n";

    std::string raw = 
        "POST /upload HTTP/1.1\r\n"
        "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;

    Request req = detail::parse_request(raw);
    EXPECT_EQ(req.form_get("username"), "john_doe");
    EXPECT_EQ(req.files.size(), static_cast<size_t>(1));
    EXPECT_EQ(req.files[0].field_name, "file");
    EXPECT_EQ(req.files[0].filename, "hello.txt");
    EXPECT_EQ(req.files[0].content_type, "text/plain");
    EXPECT_EQ(req.files[0].data, "Hello World Content");
}

TEST_CASE(ServerSuite, SerializeResponseHeadersAndBody) {
    Response res;
    res.status = 200;
    res.body = "OK Payload";
    res.set_header("X-Custom", "PeregrineHeader");
    res.set_cookie("session_id", "abc12345", 3600, "/", "", true, true, "Strict");

    std::string serialized = detail::serialize_response(res);
    EXPECT_CONTAINS(serialized, "HTTP/1.1 200 OK\r\n");
    EXPECT_CONTAINS(serialized, "Content-Length: 10\r\n");
    EXPECT_CONTAINS(serialized, "X-Custom: PeregrineHeader\r\n");
    EXPECT_CONTAINS(serialized, "Set-Cookie: session_id=abc12345; Path=/; Max-Age=3600; HttpOnly; Secure; SameSite=Strict\r\n");
    EXPECT_CONTAINS(serialized, "Connection: close\r\n\r\nOK Payload");
}

TEST_CASE(ServerSuite, HttpStatusReasonPhrase) {
    EXPECT_EQ(detail::status_reason(200), "OK");
    EXPECT_EQ(detail::status_reason(201), "Created");
    EXPECT_EQ(detail::status_reason(302), "Found");
    EXPECT_EQ(detail::status_reason(400), "Bad Request");
    EXPECT_EQ(detail::status_reason(404), "Not Found");
    EXPECT_EQ(detail::status_reason(405), "Method Not Allowed");
    EXPECT_EQ(detail::status_reason(500), "Internal Server Error");
    EXPECT_EQ(detail::status_reason(503), "Service Unavailable");
}

TEST_CASE(ServerSuite, CreateSslContextWithHardenedCiphers) {
    ServerConfig cfg;
    cfg.cert_file = "non_existent_cert.pem";
    cfg.key_file = "non_existent_key.pem";
    cfg.configure_ecdsa_p256_ciphers();

    // Verify configuration options are set properly
    EXPECT_EQ(cfg.ssl_groups_list, "P-256");
    EXPECT_EQ(cfg.ssl_cipher_list, "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384");
    EXPECT_EQ(cfg.ssl_ciphersuites, "TLS_AES_128_GCM_SHA256:TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256");

    // Context creation should handle missing files safely by returning nullptr
    SSL_CTX* ctx = detail::create_ssl_context(cfg);
    EXPECT_TRUE(ctx == nullptr);
}

TEST_CASE(ServerSuite, CreateSslContextSuccessWithEcdsaCert) {
    std::string cert_file = "test_ec_cert.pem";
    std::string key_file = "test_ec_key.pem";

    // Generate temporary self-signed ECDSA P-256 certificate for testing
    std::string gen_cmd = "openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes "
                          "-keyout " + key_file + " -out " + cert_file + " -days 1 "
                          "-subj \"/CN=localhost\" >/dev/null 2>&1";
    int ret = std::system(gen_cmd.c_str());
    if (ret == 0) {
        ServerConfig cfg;
        cfg.protocol = Protocol::HTTPS;
        cfg.cert_file = cert_file;
        cfg.key_file = key_file;
        cfg.tls_version = "TLS_1_2";
        cfg.configure_ecdsa_p256_ciphers();

        SSL_CTX* ctx = detail::create_ssl_context(cfg);
        EXPECT_TRUE(ctx != nullptr);
        if (ctx) {
            SSL_CTX_free(ctx);
        }

        // Test with TLS 1.3
        cfg.tls_version = "TLS_1_3";
        SSL_CTX* ctx_tls13 = detail::create_ssl_context(cfg);
        EXPECT_TRUE(ctx_tls13 != nullptr);
        if (ctx_tls13) {
            SSL_CTX_free(ctx_tls13);
        }

        // Clean up temporary certificate files
        std::remove(cert_file.c_str());
        std::remove(key_file.c_str());
    }
}
