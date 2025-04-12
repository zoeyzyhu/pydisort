#!/usr/bin/env python3

import re
import subprocess
import sys
from pathlib import Path

VERSION_PATTERN = r"^(\d+)\.(\d+)\.(\d+)$"
VERSION_FILE = Path("python/VERSION")
PY_INIT_FILE = Path("python/__init__.py")
CPP_HEADER_FILE = Path("src/version.h")
SETUP_CFG_FILE = Path("setup.cfg")
CHANGELOG_FILE = Path("CHANGELOG.md")


def bump_version(version: str, part: str) -> str:
    match = re.match(VERSION_PATTERN, version)
    if not match:
        raise ValueError(f"Invalid version string: {version}")

    major, minor, patch = map(int, match.groups())
    if part == "major":
        return f"{major + 1}.0.0"
    elif part == "minor":
        return f"{major}.{minor + 1}.0"
    elif part == "patch":
        return f"{major}.{minor}.{patch + 1}"
    else:
        raise ValueError("Usage: bump_version.py [major|minor|patch]")


def update_file(path: Path, pattern: str, new_version: str, quote='"'):
    text = path.read_text()
    new_text = re.sub(
        pattern, lambda m: f"{m.group(1)}{quote}{new_version}{quote}", text
    )
    path.write_text(new_text)


def update_setup_cfg(path: Path, new_version: str):
    pattern = r"^(version\s*=\s*)(\d+\.\d+\.\d+)$"
    text = path.read_text()
    new_text = re.sub(
        pattern,
        lambda m: f"{m.group(1)}{new_version}",
        text,
        flags=re.MULTILINE,
    )
    path.write_text(new_text)


def generate_changelog(new_version: str):
    print("📝 Generating changelog...")
    subprocess.run(
        [
            "git-changelog",
            "--output",
            str(CHANGELOG_FILE),
        ],
        check=True,
    )
    print("✅ CHANGELOG.md updated.")


def git_commit_and_tag(new_version: str):
    subprocess.run(
        [
            "git",
            "add",
            str(VERSION_FILE),
            str(PY_INIT_FILE),
            str(CPP_HEADER_FILE),
            str(SETUP_CFG_FILE),
            str(CHANGELOG_FILE),
        ],
        check=True,
    )

    subprocess.run(
        ["git", "commit", "-m", f"chore: bump version to {new_version}"],
        check=True,
    )
    subprocess.run(["git", "tag", f"v{new_version}"], check=True)
    subprocess.run(
        ["git", "push", "origin", "main", "--follow-tags"], check=True
    )
    print(f"🚀 Committed and pushed changes with tag v{new_version}")


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in {"major", "minor", "patch"}:
        print("Usage: bump_version.py [major|minor|patch]")
        sys.exit(1)

    bump_type = sys.argv[1]

    current_version = VERSION_FILE.read_text().strip()
    new_version = bump_version(current_version, bump_type)

    print(f"🔁 Bumping version: {current_version} → {new_version}")

    VERSION_FILE.write_text(new_version + "\n")
    update_file(PY_INIT_FILE, r'(__version__\s*=\s*)".*?"', new_version)
    update_file(
        CPP_HEADER_FILE, r'(#define PYDISORT_VERSION\s+)".*?"', new_version
    )
    update_setup_cfg(SETUP_CFG_FILE, new_version)

    generate_changelog(new_version)
    git_commit_and_tag(new_version)

    print("🎉 Version bump, changelog, commit, and push complete.")


if __name__ == "__main__":
    main()
