# Example 01: Hello World

A minimal, single-file web server demonstrating the fundamentals of the Peregrine C++ Web Framework.

---

## Features Demonstrated

- Creating and naming an `App` instance.
- Registering a plain text `GET` route (`/`).
- Registering an HTML `GET` route (`/html`).
- Handling dynamic URL parameters with `<string:name>` (`/greet/<string:name>`).
- Starting the multi-threaded HTTP server on a specified host and port.

---

## Source Code Overview

The entire application is self-contained in `main.cpp`:

```cpp
#include "peregrine/peregrine.hpp"

using namespace peregrine;

int main() {
    App app("hello_world_app");

    app.get("/", [](Request&) {
        return Response::text("Hello, World!");
    });

    app.get("/greet/<string:name>", [](Request& req) {
        return Response::text("Hello, " + req.path_params["name"] + "!");
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```

---

## Build & Run

### Using GCC / Clang (Linux & macOS)

From the project root directory:

```bash
g++ -std=c++11 -I include examples/01_hello_world/main.cpp -lssl -lcrypto -lpthread -o hello_world
./hello_world
```

### Using MinGW / GCC (Windows)

```powershell
g++ -std=c++11 -I include examples/01_hello_world/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o hello_world.exe
.\hello_world.exe
```

---

## Testing the Endpoints

Once the server is running, open a terminal or browser and try the following:

### 1. Plain Text Route
```bash
curl http://127.0.0.1:8080/
```
**Expected Output:**
```text
Hello, World! Welcome to Peregrine C++ Web Framework.
```

### 2. Dynamic Route with Path Parameter
```bash
curl http://127.0.0.1:8080/greet/Alice
```
**Expected Output:**
```text
Hello, Alice! Welcome to your Peregrine application.
```

### 3. HTML Route
Open `http://127.0.0.1:8080/html` in your web browser. You should see a formatted HTML page.
