# 🛠️ Feature Guide: Building Production RESTful APIs

This guide walks through designing, structuring, and implementing a high-performance, production-ready **JSON RESTful API** using the Peregrine C++ Web Framework.

---

## 📋 Table of Contents

1. [API Design Principles](#1-api-design-principles)
2. [Uniform JSON Error Format](#2-uniform-json-error-format)
3. [Structuring Resources with Blueprints](#3-structuring-resources-with-blueprints)
4. [CRUD Endpoint Implementation](#4-crud-endpoint-implementation)
   - [List & Pagination (`GET /api/v1/products`)](#list--pagination-get-apiv1products)
   - [Get Single Item (`GET /api/v1/products/<int:id>`)](#get-single-item-get-apiv1productsintid)
   - [Create Resource (`POST /api/v1/products`)](#create-resource-post-apiv1products)
   - [Update Resource (`PUT /api/v1/products/<int:id>`)](#update-resource-put-apiv1productsintid)
   - [Delete Resource (`DELETE /api/v1/products/<int:id>`)](#delete-resource-delete-apiv1productsintid)
5. [Thread-Safe In-Memory Repository](#5-thread-safe-in-memory-repository)
6. [Complete Runnable Example](#6-complete-runnable-example)

---

## 1. API Design Principles

When designing REST APIs in Peregrine:
* **Resource-Oriented URLs**: Use plural nouns (`/api/v1/products`, `/api/v1/users`).
* **Appropriate HTTP Verbs**:
  - `GET`: Retrieve a representation of a resource without side-effects.
  - `POST`: Create a new resource; returns `201 Created` with the created entity.
  - `PUT`: Replace an existing resource in full; returns `200 OK`.
  - `PATCH`: Partially update specific fields of a resource.
  - `DELETE`: Remove a resource; returns `204 No Content` or `200 OK`.
* **Predictable HTTP Status Codes**:
  - `200 OK`: Successful standard request.
  - `201 Created`: Resource successfully created.
  - `204 No Content`: Resource successfully deleted.
  - `400 Bad Request`: Malformed or unparseable input.
  - `404 Not Found`: Resource ID does not exist.
  - `415 Unsupported Media Type`: Non-JSON payload submitted.
  - `422 Unprocessable Entity`: Validation failure on required fields.

---

## 2. Uniform JSON Error Format

A consistent error schema helps frontend clients and SDKs parse errors reliably.

Create a helper function returning uniform JSON responses:

```cpp
peregrine::Response api_error(int status_code, const std::string& message) {
    peregrine::Json err = peregrine::Json::object();
    err["status"] = "error";
    err["code"] = status_code;
    err["message"] = message;
    return peregrine::Response::json(err, status_code);
}
```

---

## 3. Structuring Resources with Blueprints

Organize each major entity domain into its own `peregrine::Blueprint`:

```cpp
peregrine::Blueprint products_bp("products", "/api/v1/products");

// Global API header injection
products_bp.after_request([](peregrine::Request& req, peregrine::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*")
       .set_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
       .set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
});
```

---

## 4. CRUD Endpoint Implementation

### List & Pagination (`GET /api/v1/products`)

Support `?page=...` and `?limit=...` query parameters:

```cpp
products_bp.get("", [](peregrine::Request& req) {
    int page = std::max(1, std::stoi(req.arg("page", "1")));
    int limit = std::min(100, std::max(1, std::stoi(req.arg("limit", "10"))));

    auto all_items = repo.list();
    size_t total = all_items.size();
    size_t start_idx = static_cast<size_t>((page - 1) * limit);

    peregrine::Json items_json = peregrine::Json::array();
    if (start_idx < total) {
        size_t end_idx = std::min(start_idx + limit, total);
        for (size_t i = start_idx; i < end_idx; ++i) {
            items_json.push_back(all_items[i].to_json());
        }
    }

    peregrine::Json response = peregrine::Json::object();
    response["page"] = page;
    response["limit"] = limit;
    response["total"] = static_cast<int>(total);
    response["data"] = items_json;

    return peregrine::Response::json(response);
});
```

---

### Get Single Item (`GET /api/v1/products/<int:id>`)

```cpp
products_bp.get("/<int:id>", [](peregrine::Request& req) {
    int id = std::stoi(req.path_params["id"]);
    
    Product p;
    if (!repo.find(id, p)) {
        return api_error(404, "Product with ID " + std::to_string(id) + " not found.");
    }

    return peregrine::Response::json(p.to_json());
});
```

---

### Create Resource (`POST /api/v1/products`)

```cpp
products_bp.post("", [](peregrine::Request& req) {
    if (!req.is_json()) {
        return api_error(415, "Payload must be application/json.");
    }

    peregrine::Json body = req.get_json();
    if (body.is_null() || !body.is_object()) {
        return api_error(400, "Malformed JSON request body.");
    }

    if (!body.has("name") || !body.has("price")) {
        return api_error(422, "Missing required fields: 'name' and 'price'.");
    }

    Product p;
    p.name = body["name"].as_string();
    p.price = body["price"].as_number();

    Product created = repo.create(p);
    return peregrine::Response::json(created.to_json(), 201);
});
```

---

### Update Resource (`PUT /api/v1/products/<int:id>`)

```cpp
products_bp.put("/<int:id>", [](peregrine::Request& req) {
    int id = std::stoi(req.path_params["id"]);
    peregrine::Json body = req.get_json();

    if (!body.has("name") || !body.has("price")) {
        return api_error(422, "Missing required fields: 'name' and 'price'.");
    }

    Product p;
    p.id = id;
    p.name = body["name"].as_string();
    p.price = body["price"].as_number();

    if (!repo.update(id, p)) {
        return api_error(404, "Product not found.");
    }

    return peregrine::Response::json(p.to_json(), 200);
});
```

---

### Delete Resource (`DELETE /api/v1/products/<int:id>`)

```cpp
products_bp.del("/<int:id>", [](peregrine::Request& req) {
    int id = std::stoi(req.path_params["id"]);
    
    if (!repo.remove(id)) {
        return api_error(404, "Product not found.");
    }

    return peregrine::Response::text("", 204);
});
```

---

## 5. Thread-Safe In-Memory Repository

Because Peregrine's worker threads execute route handlers concurrently, any shared data store must be protected by synchronization primitives:

```cpp
#include <mutex>
#include <map>
#include <vector>

struct Product {
    int id = 0;
    std::string name;
    double price = 0.0;

    peregrine::Json to_json() const {
        peregrine::Json j = peregrine::Json::object();
        j["id"] = id;
        j["name"] = name;
        j["price"] = price;
        return j;
    }
};

class ProductRepository {
public:
    std::vector<Product> list() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Product> results;
        for (const auto& kv : storage_) {
            results.push_back(kv.second);
        }
        return results;
    }

    bool find(int id, Product& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = storage_.find(id);
        if (it == storage_.end()) return false;
        out = it->second;
        return true;
    }

    Product create(Product p) {
        std::lock_guard<std::mutex> lock(mutex_);
        p.id = next_id_++;
        storage_[p.id] = p;
        return p;
    }

    bool update(int id, const Product& p) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (storage_.count(id) == 0) return false;
        storage_[id] = p;
        return true;
    }

    bool remove(int id) {
        std::lock_guard<std::mutex> lock(mutex_);
        return storage_.erase(id) > 0;
    }

private:
    std::mutex mutex_;
    int next_id_ = 1;
    std::map<int, Product> storage_;
};
```

---

## 6. Complete Runnable Example

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

// Include repository implementation from Section 5
static ProductRepository g_repo;

peregrine::Response api_error(int status_code, const std::string& message) {
    peregrine::Json err = peregrine::Json::object();
    err["status"] = "error";
    err["code"] = status_code;
    err["message"] = message;
    return peregrine::Response::json(err, status_code);
}

peregrine::Blueprint make_products_blueprint() {
    peregrine::Blueprint bp("products", "/api/v1/products");

    // GET /api/v1/products
    bp.get("", [](peregrine::Request& req) {
        auto products = g_repo.list();
        peregrine::Json list = peregrine::Json::array();
        for (const auto& p : products) {
            list.push_back(p.to_json());
        }
        peregrine::Json res = peregrine::Json::object();
        res["total"] = static_cast<int>(products.size());
        res["items"] = list;
        return peregrine::Response::json(res);
    });

    // POST /api/v1/products
    bp.post("", [](peregrine::Request& req) {
        peregrine::Json payload = req.get_json();
        if (!payload.has("name") || !payload.has("price")) {
            return api_error(422, "Missing 'name' or 'price'.");
        }
        Product p;
        p.name = payload["name"].as_string();
        p.price = payload["price"].as_number();
        Product saved = g_repo.create(p);
        return peregrine::Response::json(saved.to_json(), 201);
    });

    // GET /api/v1/products/<int:id>
    bp.get("/<int:id>", [](peregrine::Request& req) {
        int id = std::stoi(req.path_params["id"]);
        Product p;
        if (!g_repo.find(id, p)) {
            return api_error(404, "Product not found");
        }
        return peregrine::Response::json(p.to_json());
    });

    // DELETE /api/v1/products/<int:id>
    bp.del("/<int:id>", [](peregrine::Request& req) {
        int id = std::stoi(req.path_params["id"]);
        if (!g_repo.remove(id)) {
            return api_error(404, "Product not found");
        }
        return peregrine::Response::text("", 204);
    });

    return bp;
}

int main() {
    peregrine::App app;

    // Seed test data
    Product p1; p1.name = "USB-C Cable"; p1.price = 9.99;
    g_repo.create(p1);

    app.register_blueprint(make_products_blueprint());

    std::cout << "REST API listening at http://127.0.0.1:8080/api/v1/products\n";
    app.run("127.0.0.1", 8080);
    return 0;
}
```
