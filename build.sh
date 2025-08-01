#!/bin/bash

# 获取 build.sh 所在目录（项目根目录）
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || { echo "Error: Failed to change to project root directory"; exit 1; }

# 清理编译结果
if [ "$1" == "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build
    exit 0
fi

# 默认构建类型为 Debug
BUILD_TYPE=${1:-Debug}

# 编译工程
echo "Building project in $BUILD_TYPE mode..."
mkdir -p build
cd build

# 清理现有构建，确保重新编译
rm -rf *

# 生成 CMake 构建系统（使用 Unix Makefiles）
echo "Generating CMake build system with Unix Makefiles..."
cmake -S .. -B . -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_TOOLCHAIN_FILE=../cmake/gcc-arm-none-eabi.cmake
if [ $? -ne 0 ]; then
    echo "CMake configuration failed! Please check if ../cmake/gcc-arm-none-eabi.cmake exists."
    exit 1
fi

# 将 compile_commands.json 复制到项目根目录（便于 clangd 查找）
cp compile_commands.json ..

# 编译项目
echo "Building project..."
cmake --build . --config $BUILD_TYPE
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi


# 使用 compiledb 生成 compile_commands.json
echo "Generating compile_commands.json with compiledb..."
compiledb -n make
if [ $? -ne 0 ]; then
    echo "Error: compiledb failed to generate compile_commands.json!"
    exit 1
fi

# 检查是否生成 compile_commands.json
if [ ! -f compile_commands.json ]; then
    echo "Error: compile_commands.json was not generated!"
    exit 1
fi

# 额外生成 .bin 文件
echo "Generating .bin file..."
/Applications/ArmGNUToolchain/14.2.rel1/arm-none-eabi/bin/arm-none-eabi-objcopy -O binary MY_FREE_RTOS.elf MY_FREE_RTOS.bin
if [ $? -ne 0 ]; then
    echo "Failed to generate .bin file!"
    exit 1
fi

# 清理构建
# ./build.sh clean

# 不同构建模式
# ./build.sh Debug
# ./build.sh Release
# ./build.sh RelWithDebInfo
# ./build.sh MinSizeRel

# 默认Debug模式
# ./build.sh