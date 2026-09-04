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
// peregrine/view.hpp
//
// Class-based views and RESTful MethodView implementations.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "request.hpp"
#include "response.hpp"
#include "routing.hpp"

namespace peregrine {

class View {
public:
    virtual ~View() = default;
    virtual Response dispatch_request(Request& req) = 0;
};

class MethodView : public View {
public:
    virtual ~MethodView() = default;

    virtual Response get(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response post(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response put(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response patch(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response delete_(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response options(Request&) {
        return Response::text("Method Not Allowed", 405);
    }
    virtual Response head(Request& req) {
        return get(req);
    }

    Response dispatch_request(Request& req) override {
        if (req.method == "GET") return get(req);
        if (req.method == "POST") return post(req);
        if (req.method == "PUT") return put(req);
        if (req.method == "PATCH") return patch(req);
        if (req.method == "DELETE") return delete_(req);
        if (req.method == "OPTIONS") return options(req);
        if (req.method == "HEAD") return head(req);
        return Response::text("Method Not Allowed", 405);
    }

    template <typename T>
    static Handler as_view() {
        return [](Request& req) -> Response {
            T view_instance;
            return view_instance.dispatch_request(req);
        };
    }
};

}  // namespace peregrine
