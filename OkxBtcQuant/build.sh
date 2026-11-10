#!/bin/sh
# Linux / macOS 构建脚本（需要 cmake、g++、libssl-dev）
set -e
cd "$(dirname "$0")"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
echo "构建完成: build/okxquant"
