// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "peregrine/peregrine.hpp"
#include <iostream>

using namespace peregrine;

int main() {
    // 1. Initialize the Peregrine application
    App app("hello_world_app");

    // 2. Define a simple plain-text route
    app.get("/", [](Request&) {
        return Response::text("Hello, World! Welcome to Peregrine C++ Web Framework.");
    });

    // 3. Define an HTML route
    app.get("/html", [](Request&) {
        return Response::html(
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Peregrine Hello World</title></head>"
            "<body style='font-family: sans-serif; text-align: center; padding-top: 50px;'>"
            "  <h1>🦅 Hello from Peregrine!</h1>"
            "  <p>A fast, modern, header-only C++11 web framework.</p>"
            "  <p><a href='/greet/Developer'>Click here to test dynamic routing</a></p>"
            "</body>"
            "</html>"
        );
    });

    // 4. Define a dynamic route with a path parameter
    app.get("/greet/<string:name>", [](Request& req) {
        std::string name = req.path_params["name"];
        return Response::text("Hello, " + name + "! Welcome to your Peregrine application.");
    });

    // 5. Start the multi-threaded HTTP server
    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "==================================================" << std::endl;
    std::cout << "  Peregrine Hello World Server Running" << std::endl;
    std::cout << "  Listening on: http://" << host << ":" << port << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  - GET http://" << host << ":" << port << "/" << std::endl;
    std::cout << "  - GET http://" << host << ":" << port << "/html" << std::endl;
    std::cout << "  - GET http://" << host << ":" << port << "/greet/<your-name>" << std::endl;

    app.run(host, port);
    return 0;
}
