# SI IPC Schema (JSON-RPC 2.0)

This document defines the communication protocol between the **SI Core (Headless)** and external clients (GUI, IDE plugins, etc.).

## Transport
- Primary: Unix Domain Socket (Linux/macOS) or Named Pipe (Windows).
- Secondary: Stdio (useful for simple wrapper scripts).

## Base Protocol
Follows [JSON-RPC 2.0 Specification](https://www.jsonrpc.org/specification).

### Request
```json
{
  "jsonrpc": "2.0",
  "method": "namespace.method",
  "params": { ... },
  "id": 1
}
```

### Response
```json
{
  "jsonrpc": "2.0",
  "result": { ... },
  "id": 1
}
```

## Methods (Client -> Core)

### Session
*   **`session.init`**
    *   Params: `{"client_name": "string", "capabilities": ["string"]}`
    *   Result: `{"version": "string", "session_id": "string", "config": {...}}`
*   **`session.shutdown`**
    *   Params: `{}`
    *   Result: `null`

### Shell Execution
*   **`shell.execute`**
    *   Params: `{"command": "string", "cwd": "string" (optional)}`
    *   Result: `{"command_id": "string", "is_background": boolean}`
*   **`shell.input`**
    *   Params: `{"command_id": "string", "data": "string" (base64 or utf8)}`
    *   Result: `null`
*   **`shell.resize`**
    *   Params: `{"command_id": "string", "rows": int, "cols": int}`
    *   Result: `null`
*   **`shell.signal`**
    *   Params: `{"command_id": "string", "signal": "string" (SIGINT, SIGTERM, etc.)}`
    *   Result: `null`

### AI & Agent
*   **`ai.complete`**
    *   Params: `{"prompt": "string", "cursor_pos": int, "context": {...}}`
    *   Result: `{"suggestions": [{"text": "string", "description": "string"}]}`
*   **`ai.chat`**
    *   Params: `{"messages": [{"role": "user|system|assistant", "content": "string"}], "model": "string"}`
    *   Result: `{"stream_id": "string"}` (followed by `ai.chunk` notifications)

### MCP (Model Context Protocol)
*   **`mcp.list_servers`**
    *   Params: `{}`
    *   Result: `{"servers": [{"name": "string", "status": "connected|disconnected"}]}`
*   **`mcp.call_tool`**
    *   Params: `{"server": "string", "tool": "string", "args": {...}}`
    *   Result: `{"result": {...}}`

## Notifications (Core -> Client)

### Shell Events
*   **`shell.output`**
    *   Params: `{"command_id": "string", "fd": 1|2, "data": "string"}`
*   **`shell.exit`**
    *   Params: `{"command_id": "string", "exit_code": int, "duration_ms": int}`

### State Updates
*   **`state.cwd`**
    *   Params: `{"path": "string"}`
*   **`state.git`**
    *   Params: `{"branch": "string", "is_dirty": boolean, "ahead": int, "behind": int}`
*   **`state.history_update`**
    *   Params: `{"entry": {"id": int, "command": "string", "timestamp": int}}`

### AI Streaming
*   **`ai.chunk`**
    *   Params: `{"stream_id": "string", "content": "string", "finish_reason": "string|null"}`

### System
*   **`log.entry`**
    *   Params: `{"level": "info|warn|error", "message": "string", "timestamp": int}`
