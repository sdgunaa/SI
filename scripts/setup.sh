#!/bin/bash
# SI Development Setup Script
set -e

echo "🔧 SI Development Setup"
echo "========================"

# Check required tools
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "❌ $1 is required but not installed"
        return 1
    fi
    echo "✓ $1 found"
}

echo ""
echo "Checking dependencies..."
check_tool cmake || exit 1
check_tool npm || exit 1
check_tool g++ || echo "⚠️  g++ not found, trying clang..."

# Check for required libraries
echo ""
echo "Note: The following system packages are required:"
echo "  - libspdlog-dev"
echo "  - libfmt-dev"
echo "  - nlohmann-json3-dev"
echo "  - libboost-filesystem-dev"
echo "  - libsqlite3-dev"
echo "  - libssl-dev"
echo "  - catch2"

# Build backend
echo ""
echo "📦 Building backend..."
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Setup frontend
echo ""
echo "📦 Setting up frontend..."
cd frontend
npm install
cd ..

echo ""
echo "✅ Setup complete!"
echo ""
echo "To run SI:"
echo "  1. Start backend:  ./build/bin/sicore --server"
echo "  2. Start frontend: cd frontend && npm run dev:electron"
