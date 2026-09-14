/*
 * =========================================================================
 *  Peregrine++ Web Application Framework
 *  In-Memory Test Client
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#define private public
#include "peregrine/app.hpp"
#undef private

namespace peregrine {

class TestClient {
public:
    explicit TestClient(App& app) : app_(app) {}

    Response get(const std::string& path, const std::map<std::string, std::string>& headers = {}) {
        Request req;
        req.method = "GET";
        setup_path_and_query(req, path);
        req.headers = headers;
        return send_request(req);
    }

    Response post(const std::string& path,
                  const std::map<std::string, std::string>& form_data = {},
                  const std::map<std::string, std::string>& headers = {}) {
        Request req;
        req.method = "POST";
        setup_path_and_query(req, path);
        req.headers = headers;
        req.form = form_data;

        std::string body;
        for (const auto& kv : form_data) {
            if (!body.empty()) body += "&";
            body += url_encode(kv.first) + "=" + url_encode(kv.second);
        }
        req.body = body;

        if (req.headers.find("content-type") == req.headers.end()) {
            req.headers["content-type"] = "application/x-www-form-urlencoded";
        }
        return send_request(req);
    }

    Response post_json(const std::string& path,
                       const Json& json_payload,
                       const std::map<std::string, std::string>& headers = {}) {
        Request req;
        req.method = "POST";
        setup_path_and_query(req, path);
        req.headers = headers;
        req.body = json_payload.dump();
        req.headers["content-type"] = "application/json";
        return send_request(req);
    }

    Response put(const std::string& path,
                 const std::map<std::string, std::string>& form_data = {},
                 const std::map<std::string, std::string>& headers = {}) {
        Request req;
        req.method = "PUT";
        setup_path_and_query(req, path);
        req.headers = headers;
        req.form = form_data;
        return send_request(req);
    }

    Response del(const std::string& path, const std::map<std::string, std::string>& headers = {}) {
        Request req;
        req.method = "DELETE";
        setup_path_and_query(req, path);
        req.headers = headers;
        return send_request(req);
    }

    bool has_cookie(const std::string& name) const {
        return cookies_.count(name) > 0;
    }

    std::string get_cookie(const std::string& name) const {
        auto it = cookies_.find(name);
        return (it != cookies_.end()) ? it->second : "";
    }

    void set_cookie(const std::string& name, const std::string& value) {
        cookies_[name] = value;
    }

    void clear_cookies() {
        cookies_.clear();
    }

private:
    App& app_;
    std::map<std::string, std::string> cookies_;

    static void setup_path_and_query(Request& req, const std::string& target) {
        req.raw_target = target;
        auto qpos = target.find('?');
        if (qpos != std::string::npos) {
            req.path = target.substr(0, qpos);
            req.query = parse_urlencoded(target.substr(qpos + 1));
        } else {
            req.path = target;
        }
        req.path = url_decode(req.path);
    }

    Response send_request(Request& req) {
        req.cookies = cookies_;

        std::string cookie_name = app_.config.get("SESSION_COOKIE_NAME", app_.session_cookie_name);
        int lifetime = app_.config.get_int("PERMANENT_SESSION_LIFETIME", app_.session_lifetime_seconds);

        if (!app_.secret_key.empty() && req.cookies.count(cookie_name)) {
            verify_and_load_session(app_.secret_key, req.cookies[cookie_name], lifetime, req.session.data());
        }

        // Global before_request hooks
        for (const auto& fn : app_.before_) {
            fn(req);
        }

        // Dispatch route
        Response res = app_.dispatch(req);

        // Global after_request hooks
        for (const auto& fn : app_.after_) {
            fn(req, res);
        }

        // Save session if modified
        if (!app_.secret_key.empty() && req.session.modified) {
            std::string token = sign_session(app_.secret_key, req.session.data());
            res.set_cookie(cookie_name, token, lifetime);
            cookies_[cookie_name] = token;
        }

        // Synchronize Cookie Jar from raw_set_cookies
        for (const auto& sc : res.raw_set_cookies) {
            auto eq = sc.find('=');
            auto semi = sc.find(';');
            if (eq != std::string::npos) {
                std::string name = sc.substr(0, eq);
                std::string val = (semi != std::string::npos) ? sc.substr(eq + 1, semi - eq - 1) : sc.substr(eq + 1);
                cookies_[name] = val;
            }
        }

        return res;
    }
};

} // namespace peregrine
