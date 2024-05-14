#!/bin/bash

# https://stackoverflow.com/a/57791854
# Works, provided you used command from README.md
#   that sets git hook directory.

files=$(echo "$(dirname $0)"/*.cpp "$(dirname $0)"/*.hpp "$(dirname $0)"/*.cc "$(dirname $0)"/*.hh "$(dirname $0)"/*.c "$(dirname $0)"/*.h)
echo $files
clang-format --style=LLVM -i $(git diff --cached --name-only)
