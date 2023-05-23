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
