cmake_minimum_required(VERSION 3.14)
project(si VERSION 0.1.0 LANGUAGES C CXX)

# Build the backend
add_subdirectory(backend)

# Note: Frontend is built separately via npm/electron-vite
# See frontend/README.md for build instructions
