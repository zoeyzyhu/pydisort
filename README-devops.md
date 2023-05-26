# Developer's guide to this repo

## pre-commit hooks

This repo uses `pre-commit` hooks to ensure that code is formatted correctly and that tests pass before committing.
This `pre-commit` hook is defined in the `.pre-commit-config.yaml` file in the root directory of this repo.
To install the `pre-commit` hooks, run `pre-commit install` in the root directory.
This will install the `pre-commit` hooks in the local `.git` directory.
The `pre-commit` hooks will run automatically when you try to commit code.
If the `pre-commit` hooks fail, the commit will be aborted.
To run the `pre-commit` hooks manually, run `pre-commit run --all-files` in the root directory of this repo.

The following hooks are installed

- [clang-format](https://clang.llvm.org/docs/ClangFormatStyleOptions.html)
- [cmake-format](https://cmake-format.readthedocs.io/en/latest/index.html)
- [pre-commit](https://pre-commit.com/)
  requirements-txt-fixer, trailing-whitespace, end-of-file-fixer, check-yaml
- [black](https://github.com/psf/black)

# Reference articles

- https://www.the-analytics.club/python-code-formatting-git-pre-commit-hook
