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
option(SI_HAS_LLAMACPP "Enable llama.cpp support" ON)
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

# llama.cpp
if(SI_HAS_LLAMACPP)
    # Use pre-built libraries to avoid long rebuild times
    set(LLAMA_PREBUILT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/build/bin")
    
    # Import llama library
    add_library(llama SHARED IMPORTED)
    set_target_properties(llama PROPERTIES
        IMPORTED_LOCATION "${LLAMA_PREBUILT_DIR}/libllama.so"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/include;${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/common;${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/ggml/include"
    )
    
    # Import ggml library
    add_library(ggml SHARED IMPORTED)
    set_target_properties(ggml PROPERTIES
        IMPORTED_LOCATION "${LLAMA_PREBUILT_DIR}/libggml.so"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/ggml/include"
    )
    
    # We also need ggml-cpu and ggml-cuda if they exist
    if(EXISTS "${LLAMA_PREBUILT_DIR}/libggml-cuda.so")
        add_library(ggml-cuda SHARED IMPORTED)
        set_target_properties(ggml-cuda PROPERTIES
            IMPORTED_LOCATION "${LLAMA_PREBUILT_DIR}/libggml-cuda.so"
        )
        target_link_libraries(ggml INTERFACE ggml-cuda)
    endif()
    
    if(EXISTS "${LLAMA_PREBUILT_DIR}/libggml-cpu.so")
        add_library(ggml-cpu SHARED IMPORTED)
        set_target_properties(ggml-cpu PROPERTIES
            IMPORTED_LOCATION "${LLAMA_PREBUILT_DIR}/libggml-cpu.so"
        )
        target_link_libraries(ggml INTERFACE ggml-cpu)
    endif()
endif()

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
)

# AI Gateway Library
add_library(ai_gateway
    src/ai/gateway.cpp
)

target_include_directories(ai_gateway PUBLIC include)
target_link_libraries(ai_gateway PUBLIC 
    core_foundation
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# LlamaCpp Provider
if(SI_HAS_LLAMACPP)
    target_sources(ai_gateway PRIVATE src/ai/providers/llamacpp_provider.cpp)
    target_link_libraries(ai_gateway PRIVATE llama ggml)
    target_compile_definitions(ai_gateway PRIVATE SI_HAS_LLAMACPP)
endif()

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

# Features Library
add_library(features
    src/features/interpreter.cpp
)

target_include_directories(features PUBLIC include)
target_link_libraries(features PUBLIC 
    core_foundation
    ai_gateway
    nlohmann_json::nlohmann_json
)

# Main Executable
add_executable(si src/main.cpp)
target_link_libraries(si PRIVATE core_foundation ai_gateway features)

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
endif()
