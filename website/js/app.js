/* ==========================================================================
   Peregrine Documentation Client Application
   Hash-based SPA Router, Search Engine, Code Copy, and Theme Management
   ========================================================================== */

(function () {
  "use strict";

  // State
  let currentTheme = localStorage.getItem("peregrine_docs_theme") || "light";
  let activeSearchIndex = -1;

  // Initialize theme
  document.documentElement.setAttribute("data-theme", currentTheme);

  // DOM Elements
  const themeToggleBtn = document.getElementById("themeToggleBtn");
  const mobileMenuBtn = document.getElementById("mobileMenuBtn");
  const sideNav = document.getElementById("sideNav");
  const searchModal = document.getElementById("searchModal");
  const searchInput = document.getElementById("searchInput");
  const searchResults = document.getElementById("searchResults");
  const homeView = document.getElementById("homeView");
  const articleView = document.getElementById("articleView");
  const breadcrumbCategory = document.getElementById("breadcrumbCategory");
  const breadcrumbCurrent = document.getElementById("breadcrumbCurrent");

  // ==========================================================================
  // Theme Toggle
  // ==========================================================================
  if (themeToggleBtn) {
    themeToggleBtn.addEventListener("click", () => {
      currentTheme = currentTheme === "light" ? "dark" : "light";
      document.documentElement.setAttribute("data-theme", currentTheme);
      localStorage.setItem("peregrine_docs_theme", currentTheme);
      updateThemeIcon();
    });
    updateThemeIcon();
  }

  function updateThemeIcon() {
    if (!themeToggleBtn) return;
    themeToggleBtn.innerHTML = currentTheme === "light"
      ? `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path></svg>`
      : `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line><line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line><line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line></svg>`;
  }

  // ==========================================================================
  // Mobile Menu Toggle
  // ==========================================================================
  if (mobileMenuBtn && sideNav) {
    mobileMenuBtn.addEventListener("click", () => {
      sideNav.classList.toggle("mobile-open");
    });
  }

  // ==========================================================================
  // Sidebar Accordion
  // ==========================================================================
  document.querySelectorAll(".nav-section-title").forEach((title) => {
    title.addEventListener("click", () => {
      const section = title.closest(".nav-section");
      if (section) {
        section.classList.toggle("collapsed");
      }
    });
  });

  // ==========================================================================
  // Copy to Clipboard
  // ==========================================================================
  document.querySelectorAll(".copy-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const wrapper = btn.closest(".code-block-wrapper");
      if (!wrapper) return;
      const code = wrapper.querySelector("code");
      if (!code) return;

      navigator.clipboard.writeText(code.innerText).then(() => {
        const originalText = btn.innerHTML;
        btn.innerHTML = `<svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"></polyline></svg> Copied!`;
        btn.classList.add("copied");
        setTimeout(() => {
          btn.innerHTML = originalText;
          btn.classList.remove("copied");
        }, 2000);
      });
    });
  });

  // ==========================================================================
  // Search Modal & Shortcut (Ctrl+K)
  // ==========================================================================
  const searchTriggers = document.querySelectorAll("[data-action='open-search']");
  searchTriggers.forEach((btn) => {
    btn.addEventListener("click", openSearch);
  });

  window.addEventListener("keydown", (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "k") {
      e.preventDefault();
      openSearch();
    } else if (e.key === "Escape" && searchModal && searchModal.classList.contains("open")) {
      closeSearch();
    }
  });

  if (searchModal) {
    searchModal.addEventListener("click", (e) => {
      if (e.target === searchModal) {
        closeSearch();
      }
    });
  }

  function openSearch() {
    if (!searchModal) return;
    searchModal.classList.add("open");
    if (searchInput) {
      searchInput.value = "";
      searchInput.focus();
      renderSearchResults("");
    }
  }

  function closeSearch() {
    if (!searchModal) return;
    searchModal.classList.remove("open");
  }

  if (searchInput) {
    searchInput.addEventListener("input", (e) => {
      renderSearchResults(e.target.value.trim().toLowerCase());
    });

    searchInput.addEventListener("keydown", (e) => {
      const items = searchResults.querySelectorAll(".search-result-item");
      if (e.key === "ArrowDown") {
        e.preventDefault();
        activeSearchIndex = Math.min(activeSearchIndex + 1, items.length - 1);
        highlightSearchResult(items);
      } else if (e.key === "ArrowUp") {
        e.preventDefault();
        activeSearchIndex = Math.max(activeSearchIndex - 1, 0);
        highlightSearchResult(items);
      } else if (e.key === "Enter" && activeSearchIndex >= 0 && items[activeSearchIndex]) {
        e.preventDefault();
        items[activeSearchIndex].click();
      }
    });
  }

  function renderSearchResults(query) {
    if (!searchResults) return;
    activeSearchIndex = -1;

    if (!query) {
      searchResults.innerHTML = `<div style="padding: 16px; font-size: 0.88rem; color: var(--text-muted); text-align: center;">Type keywords to search documentation, modules, and recipes...</div>`;
      return;
    }

    const matches = PEREGRINE_SEARCH_INDEX.filter((item) => {
      return (
        item.title.toLowerCase().includes(query) ||
        item.snippet.toLowerCase().includes(query) ||
        item.category.toLowerCase().includes(query) ||
        item.keywords.some((k) => k.includes(query))
      );
    });

    if (matches.length === 0) {
      searchResults.innerHTML = `<div style="padding: 20px; font-size: 0.88rem; color: var(--text-muted); text-align: center;">No matching articles found for "<strong>${escapeHtml(query)}</strong>"</div>`;
      return;
    }

    searchResults.innerHTML = matches
      .slice(0, 8)
      .map(
        (m, idx) => `
        <a href="#${m.id}" class="search-result-item" data-idx="${idx}">
          <div class="search-result-category">${m.category}</div>
          <div class="search-result-title">${m.title}</div>
          <div class="search-result-snippet">${m.snippet}</div>
        </a>
      `
      )
      .join("");

    searchResults.querySelectorAll(".search-result-item").forEach((el) => {
      el.addEventListener("click", () => {
        closeSearch();
      });
    });
  }

  function highlightSearchResult(items) {
    items.forEach((item, idx) => {
      item.classList.toggle("selected", idx === activeSearchIndex);
      if (idx === activeSearchIndex) {
        item.scrollIntoView({ block: "nearest" });
      }
    });
  }

  function escapeHtml(str) {
    return str.replace(/[&<>"']/g, function (m) {
      return {
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#039;",
      }[m];
    });
  }

  // ==========================================================================
  // Hash-based Router
  // ==========================================================================
  function handleRoute() {
    const hash = window.location.hash.replace("#", "") || "home";

    // Close mobile nav on route change
    if (sideNav) sideNav.classList.remove("mobile-open");

    // Update active nav links
    document.querySelectorAll(".nav-item-link").forEach((link) => {
      const target = link.getAttribute("href").replace("#", "");
      link.classList.toggle("active", target === hash);
    });

    if (hash === "home") {
      if (homeView) homeView.style.display = "flex";
      if (articleView) articleView.style.display = "none";
      window.scrollTo({ top: 0, behavior: "smooth" });
      return;
    }

    // Hide home view, show article view
    if (homeView) homeView.style.display = "none";
    if (articleView) articleView.style.display = "flex";

    // Hide all article sections, show the matched article
    let found = false;
    document.querySelectorAll(".doc-article-content").forEach((section) => {
      const isTarget = section.getAttribute("data-doc-id") === hash;
      section.style.display = isTarget ? "block" : "none";
      if (isTarget) {
        found = true;
        const category = section.getAttribute("data-category") || "Documentation";
        const title = section.getAttribute("data-title") || "Article";

        if (breadcrumbCategory) breadcrumbCategory.innerText = category;
        if (breadcrumbCurrent) breadcrumbCurrent.innerText = title;
        document.title = `${title} — Peregrine C++ Framework`;
      }
    });

    if (!found && homeView) {
      // Fallback to home
      homeView.style.display = "flex";
      if (articleView) articleView.style.display = "none";
    }

    window.scrollTo({ top: 0, behavior: "smooth" });
  }

  window.addEventListener("hashchange", handleRoute);
  window.addEventListener("DOMContentLoaded", handleRoute);
})();
