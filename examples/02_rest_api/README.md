# Example 02: RESTful JSON CRUD API

A production-style, thread-safe JSON API service demonstrating modular routing with `Blueprint`, JSON serialization, request validation, and HTTP status codes.

---

## Features Demonstrated

- **Modular Blueprints**: Scoping endpoints under `/api/v1` using `Blueprint`.
- **JSON Engine**: Reading request JSON with `req.get_json()` and emitting JSON responses via `Response::json()`.
- **Typed Path Parameters**: Capturing integer IDs with `/tasks/<int:id>`.
- **HTTP Status Codes**: Using proper REST semantics (`200 OK`, `201 Created`, `400 Bad Request`, `404 Not Found`).
- **Blueprint Middleware**: Using `api.after_request` to inject standard headers (`X-API-Version`, CORS `Access-Control-Allow-Origin`).
- **Thread Safety**: Protecting shared in-memory data structures with `std::mutex`.

---

## API Endpoints Reference

| Method | URL | Description | Response Status |
| :--- | :--- | :--- | :---: |
| `GET` | `/api/v1/tasks` | List all tasks | `200 OK` |
| `GET` | `/api/v1/tasks/<id>` | Retrieve a single task by ID | `200 OK` or `404 Not Found` |
| `POST` | `/api/v1/tasks` | Create a new task (JSON body) | `201 Created` or `400 Bad Request` |
| `PUT` | `/api/v1/tasks/<id>` | Update an existing task (JSON body) | `200 OK` or `404 Not Found` |
| `DELETE` | `/api/v1/tasks/<id>` | Delete a task by ID | `200 OK` or `404 Not Found` |

---

## Build & Run

### Linux & macOS (GCC / Clang)

From the project root:

```bash
g++ -std=c++11 -I include examples/02_rest_api/main.cpp -lssl -lcrypto -lpthread -o rest_api
./rest_api
```

### Windows (MinGW / GCC)

```powershell
g++ -std=c++11 -I include examples/02_rest_api/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o rest_api.exe
.\rest_api.exe
```

---

## Testing with cURL

Once the server is running on `http://127.0.0.1:8080`, test the endpoints using `curl`:

### 1. List All Tasks
```bash
curl -i http://127.0.0.1:8080/api/v1/tasks
```

### 2. Get Task by ID
```bash
curl -i http://127.0.0.1:8080/api/v1/tasks/1
```

### 3. Create a New Task (POST)
```bash
curl -i -X POST http://127.0.0.1:8080/api/v1/tasks \
  -H "Content-Type: application/json" \
  -d '{"title": "Deploy to Production", "description": "Release Peregrine app to cloud", "completed": false}'
```

### 4. Update an Existing Task (PUT)
```bash
curl -i -X PUT http://127.0.0.1:8080/api/v1/tasks/1 \
  -H "Content-Type: application/json" \
  -d '{"title": "Learn Peregrine", "description": "Finished reading all documentation", "completed": true}'
```

### 5. Delete a Task (DELETE)
```bash
curl -i -X DELETE http://127.0.0.1:8080/api/v1/tasks/1
```

### 6. Verify 404 Handling
```bash
curl -i http://127.0.0.1:8080/api/v1/tasks/999
```
