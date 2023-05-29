## Developer's guide to this repo 📌

### pre-commit hooks

This repo uses `pre-commit` hooks to ensure that code is formatted correctly and that tests pass before committing.
This `pre-commit` hook is defined in the `.pre-commit-config.yaml` file in the root directory of this repo.
To install the `pre-commit` hooks, run `pre-commit install` in the root directory.
This will install the `pre-commit` hooks in the local `.git` directory.
The `pre-commit` hooks will run automatically when you try to commit code.
If the `pre-commit` hooks fail, the commit will be aborted.
To run the `pre-commit` hooks manually, run `pre-commit run --all-files` in the root directory of this repo.

### CI/CD

Placeholder.

### cmake

Placeholder.

### Reference articles

- https://www.the-analytics.club/python-code-formatting-git-pre-commit-hook
