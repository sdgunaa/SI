cmake_minimum_required(VERSION 3.14)
project(si_core VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(SI_BUILD_TESTS "Build tests" ON)

option(SI_ENABLE_OLLAMA "Enable Ollama support" ON)
option(SI_ENABLE_OPENAI "Enable OpenAI support" ON)

# Dependencies
include(FetchContent)

# spdlog
find_package(spdlog REQUIRED)

# fmt
find_package(fmt REQUIRED)

# nlohmann_json
find_package(nlohmann_json REQUIRED)

# toml++
find_package(tomlplusplus REQUIRED)

# Boost (Filesystem, System)
find_package(Boost REQUIRED COMPONENTS filesystem system)

# SQLite3
find_package(SQLite3 REQUIRED)

# cpp-httplib (header only)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG master
)

FetchContent_MakeAvailable(cpp-httplib)



# Core Foundation Library
add_library(core_foundation
    src/foundation/config.cpp
    src/foundation/logging.cpp
    src/foundation/platform.cpp
    src/foundation/signals.cpp
)

target_include_directories(core_foundation PUBLIC include)
target_link_libraries(core_foundation PUBLIC 
    fmt::fmt 
    spdlog::spdlog 
    tomlplusplus::tomlplusplus
    Boost::filesystem
    Boost::system
)

# AI Gateway Library
add_library(ai_gateway
    src/ai/gateway.cpp
    src/ai/context_builder.cpp
)

target_include_directories(ai_gateway PUBLIC include)
target_link_libraries(ai_gateway PUBLIC 
    core_foundation
    nlohmann_json::nlohmann_json
    httplib::httplib
)



# Ollama Provider
if(SI_ENABLE_OLLAMA)
    target_sources(ai_gateway PRIVATE src/ai/providers/ollama_provider.cpp)
    target_compile_definitions(ai_gateway PRIVATE SI_ENABLE_OLLAMA)
endif()

# OpenAI Provider
if(SI_ENABLE_OPENAI)
    find_package(OpenSSL REQUIRED)
    target_sources(ai_gateway PRIVATE src/ai/providers/openai_provider.cpp)
    target_link_libraries(ai_gateway PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(ai_gateway PRIVATE SI_ENABLE_OPENAI)
endif()

# Shell Execution Library
add_library(shell
    src/shell/executor.cpp
    src/shell/interactive_shell.cpp
    src/shell/block_manager.cpp
    src/shell/workflow_engine.cpp
)

target_include_directories(shell PUBLIC include)
target_link_libraries(shell PUBLIC 
    core_foundation
    util
)

# Session Library
add_library(session
    src/session/history.cpp
)

target_include_directories(session PUBLIC include)
target_link_libraries(session PUBLIC 
    core_foundation
    SQLite::SQLite3
)

# Features Library
add_library(features
    src/features/interpreter.cpp
    src/features/error_analyzer.cpp
    src/features/file_ops.cpp
    src/features/git_context.cpp
)

target_include_directories(features PUBLIC include)
target_link_libraries(features PUBLIC 
    core_foundation
    ai_gateway
    nlohmann_json::nlohmann_json
)

# Circular dependency fix: shell needs features for AI/Error stuff in interactive_shell.cpp
# but features needs shell for executor. 
target_link_libraries(shell PRIVATE features ai_gateway session)

# MCP Client Library
add_library(mcp_client
    src/mcp/client.cpp
    src/mcp/stdio_transport.cpp
)

target_include_directories(mcp_client PUBLIC include)
target_link_libraries(mcp_client PUBLIC 
    core_foundation
    nlohmann_json::nlohmann_json
)

# Security Library
add_library(security
    src/security/permissions.cpp
)
target_include_directories(security PUBLIC include)
target_link_libraries(security PUBLIC core_foundation)

# Tools Library (Local MCP Tools)
add_library(tools
    src/tools/fs_tool.cpp
)
target_include_directories(tools PUBLIC include)
target_link_libraries(tools PUBLIC core_foundation security mcp_client)

# Settings Library
add_library(settings
    src/settings/settings_manager.cpp
)
target_include_directories(settings PUBLIC include)
target_link_libraries(settings PUBLIC core_foundation nlohmann_json::nlohmann_json)

# RPC Server Library
add_library(rpc_server
    src/rpc/server.cpp
)
target_include_directories(rpc_server PUBLIC include)
target_link_libraries(rpc_server PUBLIC core_foundation shell settings nlohmann_json::nlohmann_json)

# Main Executable
# Main Executable
add_executable(sicore src/main.cpp)
target_link_libraries(sicore PRIVATE core_foundation ai_gateway features shell session mcp_client security tools settings rpc_server)

# Tests
if(SI_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 REQUIRED)
    
    # Foundation Tests
    add_executable(test_foundation tests/test_foundation.cpp)
    target_link_libraries(test_foundation PRIVATE core_foundation Catch2::Catch2WithMain)
    add_test(NAME FoundationTests COMMAND test_foundation)
    
    # AI Gateway Tests
    add_executable(test_ai_gateway tests/test_ai_gateway.cpp)
    target_link_libraries(test_ai_gateway PRIVATE ai_gateway core_foundation Catch2::Catch2WithMain)
    add_test(NAME AIGatewayTests COMMAND test_ai_gateway)

    # Interpreter Tests
    add_executable(test_interpreter tests/test_interpreter.cpp)
    target_link_libraries(test_interpreter PRIVATE features ai_gateway core_foundation Catch2::Catch2WithMain)
    add_test(NAME InterpreterTests COMMAND test_interpreter)

    # Executor Tests
    add_executable(test_executor tests/test_executor.cpp)
    target_link_libraries(test_executor PRIVATE shell core_foundation Catch2::Catch2WithMain)
    add_test(NAME ExecutorTests COMMAND test_executor)

    # MCP Tests
    add_executable(test_mcp tests/test_mcp.cpp)
    target_link_libraries(test_mcp PRIVATE mcp_client core_foundation Catch2::Catch2WithMain)
    add_test(NAME MCPTests COMMAND test_mcp)

    # Security/Tools Tests
    add_executable(test_tools tests/test_tools.cpp)
    target_link_libraries(test_tools PRIVATE tools security core_foundation Catch2::Catch2WithMain)
    add_test(NAME ToolsTests COMMAND test_tools)

    # Block Tests
    add_executable(test_blocks tests/test_blocks.cpp)
    target_link_libraries(test_blocks PRIVATE shell core_foundation Catch2::Catch2WithMain)
    add_test(NAME BlockTests COMMAND test_blocks)

    # RPC Tests
    add_executable(test_rpc tests/test_rpc.cpp)
    target_link_libraries(test_rpc PRIVATE rpc_server core_foundation Catch2::Catch2WithMain)
    add_test(NAME RpcTests COMMAND test_rpc)
endif()
