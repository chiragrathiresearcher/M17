import os
import sys

sys.path.insert(0, os.path.abspath("../src/python"))

project = "M17 ChiragRathi Framework"
copyright = "2026, ChiragRathi"
author = "ChiragRathi"
release = "2.0.0"

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.napoleon",
    "sphinx.ext.viewcode",
    "myst_parser",
    "autoapi.extension",
]

autoapi_dirs = ["../src/python/m17"]
autoapi_type = "python"

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "sphinx_rtd_theme"

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

master_doc = "index"
