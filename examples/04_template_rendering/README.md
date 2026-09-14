# Example 04: Dynamic HTML Template Rendering

Demonstrates how to render HTML pages dynamically using Peregrine's built-in mustache template engine.

---

## Features Demonstrated

- **Scalar Variables**: Replacing `{{variable_name}}` with string values using `ctx.set("variable_name", value)`.
- **Collection Loops**: Iterating over lists with `{{#each list_name}} ... {{this.field}} ... {{/each}}` using `ctx.append("list_name", item_map)`.
- **Template Files**: Loading and rendering `.html` files from disk with `render_template(filepath, ctx)`.
- **Inline Templates**: Rendering raw template strings in memory using `render_string(tmpl_str, ctx)`.
- **Zero Overhead**: Single-pass template parser that runs in microseconds without external template libraries.

---

## Template Syntax Reference

### 1. Variables
```html
<h1>{{site_title}}</h1>
<p>Welcome, {{user_name}}!</p>
```

### 2. Collection Loops
```html
<ul>
  {{#each products}}
    <li>{{this.name}} - {{this.price}} ({{this.category}})</li>
  {{/each}}
</ul>
```

---

## Build & Run

### Linux & macOS (GCC / Clang)

From the project root:

```bash
g++ -std=c++11 -I include examples/04_template_rendering/main.cpp -lssl -lcrypto -lpthread -o templates_demo
./templates_demo
```

### Windows (MinGW / GCC)

```powershell
g++ -std=c++11 -I include examples/04_template_rendering/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o templates_demo.exe
.\templates_demo.exe
```

---

## Testing in Browser

1. Open `http://127.0.0.1:8080/` in your web browser. You will see a styled product grid rendered from `templates/index.html`.
2. Open `http://127.0.0.1:8080/inline?name=Developer` to see on-the-fly string rendering.
