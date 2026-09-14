# Example 03: Authentication & Secure Sessions

A complete web authentication example demonstrating Peregrine's **stateless, cryptographically signed cookie sessions** using HMAC-SHA256.

---

## How Peregrine Sessions Work

Unlike traditional web servers that store session state in a server-side database (Redis, SQL, or in-memory hash tables), Peregrine uses **stateless client-side signed cookies** (similar to Flask / itsdangerous):

1. When you store data in `req.session["user"] = username`, Peregrine serializes the data into a Base64URL-encoded payload.
2. It generates an HMAC-SHA256 signature using `app.secret_key`.
3. The cookie format is `payload.timestamp.signature`.
4. When the browser sends this cookie on subsequent requests:
   - If a malicious client tampers with even a single byte of the payload, the HMAC signature verification fails immediately.
   - If the cookie timestamp exceeds `session_lifetime_seconds`, it is rejected as expired.
   - The server maintains **zero memory state** across requests, making the architecture inherently scalable across multiple server instances.

---

## Features Demonstrated

- Setting `app.secret_key` and configuring session lifetime.
- Handling HTML login forms with `req.form_get()`.
- Mutating session state (`req.session["key"] = value`).
- Protecting routes (`/dashboard`) by asserting `req.session.has("user")`.
- Terminating sessions (`req.session.clear()`).
- Inspecting session state via JSON (`/session-debug`).

---

## Build & Run

### Linux & macOS (GCC / Clang)

From the project root:

```bash
g++ -std=c++11 -I include examples/03_auth_sessions/main.cpp -lssl -lcrypto -lpthread -o auth_sessions
./auth_sessions
```

### Windows (MinGW / GCC)

```powershell
g++ -std=c++11 -I include examples/03_auth_sessions/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o auth_sessions.exe
.\auth_sessions.exe
```

---

## Testing the Application

### Option A: Web Browser
1. Open `http://127.0.0.1:8080/` in your browser.
2. Sign in using:
   - Username: `admin`
   - Password: `secret123`
3. Click **Go to Protected Dashboard** (`/dashboard`).
4. Click **Log Out** and verify you are redirected back to the guest home page.

### Option B: Testing with cURL & Cookie Jar

```bash
# 1. Attempt to access protected dashboard without login (Expect 401 Unauthorized)
curl -i http://127.0.0.1:8080/dashboard

# 2. Perform login and save the signed cookie to cookies.txt
curl -i -c cookies.txt -X POST http://127.0.0.1:8080/login \
  -d "username=admin&password=secret123"

# 3. Access protected dashboard sending the stored cookie (Expect 200 OK)
curl -i -b cookies.txt http://127.0.0.1:8080/dashboard

# 4. View raw session JSON
curl -b cookies.txt http://127.0.0.1:8080/session-debug

# 5. Clean up cookie jar
rm cookies.txt
```
