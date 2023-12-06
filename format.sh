#!/bin/bash

files=$(echo "$(dirname $0)"/*.cpp "$(dirname $0)"/*.hpp "$(dirname $0)"/*.cc "$(dirname $0)"/*.hh "$(dirname $0)"/*.c "$(dirname $0)"/*.h)
echo $files
clang-format --style=LLVM -i $files
