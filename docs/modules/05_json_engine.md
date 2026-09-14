# 📦 Module 05: Lightweight PIMPL JSON Engine

Peregrine includes a native, header-only JSON engine specifically engineered for embedded microservices and cloud APIs. It features recursive-descent parsing, AST manipulation, serialization, and a **PIMPL (Pointer to Implementation)** architecture that guarantees zero ABI diagnostics across 32-bit and 64-bit architectures.

---

## 📋 Table of Contents

1. [Architectural Motivation & 32-Bit ARM ABI Cleanliness](#1-architectural-motivation--32-bit-arm-abi-cleanliness)
2. [Data Types & Constructors](#2-data-types--constructors)
3. [Object Operations](#3-object-operations)
4. [Array Operations](#4-array-operations)
5. [Value Extraction with Default Fallbacks](#5-value-extraction-with-default-fallbacks)
6. [Parsing JSON Payloads](#6-parsing-json-payloads)
7. [Serializing to JSON Strings](#7-serializing-to-json-strings)
8. [Complete Code Examples](#8-complete-code-examples)

---

## 1. Architectural Motivation & 32-Bit ARM ABI Cleanliness

Defined in `include/peregrine/json.hpp`.

### The Embedded ABI Challenge
In typical C++ JSON engines, the `Json` class stores recursive containers directly by value:
```cpp
// Naive implementation:
class NaiveJson {
    Type type;
    std::string str;
    std::vector<NaiveJson> arr;      // Causes sizeof(NaiveJson) to balloon
    std::map<std::string, NaiveJson> obj;
};
```
This naive representation results in `sizeof(Json) ~ 72 bytes`. On **32-bit ARM architectures** (`arm-linux-gnueabihf`), passing objects larger than 16 bytes by value triggers GCC's **AAPCS ABI parameter-passing diagnostic (`-Wpsabi`)** whenever a container reallocates.

### Peregrine's Solution: PIMPL Encapsulation
Peregrine encapsulates internal storage behind a `std::shared_ptr<Value>`:
```cpp
class Json {
    struct Value {
        Type type = Type::Null;
        bool bool_ = false;
        double num_ = 0.0;
        std::string str_;
        std::vector<Json> arr_;
        std::map<std::string, Json> obj_;
    };
    std::shared_ptr<Value> val_;
};
```

#### Key Benefits:
* **Strict 4-Byte / 8-Byte Footprint**: `sizeof(peregrine::Json)` is exactly the size of a pointer (`4 bytes` on 32-bit ARM, `8 bytes` on x86_64).
* **Zero Compiler Warnings**: Eliminates `-Wpsabi` warnings across GCC and Clang without requiring compiler pragmas or build flags.
* **O(1) Copying**: Passing or returning `peregrine::Json` instances is an O(1) shared pointer copy, avoiding recursive AST deep-copying overhead.

---

## 2. Data Types & Constructors

The `peregrine::Json::Type` enum defines 6 distinct JSON data types:
* `Type::Null`
* `Type::Bool`
* `Type::Number`
* `Type::String`
* `Type::Array`
* `Type::Object`

### Constructing Scalars:

```cpp
#include <peregrine/peregrine.hpp>

peregrine::Json j_null;                       // Null
peregrine::Json j_bool(true);                 // Bool
peregrine::Json j_int(42);                    // Number (int)
peregrine::Json j_double(3.14159);            // Number (double)
peregrine::Json j_str("Hello World");         // String
```

### Static Initializers for Compounds:

```cpp
peregrine::Json j_obj = peregrine::Json::object(); // Empty {}
peregrine::Json j_arr = peregrine::Json::array();  // Empty []
```

---

## 3. Object Operations

Use bracket notation `operator[]` to read or insert key-value pairs:

```cpp
peregrine::Json user = peregrine::Json::object();

// Insert / Mutate fields
user["id"] = 101;
user["username"] = "alice";
user["is_active"] = true;
user["score"] = 98.5;

// Check field existence
if (user.has("username")) {
    std::string name = user["username"].as_string();
}

// Iterate over object entries
for (const auto& kv : user.object_items()) {
    std::cout << kv.first << ": " << kv.second.dump() << "\n";
}
```

*Note: Accessing a non-existent key on a `const Json&` returns a static Null `Json` instance without throwing exceptions.*

---

## 4. Array Operations

Use `push_back()` to append items, and bracket indexing to access elements:

```cpp
peregrine::Json tags = peregrine::Json::array();

tags.push_back("cplusplus");
tags.push_back("networking");
tags.push_back("embedded");

// Array size
size_t count = tags.size(); // 3

// Random access
std::string first_tag = tags[0].as_string();

// Vector iteration
for (const auto& item : tags.items()) {
    std::cout << "- " << item.as_string() << "\n";
}
```

---

## 5. Value Extraction with Default Fallbacks

Peregrine provides defensive, type-safe accessor methods that accept an optional fallback default if the key is missing or is the wrong type:

| Method | Fallback Signature | Description |
| :--- | :--- | :--- |
| `as_string()` | `as_string(const std::string& def = "")` | Returns string value or fallback default. |
| `as_int()` | `as_int(int def = 0)` | Truncates number to `int` or returns fallback. |
| `as_number()`| `as_number(double def = 0.0)` | Returns `double` representation or fallback. |
| `as_bool()` | `as_bool(bool def = false)` | Returns boolean flag or fallback. |

```cpp
peregrine::Json config = peregrine::Json::parse(R"({"port": 8080})");

// Type-safe extraction with sensible defaults
int port = config["port"].as_int(5000);             // Returns 8080
int timeout = config["timeout"].as_int(30);         // Key missing -> returns 30
std::string host = config["host"].as_string("0.0.0.0"); // Key missing -> returns "0.0.0.0"
bool ssl = config["ssl"].as_bool(false);            // Key missing -> returns false
```

---

## 6. Parsing JSON Payloads

Use `peregrine::Json::parse()` to parse a raw JSON string into an AST:

```cpp
std::string raw_payload = R"({
    "service": "telemetry",
    "nodes": [
        {"id": "node-1", "temp": 24.2},
        {"id": "node-2", "temp": 28.9}
    ]
})";

peregrine::Json doc = peregrine::Json::parse(raw_payload);

if (doc.is_null()) {
    std::cerr << "Invalid or malformed JSON payload!\n";
} else {
    std::string service = doc["service"].as_string();
    for (const auto& node : doc["nodes"].items()) {
        std::cout << node["id"].as_string() << ": " 
                  << node["temp"].as_number() << " C\n";
    }
}
```

### String Escape Handling:
The parser automatically decodes standard JSON escape sequences:
* `\"` -> Quotation mark
* `\\` -> Reverse solidus
* `\/` -> Solidus
* `\b` -> Backspace
* `\f` -> Form feed
* `\n` -> Newline
* `\r` -> Carriage return
* `\t` -> Tab

---

## 7. Serializing to JSON Strings

Call `.dump()` on any `peregrine::Json` instance:

```cpp
peregrine::Json root = peregrine::Json::object();
root["title"] = "System Report";
root["count"] = 3;

peregrine::Json items = peregrine::Json::array();
items.push_back("CPU");
items.push_back("Memory");
items.push_back("Disk");
root["metrics"] = items;

std::string json_wire = root.dump();
// Result: {"count":3,"metrics":["CPU","Memory","Disk"],"title":"System Report"}
```

Serialization automatically escapes special characters (`"`, `\`, control characters) to ensure strict compliance with RFC 8259.

---

## 8. Complete Code Examples

### REST Endpoint with Full JSON Input Validation

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    app.post("/api/register", [](peregrine::Request& req) {
        if (!req.is_json()) {
            peregrine::abort(415, "Content-Type must be application/json");
        }

        peregrine::Json payload = req.get_json();
        if (payload.is_null() || !payload.is_object()) {
            peregrine::abort(400, "Invalid JSON body");
        }

        // Validate required fields
        if (!payload.has("email") || !payload.has("password")) {
            peregrine::Json err = peregrine::Json::object();
            err["status"] = "error";
            err["message"] = "Fields 'email' and 'password' are required.";
            return peregrine::Response::json(err, 422);
        }

        std::string email = payload["email"].as_string();
        int age = payload["age"].as_int(18); // Default age 18 if omitted

        // Build response JSON
        peregrine::Json resp = peregrine::Json::object();
        resp["status"] = "success";
        resp["registered_email"] = email;
        resp["assigned_age"] = age;

        return peregrine::Response::json(resp, 201);
    });

    app.run("127.0.0.1", 8080);
    return 0;
}
```
