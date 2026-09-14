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
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

// ============================================================================
// peregrine/blueprint.hpp
//
// Blueprints for modular route and middleware organization.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common.hpp"
#include "request.hpp"
#include "response.hpp"
#include "routing.hpp"

namespace peregrine {

class Blueprint {
public:
    struct DeferredRoute {
        std::string method;
        std::string pattern;
        Handler handler;
    };

    Blueprint(std::string name, std::string url_prefix = "")
        : name_(std::move(name)), url_prefix_(std::move(url_prefix)) {
        if (!url_prefix_.empty() && url_prefix_.back() == '/') {
            url_prefix_.pop_back();
        }
    }

    const std::string& name() const { return name_; }
    const std::string& url_prefix() const { return url_prefix_; }

    void set_url_prefix(const std::string& prefix) {
        url_prefix_ = prefix;
        if (!url_prefix_.empty() && url_prefix_.back() == '/') {
            url_prefix_.pop_back();
        }
    }

    // ------------------------------------------------------------------------
    // Route Registration
    // ------------------------------------------------------------------------

    void route(const std::string& path, const std::vector<std::string>& methods, Handler h) {
        for (const auto& m : methods) {
            routes_.push_back({m, path, h});
        }
    }

    void get(const std::string& path, Handler h) {
        routes_.push_back({"GET", path, std::move(h)});
    }

    void post(const std::string& path, Handler h) {
        routes_.push_back({"POST", path, std::move(h)});
    }

    void put(const std::string& path, Handler h) {
        routes_.push_back({"PUT", path, std::move(h)});
    }

    void patch(const std::string& path, Handler h) {
        routes_.push_back({"PATCH", path, std::move(h)});
    }

    void del(const std::string& path, Handler h) {
        routes_.push_back({"DELETE", path, std::move(h)});
    }

    void delete_(const std::string& path, Handler h) {
        routes_.push_back({"DELETE", path, std::move(h)});
    }

    void options(const std::string& path, Handler h) {
        routes_.push_back({"OPTIONS", path, std::move(h)});
    }

    void add_url_rule(const std::string& path, const std::vector<std::string>& methods, Handler h) {
        route(path, methods, std::move(h));
    }

    // ------------------------------------------------------------------------
    // Middleware Hooks & Error Handlers
    // ------------------------------------------------------------------------

    void before_request(std::function<void(Request&)> fn) {
        before_hooks_.push_back(std::move(fn));
    }

    void after_request(std::function<void(Request&, Response&)> fn) {
        after_hooks_.push_back(std::move(fn));
    }

    void error_handler(int code, std::function<Response(Request&)> fn) {
        error_handlers_[code] = std::move(fn);
    }

    const std::vector<DeferredRoute>& routes() const { return routes_; }
    const std::vector<std::function<void(Request&)>>& before_hooks() const { return before_hooks_; }
    const std::vector<std::function<void(Request&, Response&)>>& after_hooks() const {
        return after_hooks_;
    }
    const std::map<int, std::function<Response(Request&)>>& error_handlers() const {
        return error_handlers_;
    }

private:
    std::string name_;
    std::string url_prefix_;
    std::vector<DeferredRoute> routes_;
    std::vector<std::function<void(Request&)>> before_hooks_;
    std::vector<std::function<void(Request&, Response&)>> after_hooks_;
    std::map<int, std::function<Response(Request&)>> error_handlers_;
};

}  // namespace peregrine
