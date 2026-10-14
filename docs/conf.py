"""Build against the installed native package: uv run sphinx-build docs ..."""
import pybsp

project = "pybsp"
author = "BSP contributors"
release = pybsp.__version__
extensions = ["myst_parser", "sphinx.ext.autodoc", "sphinx.ext.napoleon", "sphinx.ext.viewcode", "sphinx.ext.mathjax"]
source_suffix = {".rst": "restructuredtext", ".md": "markdown"}
exclude_patterns = ["_build", "README.md", ".DS_Store"]
html_theme = "pydata_sphinx_theme"
html_title = f"pybsp {release}"
html_static_path = ["_static"]
html_css_files = ["api.css"]
html_theme_options = {"navigation_depth": 2, "show_toc_level": 2}
autodoc_typehints = "none"
autodoc_member_order = "bysource"
myst_enable_extensions = ["colon_fence", "dollarmath"]
myst_heading_anchors = 3


def install_plotly_assets(app):
    """Stage Plotly's local runtime so embedded figures also work offline."""
    if app.builder.format == "html":
        from pathlib import Path
        from plotly.offline import get_plotlyjs
        destination = Path(app.outdir) / "_static" / "plotly.min.js"
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(get_plotlyjs())


def setup(app):
    app.connect("builder-inited", install_plotly_assets)
