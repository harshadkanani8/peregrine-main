# Peregrine Testing Guide & User Documentation

Welcome to the **Peregrine Testing Guide**.

This documentation is designed for developers, contributors, and users who want to run the test suite, test their own web applications built with Peregrine, or add new unit and integration tests.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Quick Start: Running Tests](#quick-start-running-tests)
   - [Windows (PowerShell)](#windows-powershell)
   - [Windows (Command Prompt / CMD)](#windows-command-prompt--cmd)
   - [Linux & macOS (GCC / Clang)](#linux--macos-gcc--clang)
   - [Expected Output](#expected-output)
3. [Testing Your Applications with TestClient](#testing-your-applications-with-testclient)
   - [Overview](#overview)
   - [Example 1: Testing Basic Routes](#example-1-testing-basic-routes)
   - [Example 2: Testing JSON REST APIs](#example-2-testing-json-rest-apis)
   - [Example 3: Testing Form Submissions](#example-3-testing-form-submissions)
   - [Example 4: Testing Authentication & Sessions (Cookie Jar)](#example-4-testing-authentication--sessions-cookie-jar)
   - [Example 5: Testing Middlewares & Security Headers](#example-5-testing-middlewares--security-headers)
   - [Example 6: Testing Custom Error Handlers (404 / 500)](#example-6-testing-custom-error-handlers-404--500)
   - [TestClient API Reference](#testclient-api-reference)
4. [Writing Tests with test_framework.hpp](#writing-tests-with-test_frameworkhpp)
   - [Defining Test Cases](#defining-test-cases)
   - [Available Assertions](#available-assertions)
5. [Running Specific Test Suites](#running-specific-test-suites)
6. [Test Directory Structure](#test-directory-structure)
7. [Security Verification](#security-verification)
8. [Frequently Asked Questions (FAQ) & Troubleshooting](#frequently-asked-questions-faq--troubleshooting)

---

## Introduction

Peregrine includes a self-contained, header-only testing framework designed for speed, simplicity, and zero external dependencies:

- **In-Memory Testing**: Tests execute directly in memory without binding to network ports (`localhost:8080`). This avoids port collisions, firewall prompts, and network delays.
- **Fast Execution**: The complete 52-test verification suite executes in under 35 milliseconds.
- **Zero External Dependencies**: Requires no third-party test libraries (such as GoogleTest, Catch2, or Boost).
- **Production Code Integrity**: Tests compile directly against untouched framework headers in `include/peregrine/`.

---

## Quick Start: Running Tests

Run the full verification suite from the project root directory using any standard C++11 compiler.

### Windows (PowerShell)

```powershell
g++ -std=c++11 -I tests -I tests/compat -I include `
    tests/test_main.cpp `
    tests/test_common.cpp `
    tests/test_config.cpp `
    tests/test_types.cpp `
    tests/test_json.cpp `
    tests/test_template.cpp `
    tests/test_routing.cpp `
    tests/test_blueprint.cpp `
    tests/test_view.cpp `
    tests/test_session.cpp `
    tests/test_csrf.cpp `
    tests/test_helpers.cpp `
    tests/test_server.cpp `
    -lws2_32 -o test_suite.exe; .\test_suite.exe
```

### Windows (Command Prompt / CMD)

```cmd
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_common.cpp tests/test_config.cpp tests/test_types.cpp tests/test_json.cpp tests/test_template.cpp tests/test_routing.cpp tests/test_blueprint.cpp tests/test_view.cpp tests/test_session.cpp tests/test_csrf.cpp tests/test_helpers.cpp tests/test_server.cpp -lws2_32 -o test_suite.exe && test_suite.exe
```

### Linux & macOS (GCC / Clang)

```bash
g++ -std=c++11 -I tests -I include \
    tests/test_main.cpp \
    tests/test_common.cpp \
    tests/test_config.cpp \
    tests/test_types.cpp \
    tests/test_json.cpp \
    tests/test_template.cpp \
    tests/test_routing.cpp \
    tests/test_blueprint.cpp \
    tests/test_view.cpp \
    tests/test_session.cpp \
    tests/test_csrf.cpp \
    tests/test_helpers.cpp \
    tests/test_server.cpp \
    -lssl -lcrypto -lpthread -o test_suite && ./test_suite
```

### Expected Output

```text
======================================================================
  ____                              _             __ __ 
 /  __\___  ________  ____ ________(_)_  _____   / // / 
/ /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \/ -_) / // /_ 
/ .___/\__//_/  \__/\_, /_/  /_/_/_/_//_/\__/ /__  __/ 
/_/                 /___/                        /_/    
             Peregrine Unit Tests & Verification Suite
======================================================================

[==========] Running 52 test cases.
[----------] Global test suite: ServerSuite (5 tests)  -> All Passed
[----------] Global test suite: HelpersSuite (3 tests) -> All Passed
[----------] Global test suite: CsrfSuite (5 tests)    -> All Passed
[----------] Global test suite: SessionSuite (3 tests) -> All Passed
[----------] Global test suite: ViewSuite (1 test)     -> All Passed
[----------] Global test suite: BlueprintSuite (3 tests)-> All Passed
[----------] Global test suite: RoutingSuite (3 tests) -> All Passed
[----------] Global test suite: TemplateSuite (5 tests)-> All Passed
[----------] Global test suite: JsonSuite (7 tests)    -> All Passed
[----------] Global test suite: TypesSuite (3 tests)   -> All Passed
[----------] Global test suite: ConfigSuite (4 tests)  -> All Passed
[----------] Global test suite: CommonSuite (10 tests) -> All Passed

[==========] Total tests executed: 52 across 31 ms.
[  PASSED  ] 52 tests passed.
[  ALL TESTS PASSED!  ]
```

---

## Testing Your Applications with TestClient

### Overview

`TestClient` (defined in `tests/test_client.hpp`) is an in-memory client that simulates HTTP requests against your `App` instance. It invokes routing, middleware, templates, and sessions directly in memory and returns real `Response` objects.

To use it in a test file:

```cpp
#include "test_framework.hpp"
#include "test_client.hpp"

using namespace peregrine;
```

---

### Example 1: Testing Basic Routes

Verify that an endpoint returns HTTP 200 and expected text or HTML:

```cpp
TEST_CASE(ApplicationSuite, IndexRouteReturnsGreeting) {
    App app("demo_app");

    app.get("/", [](Request& req) {
        return Response::html("<h1>Welcome to Peregrine</h1>");
    });

    TestClient client(app);
    Response res = client.get("/");

    EXPECT_EQ(res.status, 200);
    EXPECT_CONTAINS(res.body, "Welcome to Peregrine");
}
```

---

### Example 2: Testing JSON REST APIs

Verify route parameters and JSON serialization:

```cpp
TEST_CASE(ApplicationSuite, UserApiReturnsJson) {
    App app("demo_app");

    app.get("/api/users/<int:id>", [](Request& req) {
        int user_id = std::stoi(req.path_params["id"]);

        Json data = Json::object();
        data["id"] = user_id;
        data["name"] = "Alice";
        data["active"] = true;

        return Response::json(data);
    });

    TestClient client(app);
    Response res = client.get("/api/users/101");

    EXPECT_EQ(res.status, 200);
    EXPECT_EQ(res.headers["content-type"], "application/json");
    EXPECT_CONTAINS(res.body, "\"id\":101");
    EXPECT_CONTAINS(res.body, "\"name\":\"Alice\"");
}
```

---

### Example 3: Testing Form Submissions

Send form data using `client.post` with key-value pairs:

```cpp
TEST_CASE(ApplicationSuite, ContactFormValidation) {
    App app("demo_app");

    app.post("/contact", [](Request& req) {
        std::string email = req.form_get("email");
        if (email.empty()) {
            return Response::text("Email is required", 400);
        }
        return Response::text("Thank you for contacting us");
    });

    TestClient client(app);

    // 1. Missing field returns HTTP 400
    Response bad_req = client.post("/contact", {});
    EXPECT_EQ(bad_req.status, 400);
    EXPECT_EQ(bad_req.body, "Email is required");

    // 2. Valid field returns HTTP 200
    Response good_req = client.post("/contact", {{"email", "user@example.com"}});
    EXPECT_EQ(good_req.status, 200);
    EXPECT_EQ(good_req.body, "Thank you for contacting us");
}
```

---

### Example 4: Testing Authentication & Sessions (Cookie Jar)

`TestClient` includes an automated **Cookie Jar**. When a route sets cookies (such as a signed session cookie on login), `TestClient` stores them and includes them in subsequent requests:

```cpp
TEST_CASE(ApplicationSuite, LoginAndProtectedDashboardFlow) {
    App app("demo_app");
    app.secret_key = "secret_verification_key";

    // Login endpoint
    app.post("/login", [](Request& req) {
        std::string username = req.form_get("username");
        if (username == "admin") {
            req.session["user"] = username;
            req.session.permanent = true;
            return Response::text("Login Successful");
        }
        return Response::text("Invalid Credentials", 401);
    });

    // Protected endpoint
    app.get("/dashboard", [](Request& req) {
        if (!req.session.has("user")) {
            return Response::text("Unauthorized", 401);
        }
        return Response::text("Welcome, " + req.session["user"]);
    });

    TestClient client(app);

    // Step 1: Unauthenticated access is rejected
    Response r1 = client.get("/dashboard");
    EXPECT_EQ(r1.status, 401);

    // Step 2: Login sets session cookie
    Response r2 = client.post("/login", {{"username", "admin"}});
    EXPECT_EQ(r2.status, 200);
    EXPECT_TRUE(client.has_cookie("session"));

    // Step 3: Subsequent request automatically includes cookie
    Response r3 = client.get("/dashboard");
    EXPECT_EQ(r3.status, 200);
    EXPECT_EQ(r3.body, "Welcome, admin");
}
```

---

### Example 5: Testing Middlewares & Security Headers

Verify that global middlewares execute on requests and responses:

```cpp
TEST_CASE(ApplicationSuite, MiddlewareSecurityHeaders) {
    App app("demo_app");

    app.after_request([](Request&, Response& res) {
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
    });

    app.get("/ping", [](Request&) {
        return Response::text("pong");
    });

    TestClient client(app);
    Response res = client.get("/ping");

    EXPECT_EQ(res.status, 200);
    EXPECT_EQ(res.headers["X-Content-Type-Options"], "nosniff");
    EXPECT_EQ(res.headers["X-Frame-Options"], "DENY");
}
```

---

### Example 6: Testing Custom Error Handlers (404 / 500)

Verify custom error handling when routes are missing or `abort()` is called:

```cpp
TEST_CASE(ApplicationSuite, Custom404Page) {
    App app("demo_app");

    app.error_handler(404, [](Request& req) {
        return Response::html("<h1>Page Not Found</h1>", 404);
    });

    TestClient client(app);
    Response res = client.get("/unknown_endpoint");

    EXPECT_EQ(res.status, 404);
    EXPECT_CONTAINS(res.body, "Page Not Found");
}
```

---

### TestClient API Reference

| Method | Description |
| :--- | :--- |
| `get(path, headers)` | Simulates an HTTP GET request |
| `post(path, form_data, headers)` | Simulates an HTTP POST request with URL-encoded form data |
| `post_json(path, json_payload, headers)` | Simulates an HTTP POST request with serialized JSON body |
| `put(path, body, headers)` | Simulates an HTTP PUT request |
| `del(path, headers)` | Simulates an HTTP DELETE request |
| `set_cookie(name, value)` | Manually sets a cookie in the client cookie jar |
| `get_cookie(name)` | Retrieves a stored cookie value |
| `has_cookie(name)` | Checks if a cookie exists in the jar |
| `clear_cookies()` | Clears all stored cookies from the jar |

---

## Writing Tests with test_framework.hpp

Peregrine's test runner is header-only and adheres to C++11.

### Defining Test Cases

Use the `TEST_CASE` macro. Tests register automatically at startup:

```cpp
#include "test_framework.hpp"
#include "peregrine/common.hpp"

using namespace peregrine;

TEST_CASE(ExampleSuite, StringTransformation) {
    std::string text = "Hello World";
    EXPECT_EQ(to_lower(text), "hello world");
    EXPECT_TRUE(starts_with(text, "Hello"));
}
```

### Available Assertions

| Macro | Description |
| :--- | :--- |
| `EXPECT_EQ(a, b)` | Asserts that `a == b`. Prints values on mismatch. |
| `EXPECT_NE(a, b)` | Asserts that `a != b`. Prints values if equal. |
| `EXPECT_TRUE(cond)` | Asserts that condition evaluates to `true`. |
| `EXPECT_FALSE(cond)` | Asserts that condition evaluates to `false`. |
| `EXPECT_CONTAINS(haystack, needle)` | Asserts that `needle` is a substring of `haystack`. |
| `EXPECT_THROW(expr, ExceptionType)` | Asserts that `expr` throws `ExceptionType`. |

When an assertion fails, the runner reports the file, line number, expected value, and actual value, then continues executing remaining tests.

---

## Running Specific Test Suites

Any test file can be compiled with `test_main.cpp` to run in isolation:

```powershell
# Run JSON tests only
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_json.cpp -o test_json.exe
.\test_json.exe

# Run Routing tests only
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_routing.cpp -o test_routing.exe
.\test_routing.exe

# Run Template tests only
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_template.cpp -o test_template.exe
.\test_template.exe

# Run Session & Security tests only
g++ -std=c++11 -I tests -I tests/compat -I include tests/test_main.cpp tests/test_session.cpp -o test_session.exe
.\test_session.exe
```

---

## Test Directory Structure

| File | Description |
| :--- | :--- |
| `test_framework.hpp` | Header-only test runner with assertions, timing, and formatted output |
| `test_client.hpp` | In-memory mock HTTP client with automated Cookie Jar |
| `test_main.cpp` | Main entry point that executes all registered suites |
| `test_common.cpp` | Unit tests for string utilities, URL encoding, Base64URL, and timing comparisons |
| `test_config.cpp` | Unit tests for application configuration and type conversions |
| `test_types.cpp` | Unit tests for MIME resolution and uploaded file persistence |
| `test_json.cpp` | Unit tests for PIMPL JSON parsing, AST, and serialization |
| `test_template.cpp` | Unit tests for mustache templating and loop rendering |
| `test_routing.cpp` | Unit tests for static routing, path parameters, and HTTP 405 validation |
| `test_blueprint.cpp` | Unit tests for modular blueprints and scoped middlewares |
| `test_view.cpp` | Unit tests for class-based REST MethodView dispatching |
| `test_session.cpp` | Unit tests for HMAC-SHA256 session signing and tamper rejection |
| `test_csrf.cpp` | Unit tests for CSRF token generation and multi-channel verification |
| `test_helpers.cpp` | Unit tests for response helpers and Path Traversal defense |
| `test_server.cpp` | Unit tests for HTTP request parsing and response serialization |
| `compat/` | Portability layer for standalone crypto and cross-platform socket headers |

---

## Security Verification

The test suite validates protection against common web vulnerabilities:

1. **Path Traversal Defense (CWE-22)** (`test_helpers.cpp`):
   Verifies that attempts to traverse directories (`../../etc/passwd`, `..\..\windows\win.ini`) or embed null bytes (`\0`) are rejected.
2. **Session Tamper Resistance** (`test_session.cpp`):
   Verifies that payload bit alterations cause HMAC-SHA256 signature verification to fail and discard invalid sessions.
3. **Cross-Site Request Forgery (CSRF)** (`test_csrf.cpp`):
   Verifies validation across form fields, headers (`X-CSRF-Token`, `X-CSRFToken`), and query strings.
4. **Timing Attack Protection** (`test_common.cpp`):
   Verifies constant-time bitwise comparison for cryptographic signatures.

---

## Frequently Asked Questions (FAQ) & Troubleshooting

### Q: Do I need to install external test frameworks?
**A:** No. Peregrine includes its own header-only test runner in `test_framework.hpp`. Standard C++11 is the only requirement.

### Q: Why is `-lws2_32` used on Windows?
**A:** On Windows, socket APIs reside in the `ws2_32` system library. The `-lws2_32` flag links against it.

### Q: What is the purpose of `tests/compat/`?
**A:** The `compat/` directory provides test-time compatibility headers, including an RFC 4231-compliant standalone HMAC-SHA256 implementation and POSIX socket wrappers. This allows tests to run without requiring system-level OpenSSL development packages on Windows.

### Q: Are any files in `include/` modified by tests?
**A:** No. Tests strictly compile against the original production headers. No production code is modified.
