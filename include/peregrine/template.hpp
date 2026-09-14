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
// peregrine/template.hpp
//
// Lightweight template rendering engine supporting variable substitution ({{var}})
// and collection iterations ({{#each list}}...{{this.field}}...{{/each}}).
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "common.hpp"

namespace peregrine {

struct TemplateContext {
    std::map<std::string, std::string> vars;
    std::map<std::string, std::vector<std::map<std::string, std::string>>> lists;

    TemplateContext& set(const std::string& key, const std::string& value) {
        vars[key] = value;
        return *this;
    }

    TemplateContext& append(const std::string& list_name,
                            const std::map<std::string, std::string>& item) {
        lists[list_name].push_back(item);
        return *this;
    }
};

inline std::string render_string(const std::string& tmpl, const TemplateContext& ctx) {
    std::string out;
    size_t i = 0;

    while (i < tmpl.size()) {
        if (tmpl.compare(i, 2, "{{") == 0) {
            size_t end = tmpl.find("}}", i);
            if (end == std::string::npos) {
                out += tmpl[i++];
                continue;
            }

            std::string tag = trim(tmpl.substr(i + 2, end - i - 2));

            // Check for loop block: {{#each list_name}}
            if (starts_with(tag, "#each ")) {
                std::string list_name = trim(tag.substr(6));
                std::string close_tag = "{{/each}}";
                size_t body_start = end + 2;
                size_t close_pos = tmpl.find(close_tag, body_start);

                if (close_pos == std::string::npos) {
                    i = end + 2;
                    continue;
                }

                std::string loop_body = tmpl.substr(body_start, close_pos - body_start);
                auto it = ctx.lists.find(list_name);
                if (it != ctx.lists.end()) {
                    for (const auto& item : it->second) {
                        std::string rendered = loop_body;
                        size_t pos = 0;
                        while ((pos = rendered.find("{{this.", pos)) != std::string::npos) {
                            size_t close = rendered.find("}}", pos);
                            if (close == std::string::npos) break;
                            std::string field = rendered.substr(pos + 7, close - (pos + 7));
                            auto field_it = item.find(field);
                            std::string val = (field_it != item.end()) ? field_it->second : "";
                            rendered.replace(pos, close - pos + 2, val);
                            pos += val.size();
                        }
                        out += rendered;
                    }
                }
                i = close_pos + close_tag.size();
                continue;
            }

            // Scalar variable lookup
            auto vit = ctx.vars.find(tag);
            if (vit != ctx.vars.end()) {
                out += vit->second;
            }
            i = end + 2;
        } else {
            out += tmpl[i++];
        }
    }
    return out;
}

inline std::string render_template_file(const std::string& path, const TemplateContext& ctx) {
    std::ifstream in(path);
    if (!in) {
        return "<!-- Template Error: File not found '" + path + "' -->";
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return render_string(ss.str(), ctx);
}

inline std::string render_template(const std::string& path, const TemplateContext& ctx) {
    return render_template_file(path, ctx);
}

}  // namespace peregrine
