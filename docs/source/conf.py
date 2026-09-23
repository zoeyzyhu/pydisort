# Configuration file for the Sphinx documentation builder.

from docutils import nodes
from docutils.parsers.rst import roles
from datetime import date
import sys
from pathlib import Path


def greyed_out_role(
    name, rawtext, text, lineno, inliner, options={}, content=[]
):
    node = nodes.inline(rawtext, text, classes=["greyed-out"])
    return [node], []


roles.register_local_role("grey", greyed_out_role)

# -- Project information

project = "pydisort"
copyright = f"2025–{date.today().year}, Zoey Hu"
author = "Zoey Hu"

# Don't show package name
add_module_names = False

# Don't show the function parentheses
add_function_parentheses = False

# Read API descriptions from this checkout, not an installed PyPI binary.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "_ext"))

# -- General configuration

extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.napoleon",
    "stub_api",
    "sphinx.ext.githubpages",
    "sphinx.ext.intersphinx",
]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "sphinx": ("https://www.sphinx-doc.org/en/master/", None),
}
intersphinx_disabled_domains = ["std"]

latex_engine = "xelatex"

latex_elements = {
    "preamble": r"""
    \usepackage{fontspec}
    \usepackage{svg}
    \setmainfont{Arial Unicode MS}
    """
}

templates_path = ["_templates"]
exclude_patterns = ["_snippets/**"]
# Only explicitly marked executable examples are part of the doctest suite.
doctest_test_doctest_blocks = ""

# -- Options for HTML output -------------------------------------------------

html_theme = "sphinx_rtd_theme"

# -- Options for EPUB output
epub_show_urls = "footnote"

# -- Custom options
html_static_path = ["../_static"]

html_css_files = [
    "custom.css",
]

# -- napoleon options
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = False
napoleon_include_private_with_doc = False
napoleon_use_param = True
napoleon_use_rtype = True
