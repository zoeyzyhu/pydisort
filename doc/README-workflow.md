# How to use git for this project

## Git workflow

- submit an issue ticket on GitHub website
- create a new branch locally

```
git checkout -b <branch_name>
```

`branch_name` should begin with `username` followed by a slash `/` followed by the
nature of this branch `job`. For example, `cli/add_cpptest_case` is a good branch name
indicating that this branch is created by user `cli` and works on adding cpp test cases.

- work on this branch
- update the `.gitignore` file
  Each folder can have its own `.gitignore` file. This file keeps tracking the files that
  you don't wish to be added to the git system. For example, model output files should not
  be added to the git.
- add changed files to git system

```
git add .
```

This command adds all untracked files to git excluding the files listed in the
`.gitignore` file.

- commit your message

```
git commit -m <message>
```

The content of `message` is not important. All commits within a PR (pull request) will
be squashed into one message, which you can change later. You can either use a meaning
message like "work on XXX", "working XXX", or an unmeanding message like "wip", which
stands for "work in progress".

- upload your branch to GitHub

```
git push origin <branch_name>
```

At this point, GitHub shall have a remote branch that tracks your local branch. You
should be able to see branch by going to the git repo and look for "insights/network"

- submit a pull request (PR)
  This step is done on GitHub site. At this stage, only fill in the title of the PR to
  indicate what this branch is for. No content is needed.

From now on, all subsequent commits and pushes to `<branch_name>` will be staged in this
PR and when the PR is merged to the main branch. All commits in this PR will be
squashed. Then, you will write a meaningful title and contents documenting the changes,
use cases and notes of this PR.

## Quick tips

- undo a "git add"

```
git reset <file>
```

## Git Workflow 📌

We adopt the idea of **linear history** and a **squash merging** approach in this repository, meaning there is only one permanent branch (`main`), and the only way to push changes to main is by submitting a Pull Request (PR). The main branch is protected to prevent direct pushes. **A linear history ensures that the main branch remains clean and organized**. Squash merging means that **the smallest unit of change is a PR, rather than a commit**.

This workflow differs from some individual workflows where the smallest unit is usually a commit. For collaborative projects, commits can be too fine-grained and don't track issues effectively. Our aim is to ensure that each stage in the history solves a problem that can be traced back, providing context for that problem. In other words, development is **_issue-driven_**. The git workflow recommended for this repository goes as follows:

📍 Step 1. Submit an issue ticket on the GitHub website:

Before starting any work, create an issue ticket on GitHub to describe the problem or task you want to address.

- If you plan to solve the issue yourself, a brief title without extensive details is sufficient.
- If you want someone else to solve it, provide a more detailed explanation.

📍 Step 2. Create a new branch locally

Once you have identified an issue or task, create a new branch locally using the following command:

```bash
git checkout -b <branch_name>
```

The branch name should start with your username, followed by a slash `/` and then a brief description of the nature of the issue or task you're working on (for example, `zyhu/add_cpptest_case`). This naming convention helps keep the branch names consistent and makes it easier to identify the purpose of each branch, as well as who is working on it.

📍 Step 3. Work on the branch

Start working on the branch by creating a document file under the `doc/` folder. This document should begin with a paragraph explaining the problem you're addressing and your proposed solution. Remember to update the document as you make progress.

📍 Step 4. Update the .gitignore file
The `.gitignore` file helps keep your working directory clean. Each folder can have its own `.gitignore` file, which lists files that should not be tracked by the git system. For example, model output files should not be added to git. Ideally, when you run `git status`, there should be no untracked files in your working directory.

📍 Step 5. Add changed files to git
When you want to pause your work on the issue, add your changes to git using the command:

```bash
git add .
```

This command adds all the modified files to git, excluding the files listed in the `.gitignore` file.

📍 Step 6. Commit your changes locally

After adding the changed files, use `git status` to review the modifications. If you accidentally added files that you don't want to include, you can undo the add by `git reset <file>`. Then, commit your changes locally with a descriptive message:

```bash
git commit -m "<message>"
```

The content of the message is not crucial at this stage since all the commits within a PR will be squashed later into a single message that you'll write later. You can use a meaningful message like "Work on XXX" or "Working XXX," or a generic message like "WIP" (work in progress).

📍 Step 7. Upload your branch to GitHub

The previous command only commits the changes locally. To push your changes to GitHub, use:

```bash
git push origin <branch_name>
```

This command pushes your branch to the remote repository on GitHub. You should be able to see the remote branch by visiting the repository's page and looking for "insights/network."

📍 Step 8. Submit a pull request (PR)

This step is performed on the GitHub site. At this stage, only provide a title for the PR to indicate the purpose of the branch. No additional content is required.

All subsequent commits and pushes to <branch_name> will be included in this PR. When the PR is merged into the `main` branch, all the commits in the PR will be squashed. At that point, you can write a meaningful title and description that document the changes, use cases, and any additional notes related to the PR.

Following this workflow ensures a systematic approach to contributions, promotes collaboration, and maintains a clean and organized codebase.
