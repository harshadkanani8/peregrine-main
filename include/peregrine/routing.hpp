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
// peregrine/routing.hpp
//
// URL routing, parameter extraction (<int:id>, <string:name>, <path:filepath>),
// HTTP method matching, and 405 Method Not Allowed handling.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <functional>
#include <map>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "common.hpp"
#include "request.hpp"
#include "response.hpp"

namespace peregrine {

using Handler = std::function<Response(Request&)>;

struct Route {
    std::string method;
    std::string pattern;
    std::regex compiled;
    std::vector<std::string> param_names;
    Handler handler;
};

class Router {
public:
    void add(const std::string& method, const std::string& pattern, Handler handler) {
        Route route;
        route.method = method;
        route.pattern = pattern;
        route.handler = std::move(handler);
        compile(route);
        routes_.push_back(std::move(route));
    }

    const Route* match(const std::string& method, const std::string& path,
                       std::map<std::string, std::string>& path_params,
                       bool& method_mismatch, std::vector<std::string>& allowed) const {
        method_mismatch = false;
        const Route* matched_route = nullptr;

        for (const auto& r : routes_) {
            std::smatch m;
            if (std::regex_match(path, m, r.compiled)) {
                if (r.method == method) {
                    for (size_t i = 0; i < r.param_names.size(); i++) {
                        path_params[r.param_names[i]] = m[i + 1].str();
                    }
                    return &r;
                } else {
                    method_mismatch = true;
                    if (std::find(allowed.begin(), allowed.end(), r.method) == allowed.end()) {
                        allowed.push_back(r.method);
                    }
                }
            }
        }
        return matched_route;
    }

    const std::vector<Route>& routes() const { return routes_; }

private:
    std::vector<Route> routes_;

    void compile(Route& route) {
        std::string regex_str = "^";
        for (const auto& seg : split(route.pattern, '/')) {
            if (seg.empty()) continue;
            regex_str += "/";
            if (seg.front() == '<' && seg.back() == '>') {
                std::string inner = seg.substr(1, seg.size() - 2);
                std::string type = "string";
                std::string name = inner;
                auto colon = inner.find(':');
                if (colon != std::string::npos) {
                    type = inner.substr(0, colon);
                    name = inner.substr(colon + 1);
                }
                route.param_names.push_back(name);
                if (type == "int") {
                    regex_str += "([0-9]+)";
                } else if (type == "path") {
                    regex_str += "(.+)";
                } else {
                    regex_str += "([^/]+)";
                }
            } else {
                static const std::string special = ".[{()*+?^$|";
                for (char c : seg) {
                    if (special.find(c) != std::string::npos) {
                        regex_str += '\\';
                    }
                    regex_str += c;
                }
            }
        }

        if (regex_str == "^") regex_str = "^/";
        regex_str += "$";
        if (route.pattern == "/") regex_str = "^/$";
        route.compiled = std::regex(regex_str);
    }
};

}  // namespace peregrine
