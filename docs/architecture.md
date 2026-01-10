# SI Architecture

## Overview

SI is a desktop terminal application with AI integration, built as a hybrid C++/Electron system.

```
┌─────────────────────────────────────────────────────────┐
│                    Electron Frontend                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │
│  │   React UI  │  │  Preload    │  │   Main Process  │  │
│  │  Components │  │   Bridge    │  │   (IPC/Window)  │  │
│  └─────────────┘  └─────────────┘  └─────────────────┘  │
└──────────────────────────┬──────────────────────────────┘
                           │ Unix Domain Socket
                           │ (JSON-RPC 2.0)
┌──────────────────────────▼──────────────────────────────┐
│                      C++ Backend                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐  │
│  │   RPC    │  │  Shell   │  │   AI     │  │ Session │  │
│  │  Server  │  │ Executor │  │ Gateway  │  │ Manager │  │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Module Layers

| Layer | Purpose | Dependencies |
|-------|---------|--------------|
| **foundation** | Core primitives (logging, config, platform) | External libs only |
| **security** | Permissions, sandboxing | foundation |
| **session** | Session & history persistence | foundation, SQLite |
| **mcp** | Model Context Protocol client | foundation, JSON |
| **ai** | AI provider gateway (Ollama, OpenAI) | foundation, HTTP |
| **shell** | PTY, process exec, blocks | foundation, session |
| **features** | Interpreter, error analysis | ai, shell |
| **rpc** | JSON-RPC server | foundation, shell |
| **tools** | MCP tool implementations | foundation, mcp |

## Data Flow

1. **User types command** → React captures input
2. **IPC to main process** → Electron preload bridge
3. **JSON-RPC to backend** → Unix socket to sicore
4. **Block created** → Shell spawns PTY process
5. **Output streamed** → Backend notifies via JSON-RPC
6. **UI updates** → React renders block output

## Key Files

| Path | Description |
|------|-------------|
| `backend/src/main.cpp` | Backend entry point |
| `backend/src/rpc/server.cpp` | JSON-RPC handler |
| `backend/src/shell/block_manager.cpp` | Block/session management |
| `frontend/electron/main.ts` | Electron main process |
| `frontend/electron/ipc-bridge.ts` | Backend communication |
| `frontend/src/pages/TerminalPage.tsx` | Terminal UI |

## IPC Protocol

Backend communicates via JSON-RPC 2.0 over Unix socket (`si.sock`).

**Request**
```json
{"jsonrpc":"2.0","id":1,"method":"block.execute","params":{"command":"ls"}}
```

**Notification** (output)
```json
{"method":"block.output","params":{"block_id":"...","data":"file.txt\n"}}
```
