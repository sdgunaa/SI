#!/bin/bash
set -e

MODEL_DIR="$HOME/.local/share/si/models"
mkdir -p "$MODEL_DIR"

# URL for Qwen 2.5 1.5B Instruct Q4_K_M (Small, fast, good quality)
MODEL_URL="https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf"
MODEL_FILE="$MODEL_DIR/qwen2.5-1.5b-instruct-q4_k_m.gguf"

if [ -f "$MODEL_FILE" ]; then
    echo "Model already exists at $MODEL_FILE"
    exit 0
fi

echo "Downloading Qwen 2.5 1.5B model..."
wget -O "$MODEL_FILE" "$MODEL_URL"

echo "Download complete!"
ls -lh "$MODEL_FILE"
