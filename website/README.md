# 🌐 Peregrine Documentation Website

A clean, modern, zero-build technical documentation web application for the Peregrine C++ Web Framework.

---

## ✨ Features

- **Peregrine Design System**: Clean typography, card elevations, subtle geometric hero shape accents, and elegant border tokens.
- **Zero Build Tools Required**: Built using standard HTML5, CSS3, and modern vanilla JavaScript. No `npm install`, No Webpack/Vite bundler, and zero runtime dependencies.
- **Client-Side Hash Routing**: Instant view transitions between Home (Categories & Featured Articles) and deep Article reading views without page reloads.
- **Live Search Modal (`Ctrl + K` / `Cmd + K`)**: Instant search across all framework modules, guides, architecture, and APIs with keyboard navigation (`Up`/`Down`/`Enter`).
- **Interactive Code Blocks**: Syntax highlighted C++11 and Bash snippets with one-click copy to clipboard functionality.
- **Light & Dark Mode**: Seamless theme switching with persistent user preference stored in `localStorage`.
- **Responsive Mobile Layout**: Collapsible sidebar navigation drawer for phones and tablets.
- **Deploy Anywhere**: Can be hosted on GitHub Pages, Cloudflare Pages, NGINX, or served directly by Peregrine's own `serve_spa()`.

---

## 🚀 How to View Locally

### Option 1: Direct File Open (Fastest)
Double-click `index.html` in your file explorer to open it directly in any modern browser (Chrome, Edge, Firefox, Safari).

### Option 2: Python Local Server
```bash
cd website
python -m http.server 8000
# Open http://localhost:8000 in your browser
```

### Option 3: Serve via Peregrine Itself
Peregrine can host its own documentation using its single-line `serve_spa()` engine:

```cpp
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;
    app.serve_spa("./website", "index.html");
    app.run("127.0.0.1", 8080);
    return 0;
}
```

---

## 📁 Directory Structure

```text
website/
├── index.html           # Main Single-Page Documentation Web Application
├── css/
│   ├── notate-theme.css # Complete Notate design tokens, cards, grids & typography
│   └── syntax.css       # Clean syntax highlighting for code blocks
├── js/
│   ├── app.js           # Client router, search modal, code copy, and theme toggle
│   └── search_index.js  # Instant search dataset covering all Peregrine topics
└── README.md            # Documentation guide (this file)
```
