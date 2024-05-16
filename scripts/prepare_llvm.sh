#!/bin/bash

set -e

echo -e "\e[31mОперация удалит сборку, которая была раньше (опять ждать 30+ минут, пока llvm собирается). \e[39m"
read -p "Вы уверены? (д/н) " -r
if ! [[ $REPLY =~ ^[ДдYy]$ ]]
then
    exit 0
fi


cd $(dirname $(readlink -e $0))

if [ ! -d llvm-project ]; then
  git clone https://github.com/llvm/llvm-project.git
fi

cd llvm-project
git checkout llvmorg-18.1.0-rc3 # Тут последняя версия (18.1.0-rc3), потому что на последнем релизе llvmorg-17.0.6 у меня не получилось aarch64 добавить в сборку.

# LLVM_BUILD_LLVM_DYLIB включаем, чтобы не собирать статически, иначе линковщик
#   занимает прорву оперативной памяти. Он в каждый исполняемый файл записывает
#   все библиотеки, которые очень много занимают.

# LLVM_PARALLEL_LINK_JOBS=1, т.к. некоторые линковки могут занимать 6 гигабайт
#   оперативки. Если у вас достаточно оперативной памяти, поменяйте это
#   значение.
# На линковку SampleAnalyzerPlugin (по-моему, так называлось), потребовалось 6.6 гигабайт.
# На линковку clang-18 потребовалось 12 гигабайт.
# На несколько подобных линковок может не хватить памяти.
# clang-scan-deps - >=12.1 гигабайт.
# Тут может быть ошибка, что ninja показывает не текущую задачу, а уже завершенную.
#   Не знаю до конца, как работает его отображение, что он показывает в каждый
#   момент времени.

# -DLLVM_INCLUDE_TESTS=OFF, чтобы собиралось быстрее.
# Источник: https://stackoverflow.com/questions/65609569/how-build-clang-tools-make-clang-tools-does-nothing#comment116031006_65612834

# rm -rf build
cmake -S llvm -G Ninja -B build         \
  -DLLVM_ENABLE_PROJECTS=''             \
  -DLLVM_TARGETS_TO_BUILD='X86;AArch64' \
  -DCMAKE_BUILD_TYPE=Debug              \
  -DLLVM_BUILD_LLVM_DYLIB=ON            \
  -DLLVM_PARALLEL_LINK_JOBS=1           \
  -DLLVM_BUILD_TOOLS=OFF                \
  -DLLVM_INCLUDE_TESTS=OFF              \
  -DLLVM_DYLIB_COMPONENTS=all .
echo -e "\e[36mВ директории llvm-project есть появилась поддиректория build, которая готова к сборке командой ninja all clang\e[39m"
