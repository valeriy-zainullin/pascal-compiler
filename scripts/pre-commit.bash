#!/bin/bash

# https://stackoverflow.com/a/57791854
#   Works, provided you used command from README.md
#   that sets git hook directory.

set -e

repository_dir=$(dirname "$0")/../..
cd "$repository_dir"

git diff --cached --name-only | \
grep -P '\.cpp$|\.hpp$|\.cc$|\.hh$|\.h$|\.c$' | \
while IFS= read -r file || [[ -n "$file" ]]; do
    echo "Formatting $file"
    clang-format -i --style=file:"$repository_dir"/.clang-format "$file"
    git add "$file"
done
