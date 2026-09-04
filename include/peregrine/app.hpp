 /*
 * =========================================================================
 *     ____                              _             __ __ 
 *    /  __\___  ________  ____ ________(_)_  _____   / // / 
 *   / /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \/ -_) / // /_ 
 *  / .___/\__//_/  \__/\_, /_/  /_/_/_/_//_/\__/ /__  __/ 
 * /_/                 /___/                        /_/    
 *
 *  Peregrine++ Web Application Framework
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 * =========================================================================
 */


// ============================================================================
// peregrine/app.hpp
//
// Application coordinator: routing, Blueprints, static/SPA engine,
// lifecycle hooks (before/after_request), session signing, and the multi-threaded
// POSIX / TLS socket listener loop.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "blueprint.hpp"
#include "config.hpp"
#include "connection.hpp"
#include "csrf.hpp"
#include "helpers.hpp"
#include "request.hpp"
#include "response.hpp"
#include "routing.hpp"
#include "server.hpp"
#include "session.hpp"
#include "thread_pool.hpp"
#include "types.hpp"
#include "view.hpp"

namespace peregrine {

class App {
public:
    Config config;

    // Session Security Configuration
    std::string secret_key;
    std::vector<std::string> secret_key_fallbacks;
    std::string session_cookie_name = "session";
    int session_lifetime_seconds = 86400;  // 24 hours

    // CSRF Protection
    bool csrf_protect = false;
    std::vector<std::string> csrf_exempt_paths;

    // Application identification & Static/Template directory configuration
    std::string import_name = "app";
    std::string static_folder = "";
    std::string static_url_path = "";
    std::string template_folder = "templates";

    App() = default;

    // Constructor: App("my_app", static_folder="dist", static_url_path="")
    App(std::string name, std::string static_folder_dir = "", std::string static_url = "")
        : import_name(std::move(name)),
          static_folder(std::move(static_folder_dir)),
          static_url_path(std::move(static_url)) {}

    // ------------------------------------------------------------------------
    // CSRF Configuration
    // ------------------------------------------------------------------------

    void enable_csrf_protection(std::vector<std::string> exempt_paths = {}) {
        csrf_protect = true;
        csrf_exempt_paths = std::move(exempt_paths);
    }

    // ------------------------------------------------------------------------
    // Route Registration
    // ------------------------------------------------------------------------

    void route(const std::string& path, const std::vector<std::string>& methods, Handler handler) {
        for (const auto& m : methods) {
            router_.add(m, path, handler);
        }
    }

    void get(const std::string& path, Handler handler) {
        router_.add("GET", path, std::move(handler));
    }

    void post(const std::string& path, Handler handler) {
        router_.add("POST", path, std::move(handler));
    }

    void put(const std::string& path, Handler handler) {
        router_.add("PUT", path, std::move(handler));
    }

    void patch(const std::string& path, Handler handler) {
        router_.add("PATCH", path, std::move(handler));
    }

    void del(const std::string& path, Handler handler) {
        router_.add("DELETE", path, std::move(handler));
    }

    void delete_(const std::string& path, Handler handler) {
        router_.add("DELETE", path, std::move(handler));
    }

    void options(const std::string& path, Handler handler) {
        router_.add("OPTIONS", path, std::move(handler));
    }

    void add_url_rule(const std::string& path, const std::vector<std::string>& methods, Handler handler) {
        route(path, methods, std::move(handler));
    }

    template <typename ViewClass>
    void add_url_rule(const std::string& path, const std::string& /*endpoint*/ = "") {
        auto h = MethodView::as_view<ViewClass>();
        route(path, {"GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS", "HEAD"}, h);
    }

    // ------------------------------------------------------------------------
    // Blueprint Registration
    // ------------------------------------------------------------------------

