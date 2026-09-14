# 📦 Module 06: Stateless Sessions & Cookie Security

Peregrine implements a production-grade, client-side **Stateless Signed Cookie Session Engine** powered by HMAC-SHA256. It eliminates the need for server-side state (such as Redis or Memcached) while guaranteeing tamper-proof session data, zero-downtime key rotation, and strict modern cookie security controls.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [Session Lifecycle in Peregrine](#2-session-lifecycle-in-peregrine)
   - [Opening the Session (`open_session`)](#opening-the-session-open_session)
   - [Access Tracking & `Vary: Cookie`](#access-tracking--vary-cookie)
   - [Saving the Session (`save_session`)](#saving-the-session-save_session)
3. [Cryptographic Integrity & Key Rotation](#3-cryptographic-integrity--key-rotation)
   - [HMAC-SHA256 Wire Format](#hmac-sha256-wire-format)
   - [Zero-Downtime Key Rotation (`secret_key_fallbacks`)](#zero-downtime-key-rotation-secret_key_fallbacks)
4. [Permanent vs. Browser Sessions](#4-permanent-vs-browser-sessions)
5. [Security Flags & Cookie Configuration](#5-security-flags--cookie-configuration)
6. [Complete Code Examples](#6-complete-code-examples)

---

## 1. Architectural Overview

Defined in `include/peregrine/session.hpp`.

In high-concurrency microservices, managing server-side session stores introduces database network round-trips and shared single points of failure. 

Peregrine's stateless session architecture signs the session payload cryptographically on the server using HMAC-SHA256:

```text
 Client Browser                                      Peregrine Server
       │                                                     │
       │─── GET /dashboard (Cookie: session=<token>) ───────►│
       │                                                     │ 1. Decrypt Base64 & Timestamp
       │                                                     │ 2. Compute HMAC-SHA256(secret)
       │                                                     │ 3. Constant-time verify signature
       │                                                     │ 4. Populate req.session
       │                                                     │
       │                                                     │ [Handler mutates session]
       │                                                     │
       │                                                     │ 5. Compute new HMAC signature
       │                                                     │ 6. Set-Cookie: session=<new_token>
       │◄── HTTP/1.1 200 OK (Set-Cookie: session=...) ───────│
```

---

## 2. Session Lifecycle in Peregrine

### Opening the Session (`open_session`)

Before any route handler executes:
1. Peregrine inspects `req.cookies` for the configured `session_cookie_name`.
2. If found, it parses the token, extracts the timestamp, and verifies the HMAC-SHA256 signature against `app.secret_key`.
3. If the signature is valid and the timestamp has not expired (`PERMANENT_SESSION_LIFETIME`), `req.session` is populated.
4. If invalid or tampered with, the cookie is silently discarded and an empty new session (`req.session.is_new = true`) is initialized.

---

### Access Tracking & `Vary: Cookie`

Whenever your application code reads or writes a session value (e.g. `req.session.get("user")` or `req.session["role"]`), the session marks itself as **accessed**.

To prevent downstream caching proxies (like CDNs or Varnish) from inadvertently caching personalized authenticated responses for anonymous users, Peregrine automatically injects the HTTP response header:
```text
Vary: Cookie
```

---

### Saving the Session (`save_session`)

After the route handler returns:
1. If `req.session` was modified, Peregrine serializes the data to JSON, generates a new HMAC-SHA256 token, and calls `res.set_cookie()`.
2. If `req.session.clear()` was called or the session is empty, Peregrine emits a deletion cookie with `Max-Age=0`.
3. If the session was only read (unmodified), no redundant `Set-Cookie` header is transmitted over the wire.

---

## 3. Cryptographic Integrity & Key Rotation

### HMAC-SHA256 Wire Format

A Peregrine session token has the format:
```text
<Base64-Payload>.<Timestamp>.<Base64-HMAC-SHA256-Signature>
```
* **Base64-Payload**: URL-safe Base64-encoded JSON representation of the session dictionary.
* **Timestamp**: Unix epoch timestamp recording when the token was signed.
* **Signature**: HMAC-SHA256 computed over `<Base64-Payload>.<Timestamp>` using `app.secret_key`.

Signature verification uses constant-time XOR comparison to prevent timing attacks.

---

### Zero-Downtime Key Rotation (`secret_key_fallbacks`)

When updating secret keys in production, you cannot invalidate thousands of currently active user sessions at once. Peregrine provides seamless cryptographic key rotation:

```cpp
peregrine::App app;

// Primary key: Used for signing all NEW and MODIFIED session cookies
app.secret_key = "current_production_key_2026";

// Fallback keys: Checked if primary key validation fails
app.secret_key_fallbacks = {
    "previous_production_key_2025",
    "legacy_migration_key_2024"
};
```

#### How Rotation Works:
1. An incoming cookie signed with `previous_production_key_2025` arrives.
2. Verification against `current_production_key_2026` fails.
3. Peregrine tries the fallback keys in order. It validates successfully against the 2025 key!
4. Peregrine automatically flags the session as `modified = true`.
5. When the response is returned, the session is re-signed with the **new primary key** (`current_production_key_2026`).
6. The client seamlessly transitions to the new key without being logged out.

---

## 4. Permanent vs. Browser Sessions

By default, sessions are browser-lifetime cookies (deleted when the user closes their browser).

To create a persistent session (persisting across browser restarts):

```cpp
app.post("/login", [](peregrine::Request& req) {
    req.session["user_id"] = "42";
    req.session["username"] = "alice";

    // Set permanent flag
    req.session.permanent = true;

    return peregrine::Response::redirect("/dashboard");
});
```

When `permanent = true`:
* The cookie receives `Max-Age = <session_lifetime_seconds>` (default: 86400s / 24h).
* The session persists until the expiration timestamp elapses.

---

## 5. Security Flags & Cookie Configuration

Configure cookie security attributes through `app.config`:

```cpp
// Change cookie name (default: "session")
app.config["SESSION_COOKIE_NAME"] = "secure_app_sid";

// Enforce HTTPS-only cookie transmission
app.config["SESSION_COOKIE_SECURE"] = "true";

// Prevent JavaScript access via document.cookie (mitigates XSS)
app.config["SESSION_COOKIE_HTTPONLY"] = "true";

// Restrict cross-site requests ("Strict", "Lax", "None")
app.config["SESSION_COOKIE_SAMESITE"] = "Strict";

// Restrict cookie to specific URL path
app.config["SESSION_COOKIE_PATH"] = "/";

// Enable CHIPS (Cookies Having Independent Partitioned State)
app.config["SESSION_COOKIE_PARTITIONED"] = "true";

// Configure lifetime in seconds (e.g. 7 days)
app.config["PERMANENT_SESSION_LIFETIME"] = "604800";
```

---

## 6. Complete Code Examples

### Authentication Controller with Stateless Sessions

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    // Set production signing key
    app.secret_key = "k9#dF!29xLmP@71zWq0$vBtR";
    app.config["SESSION_COOKIE_HTTPONLY"] = "true";
    app.config["SESSION_COOKIE_SAMESITE"] = "Lax";

    // Public Home Route
    app.get("/", [](peregrine::Request& req) {
        if (req.session.has("user")) {
            return peregrine::Response::text("Welcome back, " + req.session["user"] + "! <a href='/logout'>Logout</a>");
        }
        return peregrine::Response::text("Welcome Guest. <a href='/login'>Login</a>");
    });

    // Login Handler
    app.get("/login", [](peregrine::Request& req) {
        // Authenticate user credentials...
        req.session["user"] = "Alice";
        req.session["role"] = "Administrator";
        req.session.permanent = true; // Retain cookie across browser restarts

        return peregrine::Response::text("Logged in as Alice! Return <a href='/'>Home</a>");
    });

    // Logout Handler
    app.get("/logout", [](peregrine::Request& req) {
        // Clears all keys and flags session for cookie deletion
        req.session.clear();
        return peregrine::Response::redirect("/");
    });

    // Protected Route
    app.get("/admin", [](peregrine::Request& req) {
        if (!req.session.has("user") || req.session["role"] != "Administrator") {
            peregrine::abort(403, "Forbidden: Administrative session required.");
        }
        return peregrine::Response::text("Welcome to the Admin Control Panel, " + req.session["user"]);
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```
