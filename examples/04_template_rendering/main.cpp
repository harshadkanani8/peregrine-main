#include "peregrine/peregrine.hpp"
#include <iostream>

using namespace peregrine;

int main() {
    App app("template_example_app");

    // ------------------------------------------------------------------------
    // GET / - Render template file from disk
    // ------------------------------------------------------------------------
    app.get("/", [](Request&) {
        TemplateContext ctx;

        // 1. Set scalar variables
        ctx.set("site_title", "Peregrine Hardware Store");
        ctx.set("user_name", "Valued Customer");
        ctx.set("current_year", "2026");

        // 2. Append collection items for {{#each products}} loop
        ctx.append("products", {
            {"name", "Embedded ARM Linux Board"},
            {"price", "$49.99"},
            {"category", "Hardware"},
            {"description", "Ultra-low power quad-core board optimized for native C++ microservices."}
        });

        ctx.append("products", {
            {"name", "Gigabit Ethernet Shield"},
            {"price", "$19.99"},
            {"category", "Networking"},
            {"description", "High-throughput network adapter with hardware offloading."}
        });

        ctx.append("products", {
            {"name", "Industrial NVMe SSD 512GB"},
            {"price", "$79.99"},
            {"category", "Storage"},
            {"description", "Ruggedized flash memory designed for continuous telemetry logging."}
        });

        // 3. Render template file
        std::string rendered = render_template("examples/04_template_rendering/templates/index.html", ctx);
        return Response::html(rendered);
    });

    // ------------------------------------------------------------------------
    // GET /inline - Render dynamic template string directly in memory
    // ------------------------------------------------------------------------
    app.get("/inline", [](Request& req) {
        std::string name = req.arg("name", "Explorer");

        TemplateContext ctx;
        ctx.set("name", name);

        std::string tmpl = "<h3>Inline Template</h3><p>Hello, {{name}}! This string was rendered on-the-fly.</p>";
        return Response::html(render_string(tmpl, ctx));
    });

    // Start server
    const std::string host = "127.0.0.1";
    const int port = 8080;

    std::cout << "==================================================" << std::endl;
    std::cout << "  Peregrine Template Rendering Demo" << std::endl;
    std::cout << "  Listening on: http://" << host << ":" << port << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  - GET http://" << host << ":" << port << "/" << std::endl;
    std::cout << "  - GET http://" << host << ":" << port << "/inline?name=Alice" << std::endl;

    app.run(host, port);
    return 0;
}
