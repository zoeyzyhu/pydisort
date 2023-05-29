## How to make changes to the `cdisort` source code 📌

You may occasionally need to modify the cdisort source code. To ensure a clean version of the code is maintained, follow the procedure outlined below to update the cdisort source code.

### 1. Create a New Branch

Start by creating a new branch in the repository and name it using the format `<username>/<branch>`. This will serve as your dedicated branch for making changes.

### 2. Update the `cdisort` Code

Make the necessary modifications to the `cdisort` source code. You can use any suitable code editor or integrated development environment (IDE) to make these changes.

### 3. Commit Your Changes

Once you have finished updating the code, commit your changes to the branch. Provide a clear and concise commit message that describes the purpose of your modifications.

### 4. Pass the Tests

Before proceeding further, ensure that your changes pass the existing tests. Run the test suite to verify that the modified code functions as intended and does not introduce any regressions.

### 5. Submit a Pull Request and Wait for Approval

After successfully passing the tests, submit a pull request (PR) to merge your changes into the main repository. Provide a detailed description of your changes and any relevant information that might help the reviewers understand the purpose and impact of your modifications. Then, wait for the approval from the repository maintainers.

### 6. Obtain a Patch File

Once your pull request is approved, obtain a patch file containing your changes using `git diff`. Save the differences to a file on your local branch.

### 7. Switch to Branch `cdisort_patches`

Switch to the branch named `cdisort_patches`. This branch is specifically designated for storing patch files related to the cdisort source code.

### 8. Archive the Patch File to Branch `cdisort_patches`

Archive the patch file you obtained in step 6 by adding it to the `cdisort_patches` branch. Ensure that the patch file is placed in the appropriate directory structure, following any existing conventions.

### 9. Commit and Push

Commit your changes to the cdisort_patches branch. Provide a meaningful commit message that briefly describes the purpose of adding the patch file. After committing, push the changes to the remote repository.

### 10. Switch Back to Your Branch

Once you have completed archiving the patch file, switch back to your branch (`<username>/<branch>`) to continue your development work or start working on other changes.

By following these steps, you can make changes to the cdisort source code while maintaining a clean version of the codebase.
