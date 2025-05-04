# Configuration file for the Sphinx documentation builder.

from docutils import nodes
from docutils.parsers.rst import roles
import sys
import os


def greyed_out_role(
    name, rawtext, text, lineno, inliner, options={}, content=[]
):
    node = nodes.inline(rawtext, text, classes=["greyed-out"])
    return [node], []


roles.register_local_role("grey", greyed_out_role)

# -- Project information

project = "pydisort"
copyright = "2025, Zoey Hu"
author = "Zoey Hu"

autosummary_generate = True

# Don't show package name
add_module_names = False

# only show the class name
autodoc_typehints = "both"

# Don't show the function parentheses
add_function_parentheses = False

# Adjust the path accordingly
sys.path.insert(0, os.path.abspath("../python"))

# -- General configuration

extensions = [
    "sphinx.ext.duration",
    "sphinx.ext.doctest",
    "sphinx.ext.autodoc",
    "sphinx.ext.napoleon",
    "sphinx.ext.autosummary",
    "sphinx.ext.viewcode",
    "sphinx.ext.githubpages",
    "sphinx.ext.intersphinx",
    "sphinx.ext.autodoc.typehints",
]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3/", None),
    "sphinx": ("https://www.sphinx-doc.org/en/master/", None),
    "torch": ("https://pytorch.org/docs/stable/", None),
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