    void register_blueprint(const Blueprint& bp, const std::string& url_prefix_override = "") {
        std::string prefix = url_prefix_override.empty() ? bp.url_prefix() : url_prefix_override;
        if (!prefix.empty() && prefix.back() == '/') {
            prefix.pop_back();
        }

        for (const auto& dr : bp.routes()) {
            std::string full_path = prefix + dr.pattern;
            if (full_path.empty()) full_path = "/";

            auto bp_before = bp.before_hooks();
            auto bp_after = bp.after_hooks();
            Handler inner = dr.handler;

            Handler wrapped = [bp_before, bp_after, inner](Request& req) -> Response {
                for (const auto& bh : bp_before) {
                    bh(req);
                }
                Response res = inner(req);
                for (const auto& ah : bp_after) {
                    ah(req, res);
                }
                return res;
            };

            router_.add(dr.method, full_path, wrapped);
        }

        for (const auto& item : bp.error_handlers()) {
            int code = item.first;
            const auto& handler = item.second;
            if (error_handlers_.find(code) == error_handlers_.end()) {
                error_handlers_[code] = handler;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Middleware Hooks & Error Handlers
    // ------------------------------------------------------------------------

    void before_request(std::function<void(Request&)> fn) {
        before_.push_back(std::move(fn));
    }

    void after_request(std::function<void(Request&, Response&)> fn) {
        after_.push_back(std::move(fn));
    }

    void error_handler(int status_code, std::function<Response(Request&)> fn) {
        error_handlers_[status_code] = std::move(fn);
    }

    // ------------------------------------------------------------------------
    // Static Files & Single Page Application (SPA) Serving
    // ------------------------------------------------------------------------

    void static_dir(const std::string& url_prefix, const std::string& directory_path) {
        std::string prefix = url_prefix;
        if (!prefix.empty() && prefix.back() == '/') prefix.pop_back();
        std::string pattern = prefix + "/<path:filepath>";
        std::string dir = directory_path;

        router_.add("GET", pattern, [dir](Request& req) -> Response {
            std::string file = req.path_params["filepath"];
            std::string body, mime;
            if (!read_static_file(dir, file, body, mime)) {
                return Response::text("File Not Found", 404);
            }
            Response r;
            r.status = 200;
            r.body = std::move(body);
            r.headers["Content-Type"] = std::move(mime);
            return r;
        });
    }

    void serve_spa(const std::string& dist_directory = "dist",
                   const std::string& index_file = "index.html",
                   const std::string& api_prefix = "api") {
        router_.add("GET", "/", [dist_directory, index_file](Request&) -> Response {
            std::string body, mime;
            if (!read_static_file(dist_directory, index_file, body, mime)) {
                return Response::text("SPA Error: '" + index_file + "' not found in " + dist_directory, 404);
            }
            Response r;
            r.status = 200;
            r.body = std::move(body);
            r.headers["Content-Type"] = "text/html; charset=utf-8";
            return r;
        });

        router_.add("GET", "/<path:filepath>", [dist_directory, index_file, api_prefix](Request& req) -> Response {
            std::string requested = req.path_params["filepath"];

            std::string body, mime;
            if (read_static_file(dist_directory, requested, body, mime)) {
                Response r;
                r.status = 200;
                r.body = std::move(body);
                r.headers["Content-Type"] = std::move(mime);
                return r;
            }

            if (!api_prefix.empty() && (starts_with(requested, api_prefix + "/") || requested == api_prefix)) {
                Json j = Json::object();
                j["error"] = "API endpoint not found";
                return Response::json(j, 404);
            }

            // Return 404 for system probes (e.g. Chrome DevTools .well-known, .git)
            if (starts_with(requested, ".well-known") || starts_with(requested, ".")) {
                return Response::text("Not Found", 404);
            }

            std::string idx_body, idx_mime;
            if (read_static_file(dist_directory, index_file, idx_body, idx_mime)) {
                Response r;
                r.status = 200;
                r.body = std::move(idx_body);
                r.headers["Content-Type"] = "text/html; charset=utf-8";
                return r;
            }

            return Response::text("File Not Found", 404);
        });
    }

    // ------------------------------------------------------------------------
    // Server Execution
    // ------------------------------------------------------------------------

    void run(const std::string& host, int port) {
        ServerConfig cfg;
        cfg.host = host;
        cfg.port = port;
        cfg.protocol = Protocol::HTTP;
        run(cfg);
    }

    void run(const ServerConfig& config) {
        if (!static_folder.empty()) {
            if (static_url_path == "" || static_url_path == "/") {
                serve_spa(static_folder);
            } else {
                static_dir(static_url_path, static_folder);
            }
        }

        SSL_CTX* ssl_ctx = nullptr;
        if (config.protocol == Protocol::HTTPS) {
            ssl_ctx = detail::create_ssl_context(config.cert_file, config.key_file, config.tls_version);
            if (!ssl_ctx) {
                std::cerr << "Failed to initialize HTTPS context. Terminating.\n";
                return;
            }
        }

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket failed");
            if (ssl_ctx) SSL_CTX_free(ssl_ctx);
            return;
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(config.port));
        if (inet_pton(AF_INET, config.host.c_str(), &address.sin_addr) <= 0) {
            address.sin_addr.s_addr = INADDR_ANY;
        }

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            close(server_fd);
            if (ssl_ctx) SSL_CTX_free(ssl_ctx);
            return;
        }

        if (listen(server_fd, config.backlog) < 0) {
            perror("listen failed");
            close(server_fd);
            if (ssl_ctx) SSL_CTX_free(ssl_ctx);
            return;
        }

        std::string scheme = (config.protocol == Protocol::HTTPS) ? "https" : "http";

        std::cout << "\033[1;36m";
        std::cout << "======================================================================\n";
        std::cout << "  ____                              _             \n";
        std::cout << " |  _ \\ ___ _ __ ___  __ _ _ __(_)_ __   ___   \n";
        std::cout << " | |_) / _ \\ '__/ _ \\/ _` | '__| | '_ \\ / _ \\  \n";
        std::cout << " |  __/  __/ | |  __/ (_| | |  | | | | |  __/  \n";
        std::cout << " |_|   \\___|_|  \\___|\\__, |_|  |_|_| |_|\\___|  \n";
        std::cout << "                     |___/                     \n";
        std::cout << "             Peregrine++ C++ Web Framework     \n";
        std::cout << "  Author   : Harshad M. Kanani                 \n";
        std::cout << "  Copyright: (c) 2026 Harshad Kanani. All rights reserved.\n";
        std::cout << "======================================================================\033[0m\n\n";

        std::cout << " * Serving Peregrine C++ Web Framework\n";
        std::cout << " * Running on " << scheme << "://" << config.host << ":" << config.port << "/\n";

        // Initialize C++11 Thread Pool for bounded concurrent request execution
        ThreadPool pool(config.thread_pool_size, config.max_queue_size);
        std::cout << " * Worker Thread Pool : " << pool.thread_count()
                  << " threads (Queue limit: " << config.max_queue_size << ")\n";
        std::cout << " * Press CTRL+C to quit\n\n";

        while (true) {
            sockaddr_in client_addr{};
            socklen_t addrlen = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
            if (client_fd < 0) continue;

            // Apply socket read/write timeouts (protects against Slowloris hanging attacks)
            if (config.socket_timeout_seconds > 0) {
                struct timeval tv;
                tv.tv_sec = config.socket_timeout_seconds;
                tv.tv_usec = 0;
                setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
                setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
            }

            char client_ip[INET_ADDRSTRLEN] = "127.0.0.1";
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            std::string remote_addr(client_ip);

            if (config.protocol == Protocol::HTTPS) {
                SSL* ssl = SSL_new(ssl_ctx);
                SSL_set_fd(ssl, client_fd);
                bool queued = pool.enqueue([this, ssl, client_fd, scheme, remote_addr]() {
                    if (SSL_accept(ssl) <= 0) {
                        ERR_print_errors_fp(stderr);
                        SSL_shutdown(ssl);
                        SSL_free(ssl);
                        close(client_fd);
                        return;
                    }
                    Connection conn(client_fd, ssl);
                    handle_connection(conn, scheme, remote_addr);
                });

                if (!queued) {
                    // Queue full: reject gracefully
                    SSL_shutdown(ssl);
                    SSL_free(ssl);
                    close(client_fd);
                }
            } else {
                bool queued = pool.enqueue([this, client_fd, scheme, remote_addr]() {
                    Connection conn(client_fd);
                    handle_connection(conn, scheme, remote_addr);
                });

                if (!queued) {
                    // Queue full: send 503 Overloaded response and close
                    static const char* busy_resp =
                        "HTTP/1.1 503 Service Unavailable\r\n"
                        "Content-Type: text/plain; charset=utf-8\r\n"
                        "Content-Length: 34\r\n"
                        "Connection: close\r\n\r\n"
                        "503 Service Temporarily Overloaded";
                    send(client_fd, busy_resp, std::strlen(busy_resp), 0);
                    close(client_fd);
                }
            }
        }

        pool.stop();
        close(server_fd);
        if (ssl_ctx) SSL_CTX_free(ssl_ctx);
    }

private:
    Router router_;
    std::vector<std::function<void(Request&)>> before_;
    std::vector<std::function<void(Request&, Response&)>> after_;
    std::map<int, std::function<Response(Request&)>> error_handlers_;

    void handle_connection(Connection conn, std::string scheme, std::string remote_addr) {
        std::string raw = detail::read_full_request(conn, 16 * 1024 * 1024);
        if (raw.empty()) {
            conn.close_conn();
            return;
        }

        Request req = detail::parse_request(raw);
        req.scheme = std::move(scheme);
        req.remote_addr = std::move(remote_addr);

        // --------------------------------------------------------------------
        // Open Session (mirrors Peregrine SecureCookieSessionInterface.open_session)
        // --------------------------------------------------------------------
        bool had_cookie = false;
        std::string cookie_name = config.get("SESSION_COOKIE_NAME", session_cookie_name);
        int lifetime = config.get_int("PERMANENT_SESSION_LIFETIME", session_lifetime_seconds);

        if (!secret_key.empty() && req.cookies.count(cookie_name)) {
            had_cookie = verify_and_load_session(secret_key, req.cookies[cookie_name],
                                                 lifetime, req.session.data());
            // Fallback key rotation check
            if (!had_cookie) {
                for (const auto& fallback_key : secret_key_fallbacks) {
                    had_cookie = verify_and_load_session(fallback_key, req.cookies[cookie_name],
                                                         lifetime, req.session.data());
                    if (had_cookie) {
                        req.session.modified = true; // Re-sign with primary key on save
                        break;
                    }
                }
            }

            if (had_cookie) {
                req.session.permanent = (req.session.get("_permanent") == "1" || req.session.get("_permanent") == "true");
                req.session.is_new = false;
                req.session.modified = false;
            } else {
                req.session.is_new = true;
                req.session.modified = false;
            }
        } else {
            req.session.is_new = true;
            req.session.modified = false;
        }

        if (csrf_protect && is_mutating_method(req.method)) {
            bool exempt = false;
            for (const auto& p : csrf_exempt_paths) {
                if (req.path == p) {
                    exempt = true;
                    break;
                }
            }
            if (!exempt && !csrf::verify_token(req)) {
                Response res = run_error(400, req);
                res.body = "CSRF Token verification failed";
                res.headers["Content-Type"] = "text/plain; charset=utf-8";
                std::string out = detail::serialize_response(res);
                conn.write(out.data(), out.size());
                conn.close_conn();
                return;
            }
        }

        for (const auto& fn : before_) {
            fn(req);
        }

        Response res = dispatch(req);

        for (const auto& fn : after_) {
            fn(req, res);
        }

        // --------------------------------------------------------------------
        // Save Session (mirrors Peregrine SecureCookieSessionInterface.save_session)
        // --------------------------------------------------------------------
        if (!secret_key.empty()) {
            std::string cookie_path = config.get("SESSION_COOKIE_PATH", "/");
            std::string cookie_domain = config.get("SESSION_COOKIE_DOMAIN", "");
            bool httponly = config.get_bool("SESSION_COOKIE_HTTPONLY", true);
            bool secure = config.get_bool("SESSION_COOKIE_SECURE", req.is_secure());
            std::string samesite = config.get("SESSION_COOKIE_SAMESITE", "Lax");
            bool partitioned = config.get_bool("SESSION_COOKIE_PARTITIONED", false);
            int max_age = req.session.permanent ? lifetime : -1;

            // Add "Vary: Cookie" header if session was accessed (mirrors Peregrine 3.1+)
            if (req.session.accessed) {
                res.set_header("Vary", "Cookie");
            }

            bool refresh_each = config.get_bool("SESSION_REFRESH_EACH_REQUEST", false);
            bool should_set = req.session.modified || (req.session.permanent && refresh_each);

            if (req.session.empty()) {
                if (had_cookie || req.session.modified) {
                    res.delete_cookie(cookie_name, cookie_path, cookie_domain);
                }
            } else if (should_set) {
                if (req.session.permanent) {
                    req.session.data()["_permanent"] = "1";
                } else {
                    req.session.data().erase("_permanent");
                }
                std::string token = sign_session(secret_key, req.session.data());
                res.set_cookie(cookie_name, token, max_age, cookie_path, cookie_domain,
                               httponly, secure, samesite, partitioned);
            }
        }

        if (res.is_stream()) {
            std::string header_bytes = detail::serialize_response_headers(res);
            conn.write(header_bytes.data(), header_bytes.size());
            try {
                res.streamer(conn);
            } catch (...) {}
            conn.close_conn();
            return;
        }

        bool head_only = (req.method == "HEAD");
        std::string out = detail::serialize_response(res, head_only);
        conn.write(out.data(), out.size());
        conn.close_conn();
    }

    Response dispatch(Request& req) {
        std::map<std::string, std::string> path_params;
        bool method_mismatch = false;
        std::vector<std::string> allowed;

        std::string lookup_method = (req.method == "HEAD") ? "GET" : req.method;
        const Route* route =
            router_.match(lookup_method, req.path, path_params, method_mismatch, allowed);

        if (!route) {
            if (method_mismatch) {
                std::string allow_list;
                for (size_t i = 0; i < allowed.size(); i++) {
                    if (i > 0) allow_list += ", ";
                    allow_list += allowed[i];
                }
                Response r = run_error(405, req);
                r.headers["Allow"] = allow_list;
                return r;
            }
            return run_error(404, req);
        }

        req.path_params = std::move(path_params);

        try {
            return route->handler(req);
        } catch (const HTTPException& ex) {
            return run_error(ex.code, req);
        } catch (const std::exception& ex) {
            std::cerr << "Unhandled Exception in route handler: " << ex.what() << "\n";
            return run_error(500, req);
        } catch (...) {
            std::cerr << "Unhandled unknown exception in route handler\n";
            return run_error(500, req);
        }
    }

    Response run_error(int status_code, Request& req) {
        auto it = error_handlers_.find(status_code);
        if (it != error_handlers_.end()) {
            try {
                Response res = it->second(req);
                res.status = status_code;
                return res;
            } catch (...) {
                return fallback_error(500);
            }
        }
        return fallback_error(status_code);
    }

    static Response fallback_error(int code) {
        Response r;
        r.status = code;
        r.headers["Content-Type"] = "text/html; charset=utf-8";
        r.body = "<!DOCTYPE html><html><head><title>" + std::to_string(code) + " " +
                 detail::status_reason(code) + "</title></head><body><h1>" +
                 std::to_string(code) + " " + detail::status_reason(code) +
                 "</h1><p>The requested URL returned an error.</p></body></html>";
        return r;
    }

    static bool is_mutating_method(const std::string& method) {
        return (method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE");
    }
};

using Peregrine = App;

}  // namespace peregrine
