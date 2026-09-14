# 📦 Module 07: Security Architecture & CSRF Defense

Security in Peregrine is not an afterthought or an optional add-on. The framework includes built-in cryptographic defenses against Cross-Site Request Forgery (CSRF), Path Traversal (CWE-22), Null-Byte Injections, Slowloris resource exhaustion, and timing side-channel attacks.

---

## 📋 Table of Contents

1. [Architectural Overview](#1-architectural-overview)
2. [Cross-Site Request Forgery (CSRF) Defense](#2-cross-site-request-forgery-csrf-defense)
   - [How CSRF Protection Works](#how-csrf-protection-works)
   - [Multi-Vector Token Verification](#multi-vector-token-verification)
   - [Exempting Specific Endpoints](#exempting-specific-endpoints)
3. [Constant-Time Comparison & Side-Channel Mitigation](#3-constant-time-comparison--side-channel-mitigation)
4. [Path Traversal & File Injection Defense (CWE-22)](#4-path-traversal--file-injection-defense-cwe-22)
5. [Slowloris & Socket Resource Exhaustion Mitigation](#5-slowloris--socket-resource-exhaustion-mitigation)
6. [Handler Exception Shielding](#6-handler-exception-shielding)
7. [Complete Code Examples](#7-complete-code-examples)

---

## 1. Architectural Overview

Peregrine operates under a **defense-in-depth model**. Inbound network bytes must clear multiple security validation checkpoints before reaching business logic:

```text
 Incoming Socket Connection
             │
             ├─► 1. Socket Timeout Applied (SO_RCVTIMEO) [Slowloris Shield]
             │
             ├─► 2. Wire Header Parsing (Max Body Size Check)
             │
             ├─► 3. HMAC-SHA256 Signature Verification [Tamper Shield]
             │
             ├─► 4. CSRF Multi-Channel Interception [Forgery Shield]
             │
             ├─► 5. Static Path Sanitization [CWE-22 Traversal Shield]
             │
             └─► 6. Exception Shielding Wrappers [Crash Prevention]
```

---

## 2. Cross-Site Request Forgery (CSRF) Defense

Defined in `include/peregrine/csrf.hpp`.

CSRF attacks trick authenticated users into submitting malicious state-changing requests (e.g. changing passwords or transferring funds) from unauthorized third-party origins.

### How CSRF Protection Works

1. Enable protection on the `App` instance:
   ```cpp
   app.enable_csrf_protection();
   ```
2. When CSRF protection is active, Peregrine automatically validates every mutating HTTP method (`POST`, `PUT`, `PATCH`, `DELETE`).
3. Safe methods (`GET`, `HEAD`, `OPTIONS`) are exempt from CSRF checks.
4. If a mutating request arrives without a valid token matching the signed session token, Peregrine terminates processing immediately and emits an `HTTP 400 Bad Request` error:
   ```text
   CSRF Token verification failed
   ```

---

### Multi-Vector Token Verification

In distributed web applications, tokens may be transmitted via HTML forms, AJAX headers, or query parameters. Peregrine inspects the following channels in order:

1. **Form Body Field**: `csrf_token` (e.g. `<input type="hidden" name="csrf_token" value="...">`)
2. **HTTP Header**: `X-CSRF-Token` (standard frontend framework header)
3. **HTTP Header**: `X-CSRFToken` (standard Django/Flask frontend convention)
4. **Query Parameter**: `?csrf_token=...`

#### Injecting Tokens into Views:
```cpp
app.get("/transfer-form", [](peregrine::Request& req) {
    // Generates or retrieves token stored in req.session["_csrf_token"]
    std::string token = peregrine::csrf::get_token(req);

    std::string html = "<form method='POST' action='/transfer'>"
                       "<input type='hidden' name='csrf_token' value='" + token + "'>"
                       "<input type='text' name='amount' value='100'>"
                       "<button type='submit'>Transfer</button>"
                       "</form>";
    return peregrine::Response::html(html);
});
```

---

### Exempting Specific Endpoints

External webhooks (e.g. Stripe, GitHub) and machine-to-machine APIs cannot supply browser session CSRF tokens. You can exempt them during activation:

```cpp
app.enable_csrf_protection({
    "/api/v1/webhooks/stripe",
    "/api/v1/webhooks/github"
});
```

---

## 3. Constant-Time Comparison & Side-Channel Mitigation

Naive string comparisons (`if (submitted == expected)`) terminate at the first mismatched byte. Attackers can measure response time variations down to nanoseconds to deduce tokens character-by-character.

Peregrine uses `peregrine::constant_time_equal()` in `include/peregrine/common.hpp`:

```cpp
inline bool constant_time_equal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char result = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        result |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return result == 0;
}
```

This XOR-based accumulation ensures that string comparisons always execute in identical time regardless of where or whether characters match.

---

## 4. Path Traversal & File Injection Defense (CWE-22)

Defined in `include/peregrine/helpers.hpp` inside `read_static_file()`.

When serving static files or accepting paths, malicious users may attempt to escape the designated folder using directory traversal sequences:
```text
GET /static/../../../../etc/passwd HTTP/1.1
```

### Peregrine's Multi-Stage Guard:
1. **Directory Traversal Detection**:
   ```cpp
   if (requested_path.find("..") != std::string::npos) return false;
   ```
   Any appearance of `..` anywhere in the path causes an immediate rejection (`404 Not Found`).

2. **Null-Byte Injection Shield**:
   ```cpp
   if (requested_path.find('\0') != std::string::npos) return false;
   ```
   Prevents C-string truncation attacks (e.g. `secret.pdf%00.png`).

3. **Leading Slash Normalization**:
   ```cpp
   size_t pos = requested_path.find_first_not_of('/');
   std::string clean_rel = (pos == std::string::npos) ? "" : requested_path.substr(pos);
   ```
   Ensures that paths can never be resolved against the absolute root directory (`/`).

---

## 5. Slowloris & Socket Resource Exhaustion Mitigation

In a **Slowloris attack**, an adversary opens dozens of TCP connections and transmits data at an extremely slow rate (e.g. 1 byte every 10 seconds), exhausting the server's thread pool and connection table.

Peregrine mitigates this directly at the OS socket layer:

```cpp
// Configured via ServerConfig:
cfg.socket_timeout_seconds = 10;
```

Immediately upon socket acceptance:
```cpp
struct timeval tv;
tv.tv_sec = config.socket_timeout_seconds;
tv.tv_usec = 0;
setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
```

If a client fails to transmit data within the configured timeout window, the read call returns an error, closing the socket and freeing the worker thread for legitimate requests.

---

## 6. Handler Exception Shielding

If a route handler throws an exception, Peregrine guarantees that the worker thread and the server process do not crash:

```text
               [ Handler Execution ]
                         │
        ┌────────────────┼────────────────┐
        ▼                ▼                ▼
 [ HTTPException ]  [ std::exception ] [ Unknown ... ]
        │                │                │
        ▼                ▼                ▼
  run_error(code)   Log error &      Log error &
                    run_error(500)   run_error(500)
```

1. **`peregrine::HTTPException`**: Dispatches the configured error handler for `ex.code` (e.g. `404`, `403`).
2. **`std::exception`**: Intercepts the exception message, logs it to `stderr`, and responds with a safe `500 Internal Server Error` page.
3. **Ellipsis `catch (...)`**: Catches unhandled non-standard throws, guaranteeing 100% process uptime.

---

## 7. Complete Code Examples

### Secured Banking Form with CSRF Protection

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;
    app.secret_key = "production_secure_signing_key";

    // Activate CSRF protection globally
    app.enable_csrf_protection();

    // GET /form: Render form with CSRF token
    app.get("/transfer", [](peregrine::Request& req) {
        std::string token = peregrine::csrf::get_token(req);
        
        std::string html = 
            "<html><body>"
            "<h2>Wire Transfer</h2>"
            "<form method='POST' action='/transfer'>"
            "  <input type='hidden' name='csrf_token' value='" + token + "'/>"
            "  <label>Recipient: <input type='text' name='to'/></label><br/>"
            "  <label>Amount $: <input type='number' name='amount'/></label><br/>"
            "  <button type='submit'>Send Money</button>"
            "</form>"
            "</body></html>";

        return peregrine::Response::html(html);
    });

    // POST /transfer: Protected by CSRF layer automatically
    app.post("/transfer", [](peregrine::Request& req) {
        std::string to = req.form_get("to");
        std::string amount = req.form_get("amount");

        return peregrine::Response::text("Transferred $" + amount + " to " + to + " successfully!");
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```
