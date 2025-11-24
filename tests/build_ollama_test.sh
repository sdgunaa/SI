#!/bin/bash
set -e


# Compiler settings
CXX=g++
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g"
INCLUDES="-I../include -I../src"

echo "Building Ollama Test..."


# Libraries
LIBS="-lfmt -lspdlog -ltomlplusplus -lpthread"

# Compile
$CXX $CXX_FLAGS $INCLUDES \
    ./src/foundation/logging.cpp \
    ./src/foundation/config.cpp \
    ./src/foundation/platform.cpp \
    ./src/ai/providers/ollama_provider.cpp \
    test_ollama.cpp \
    -o test_ollama \
    $LIBS

echo "Build complete! Running test..."
./test_ollama
