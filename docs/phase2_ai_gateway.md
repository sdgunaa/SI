# Phase 2: AIEngineGateway Implementation Plan

## Overview
Implement the AI abstraction layer that will enable SI-Core to work with multiple LLM backends.

## Architecture

```
AIGateway (Router)
    │
    ├── LlamaCppProvider (Local, GPU-capable)
    ├── OllamaProvider (Local REST API)
    └── OpenAIProvider (Cloud API)
```

## Components

### 1. Provider Interface ✅
- [x] `include/ai/provider.hpp` - Abstract interface
- [x] CompletionRequest/Response structures
- [x] Streaming support with callbacks
- [x] ModelInfo structure

### 2. AI Gateway ✅
- [x] `include/ai/gateway.hpp` - Gateway header
- [x] `src/ai/gateway.cpp` - Gateway implementation
- [x] Provider registration and management
- [x] Request routing to active provider
- [x] Error handling and logging

### 3. LlamaCpp Provider (Priority 1)
- [ ] Download and build llama.cpp
- [ ] `include/ai/providers/llamacpp_provider.hpp`
- [ ] `src/ai/providers/llamacpp_provider.cpp`
- [ ] Model loading (GGUF format)
- [ ] GPU detection and configuration
- [ ] Token generation and sampling
- [ ] Context management

### 4. Ollama Provider (Priority 2)
- [ ] `include/ai/providers/ollama_provider.hpp`
- [ ] `src/ai/providers/ollama_provider.cpp`
- [ ] REST API client (using cpp-httplib)
- [ ] Model availability checking
- [ ] Streaming response handling

### 5. OpenAI Provider (Priority 3 - Optional)
- [ ] `include/ai/providers/openai_provider.hpp`
- [ ] `src/ai/providers/openai_provider.cpp`
- [ ] OpenAI API client
- [ ] API key management
- [ ] Rate limiting

## Implementation Steps

### Step 1: llama.cpp Setup
```bash
cd /home/guna/Projects/2025/SI/third_party
git clone https://github.com/ggerganov/llama.cpp.git
cd llama.cpp
mkdir build && cd build
cmake .. -DLLAMA_CUBLAS=ON  # or -DLLAMA_METAL=ON for Apple
cmake --build . --config Release
```

### Step 2: Download Test Model
```bash
mkdir -p ~/.local/share/si/models
# Download CodeLlama 7B Q4 (recommended for testing)
# Or TinyLlama for faster testing
```

### Step 3: Implement LlamaCppProvider
- Wrap llama.cpp C API
- Handle model loading
- Implement completion with sampling
- Add streaming support

### Step 4: Integration Testing
- Test with simple prompts
- Verify GPU acceleration
- Benchmark performance

### Step 5: Update CMakeLists.txt
- Add ai_gateway library
- Link llama.cpp
- Add provider sources

## Success Criteria
- [ ] Gateway can register and switch between providers
- [ ] LlamaCpp provider can load and run inference
- [ ] Simple completion request works end-to-end
- [ ] Streaming responses work
- [ ] GPU acceleration functional (if available)

## Next Phase
After AIEngineGateway is working, move to Phase 3: CommandInterpreter which will use the gateway to translate natural language to commands.
