# SI Codebase Architecture

This document maps the implementation plan to the codebase structure.

## Directory Structure

| Directory | Component | Description |
|-----------|-----------|-------------|
| **`src/ai`** | **AI Router** | Multi-provider AI router (vLLM, Ollama, OpenAI) and Gateway. corresponds to `AgentEngine`. |
| **`src/foundation`** | **Foundation** | Shared resources: Config, Logging, Platform, Signals. |
| **`src/features`** | **Tools/Features** | Specific agent capabilities: Interpreter, ErrorAnalyzer, FileOps. |
| **`src/mcp`** | **MCP Host** | Model Context Protocol implementation (Host, Client, Servers). *In progress* |
| **`src/security`** | **Security** | Permissioning, Key Vault, Rate Limiting. *In progress* |
| **`src/session`** | **Session** | Session management, History, User Context (`session/manager`). |
| **`src/shell`** | **Interfaces** | **Headless Mode** (CLI/TUI) implementation (Executor, InteractiveShell). |

## Dual-Mode Architecture

The codebase is structured to support two distinct modes sharing the same core:

1.  **Headless Mode (`src/shell`)**:
    *   Entry point: `main.cpp` -> `InteractiveShell`.
    *   Pure C++ CLI/TUI.
    *   Dependencies: `ncurses` (future), `replxx` (future).

2.  **GUI Mode** (Future):
    *   Will reside in separate repo or `src/gui` folder.
    *   Will consume `libsi_core` (composed of `ai`, `session`, `mcp`, `security`, `foundation`).

## Alignment with Implementation Plan

*   **Shared Core**: `ai` + `session` + `mcp` + `security` + `foundation`.
*   **Interfaces**: `shell` (Headless).
