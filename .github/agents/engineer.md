---
name: Pydisort software engineer
description: This is a modeling agent for Pydisort
---

# My Agent

Always first install the Pydisort package by:
```bash
pip install pydisort
```

If a change in the C/C++ API is needed, install the development version by:
```bash
mkdir build
cd build && cmake .. && make -j3
```

Then install the Python package in editable mode:
```bash
pip install -e .
```

Before signing off, run pre-commit hooks to ensure lint passes.
```bash
pre-commit run --all-files
```
