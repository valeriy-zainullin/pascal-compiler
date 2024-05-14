#!/bin/bash

# https://stackoverflow.com/a/57791854
#   Works, provided you used command from README.md
#   that sets git hook directory.

exit 0

repository_dir=$(dirname "$0")/..

git diff --cached --name-only | \
grep -P '.cpp$|.hpp$|.cc$|.hh$|.h$|.c$' | \
IFS= while read -r file; do
    echo "$file"
    clang-format --style=file:"$repository_dir"/.clang-format "$file"
done
