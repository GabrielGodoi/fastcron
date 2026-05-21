# -- FastCron Sphinx Configuration ------------------------------------------

project   = "FastCron"
copyright = "2026, FastCron Contributors"
author    = "FastCron Contributors"
release   = "1.0.0"

# -- Extensions -------------------------------------------------------------
extensions = [
    "breathe",
]

# -- Breathe configuration --------------------------------------------------
breathe_projects         = {"FastCron": "_doxygen/xml"}
breathe_default_project  = "FastCron"
breathe_default_members  = ("members", "undoc-members")

# -- Theme -------------------------------------------------------------------
html_theme       = "sphinx_rtd_theme"
html_theme_options = {
    "navigation_depth": 4,
    "collapse_navigation": False,
    "style_nav_header_background": "#1a1a2e",
}

# -- General -----------------------------------------------------------------
templates_path   = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]
