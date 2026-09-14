#include "peregrine/peregrine.hpp"
#include <iostream>
#include <map>
#include <mutex>
#include <string>

using namespace peregrine;

// Task data structure
struct Task {
    int id;
    std::string title;
    std::string description;
    bool completed;

    Json to_json() const {
        Json j = Json::object();
        j["id"] = id;
        j["title"] = title;
        j["description"] = description;
        j["completed"] = completed;
        return j;
    }
};

// Thread-safe in-memory task database
class TaskDatabase {
private:
    std::mutex mtx_;
    std::map<int, Task> tasks_;
    int next_id_ = 1;

public:
    TaskDatabase() {
        // Seed with sample tasks
        create_task("Learn Peregrine", "Read the documentation and explore examples", true);
        create_task("Build REST API", "Implement task manager using Blueprints and JSON", false);
    }

    Task create_task(const std::string& title, const std::string& description, bool completed = false) {
        std::lock_guard<std::mutex> lock(mtx_);
        Task t;
        t.id = next_id_++;
        t.title = title;
        t.description = description;
        t.completed = completed;
        tasks_[t.id] = t;
        return t;
    }

    bool get_task(int id, Task& out_task) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = tasks_.find(id);
        if (it == tasks_.end()) return false;
        out_task = it->second;
        return true;
    }

    std::vector<Task> list_tasks() {
        std::lock_guard<std::mutex> lock(mtx_);
        std::vector<Task> list;
        list.reserve(tasks_.size());
        for (const auto& kv : tasks_) {
            list.push_back(kv.second);
        }
        return list;
    }

    bool update_task(int id, const std::string& title, const std::string& description, bool completed) {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = tasks_.find(id);
        if (it == tasks_.end()) return false;
        it->second.title = title;
        it->second.description = description;
        it->second.completed = completed;
        return true;
    }

    bool delete_task(int id) {
        std::lock_guard<std::mutex> lock(mtx_);
        return tasks_.erase(id) > 0;
    }
};

int main() {
    App app("task_api_app");
    TaskDatabase db;

    // 1. Create a modular Blueprint with URL prefix "/api/v1"
    Blueprint api("api", "/api/v1");

    // 2. Blueprint-scoped middleware: set API response headers
    api.after_request([](Request&, Response& res) {
        res.set_header("X-API-Version", "1.0.0");
        res.set_header("Access-Control-Allow-Origin", "*");
    });

    // ------------------------------------------------------------------------
    // GET /api/v1/tasks - List all tasks
    // ------------------------------------------------------------------------
    api.get("/tasks", [&db](Request&) {
        std::vector<Task> tasks = db.list_tasks();
        Json arr = Json::array();
        for (const auto& t : tasks) {
            arr.push_back(t.to_json());
        }
        return Response::json(arr);
    });

    // ------------------------------------------------------------------------
    // GET /api/v1/tasks/<int:id> - Get single task by ID
    // ------------------------------------------------------------------------
    api.get("/tasks/<int:id>", [&db](Request& req) {
        int id = std::stoi(req.path_params["id"]);
        Task t;
        if (!db.get_task(id, t)) {
            Json err = Json::object();
            err["error"] = "Task not found";
            err["id"] = id;
            return Response::json(err, 404);
        }
        return Response::json(t.to_json());
    });

    // ------------------------------------------------------------------------
    // POST /api/v1/tasks - Create a new task
    // ------------------------------------------------------------------------
    api.post("/tasks", [&db](Request& req) {
        Json payload = req.get_json();
        if (payload.is_null() || !payload.has("title")) {
            Json err = Json::object();
            err["error"] = "Invalid JSON: 'title' field is required";
            return Response::json(err, 400);
        }

        std::string title = payload["title"].as_string();
        std::string desc = payload.has("description") ? payload["description"].as_string() : "";
        bool completed = payload.has("completed") ? payload["completed"].as_bool() : false;

        Task created = db.create_task(title, desc, completed);
        return Response::json(created.to_json(), 201);
    });

    // ------------------------------------------------------------------------
    // PUT /api/v1/tasks/<int:id> - Update existing task
    // ------------------------------------------------------------------------
    api.put("/tasks/<int:id>", [&db](Request& req) {
        int id = std::stoi(req.path_params["id"]);
        Json payload = req.get_json();

        Task existing;
        if (!db.get_task(id, existing)) {
            Json err = Json::object();
            err["error"] = "Task not found";
            return Response::json(err, 404);
        }

        std::string title = payload.has("title") ? payload["title"].as_string() : existing.title;
        std::string desc = payload.has("description") ? payload["description"].as_string() : existing.description;
        bool completed = payload.has("completed") ? payload["completed"].as_bool() : existing.completed;

        db.update_task(id, title, desc, completed);

        Task updated;
        db.get_task(id, updated);
        return Response::json(updated.to_json(), 200);
    });

    // ------------------------------------------------------------------------
    // DELETE /api/v1/tasks/<int:id> - Delete task
    // ------------------------------------------------------------------------
    api.del("/tasks/<int:id>", [&db](Request& req) {
        int id = std::stoi(req.path_params["id"]);
        if (!db.delete_task(id)) {
            Json err = Json::object();
            err["error"] = "Task not found";
            return Response::json(err, 404);
        }

        Json res = Json::object();
        res["message"] = "Task deleted successfully";
        res["id"] = id;
        return Response::json(res, 200);
    });

    // 3. Register Blueprint into Application
    app.register_blueprint(api);

    // 4. Start Server
    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "==================================================" << std::endl;
    std::cout << "  Peregrine RESTful Task API Running" << std::endl;
    std::cout << "  Base URL: http://" << host << ":" << port << "/api/v1" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Endpoints:" << std::endl;
    std::cout << "  - GET    /api/v1/tasks" << std::endl;
    std::cout << "  - GET    /api/v1/tasks/<id>" << std::endl;
    std::cout << "  - POST   /api/v1/tasks" << std::endl;
    std::cout << "  - PUT    /api/v1/tasks/<id>" << std::endl;
    std::cout << "  - DELETE /api/v1/tasks/<id>" << std::endl;

    app.run(host, port);
    return 0;
}
