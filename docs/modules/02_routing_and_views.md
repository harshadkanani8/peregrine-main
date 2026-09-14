# 📦 Module 02: URL Routing & Class-Based Views

Peregrine features an expressive, dynamic URL routing engine supporting parameter extraction, pattern compilation, HTTP method matching, and class-based RESTful views.

---

## 📋 Table of Contents

1. [Routing Architecture](#1-routing-architecture)
2. [Registering Route Handlers](#2-registering-route-handlers)
   - [Convenience Verb Methods](#convenience-verb-methods)
   - [Multi-Method Routes with `route()` & `add_url_rule()`](#multi-method-routes-with-route--add_url_rule)
3. [Dynamic Route Parameters](#3-dynamic-route-parameters)
   - [Supported Converter Types](#supported-converter-types)
   - [Extracting Parameters in Handlers](#extracting-parameters-in-handlers)
4. [HTTP 405 Method Not Allowed & `Allow:` Header](#4-http-405-method-not-allowed--allow-header)
5. [Class-Based Views (`MethodView`)](#5-class-based-views-methodview)
   - [The `MethodView` Lifecycle](#the-methodview-lifecycle)
   - [Registering Class-Based Views](#registering-class-based-views)
6. [Complete Code Examples](#6-complete-code-examples)

---

## 1. Routing Architecture

Routing is managed by `peregrine::Router` in `include/peregrine/routing.hpp`.

When you register a route, Peregrine parses the route pattern segment-by-segment into a standard C++11 regular expression (`std::regex`), identifying captured parameters and saving their names in order:

```text
 Pattern: "/api/users/<int:id>/files/<path:filepath>"
                    │                      │
                    ▼                      ▼
 Regex  : "^/api/users/([0-9]+)/files/(.+)$"
 Param 1: "id"
 Param 2: "filepath"
```

During request dispatch:
1. Peregrine attempts to match `req.path` against registered route patterns.
2. If the path matches and the HTTP method matches, captured regex groups are populated into `req.path_params`.
3. If the path matches but the method does not match, Peregrine notes the mismatch and records the method in an `allowed` list. If no other route matches, an HTTP 405 response is returned with the valid `Allow` header.

---

## 2. Registering Route Handlers

A route handler in Peregrine is any callable conforming to the signature:

```cpp
std::function<peregrine::Response(peregrine::Request&)>
```

### Convenience Verb Methods

`peregrine::App` provides direct methods for common HTTP verbs:

```cpp
// GET request
app.get("/items", [](peregrine::Request& req) {
    return peregrine::Response::text("List of items");
});

// POST request
app.post("/items", [](peregrine::Request& req) {
    return peregrine::Response::text("Created item", 201);
});

// PUT request
app.put("/items/<int:id>", [](peregrine::Request& req) {
    return peregrine::Response::text("Updated item");
});

// PATCH request
app.patch("/items/<int:id>", [](peregrine::Request& req) {
    return peregrine::Response::text("Patched item");
});

// DELETE request (app.del and app.delete_ are identical)
app.del("/items/<int:id>", [](peregrine::Request& req) {
    return peregrine::Response::text("Deleted item");
});

// OPTIONS request
app.options("/items", [](peregrine::Request& req) {
    peregrine::Response r;
    r.headers["Allow"] = "GET, POST, OPTIONS";
    return r;
});
```

---

### Multi-Method Routes with `route()` & `add_url_rule()`

When a single handler handles multiple HTTP verbs, use `app.route()` or `app.add_url_rule()`:

```cpp
app.route("/api/resource", {"GET", "POST"}, [](peregrine::Request& req) {
    if (req.method == "GET") {
        return peregrine::Response::text("Retrieving resource");
    } else {
        return peregrine::Response::text("Modifying resource");
    }
});

// Flask-compatible syntax alias
app.add_url_rule("/legacy/endpoint", {"GET", "POST"}, legacy_handler);
```

---

## 3. Dynamic Route Parameters

Route segments enclosed in angle brackets `<...>` denote dynamic parameters.

### Supported Converter Types

| Converter Syntax | Matched Regex | Example Match | Description |
| :--- | :--- | :--- | :--- |
| `<int:name>` | `([0-9]+)` | `/users/42` | Matches one or more ASCII digits. |
| `<string:name>` | `([^/]+)` | `/users/alice` | Matches any characters except `/`. (Default if type omitted) |
| `<name>` | `([^/]+)` | `/items/laptop` | Short form equivalent to `<string:name>`. |
| `<path:name>` | `(.+)` | `/static/img/logo.png` | Greedy match including slashes (`/`). Ideal for file systems. |

---

### Extracting Parameters in Handlers

Extracted path variables are stored in `req.path_params` as string pairs:

```cpp
app.get("/users/<int:user_id>/posts/<string:slug>", [](peregrine::Request& req) {
    std::string id_str = req.path_params["user_id"];
    std::string slug = req.path_params["slug"];

    int user_id = std::stoi(id_str);

    peregrine::Json res = peregrine::Json::object();
    res["user_id"] = user_id;
    res["slug"] = slug;
    return peregrine::Response::json(res);
});
```

---

## 4. HTTP 405 Method Not Allowed & `Allow:` Header

Peregrine automatically enforces HTTP/1.1 RFC compliance for disallowed methods.

Suppose your application registers only `GET` and `POST` for `/api/books`:

```cpp
app.get("/api/books", get_books);
app.post("/api/books", create_book);
```

If a client sends `DELETE /api/books`:
1. Peregrine matches the URL pattern `/api/books`.
2. Detects that `DELETE` is not registered.
3. Automatically synthesizes an HTTP 405 response:
   ```text
   HTTP/1.1 405 Method Not Allowed
   Allow: GET, POST
   Content-Type: text/html; charset=utf-8
   ```

You can customize the body of 405 errors globally:

```cpp
app.error_handler(405, [](peregrine::Request& req) {
    peregrine::Json err = peregrine::Json::object();
    err["error"] = "Method Not Allowed";
    err["path"] = req.path;
    return peregrine::Response::json(err, 405);
});
```

---

## 5. Class-Based Views (`MethodView`)

Defined in `include/peregrine/view.hpp`.

Class-based views provide clean object-oriented encapsulation for RESTful resources instead of sprawling `if-else` blocks inside a single lambda.

### The `MethodView` Lifecycle

Inherit from `peregrine::MethodView` and override the HTTP methods you wish to support. Any method you do not override automatically returns `405 Method Not Allowed`.

```cpp
#include <peregrine/peregrine.hpp>

class UserAPI : public peregrine::MethodView {
public:
    peregrine::Response get(peregrine::Request& req) override {
        std::string user_id = req.path_params["id"];
        peregrine::Json j = peregrine::Json::object();
        j["id"] = user_id;
        j["name"] = "Alice";
        return peregrine::Response::json(j);
    }

    peregrine::Response put(peregrine::Request& req) override {
        std::string user_id = req.path_params["id"];
        peregrine::Json body = req.get_json();
        return peregrine::Response::text("User " + user_id + " updated.");
    }

    peregrine::Response delete_(peregrine::Request& req) override {
        std::string user_id = req.path_params["id"];
        return peregrine::Response::text("User " + user_id + " deleted.", 204);
    }
    
    // post(), patch(), options() remain un-overridden and return 405
};
```

*Note: In C++, `delete` is a reserved keyword, so the method override is named `delete_`.*

---

### Registering Class-Based Views

Register the class using the templated `add_url_rule` helper:

```cpp
app.add_url_rule<UserAPI>("/api/users/<int:id>");
```

Peregrine automatically instantiates and invokes `UserAPI` for all standard HTTP methods (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`, `OPTIONS`, `HEAD`).

---

## 6. Complete Code Examples

### Full CRUD API Using `MethodView`

```cpp
#include <map>
#include <peregrine/peregrine.hpp>

struct Article {
    std::string title;
    std::string content;
};

static std::map<int, Article> g_articles = {
    {1, {"First Post", "Welcome to Peregrine C++!"}},
    {2, {"Architecture", "Deep dive into POSIX threading."}}
};

class ArticleItemView : public peregrine::MethodView {
public:
    peregrine::Response get(peregrine::Request& req) override {
        int id = std::stoi(req.path_params["id"]);
        auto it = g_articles.find(id);
        if (it == g_articles.end()) {
            peregrine::abort(404, "Article not found");
        }

        peregrine::Json j = peregrine::Json::object();
        j["id"] = id;
        j["title"] = it->second.title;
        j["content"] = it->second.content;
        return peregrine::Response::json(j);
    }

    peregrine::Response put(peregrine::Request& req) override {
        int id = std::stoi(req.path_params["id"]);
        peregrine::Json payload = req.get_json();

        if (!payload.has("title") || !payload.has("content")) {
            peregrine::abort(400, "Missing required fields 'title' or 'content'");
        }

        g_articles[id] = {payload["title"].as_string(), payload["content"].as_string()};

        peregrine::Json j = peregrine::Json::object();
        j["status"] = "updated";
        j["id"] = id;
        return peregrine::Response::json(j);
    }

    peregrine::Response delete_(peregrine::Request& req) override {
        int id = std::stoi(req.path_params["id"]);
        g_articles.erase(id);
        return peregrine::Response::text("", 204);
    }
};

int main() {
    peregrine::App app;

    // Register REST endpoint
    app.add_url_rule<ArticleItemView>("/api/articles/<int:id>");

    app.run("127.0.0.1", 8080);
    return 0;
}
```
