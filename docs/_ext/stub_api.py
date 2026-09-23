"""Render the checked-out public type stub without importing the extension."""

import ast
from pathlib import Path

from sphinx.ext.napoleon.docstring import GoogleDocstring


def render_members(body, config, nested=False):
    """Render public classes, overloads, functions and annotated attributes."""
    lines = []
    seen = set()
    for index, node in enumerate(body):
        name = getattr(node, "name", None)
        if name and name.startswith("_") and name != "__init__":
            continue
        if isinstance(node, ast.ClassDef):
            lines.extend([f".. py:class:: {name}", ""])
            content = str(
                GoogleDocstring(ast.get_docstring(node) or "", config)
            ).splitlines()
            content += [""] + render_members(node.body, config, nested=True)
        elif isinstance(node, ast.FunctionDef):
            if name in seen:
                continue
            seen.add(name)
            overloads = [
                item
                for item in body
                if isinstance(item, ast.FunctionDef) and item.name == name
            ]
            kind = "method" if nested else "function"
            for number, item in enumerate(overloads):
                args = ast.unparse(item.args)
                if nested:
                    args = args.removeprefix("self, ")
                    if args == "self":
                        args = ""
                returns = (
                    f" -> {ast.unparse(item.returns)}" if item.returns else ""
                )
                prefix = (
                    f".. py:{kind}:: "
                    if number == 0
                    else " " * (len(kind) + 9)
                )
                lines.append(f"{prefix}{name}({args}){returns}")
            lines.append("")
            content = []
            for item in overloads:
                content += str(
                    GoogleDocstring(ast.get_docstring(item) or "", config)
                ).splitlines()
                content.append("")
        elif isinstance(node, ast.AnnAssign) and isinstance(
            node.target, ast.Name
        ):
            kind = "attribute" if nested else "data"
            lines.extend(
                [
                    f".. py:{kind}:: {node.target.id}",
                    f"   :type: {ast.unparse(node.annotation)}",
                    "",
                ]
            )
            following = body[index + 1] if index + 1 < len(body) else None
            content = []
            if (
                isinstance(following, ast.Expr)
                and isinstance(following.value, ast.Constant)
                and isinstance(following.value.value, str)
            ):
                content = following.value.value.splitlines()
        else:
            continue
        lines.extend("   " + line if line else "" for line in content)
        lines.append("")
    return lines


def render_api(app, docname, source):
    if docname != "api":
        return
    stub = Path(app.confdir).parents[1] / "python" / "pydisort.pyi"
    app.env.note_dependency(str(stub))
    tree = ast.parse(stub.read_text(encoding="utf-8"))
    source[0] += "\n.. py:module:: pydisort\n\n"
    source[0] += "\n".join(render_members(tree.body, app.config))


def setup(app):
    app.connect("source-read", render_api)
    return {"version": "1", "parallel_read_safe": True}
