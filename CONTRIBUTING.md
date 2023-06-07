<h4 align="center">
    <img src="doc/img/team.jpg" alt="Pydisort"  style="display: block; margin: 0 auto">
</h4>

We welcome contributions to this repository! If you would like to contribute, please follow the guidelines outlined below to ensure a smooth workflow and maintain code quality. This repository is configured to run automated tests, including pre-commit hooks and language lints such as `cpplint`.

- Pre-commit hooks are triggered when you perform a git commit. The configuration file can be found at `.pre-commit-config.yaml`. These hooks apply rules to automatically format your code, making it more organized and tidy. The hooks will make the necessary changes, and you will need to add the modified files again using `git add` . Typically, you don't need to review the changes made by the hooks.

- Additionally, language lints check your code for style issues and provide suggestions. It's recommended to fix your code according to the suggestions to ensure it passes the lints.

> ❗ Code that doesn't pass the lints won't be able to merge into the `main` branch. Please refer to the [naming conventions](#naming-conventions) at the end of the document, which will let you pass lints easier and help you write better and readable codes.

## Table of Contents

- [Git Philosophy](#git-philosophy)
- [Git Workflow](#git-workflow)
  - [Step 1. Submit an issue on the GitHub website](#step1)
  - [Step 2. Fork and clone the repository](#step2)
  - [Step 3. Update the .gitignore file](#step3)
  - [Step 4. Add changed files to git](#step4)
  - [Step 5. Commit your changes locally](#step5)
  - [Step 6. Upload your branch to GitHub](#step6)
  - [Step 7. Submit a pull request (PR)](#step7)
  - [Step 8. Squash and merge](#step8)
  - [Step 9. Update your local branch](#step9)
- [Testing](#testing)
- [Naming Conventions](#naming-conventions)
  - [Folder and file naming](#folder-and-file-naming)
  - [Variable and class naming](#variable-and-class-naming)
  - [Functions naming](#functions-naming)
  - [Converting from C to C++](#converting-from-c-to-c)

## Git Philosophy

We adopt the idea of **linear history** and a **squash merging** approach in this repository, meaning there is only one permanent branch (`main`), and the only way to push changes to main is by submitting a Pull Request (PR). The main branch is protected to prevent direct pushes. **A linear history ensures that the main branch remains clean and organized**. Squash merging means that **the smallest unit of change is a PR, rather than a commit**.

This workflow differs from some individual workflows where the smallest unit is usually a commit. For collaborative projects, commits can be too fine-grained and don't track issues effectively. Our aim is to ensure that each stage in the history solves a problem that can be traced back, providing context for that problem. In other words, development is <span style="color:red">issue-driven<span>.

## Git Workflow

### <a id='step1'>📍 Step 1. Submit an issue on the GitHub website:</a>

Before starting any work, create an issue ticket on GitHub to describe the problem or task you want to address.

- If you plan to solve the issue yourself, a brief title without extensive details is sufficient. Please also indicate that you would like to work on it.
- If you want someone else to solve it, provide a more detailed explanation.

We will review the issue and provide feedback such as compatibility with the project or if someone is already working on it. If the issue is approved, we will assign it to you by asking you to create a pull request to this repository, so that the issue will be automatically linked to the pull request. Through this design, we ensure that the development is issue-driven.

### <a id='step2'>📍 Step 2. Fork and clone the repository </a>

Before contributing to the repository, please ensure you have the necessary dependencies installed. Also, please refer to the [README.md](../README.md) file for instructions on setting up the build environment.

Fork the repository to your own GitHub account. Clone the **forked repository** to your local machine.

```bash
git clone https://github.com/your-username/repository-name.git
```

Replace your-username with your GitHub username and repository-name with the name of the repository.

### <a id='step3'>📍 Step 3. Update the .gitignore file </a>

The `.gitignore` file helps keep your working directory clean. Each folder can have its own `.gitignore` file, which lists files that should not be tracked by the git system. For example, model output files should not be added to git. Ideally, when you run `git status`, there should be no untracked files in your working directory.

### <a id='step4'>📍 Step 4. Add changed files to git </a>

When you want to pause your work on the issue, add your changes to git using the command:

```bash
git add .
```

This command adds all the modified files to git, excluding the files listed in the `.gitignore` file.

### <a id='step5'>📍 Step 5. Commit your changes locally </a>

After adding the changed files, use `git status` to review the modifications. If you accidentally added files that you don't want to include, you can undo the add by `git reset <file>`. Then, commit your changes locally with a descriptive message:

```bash
git commit -m "<message>"
```

The content of the message is not crucial at this stage since all the commits within a PR will be squashed later into a single message that you'll write later. You can use a meaningful message like "Work on XXX" or "Working XXX," or a generic message like "WIP" (work in progress).

### <a id='step6'>📍 Step 6. Upload your branch to GitHub </a>

The previous command only commits the changes locally. To push your changes to GitHub, use:

```bash
git push origin <branch_name>
```

This command pushes your branch to the remote repository on GitHub. You should be able to see the commit message and if it has passed the building tests under the PR page associated with the issue in this repository.

### <a id='step7'>📍 Step 7. Submit a pull request (PR) </a>

This step is performed on the GitHub site. At this stage, only provide a title for the PR to indicate the purpose of the branch. No additional content is required.

All subsequent commits and pushes to `main` will be included in this PR. When the PR is merged into the `main` branch, all the commits in the PR will be squashed. At that point, you can write a meaningful title and description that document the changes, use cases, and any additional notes related to the PR.

### <a id='step8'>📍 Step 8. Squash and Merge </a>

After the last step, you can continue working on the issue. When you are ready to merge the PR, please ensure that all the changes are committed and pushed to GitHub. Before merging, please also ensure that the PR has passed all the building tests. If the PR has conflicts with the `main` branch, please resolve the conflicts locally and push the changes to GitHub. Then, you can merge the PR on GitHub.

Then, you can merge the PR by requesting reviews. Add the GitHub username of the person who will review your PR. We will review your changes and provide feedback. If requested changes are necessary, make the required updates and push the changes to your branch. The pull request will be updated automatically. If no changes are required, squash and merge the PR on this repository. This will squash all the commits in the PR into a single commit with a message that you can write. Please ensure that the commit message is meaningful and descriptive.

### <a id='step9'>📍 Step 9. Update your local branch </a>

You may want to update your branch with the latest changes from the `main` branch of this repository to facilitate future work. Switch to your local `main` branch, fetch the latest changes from this remote repository, and rebase them into your local `main` branch.

#

Following this workflow ensures a systematic approach to contributions, promotes collaboration, and maintains a clean and organized codebase.

For more information and resources to assist your development, please refer to the `doc/` folder in this repository. It contains relevant documentation and guides to help you understand the codebase and contribute effectively.

<div align="right"><a href="#table-of-contents"><img src="doc/img/top_green.png" width="32px"></a></div>

## Testing

Ensure that your changes do not break any existing functionality and include appropriate test cases in the corresponding folder under the `tests/` directory for new features or bug fixes. Run the existing tests to verify the integrity of the code.

## Naming conventions

### Folder and file naming

- Use a one-word noun for **folder** names, avoid compound nouns. For example, `src`.
- You can use compound nouns or phrases for **file** names, connecting with underscores (_snake case_). For example, `file_name.cpp`.

### Variable and class naming

- Use low case letters for **variables**. You can use the snake case on compound nouns or phrases. For example, `variable_name`.
- Use upper case letters for **classes**. You can capitalize each word (_upper camel case_) on compound nouns or phrases. For example, `ClassName`.
- If a variable is a **private member** of a class, append the variable name with an underscore. For example, `private_variable_`.

### Functions naming

- **Function** names are usually **_verbal_** phrases.
- C++ function naming:
  - For public functions, use upper camel case. For example, `PublicFunction()`.
  - For private functions, use lower camel case. For example, `privateFunction()`.
- Python function naming: use snake case. For example, `public_function()`.

### Converting from C to C++

- Use full path name starting from the `src` folder for include guard. For example, `#ifndef CDISORT_SRC_FILE_NAME_H_`.
- Use `snprintf` instead of `sprintf`.
- Use `rand_r` instead of `rand`.
- Use `strtok_r` instead of `strtok`.

<div align="right"><a href="#table-of-contents"><img src="doc/img/top_green.png" width="32px"></a></div>
