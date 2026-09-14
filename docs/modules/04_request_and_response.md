# 📦 Module 04: Request & Response Objects

Every web transaction in Peregrine centers around two fundamental structures: `peregrine::Request` and `peregrine::Response`. Both structures are stack-allocated, thread-isolated, and designed for zero-overhead, intuitive inspection and mutation.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [The `peregrine::Request` Object](#2-the-peregrinerequest-object)
   - [Core Member Variables](#core-member-variables)
   - [Query String Inspection (`req.arg`)](#query-string-inspection-reqarg)
   - [Header Access (Case-Insensitive)](#header-access-case-insensitive)
   - [Form Fields & Multipart Uploads](#form-fields--multipart-uploads)
   - [JSON Payloads (`req.get_json`)](#json-payloads-reqget_json)
   - [Per-Request Context Storage (`req.g`)](#per-request-context-storage-reqg)
3. [The `peregrine::Response` Object](#3-the-peregrineresponse-object)
   - [Static Factory Methods](#static-factory-methods)
   - [Headers & Status Mutators](#headers--status-mutators)
   - [Cookie Management (`set_cookie`, `delete_cookie`)](#cookie-management-set_cookie-delete_cookie)
   - [Streaming & MJPEG Video Feeds](#streaming--mjpeg-video-feeds)
4. [Helper Functions & Exceptions (`helpers.hpp`)](#4-helper-functions--exceptions-helpershpp)
   - [`abort()` and `HTTPException`](#abort-and-httpexception)
   - [`jsonify()`, `make_response()`, `redirect()`](#jsonify-make_response-redirect)
5. [Complete Code Examples](#5-complete-code-examples)

---

## 1. Architectural Overview

Defined in `include/peregrine/request.hpp` and `include/peregrine/response.hpp`.

When an HTTP request is read from the socket:
1. The wire parser populates a local `Request` instance on the worker thread's stack.
2. The `Request` is passed by reference into your handler function.
3. Your handler constructs and returns a `Response` object by value (using move semantics for zero copies).
4. Peregrine serializes the `Response` directly to the socket connection and cleans up all stack allocations.

---

## 2. The `peregrine::Request` Object

### Core Member Variables

```cpp
struct Request {
    std::string method;          // "GET", "POST", "PUT", etc.
    std::string path;            // URL-decoded path (e.g. "/api/items")
    std::string raw_target;      // Full raw target including query string
    std::string http_version;    // "HTTP/1.1"

    std::map<std::string, std::string> headers;     // Keys stored in lowercase
    std::map<std::string, std::string> query;       // Query string parameters
    std::map<std::string, std::string> path_params; // Matched route variables
    std::map<std::string, std::string> cookies;     // Client cookies
    std::string body;                               // Raw request payload

    std::map<std::string, std::string> form;        // Form key-value pairs
    std::vector<UploadedFile> files;                // Uploaded files
    Session session;                                // Signed session container
    std::map<std::string, std::string> g;           // Per-request context storage

    std::string scheme;          // "http" or "https"
    std::string remote_addr;     // Client IP address (e.g. "192.168.1.100")
};
```

---

### Query String Inspection (`req.arg`)

Inspect query string parameters (`?key=value&sort=desc`) safely with default fallbacks:

```cpp
app.get("/search", [](peregrine::Request& req) {
    // Check for parameter existence
    if (!req.has_arg("q")) {
        return peregrine::Response::text("Missing query parameter 'q'", 400);
    }

    // Retrieve parameter with fallback default
    std::string query = req.arg("q");
    std::string page = req.arg("page", "1");
    std::string limit = req.arg("limit", "20");

    return peregrine::Response::text("Searching for: " + query + " (Page: " + page + ")");
});
```

---

### Header Access (Case-Insensitive)

HTTP header names are case-insensitive per RFC 7230. Peregrine stores all header keys in lowercase during parsing:

```cpp
app.get("/auth-check", [](peregrine::Request& req) {
    // Both retrieve "Authorization" regardless of client casing
    std::string auth1 = req.header("Authorization");
    std::string auth2 = req.header("authorization");

    bool has_agent = req.has_header("User-Agent");
    return peregrine::Response::text("Authenticated: " + auth1);
});
```

---

### Form Fields & Multipart Uploads

For `application/x-www-form-urlencoded` or `multipart/form-data`:

```cpp
app.post("/submit", [](peregrine::Request& req) {
    std::string username = req.form_get("username");
    std::string email = req.form_get("email", "unknown@example.com");

    if (!req.files.empty()) {
        const auto& file = req.files[0];
        file.save("uploads/" + file.filename);
    }

    return peregrine::Response::text("Submission received for: " + username);
});
```

---

### JSON Payloads (`req.get_json`)

Inspect and deserialize incoming JSON:

```cpp
app.post("/api/users", [](peregrine::Request& req) {
    if (!req.is_json()) {
        peregrine::abort(415, "Content-Type must be application/json");
    }

    peregrine::Json body = req.get_json();
    if (body.is_null()) {
        peregrine::abort(400, "Malformed or empty JSON body");
    }

    std::string name = body["name"].as_string();
    int age = body["age"].as_int();

    return peregrine::Response::text("Created user " + name);
});
```

---

### Per-Request Context Storage (`req.g`)

Pass arbitrary data between pre-request hooks, route handlers, and post-request hooks:

```cpp
// Set in before_request
app.before_request([](peregrine::Request& req) {
    req.g["request_start_ms"] = std::to_string(get_current_timestamp_ms());
});

// Read in route handler
app.get("/ping", [](peregrine::Request& req) {
    std::string start = req.g["request_start_ms"];
    return peregrine::Response::text("Started at: " + start);
});
```

---

## 3. The `peregrine::Response` Object

### Static Factory Methods

Construct standard responses concisely:

```cpp
// 1. Plain Text (Content-Type: text/plain; charset=utf-8)
auto r1 = peregrine::Response::text("Operation successful", 200);

// 2. HTML Document (Content-Type: text/html; charset=utf-8)
auto r2 = peregrine::Response::html("<h1>Welcome!</h1>", 200);

// 3. JSON Payload (Content-Type: application/json)
peregrine::Json j = peregrine::Json::object();
j["success"] = true;
auto r3 = peregrine::Response::json(j, 201);

// 4. HTTP Redirection (Location: /target, status: 302 or 301)
auto r4 = peregrine::Response::redirect("/dashboard", 302);
```

---

### Headers & Status Mutators

Chain or mutate headers and status codes fluently:

```cpp
app.get("/custom", [](peregrine::Request& req) {
    peregrine::Response res = peregrine::Response::text("Custom response");
    res.set_status(202)
       .set_header("X-Custom-Header", "MyValue")
       .set_header("Access-Control-Allow-Origin", "*");
    return res;
});
```

---

### Cookie Management (`set_cookie`, `delete_cookie`)

Set cookies with comprehensive security flags:

```cpp
res.set_cookie(
    "auth_token",          // Name
    "xyz987token",         // Value
    3600 * 24,             // Max-Age in seconds (1 day)
    "/",                   // Path
    "",                    // Domain
    true,                  // HttpOnly (blocks document.cookie access)
    true,                  // Secure (only transmitted over HTTPS)
    "Strict",              // SameSite ("Lax", "Strict", "None")
    false                  // Partitioned (CHIPS cookie standard)
);

// Delete cookie immediately on the client browser
res.delete_cookie("auth_token", "/");
```

---

### Streaming & MJPEG Video Feeds

Keep the underlying socket connection open to stream data chunks continuously:

```cpp
// 1. Arbitrary continuous stream
app.get("/stream/events", [](peregrine::Request& req) {
    return peregrine::Response::stream("text/event-stream", [](peregrine::Connection& conn) {
        for (int i = 0; i < 5; ++i) {
            std::string event = "data: ping " + std::to_string(i) + "\n\n";
            conn.write(event.data(), event.size());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
});

// 2. Multi-part MJPEG Camera Stream
app.get("/video/live.mjpg", [](peregrine::Request& req) {
    return peregrine::Response::mjpeg([](peregrine::Connection& conn) {
        while (camera_is_active()) {
            std::string frame = capture_jpeg_frame();
            std::string header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " +
                                 std::to_string(frame.size()) + "\r\n\r\n";
            conn.write(header.data(), header.size());
            conn.write(frame.data(), frame.size());
            conn.write("\r\n", 2);
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 fps
        }
    });
});
```

---

## 4. Helper Functions & Exceptions (`helpers.hpp`)

Defined in `include/peregrine/helpers.hpp`.

### `abort()` and `HTTPException`

Interrupt handler execution from any depth and immediately trigger an HTTP error status:

```cpp
void require_admin(peregrine::Request& req) {
    if (req.header("X-Role") != "Admin") {
        // Throws peregrine::HTTPException(403, "Admin privileges required")
        peregrine::abort(403, "Admin privileges required");
    }
}

app.get("/admin/secrets", [](peregrine::Request& req) {
    require_admin(req); // Aborts early if unauthorized
    return peregrine::Response::text("Classified Data");
});
```

---

### `jsonify()`, `make_response()`, `redirect()`

Convenience aliases matching familiar microframework conventions:

```cpp
// Returns Response::json(j, status)
return peregrine::jsonify(my_json_object, 200);

// Returns Response::text(str, status)
return peregrine::make_response("Plain text content", 200);

// Returns Response::redirect(url, status)
return peregrine::redirect("/login", 303);
```

---

## 5. Complete Code Examples

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    // Echo endpoint demonstrating request reflection
    app.post("/api/echo", [](peregrine::Request& req) {
        peregrine::Json res = peregrine::Json::object();
        res["method"] = req.method;
        res["path"] = req.path;
        res["client_ip"] = req.remote_addr;
        res["is_secure"] = req.is_secure();

        // Echo back all query arguments
        peregrine::Json args = peregrine::Json::object();
        for (const auto& kv : req.query) {
            args[kv.first] = kv.second;
        }
        res["query_params"] = args;

        // Parse body if JSON
        if (req.is_json()) {
            res["parsed_body"] = req.get_json();
        } else {
            res["raw_body_length"] = static_cast<int>(req.body.size());
        }

        return peregrine::Response::json(res, 200);
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```
