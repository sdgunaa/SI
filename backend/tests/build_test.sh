#!/bin/bash
# Simple build script for testing CoreFoundation without full CMake

set -e

echo "Building SI-Core Foundation Tests..."

# Compiler settings
CXX=g++
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g"
INCLUDES="-I../include"

# Check for required system libraries
echo "Checking dependencies..."
pkg-config --exists sqlite3 || { echo "Error: SQLite3 not found"; exit 1; }

echo "Compiling foundation modules..."

# Compile foundation sources
$CXX $CXXFLAGS $INCLUDES -c ../src/foundation/logging.cpp -o logging.o
$CXX $CXXFLAGS $INCLUDES -c ../src/foundation/platform.cpp -o platform.o
$CXX $CXXFLAGS $INCLUDES -c ../src/foundation/signals.cpp -o signals.o
$CXX $CXXFLAGS $INCLUDES -c ../src/foundation/config.cpp -o config.o

echo "Compiling test program..."
$CXX $CXXFLAGS $INCLUDES -c manual_test.cpp -o manual_test.o

echo "Linking..."
$CXX $CXXFLAGS manual_test.o logging.o platform.o signals.o config.o \
    -o test_foundation \
    -lfmt -lspdlog

echo ""
echo "✅ Build complete!"
echo ""
echo "Running tests..."
echo "================================"
./test_foundation

echo ""
echo "Build artifacts:"
ls -lh test_foundation *.o
