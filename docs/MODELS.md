# SI-Core Recommended Models

## Small, Fast Models for Shell Commands (1-2B parameters)

These models are optimized for quick inference on CPU/GPU with minimal memory usage.

### 1. DeepSeek-R1:1.5B ⭐ Recommended
- **Size**: ~1GB (Q4 quantized)
- **Context**: 4K tokens
- **Strengths**: Excellent reasoning, good for command generation
- **Ollama**: `ollama pull deepseek-r1:1.5b`
- **GGUF**: Available on HuggingFace

### 2. Qwen 2.5:1.7B
- **Size**: ~1.1GB (Q4 quantized)
- **Context**: 32K tokens (excellent for context-aware commands)
- **Strengths**: Multilingual, strong instruction following
- **Ollama**: `ollama pull qwen2.5:1.5b`
- **GGUF**: `Qwen/Qwen2.5-1.5B-Instruct-GGUF`

### 3. Llama 3.2:1B
- **Size**: ~800MB (Q4 quantized)
- **Context**: 128K tokens
- **Strengths**: Latest Meta model, very fast
- **Ollama**: `ollama pull llama3.2:1b`
- **GGUF**: `meta-llama/Llama-3.2-1B-Instruct-GGUF`

### 4. SmolLM2:1.7B
- **Size**: ~1.1GB (Q4 quantized)
- **Context**: 8K tokens
- **Strengths**: Trained for efficiency, good balance
- **Ollama**: `ollama pull smollm2:1.7b`
- **GGUF**: `HuggingFaceTB/SmolLM2-1.7B-Instruct-GGUF`

## Installation

### Option 1: Ollama (Easiest)
```bash
# Install Ollama
curl -fsSL https://ollama.com/install.sh | sh

# Pull a model
ollama pull deepseek-r1:1.5b

# Verify
ollama list
```

### Option 2: llama.cpp (Direct)
```bash
# Build llama.cpp
cd /home/guna/Projects/2025/SI/third_party/llama.cpp
mkdir build && cd build
cmake .. -DLLAMA_CUBLAS=ON  # For NVIDIA GPU
cmake --build . --config Release

# Download model (example: Qwen 2.5 1.5B Q4)
mkdir -p ~/.local/share/si/models
cd ~/.local/share/si/models

# Download from HuggingFace
wget https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf
```

## Performance Comparison

| Model | Size | Speed (tok/s) | Memory | Best For |
|-------|------|---------------|--------|----------|
| **DeepSeek-R1:1.5B** | 1GB | ~80 | 2GB | Reasoning, complex commands |
| **Qwen 2.5:1.5B** | 1.1GB | ~75 | 2GB | Long context, multilingual |
| **Llama 3.2:1B** | 800MB | ~100 | 1.5GB | Speed, simple commands |
| **SmolLM2:1.7B** | 1.1GB | ~70 | 2GB | Balanced performance |

## Configuration

Update `~/.config/si/si.conf`:

```toml
[ai]
provider = "ollama"  # or "llamacpp"
model = "deepseek-r1:1.5b"
temperature = 0.7
max_tokens = 512  # Small for shell commands

[ai.llamacpp]
model_path = "~/.local/share/si/models/qwen2.5-1.5b-instruct-q4_k_m.gguf"
gpu_layers = 33  # Full offload for small models
threads = 8

[ai.ollama]
host = "http://localhost:11434"
model = "deepseek-r1:1.5b"
```

## Recommendations

**For CPU-only systems**: Use Qwen 2.5:1.5B (best quality/speed ratio)  
**For GPU systems**: Use DeepSeek-R1:1.5B (excellent reasoning with GPU acceleration)  
**For minimal memory**: Use Llama 3.2:1B (smallest footprint)  
**For ease of use**: Use Ollama with any model (no manual setup)

## Testing

Once installed, test with:
```bash
# With Ollama
ollama run deepseek-r1:1.5b "Generate a bash command to list all PDF files"

# With llama.cpp
./llama-cli -m ~/.local/share/si/models/qwen2.5-1.5b-instruct-q4_k_m.gguf \
  -p "Generate a bash command to list all PDF files" -n 100
```
