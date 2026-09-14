# 📦 Module 03: Blueprints & Middleware Architecture

As web applications grow in size and complexity, maintaining all routes and middleware in a single file becomes unwieldy. Peregrine provides **Blueprints** to organize applications into modular, reusable sub-components with isolated route prefixes, scoped middleware hooks, and dedicated error handlers.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [Creating & Registering Blueprints](#2-creating--registering-blueprints)
   - [Instantiating a Blueprint](#instantiating-a-blueprint)
   - [Mounting to the Application (`register_blueprint`)](#mounting-to-the-application-register_blueprint)
   - [Prefix Overrides](#prefix-overrides)
3. [Scoped Middleware Lifecycle](#3-scoped-middleware-lifecycle)
   - [Global vs. Blueprint Hooks](#global-vs-blueprint-hooks)
   - [Execution Order Diagram](#execution-order-diagram)
   - [Context Sharing via `req.g`](#context-sharing-via-reqg)
4. [Blueprint Error Handlers](#4-blueprint-error-handlers)
5. [Complete Code Examples](#5-complete-code-examples)

---

## 1. Architectural Overview

Defined in `include/peregrine/blueprint.hpp`.

A `peregrine::Blueprint` acts as a recording object for routes, hooks, and error handlers. Routes defined on a Blueprint are deferred until the Blueprint is registered onto an `App` instance.

During registration (`app.register_blueprint`):
1. Route patterns are prepended with the Blueprint's URL prefix.
2. Handlers are dynamically wrapped in a closure that executes the Blueprint's specific `before_request` and `after_request` chains.
3. Blueprint error handlers are merged into the application's fallback table.

---

## 2. Creating & Registering Blueprints

### Instantiating a Blueprint

Construct a Blueprint with a unique logical name and an optional default URL prefix:

```cpp
#include <peregrine/peregrine.hpp>

// Create an authentication blueprint with prefix "/auth"
peregrine::Blueprint auth_bp("auth", "/auth");

auth_bp.get("/login", [](peregrine::Request& req) {
    return peregrine::Response::text("Login Form");
});

auth_bp.post("/login", [](peregrine::Request& req) {
    return peregrine::Response::text("Processing Login");
});
```

---

### Mounting to the Application (`register_blueprint`)

Register the Blueprint onto your primary `App` coordinator:

```cpp
peregrine::App app;

// Routes become: GET /auth/login and POST /auth/login
app.register_blueprint(auth_bp);
```

---

### Prefix Overrides

You can override the URL prefix at registration time. This enables the same Blueprint to be mounted at different URL hierarchies or versioned endpoints:

```cpp
peregrine::Blueprint api_bp("api_v1");

api_bp.get("/users", list_users);

// Mount under /api/v1
app.register_blueprint(api_bp, "/api/v1");

// Mount the same blueprint under /api/v2
app.register_blueprint(api_bp, "/api/v2");
```

---

## 3. Scoped Middleware Lifecycle

### Global vs. Blueprint Hooks

Peregrine supports two tiers of middleware hooks:

1. **Global Hooks (`app.before_request`, `app.after_request`)**:
   - Run on **every** incoming request, regardless of which route or blueprint handles it.
   - Ideal for global logging, security headers (HSTS, CSP), and rate limiting.

2. **Blueprint-Scoped Hooks (`bp.before_request`, `bp.after_request`)**:
   - Run **only** when a route belonging to that specific Blueprint is matched.
   - Ideal for authentication guards, role validation, or blueprint-specific headers.

---

### Execution Order Diagram

```text
 Client Request
       │
       ▼
 [ Global app.before_request ]
       │
       ▼
 [ Match Blueprint Route ]
       │
       ▼
 [ Blueprint bp.before_request ]
       │
       ▼
 [ Blueprint Route Handler ]
       │
       ▼
 [ Blueprint bp.after_request ]
       │
       ▼
 [ Global app.after_request ]
       │
       ▼
 Client Wire Response
```

---

### Context Sharing via `req.g`

Because request processing is strictly thread-isolated, data can be safely passed down the pipeline using `req.g` (a per-request `std::map<std::string, std::string>`):

```cpp
peregrine::Blueprint admin_bp("admin", "/admin");

// Scoped Guard: Verify API Key before any admin route executes
admin_bp.before_request([](peregrine::Request& req) {
    std::string key = req.header("X-Admin-Key");
    if (key != "secret-master-token") {
        // Aborts early with HTTP 401
        peregrine::abort(401, "Invalid or missing administrative token.");
    }
    // Store validated identity for downstream handlers
    req.g["authenticated_role"] = "superadmin";
});

admin_bp.get("/dashboard", [](peregrine::Request& req) {
    std::string role = req.g["authenticated_role"];
    return peregrine::Response::text("Welcome, " + role);
});
```

---

## 4. Blueprint Error Handlers

Blueprints can register custom error handlers:

```cpp
admin_bp.error_handler(401, [](peregrine::Request& req) {
    peregrine::Json err = peregrine::Json::object();
    err["status"] = "unauthorized";
    err["scope"] = "admin_portal";
    return peregrine::Response::json(err, 401);
});
```

When registered, these handlers are registered with the application coordinator if not already claimed by a global handler.

---

## 5. Complete Code Examples

### Multi-Module Microservice Project

Here is a full application structuring modular Blueprints across User and Admin domains:

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

// ============================================================================
// Users Blueprint (/api/v1/users)
// ============================================================================
peregrine::Blueprint make_users_bp() {
    peregrine::Blueprint bp("users", "/api/v1/users");

    bp.get("/", [](peregrine::Request& req) {
        peregrine::Json list = peregrine::Json::array();
        list.push_back("Alice");
        list.push_back("Bob");
        return peregrine::Response::json(list);
    });

    bp.get("/<int:id>", [](peregrine::Request& req) {
        peregrine::Json user = peregrine::Json::object();
        user["id"] = std::stoi(req.path_params["id"]);
        user["name"] = "Alice";
        return peregrine::Response::json(user);
    });

    return bp;
}

// ============================================================================
// Metrics & Admin Blueprint (/admin)
// ============================================================================
peregrine::Blueprint make_admin_bp() {
    peregrine::Blueprint bp("admin", "/admin");

    // Scoped Auth Guard
    bp.before_request([](peregrine::Request& req) {
        if (req.header("Authorization") != "Bearer secret-admin-token") {
            peregrine::abort(403, "Forbidden: Administrative access required.");
        }
    });

    // Injected Response Header
    bp.after_request([](peregrine::Request& req, peregrine::Response& res) {
        res.set_header("X-Audit-Logged", "true");
    });

    bp.get("/stats", [](peregrine::Request& req) {
        peregrine::Json stats = peregrine::Json::object();
        stats["active_connections"] = 42;
        stats["memory_mb"] = 18.4;
        return peregrine::Response::json(stats);
    });

    return bp;
}

// ============================================================================
// Main Application Coordinator
// ============================================================================
int main() {
    peregrine::App app;

    // 1. Global Middleware: Logging and Latency tracking
    app.before_request([](peregrine::Request& req) {
        std::cout << "[REQ] " << req.method << " " << req.path << "\n";
    });

    // 2. Global Middleware: Security headers
    app.after_request([](peregrine::Request& req, peregrine::Response& res) {
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("X-Content-Type-Options", "nosniff");
    });

    // 3. Register Blueprints
    app.register_blueprint(make_users_bp());
    app.register_blueprint(make_admin_bp());

    // 4. Run application
    app.run("127.0.0.1", 8080);
    return 0;
}
```
