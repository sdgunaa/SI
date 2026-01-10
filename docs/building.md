# Building SI

## Prerequisites

**Linux (Ubuntu/Debian)**
```bash
sudo apt install build-essential cmake \
  libspdlog-dev libfmt-dev nlohmann-json3-dev \
  libboost-filesystem-dev libboost-system-dev \
  libsqlite3-dev libssl-dev catch2
```

**macOS**
```bash
brew install cmake spdlog fmt nlohmann-json boost sqlite openssl catch2
```

## Quick Build

```bash
# Backend
cmake -B build && cmake --build build

# Frontend
cd frontend && npm install
```

## Running

1. **Start backend** (in one terminal):
   ```bash
   ./build/bin/sicore --server
   ```

2. **Start frontend** (in another terminal):
   ```bash
   cd frontend && npm run dev:electron
   ```

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `SI_BUILD_TESTS` | Build unit tests | ON |
| `SI_ENABLE_OLLAMA` | Enable Ollama AI provider | ON |
| `SI_ENABLE_OPENAI` | Enable OpenAI provider | ON |

Example:
```bash
cmake -B build -DSI_BUILD_TESTS=OFF
```

## Running Tests

```bash
cd build && ctest --output-on-failure
```

## Production Build

```bash
# Backend (Release mode)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Frontend (Electron package)
cd frontend && npm run build
```
