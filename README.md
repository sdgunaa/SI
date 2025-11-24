# SI-Core - Shell Intelligence Core

An AI-integrated shell program that enhances command-line interaction through local LLM integration.

## Features

🤖 **Natural Language Commands** - Convert plain English to shell commands  
💡 **Intelligent Explanations** - Understand what commands do before running them  
🔧 **Auto-Fix Errors** - Analyze command failures and suggest corrections  
📁 **AI-Powered File Operations** - Summarize, search, and analyze files  

## Quick Start

### Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- SQLite3
- Optional: CUDA (for NVIDIA GPU acceleration)

### Build

```bash
# Clone the repository
git clone https://github.com/guna-sd/SI.git
cd SI

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel

# Install (optional)
sudo cmake --install .
```

### Install llama.cpp (for local LLM)

```bash
# Clone llama.cpp
cd third_party
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp

# Build with GPU support (optional)
mkdir build && cd build
cmake .. -DLLAMA_CUBLAS=ON  # or -DLLAMA_METAL=ON for Apple
cmake --build . --config Release

# Download a model (example: CodeLlama 7B Q4)
cd ../..
mkdir -p ~/.local/share/SI/models
wget https://huggingface.co/TheBloke/CodeLlama-7B-GGUF/resolve/main/codellama-7b.Q4_K_M.gguf \
     -O ~/.local/share/SI/models/codellama-7b-q4.gguf
```

### Configuration

Copy the example configuration:

```bash
mkdir -p ~/.config/si
cp config/si.conf.example ~/.config/si/si.conf
```

Edit `~/.config/si/si.conf` to set your preferences:

```toml
[ai.llamacpp]
model_path = "~/.local/share/SI/models/codellama-7b-q4.gguf"
gpu_layers = 32  # Adjust based on your GPU memory
threads = 8
```

### Usage

Run SI-Core:

```bash
si
```

#### Command Modes

| Prefix | Mode | Example |
|--------|------|---------|
| (none) | Direct shell | `ls -la` |
| `?` | Natural language | `? list all PDF files` |
| `??` | Explain command | `?? tar -xzvf archive.tar.gz` |
| `!` | Fix last error | `!` |
| `@` | File operations | `@ summarize README.md` |

#### Examples

```bash
# Natural language to command
SI> ? find all Python files modified today

Generated command: find . -name "*.py" -mtime 0
Explanation: Searches current directory for Python files modified in the last 24 hours
Risk: Safe
Execute? [Y/n]

# Explain a command
SI> ?? docker-compose up -d

Explanation: Starts Docker containers defined in docker-compose.yml in detached mode
- docker-compose: Tool to orchestrate multiple containers
- up: Create and start containers
- -d: Detached mode (run in background)

# Auto-fix errors
SI> rm file.txt
rm: cannot remove 'file.txt': Permission denied

SI> !
Error Analysis: Permission denied when deleting file
Root Cause: Insufficient permissions to write to directory

Suggested Fixes:
1. sudo rm file.txt (Confidence: 95%, Risk: Moderate)
2. chmod +w . && rm file.txt (Confidence: 80%, Risk: Moderate)

Select fix [1-2] or [c]ancel:

# File operations
SI> @ summarize main.cpp

Summary: Main entry point for SI-Core application
- Initializes CoreFoundation and AI Gateway
- Sets up REPL loop with command routing
- Handles graceful shutdown on signals
Key functions: main(), initialize_modules(), run_repl()
```

## Architecture

SI-Core is built with a modular architecture:

- **CoreFoundation** - Configuration, logging, platform abstraction
- **AIEngineGateway** - Unified interface to LLM backends (llama.cpp, Ollama, OpenAI)
- **CommandInterpreter** - Natural language → shell commands
- **ErrorAnalyzer** - Command failure analysis and fix suggestions
- **FileOperations** - AI-powered file summarization and search
- **CommandExecutor** - Safe command execution with output capture
- **HistoryManager** - Persistent command history with SQLite
- **InteractiveShell** - REPL interface with line editing

## Development

### Building from Source

```bash
# Debug build with tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DSI_BUILD_TESTS=ON
cmake --build .

# Run tests
ctest --output-on-failure
```

### Project Structure

```
SI/
├── CMakeLists.txt
├── include/               # Public headers
├── src/
│   ├── foundation/        # Core foundation
│   ├── ai/               # AI engine and providers
│   ├── features/         # Core features (interpreter, analyzer, file ops)
│   ├── services/         # Services (executor, history)
│   ├── shell/            # Interactive shell
│   └── main.cpp
├── tests/                # Unit tests
├── config/               # Configuration files
└── docs/                 # Documentation
```

## License

MIT License - see LICENSE file

## Contributing

Contributions are welcome! Please see CONTRIBUTING.md for guidelines.

## Credits

- [llama.cpp](https://github.com/ggerganov/llama.cpp) - Efficient LLM inference
- [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging
- [replxx](https://github.com/AmokHuginnsson/replxx) - REPL line editing
- [toml++](https://github.com/marzer/tomlplusplus) - TOML configuration parsing
