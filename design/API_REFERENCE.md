# SI Backend API Reference (JSON-RPC 2.0)

This document describes the API exposed by the SI Core (`libsi_core`) for use by Frontends (CLI/GUI).

**Transport**: Unix Domain Socket (path: `/tmp/si.sock` by default) or Stdio.
**Protocol**: JSON-RPC 2.0. Messages are newline-delimited JSON.

---

## Block Service

Blocks are the core data unit. Each command execution creates a Block.

### `block.create`
Create a new block and begin execution (conceptually).

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `session_id` | string | No | Session ID. Defaults to "default". |
| `command` | string | Yes | The command to run. |
| `cwd` | string | No | Working directory. |

**Result**:
```json
{ "block_id": "uuid-string" }
```

### `block.get`
Retrieve a single block by ID.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `block_id` | string | Yes | The block's UUID. |

**Result**: A `Block` object.
```json
{
  "id": "...",
  "session_id": "...",
  "command": "ls -la",
  "cwd": "/home/user",
  "start_time": 1700000000000,
  "end_time": 1700000000500,
  "exit_code": 0,
  "state": 1, // 0=RUNNING, 1=COMPLETED, 2=FAILED
  "output_chunks": [
    { "type": "stdout", "data": "file1.txt\nfile2.txt", "ts": ... }
  ],
  "metadata": {}
}
```

### `block.list`
List blocks for a session, ordered by start time.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `session_id` | string | No | Session to list. |

**Result**: Array of `Block` objects.

---

## Workflow Service

Workflows are parameterized command templates.

### `workflow.save`
Save a new or updated workflow.

**Params**: A `Workflow` object.
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | No | Workflow ID (auto-generated if empty). |
| `name` | string | Yes | Human-readable name. |
| `description` | string | Yes | Description. |
| `command_template` | string | Yes | Template with `{{var}}` placeholders. |
| `arguments` | array | No | List of `{name, description, default_value}`. |
| `tags` | array | No | List of string tags. |

**Result**:
```json
{ "workflow_id": "..." }
```

### `workflow.get`
Retrieve a workflow by ID.

**Params**: `{ "workflow_id": "..." }`

**Result**: A `Workflow` object.

### `workflow.list`
List all workflows, optionally filtered by tag.

**Params**: `{ "tag": "optional-tag" }`

**Result**: Array of `Workflow` objects.

### `workflow.render`
Substitute parameters into a workflow template to get the final command.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `workflow_id` | string | Yes | The workflow to render. |
| `params` | object | No | Map of `{ "var_name": "value" }`. |

**Result**:
```json
{ "command": "kubectl logs -f my-pod -n production" }
```

---

## AI Service

### `ai.get_context`
Get current shell context for AI operations.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `cwd` | string | No | Override working directory. |
| `session_id` | string | No | Session for history context. |

**Result**: Context JSON object (cwd, os, shell, git info, recent commands).

### `ai.generate_command`
Use AI to generate a shell command from natural language.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `prompt` | string | Yes | Natural language request (e.g., "list all .py files"). |

**Result**:
```json
{ "command": "find . -name '*.py'", "success": true }
```

### `ai.analyze_error`
Analyze a failed block and suggest fixes.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `block_id` | string | Yes | Block ID of failed command. |

**Result**:
```json
{ "analysis": "The command failed because...", "success": true }
```

---

## Block Execution

### `block.execute`
Create a new block AND execute the command (async). Returns immediately.

**Params**:
| Param | Type | Required | Description |
|-------|------|----------|-------------|
| `session_id` | string | No | Session ID. Defaults to "default". |
| `command` | string | Yes | The command to run. |
| `cwd` | string | No | Working directory. |

**Result**:
```json
{ "block_id": "uuid-string" }
```

*Note*: The command runs in background. Poll `block.get` or listen for `block.complete` event.

---

## Events (Server -> Client Notifications)

The backend sends JSON-RPC notifications (no `id` field).

### `block.output`
Emitted when a block receives new output.
```json
{
  "jsonrpc": "2.0",
  "method": "block.output",
  "params": { "block_id": "...", "chunk": { "type": "stdout", "data": "...", "ts": ... } }
}
```

### `block.complete`
Emitted when a block finishes execution.
```json
{
  "jsonrpc": "2.0",
  "method": "block.complete",
  "params": { "block_id": "...", "exit_code": 0 }
```
