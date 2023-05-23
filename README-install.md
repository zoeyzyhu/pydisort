
## Requirements
- Python:
```
pip install --upgrade pip
pip install -r requirements.txt
```
- MacOS:
```
brew update
brew bundle
```
- Ubuntu Linux:
```
sudo apt update
sudo apt install clang-format
```
- Redhat Linux:
To install clang-format on Redhat takes a few more steps.
First, you need to add the appropriate repository that contains clang.
On Red Hat Enterprise Linux (RHEL), you need to enable the Extra Packages for Enterprise Linux (EPEL) repository,
as well as the LLVM repository.

1. Run the following script in the terminal to enable EPEL repository:
```
sudo yum install epel-release
```
For the LLVM repository, you would need to add it manually via:
```
sudo bash -c "$(curl -L https://apt.llvm.org/llvm.sh)"
```
This script will add the LLVM repo to your system.

1. After adding the repositories, you can install clang-format:
```
sudo yum install clang-format
```
