How to contribute
=================

You may need to modify the pydisort source code. To ensure a clean version of the code is maintained, follow the procedure outlined below.

We adopt the idea of **linear history** and a **squash merging** approach in this repository, meaning there is only one permanent branch (`main`), and the only way to push changes to main is by submitting a Pull Request (PR). The main branch is protected to prevent direct pushes. **A linear history ensures that the main branch remains clean and organized**. Squash merging means that **the smallest unit of change is a PR, rather than a commit**.

This workflow differs from some individual workflows where the smallest unit is usually a commit. For collaborative projects, commits can be too fine-grained and don't track issues effectively. Our aim is to ensure that each stage in the history solves a problem that can be traced back, providing context for that problem. In other words, development is `issue-driven`. The git workflow recommended for this repository goes as follows:

#. **Create a New Branch**

   Start by creating a new branch in the repository, named using the format ``<username>/<branch>``. This will serve as your dedicated branch for making changes.

   Example:

   .. code-block:: bash

    git checkout -b <username/issue_description>

#. **Update the .gitignore File**

   The ``.gitignore`` file helps keep your working directory clean. Each folder can have its own ``.gitignore`` file, which lists files that should not be tracked by Git. For example, model output files should not be added to Git. Ideally, when you run ``git status``, there should be no untracked files in your working directory.

#. **Update the Source Code**

   Make the necessary modifications to the source code. You may use any suitable code editor or integrated development environment (IDE) to implement these changes.

#. **Commit Your Changes**

   After updating the code, commit your changes to the branch. Provide a clear and concise commit message that describes the purpose of your modifications. When you want to pause your work on the issue, add your changes to git using the command:

   Example:

    .. code-block:: bash

      git add .
      commit -m "<message>"

#. **Run and Pass the Tests**

   Before proceeding further, ensure that your changes pass all existing tests. Run the full test suite to verify that the modified code functions correctly and does not introduce any regressions.

   Example:

    .. code-block:: bash

      git push origin <username/issue_description>


#. **Submit a Pull Request (PR) and Await Approval**

   Once your changes have passed the tests, submit a pull request (PR) to merge your branch into the main repository. Provide a detailed description of your changes along with any relevant information that might assist the reviewers. Then, wait for approval from the repository maintainers.

#. **Update Your Local Branch**

   If the PR is approved, the maintainers will squash merge your changes into the main branch. After that, you can update your local branch to reflect the latest changes in the main repository. To do this, run the following commands:

   Example:

    .. code-block:: bash

      git checkout main
      git fetch origin main
      git rebase origin/main
      git branch -D <username/issue_description>

  where the `<username/issue_description>` refers to the branch you created in Step 2. This command will delete the local branch. You can then continue to start a new cycle and work on new issues.
