# SI - Shell Intelligence

A Warp-level, AI-powered terminal with local-first intelligence.

## Features

- 📦 **Command Blocks** — Visual output grouping with timing
- 🌿 **Git-Aware Prompt** — Branch, status, ahead/behind
- 🤖 **AI Interpretation** — Natural language → commands
- 🔧 **Auto-Fix** — AI suggests fixes for errors
- 🔒 **Local-First AI** — Full privacy with Ollama

## Quick Start

```bash
# Install dependencies & build
./scripts/setup.sh

# Run
./build/bin/sicore --server &
cd frontend && npm run dev:electron
```

## Project Structure

```
si/
├── backend/           # C++ core (shell, AI, RPC)
│   ├── include/       # Public headers
│   ├── src/           # Implementation
│   └── tests/         # Unit tests
├── frontend/          # Electron + React UI
│   ├── electron/      # Main & preload scripts
│   └── src/           # React components
├── config/            # Example configuration
├── docs/              # Documentation
└── scripts/           # Dev scripts
```

## Documentation

- [Building](docs/building.md) — Prerequisites & build instructions
- [Architecture](docs/architecture.md) — System design overview

## Configuration

`~/.config/si/si.conf`:
```toml
[ai]
provider = "ollama"
model = "deepseek-r1:1.5b"
```

## License

MIT
