// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Harshad Kanani. All rights reserved.

#include "peregrine/peregrine.hpp"
#include <iostream>
#include <string>

using namespace peregrine;

int main() {
    App app("auth_session_app");

    // 1. Configure cryptographic secret key for HMAC-SHA256 session signing
    app.secret_key = "super_secret_cryptographic_key_replace_in_production";
    app.session_cookie_name = "peregrine_session";
    app.session_lifetime_seconds = 3600; // 1 hour session validity

    // 2. Public Home / Status Page
    app.get("/", [](Request& req) {
        bool is_authenticated = req.session.has("user");
        std::string username = is_authenticated ? req.session["user"] : "Guest";

        std::string html =
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Peregrine Authentication & Sessions</title></head>"
            "<body style='font-family: sans-serif; max-width: 600px; margin: 40px auto; line-height: 1.6;'>"
            "  <h1>🦅 Peregrine Session Demo</h1>"
            "  <p>Current User: <strong>" + username + "</strong></p>";

        if (is_authenticated) {
            html +=
                "  <div style='background: #e8f5e9; padding: 15px; border-radius: 6px;'>"
                "    <p>You are successfully logged in!</p>"
                "    <p><a href='/dashboard'>Go to Protected Dashboard</a></p>"
                "    <form method='POST' action='/logout'>"
                "      <button type='submit' style='padding: 8px 16px; background: #e53935; color: white; border: none; border-radius: 4px; cursor: pointer;'>Log Out</button>"
                "    </form>"
                "  </div>";
        } else {
            html +=
                "  <div style='background: #f5f5f5; padding: 20px; border-radius: 6px;'>"
                "    <h3>Login</h3>"
                "    <form method='POST' action='/login'>"
                "      <div style='margin-bottom: 12px;'>"
                "        <label>Username: </label><br/>"
                "        <input type='text' name='username' value='admin' style='width: 100%; padding: 8px; box-sizing: border-box;' />"
                "      </div>"
                "      <div style='margin-bottom: 12px;'>"
                "        <label>Password: </label><br/>"
                "        <input type='password' name='password' value='secret123' style='width: 100%; padding: 8px; box-sizing: border-box;' />"
                "      </div>"
                "      <button type='submit' style='padding: 8px 16px; background: #1e88e5; color: white; border: none; border-radius: 4px; cursor: pointer;'>Sign In</button>"
                "    </form>"
                "    <p style='font-size: 0.9em; color: #666;'>Demo credentials: username=<strong>admin</strong>, password=<strong>secret123</strong></p>"
                "  </div>";
        }

        html +=
            "  <hr style='margin-top: 30px;' />"
            "  <p><a href='/session-debug'>View Raw Session JSON</a></p>"
            "</body>"
            "</html>";

        return Response::html(html);
    });

    // 3. Login Endpoint (POST)
    app.post("/login", [](Request& req) {
        std::string username = req.form_get("username");
        std::string password = req.form_get("password");

        // Validate credentials (in production, verify against a password hash)
        if (username == "admin" && password == "secret123") {
            req.session["user"] = username;
            req.session["role"] = "administrator";
            req.session.permanent = true; // Sets cookie Max-Age

            return Response::redirect("/");
        }

        return Response::html("<h3>Invalid credentials. <a href='/'>Try again</a></h3>", 401);
    });

    // 4. Protected Dashboard (Requires Authenticated Session)
    app.get("/dashboard", [](Request& req) {
        if (!req.session.has("user")) {
            return Response::text("Unauthorized: You must log in to view this page.", 401);
        }

        std::string user = req.session["user"];
        std::string role = req.session["role"];

        std::string html =
            "<!DOCTYPE html>"
            "<html>"
            "<head><title>Dashboard</title></head>"
            "<body style='font-family: sans-serif; max-width: 600px; margin: 40px auto;'>"
            "  <h1>Dashboard</h1>"
            "  <p>Welcome to the protected dashboard, <strong>" + user + "</strong> (" + role + ")!</p>"
            "  <p>This page verified your HMAC-SHA256 cryptographically signed session cookie.</p>"
            "  <p><a href='/'>Back to Home</a></p>"
            "</body>"
            "</html>";

        return Response::html(html);
    });

    // 5. Logout Endpoint (Clears Session)
    app.post("/logout", [](Request& req) {
        req.session.clear();
        return Response::redirect("/");
    });

    // 6. Session Debug Endpoint (Returns JSON representation of session)
    app.get("/session-debug", [](Request& req) {
        Json j = Json::object();
        j["authenticated"] = req.session.has("user");
        j["modified"] = req.session.modified;
        j["permanent"] = req.session.permanent;

        Json data = Json::object();
        for (const auto& kv : req.session.data) {
            data[kv.first] = kv.second;
        }
        j["data"] = data;

        return Response::json(j);
    });

    // 7. Start the Server
    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "==================================================" << std::endl;
    std::cout << "  Peregrine Session & Authentication Demo" << std::endl;
    std::cout << "  Listening on: http://" << host << ":" << port << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Open your browser to: http://" << host << ":" << port << "/" << std::endl;

    app.run(host, port);
    return 0;
}
