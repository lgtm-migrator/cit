#!/bin/sh

set -e

./ci/checks/yaml_files_style.sh
./ci/checks/md_files_style.sh
./ci/checks/spelling.sh
./ci/checks/git_symbols.sh
./ci/checks/conventional_git_style.sh
./ci/checks/directories_and_files_naming.sh
echo "All checks are passed."
