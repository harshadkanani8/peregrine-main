# 🔐 Feature Guide: User Authentication & Role-Based Access Control (RBAC)

This guide demonstrates how to implement a complete, secure **User Authentication and Authorization system** in Peregrine using stateless HMAC-SHA256 signed sessions, route guards, and Role-Based Access Control (RBAC).

---

## 📋 Table of Contents

1. [Architectural Security Model](#1-architectural-security-model)
2. [Session Configuration & Secret Keys](#2-session-configuration--secret-keys)
3. [User Login & Session Issuance](#3-user-login--session-issuance)
4. [Protecting Routes with Middleware Guards](#4-protecting-routes-with-middleware-guards)
5. [Role-Based Access Control (RBAC)](#5-role-based-access-control-rbac)
6. [User Logout & Session Revocation](#6-user-logout--session-revocation)
7. [Complete Runnable Example](#7-complete-runnable-example)

---

## 1. Architectural Security Model

In Peregrine, authentication state is maintained via **tamper-proof client-side session cookies**:

```text
 Unauthenticated Client               Peregrine Application Server
          │                                         │
          │─── POST /auth/login {user, pass} ──────►│
          │                                         │ 1. Validate credentials
          │                                         │ 2. req.session["user_id"] = "42"
          │                                         │ 3. req.session["role"] = "Admin"
          │                                         │ 4. Compute HMAC-SHA256 signature
          │◄── HTTP 200 (Set-Cookie: session=...) ──│
          │                                         │
          │─── GET /admin/dashboard ───────────────►│
          │    (Cookie: session=<token>)            │ 1. Verify HMAC-SHA256
          │                                         │ 2. Read req.session["role"]
          │                                         │ 3. If role == "Admin" -> Proceed
          │◄── HTTP 200 OK (Dashboard HTML) ────────│    Else -> abort(403)
```

Because the session is signed with `app.secret_key`:
* Clients cannot alter their `user_id` or escalate their `role` without invalidating the cryptographic signature.
* No centralized session database (Redis) is required, eliminating network I/O per request.

---

## 2. Session Configuration & Secret Keys

Initialize `app.secret_key` and enable cookie hardening flags:

```cpp
peregrine::App app;

// 1. High-entropy secret key
app.secret_key = "PRODUCTION_SECURE_KEY_RANDOM_BYTES_99x#2";

// 2. Cookie Security Configuration
app.config["SESSION_COOKIE_NAME"] = "auth_session";
app.config["SESSION_COOKIE_HTTPONLY"] = "true";     // Disallows document.cookie access in JS
app.config["SESSION_COOKIE_SECURE"] = "true";       // Enforces transmission over HTTPS only
app.config["SESSION_COOKIE_SAMESITE"] = "Strict";   // Mitigates cross-site CSRF attacks
app.config["PERMANENT_SESSION_LIFETIME"] = "86400"; // 24-hour expiration
```

---

## 3. User Login & Session Issuance

In the login handler, verify credentials and populate `req.session`:

```cpp
app.post("/api/login", [](peregrine::Request& req) {
    peregrine::Json body = req.get_json();
    std::string username = body["username"].as_string();
    std::string password = body["password"].as_string();

    // Verify against database (use Argon2, bcrypt, or PBKDF2 in production)
    if (username == "admin" && password == "SecretPass123!") {
        req.session["user_id"] = "1";
        req.session["username"] = "admin";
        req.session["role"] = "Administrator";
        
        // Retain cookie across browser restarts
        req.session.permanent = true;

        peregrine::Json res = peregrine::Json::object();
        res["status"] = "success";
        res["message"] = "Authenticated successfully";
        return peregrine::Response::json(res);
    }

    return peregrine::Response::json(peregrine::Json::parse(R"({"error": "Invalid credentials"})"), 401);
});
```

---

## 4. Protecting Routes with Middleware Guards

Use `before_request` hooks on Blueprints to intercept unauthorized traffic before reaching route handlers:

```cpp
peregrine::Blueprint api_bp("protected_api", "/api/v1");

// Auth Guard: Enforce valid session
api_bp.before_request([](peregrine::Request& req) {
    if (!req.session.has("user_id")) {
        peregrine::abort(401, "Authentication required. Please log in.");
    }
    
    // Inject verified identity into request context for downstream handlers
    req.g["current_user_id"] = req.session["user_id"];
    req.g["current_username"] = req.session["username"];
    req.g["current_role"] = req.session["role"];
});
```

Downstream handlers are guaranteed that `req.g["current_user_id"]` is valid:

```cpp
api_bp.get("/profile", [](peregrine::Request& req) {
    std::string user = req.g["current_username"];
    return peregrine::Response::text("Welcome to your profile, " + user);
});
```

---

## 5. Role-Based Access Control (RBAC)

Create dedicated Blueprints for higher privilege levels (e.g. Administrators):

```cpp
peregrine::Blueprint admin_bp("admin_portal", "/admin");

// Admin Guard: Enforce "Administrator" role
admin_bp.before_request([](peregrine::Request& req) {
    if (!req.session.has("user_id")) {
        peregrine::abort(401, "Please log in.");
    }
    if (req.session.get("role") != "Administrator") {
        peregrine::abort(403, "Access Denied: Administrator role required.");
    }
});

admin_bp.get("/system-diagnostics", [](peregrine::Request& req) {
    peregrine::Json diag = peregrine::Json::object();
    diag["status"] = "nominal";
    diag["cpu_load"] = "12%";
    return peregrine::Response::json(diag);
});
```

---

## 6. User Logout & Session Revocation

To terminate a session, call `req.session.clear()`:

```cpp
app.post("/api/logout", [](peregrine::Request& req) {
    req.session.clear();

    peregrine::Json res = peregrine::Json::object();
    res["status"] = "logged_out";
    return peregrine::Response::json(res);
});
```

Peregrine automatically detects that the session is empty and transmits a `Set-Cookie: auth_session=; Max-Age=0` header to delete the cookie in the client browser.

---

## 7. Complete Runnable Example

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;
    app.secret_key = "secure_production_secret_key";
    app.config["SESSION_COOKIE_NAME"] = "demo_session";

    // ------------------------------------------------------------------------
    // Public Authentication Endpoints
    // ------------------------------------------------------------------------
    app.post("/login", [](peregrine::Request& req) {
        peregrine::Json creds = req.get_json();
        std::string user = creds["username"].as_string();
        std::string pass = creds["password"].as_string();

        if (user == "admin" && pass == "admin123") {
            req.session["username"] = user;
            req.session["role"] = "Administrator";
            req.session.permanent = true;
            return peregrine::Response::text("Logged in as Admin!");
        } else if (user == "guest" && pass == "guest123") {
            req.session["username"] = user;
            req.session["role"] = "Viewer";
            return peregrine::Response::text("Logged in as Guest!");
        }

        return peregrine::Response::text("Invalid credentials", 401);
    });

    app.get("/logout", [](peregrine::Request& req) {
        req.session.clear();
        return peregrine::Response::text("Session terminated. You are logged out.");
    });

    // ------------------------------------------------------------------------
    // Protected User Area (Requires Any Authenticated Role)
    // ------------------------------------------------------------------------
    peregrine::Blueprint user_bp("user_area", "/portal");
    user_bp.before_request([](peregrine::Request& req) {
        if (!req.session.has("username")) {
            peregrine::abort(401, "Unauthorized: Please log in first.");
        }
    });

    user_bp.get("/home", [](peregrine::Request& req) {
        return peregrine::Response::text("Welcome to Portal, " + req.session["username"] + 
                                         " (Role: " + req.session["role"] + ")");
    });

    // ------------------------------------------------------------------------
    // Administrative Area (Requires "Administrator" Role)
    // ------------------------------------------------------------------------
    peregrine::Blueprint admin_bp("admin_area", "/admin");
    admin_bp.before_request([](peregrine::Request& req) {
        if (!req.session.has("username")) {
            peregrine::abort(401, "Login required.");
        }
        if (req.session.get("role") != "Administrator") {
            peregrine::abort(403, "Forbidden: Administrator role required.");
        }
    });

    admin_bp.get("/control-panel", [](peregrine::Request& req) {
        return peregrine::Response::text("Welcome to Admin Control Panel.");
    });

    // Register blueprints
    app.register_blueprint(user_bp);
    app.register_blueprint(admin_bp);

    std::cout << "Auth server running at http://127.0.0.1:8080\n";
    app.run("127.0.0.1", 8080);
    return 0;
}
```
