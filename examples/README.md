# Peregrine Application Examples & Tutorials

Welcome to the **Peregrine Examples Directory**! 

This repository of practical, ready-to-run example projects illustrates how to build modern web services, RESTful APIs, and secure web applications using the **Peregrine C++ Web Framework**.

---

## 📋 Examples Directory Index

| Directory | Name | Key Concepts Demonstrated | Difficulty |
| :--- | :--- | :--- | :---: |
| [`01_hello_world/`](01_hello_world/) | **Hello World** | Minimal server, text/HTML routes, path parameters (`<string:name>`). | Beginner |
| [`02_rest_api/`](02_rest_api/) | **RESTful Task API** | Modular `Blueprint` routing, JSON parsing/dumping, full CRUD, status codes. | Intermediate |
| [`03_auth_sessions/`](03_auth_sessions/) | **Auth & Sessions** | HMAC-SHA256 signed stateless cookie sessions, login/logout, protected pages. | Intermediate |
| [`04_template_rendering/`](04_template_rendering/) | **HTML Templates** | Mustache template engine, scalar `{{var}}`, collection loops `{{#each}}`. | Beginner |
| [`05_file_upload/`](05_file_upload/) | **File Uploads** | RFC 7578 `multipart/form-data`, binary disk saving, Path Traversal defense. | Intermediate |

---

## 🛠️ Prerequisites & Building

All examples require a standard **C++11** compiler, **OpenSSL**, and **pthreads** (plus `ws2_32` on Windows).

### Method 1: Building with CMake (Recommended)

From the project root:

```bash
# 1. Configure build directory
cmake -B build -S examples

# 2. Compile all examples
cmake --build build

# 3. Executables are created in build/
```

On Windows with MinGW or Visual Studio:
```powershell
cmake -B build -S examples
cmake --build build --config Release
```

---

### Method 2: Direct Compiler Invocation (No CMake Required)

You can compile any individual example directly with a single command from the project root:

#### Linux & macOS (GCC / Clang)
```bash
# 01_hello_world
g++ -std=c++11 -I include examples/01_hello_world/main.cpp -lssl -lcrypto -lpthread -o hello_world

# 02_rest_api
g++ -std=c++11 -I include examples/02_rest_api/main.cpp -lssl -lcrypto -lpthread -o rest_api

# 03_auth_sessions
g++ -std=c++11 -I include examples/03_auth_sessions/main.cpp -lssl -lcrypto -lpthread -o auth_sessions

# 04_template_rendering
g++ -std=c++11 -I include examples/04_template_rendering/main.cpp -lssl -lcrypto -lpthread -o templates_demo

# 05_file_upload
g++ -std=c++11 -I include examples/05_file_upload/main.cpp -lssl -lcrypto -lpthread -o file_upload
```

#### Windows (MinGW / GCC)
```powershell
# 01_hello_world
g++ -std=c++11 -I include examples/01_hello_world/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o hello_world.exe

# 02_rest_api
g++ -std=c++11 -I include examples/02_rest_api/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o rest_api.exe

# 03_auth_sessions
g++ -std=c++11 -I include examples/03_auth_sessions/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o auth_sessions.exe

# 04_template_rendering
g++ -std=c++11 -I include examples/04_template_rendering/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o templates_demo.exe

# 05_file_upload
g++ -std=c++11 -I include examples/05_file_upload/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o file_upload.exe
```

---

## 🔍 Detailed Overview of Each Example

### 1. [01_hello_world](01_hello_world/)
- **Summary**: The ideal entry point for new users.
- **Key Concepts**:
  - `App app("name")`: Initializes the master framework coordinator.
  - `app.get("/path", handler)`: Binds a lambda handler to an HTTP GET route.
  - `req.path_params["name"]`: Extracts dynamic segments from regex-matched routes.
  - `Response::text()` and `Response::html()`: Formats HTTP wire responses with proper headers.
  - `app.run("127.0.0.1", 8080)`: Starts the internal multi-threaded socket listener.

---

### 2. [02_rest_api](02_rest_api/)
- **Summary**: A production-grade JSON API service managing a task collection.
- **Key Concepts**:
  - `Blueprint api("api", "/api/v1")`: Groups routes under an isolated URL prefix.
  - `api.after_request(...)`: Automatically appends standard API headers (`X-API-Version`, CORS headers).
  - `req.get_json()`: Parses incoming JSON payload directly into a `Json` AST object.
  - `Response::json(obj, status)`: Serializes C++ data into compact JSON with `application/json` Content-Type.
  - CRUD operations mapping cleanly to HTTP verbs: `GET`, `POST`, `PUT`, `DELETE`.

---

### 3. [03_auth_sessions](03_auth_sessions/)
- **Summary**: Demonstrates stateful authentication using stateless, signed client cookies.
- **Key Concepts**:
  - `app.secret_key = "..."`: Enables HMAC-SHA256 cryptographic signing for session cookies.
  - `req.session["user"] = username`: Stores session data securely on the client.
  - `req.session.permanent = true`: Enforces cookie expiration and `Max-Age` persistence.
  - `req.session.clear()`: Destroys user session upon logout.
  - Tamper protection: If a client alters session cookie values, signature verification fails and discards the cookie automatically.

---

### 4. [04_template_rendering](04_template_rendering/)
- **Summary**: Renders dynamic HTML using Peregrine's single-pass mustache template engine.
- **Key Concepts**:
  - `TemplateContext ctx`: Binds scalar values and list collections for substitution.
  - `ctx.set("key", "value")`: Substitutes `{{key}}` in the HTML document.
  - `ctx.append("list_name", item_map)`: Iterates over lists using `{{#each list_name}} ... {{this.field}} ... {{/each}}`.
  - `render_template(filepath, ctx)`: Reads and renders external template files.
  - `render_string(tmpl, ctx)`: Renders on-the-fly inline string templates.

---

### 5. [05_file_upload](05_file_upload/)
- **Summary**: Handles binary file uploads via `multipart/form-data`.
- **Key Concepts**:
  - `req.files`: Iterates over uploaded files received from web forms.
  - `file.filename`, `file.content_type`, `file.size()`: Inspects upload properties.
  - `file.save(dest)`: Streams binary payload directly to disk.
  - **Path Traversal Defense (CWE-22)**: Sanitizes filenames to prevent directory traversal exploits (`../../`).

---

## 🧪 Testing Your Own Code

To test your applications in memory without network port overhead, see the **[Peregrine Testing Guide](../tests/README.md)**.
Peregrine includes `TestClient`, which allows you to simulate requests against any `App` instance entirely in memory in sub-millisecond execution times.
