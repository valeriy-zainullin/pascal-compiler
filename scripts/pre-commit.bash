#!/bin/bash

# https://stackoverflow.com/a/57791854
#   Works, provided you used command from README.md
#   that sets git hook directory.

repository_dir=$(dirname "$0")/..

git diff --cached --name-only | \
grep -P '\.cpp$|\.hpp$|\.cc$|\.hh$|\.h$|\.c$' | \
while IFS= read -r file || [[ -n "$file" ]]; do
    echo "Formatting $file"
    git add "$file"
    clang-format --style=file:"$repository_dir"/.clang-format "$file"
done
