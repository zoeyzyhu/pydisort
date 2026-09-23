"""Checks for the documentation-only stub renderer (no solver required)."""

import ast
import unittest

from sphinx.ext.napoleon import Config

from stub_api import render_members


class StubApiTests(unittest.TestCase):
    def render(self, source):
        return "\n".join(render_members(ast.parse(source).body, Config()))

    def test_classes_attributes_and_overloads(self):
        rendered = self.render(
            '''
class Options:
    """Configure a solve."""
    count: int
    """Number of columns."""
    def __init__(self) -> None: ...
    @overload
    def waves(self) -> int:
        """Get the count."""
        ...
    @overload
    def waves(self, count: int) -> Options:
        """Set the count."""
        ...
    def __repr__(self) -> str: ...
'''
        )
        self.assertIn(".. py:class:: Options", rendered)
        self.assertIn("   .. py:attribute:: count", rendered)
        self.assertIn("Number of columns.", rendered)
        self.assertIn(".. py:method:: __init__() -> None", rendered)
        self.assertEqual(rendered.count(".. py:method:: waves"), 1)
        self.assertIn("waves(count: int) -> Options", rendered)
        self.assertIn("Get the count.", rendered)
        self.assertIn("Set the count.", rendered)
        self.assertNotIn("__repr__", rendered)

    def test_google_parameters_and_return_descriptions(self):
        rendered = self.render(
            '''
def solve(value: float = 1.0) -> float:
    """Run the solver.

    Args:
        value (float): Incident flux.

    Returns:
        float: Outgoing flux.
    """
    ...
'''
        )
        self.assertIn(".. py:function:: solve(value: float=1.0)", rendered)
        self.assertIn(":param value: Incident flux.", rendered)
        self.assertIn(":returns: Outgoing flux.", rendered)

    def test_imports_and_module_constants_are_not_executed(self):
        rendered = self.render(
            """
import nonexistent_package
raise RuntimeError("The stub must never be executed")
kIUP: int
"""
        )
        self.assertIn(".. py:data:: kIUP", rendered)
        self.assertNotIn("nonexistent_package", rendered)


if __name__ == "__main__":
    unittest.main()
