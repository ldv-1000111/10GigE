import os
import sys

project   = 'SHR 10GigE Camera App on Linux'
copyright = '2026, Allied Vision / Vimba X Tutorial'
author    = 'Tutorial Guide'
release   = '1.0'

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.todo',
]

templates_path   = ['_templates']
exclude_patterns = []

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_css_files   = ['custom.css']

html_theme_options = {
    'navigation_depth': 4,
    'titles_only': False,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'includehidden': True,
    'logo_only': False,
    'display_version': True,
    'prev_next_buttons_location': 'both',
    'style_external_links': True,
}

pygments_style = 'monokai'
todo_include_todos = True

highlight_language = 'cpp'
