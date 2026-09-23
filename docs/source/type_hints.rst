Type Hints and IDE Support
===========================

The ``pydisort`` package includes type stub files (``.pyi``) that provide type hints for all public APIs. This enables:

- **IDE Autocomplete**: Modern IDEs like VS Code, PyCharm, and others can provide better autocomplete suggestions.
- **Type Checking**: Static type checkers like ``mypy`` can verify your code before runtime.
- **Better Documentation**: Type hints make the API more self-documenting.

Example with Type Hints
------------------------

.. testcode:: type-hints

    import torch
    from pydisort import DisortOptions, Disort

    torch.set_default_dtype(torch.float64)

    # IDEs will provide autocomplete for methods and their parameters
    op: DisortOptions = DisortOptions()
    op = op.flags("onlyfl,lamber")
    op = op.nwave(1)
    op.ds().nlyr = 4
    op.ds().nstr = 4
    op.ds().nmom = 4
    op.ds().nphase = 4

    # Type hints help catch errors before runtime
    ds: Disort = Disort(op)
    tau: torch.Tensor = torch.tensor([0.1, 0.2, 0.3, 0.4]).unsqueeze(-1)
    flux: torch.Tensor = ds.forward(tau, fbeam=torch.tensor([3.14159]))
    assert flux.shape == (1, 1, 5, 2)

Type hints check types, not physical validity or tensor shapes. Dimensions
must still be configured before constructing the solver.

Using Type Checkers
-------------------

You can use ``mypy`` to check your code for type errors:

.. code-block:: bash

    pip install mypy
    mypy your_script.py

The stub files follow `PEP 561 <https://www.python.org/dev/peps/pep-0561/>`_ standards, so type checkers will automatically find them when the package is installed.
