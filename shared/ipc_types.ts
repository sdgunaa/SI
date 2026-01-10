/**
 * Shared IPC Types for SI Frontend <-> Backend Communication
 * Generated from design/ipc_schema.md
 */

// ============================================================================
// Block Types
// ============================================================================

export interface OutputChunk {
    type: 'stdout' | 'stderr' | 'html' | 'json';
    data: string;
    timestamp: number;
}

export interface Block {
    id: string;
    session_id: string;
    command: string;
    cwd: string;
    env?: Record<string, string>;
    start_time: number;
    end_time?: number;
    exit_code?: number;
    output_chunks: OutputChunk[];
    metadata?: Record<string, unknown>;
}

export interface BlockMatch {
    block_id: string;
    command: string;
    match_positions: number[];
}

// ============================================================================
// Session Types
// ============================================================================

export interface SessionSummary {
    id: string;
    name: string;
    type: 'terminal' | 'notebook';
    created_at: number;
    last_active: number;
}

export interface SessionState {
    id: string;
    name: string;
    type: 'terminal' | 'notebook';
    blocks: Block[];
    cwd: string;
    env: Record<string, string>;
}

// ============================================================================
// Workflow Types  
// ============================================================================

export interface WorkflowArgument {
    name: string;
    description: string;
    default_value?: string;
    type: 'string' | 'number' | 'boolean' | 'select';
    options?: string[]; // For 'select' type
}

export interface Workflow {
    id: string;
    name: string;
    description: string;
    command_template: string;
    arguments: WorkflowArgument[];
    tags: string[];
}

// ============================================================================
// AI Types
// ============================================================================

export interface ChatMessage {
    role: 'user' | 'assistant' | 'system';
    content: string;
}

export interface AISuggestion {
    text: string;
    description: string;
}

export interface AIAnalysisResult {
    error_type: string;
    explanation: string;
    suggested_fix: string;
    confidence: number;
}

// ============================================================================
// Workspace Types
// ============================================================================

export interface GitStatus {
    branch: string;
    is_dirty: boolean;
    ahead: number;
    behind: number;
}

export interface WorkspaceContext {
    cwd: string;
    git?: GitStatus;
    env: Record<string, string>;
}

// ============================================================================
// JSON-RPC Types
// ============================================================================

export interface JsonRpcRequest {
    jsonrpc: '2.0';
    method: string;
    params?: Record<string, unknown>;
    id: number;
}

export interface JsonRpcResponse {
    jsonrpc: '2.0';
    result?: unknown;
    error?: {
        code: number;
        message: string;
        data?: unknown;
    };
    id: number;
}

export interface JsonRpcNotification {
    jsonrpc: '2.0';
    method: string;
    params: Record<string, unknown>;
}

// ============================================================================
// IPC Method Signatures
// ============================================================================

export namespace IPC {
    // Block Service
    export namespace Block {
        export interface StartParams { session_id: string; command: string; }
        export interface StartResult { block_id: string; }

        export interface InputParams { block_id: string; data: string; }
        export interface ResizeParams { block_id: string; rows: number; cols: number; }
        export interface KillParams { block_id: string; }
        export interface GetParams { block_id: string; }
        export interface ListParams { session_id: string; filter?: string; }
        export interface SearchParams { query: string; }

        // Notifications
        export interface OutputNotification { block_id: string; chunk_index: number; chunk_data: OutputChunk; }
        export interface CompleteNotification { block_id: string; exit_code: number; duration_ms: number; }
    }

    // Session Service
    export namespace Session {
        export interface CreateParams { name: string; type: 'terminal' | 'notebook'; }
        export interface CreateResult { session_id: string; }

        export interface RestoreParams { session_id: string; }
        export interface ExportParams { session_id: string; format: 'markdown' | 'json'; }
    }

    // AI Service
    export namespace AI {
        export interface ChatParams { messages: ChatMessage[]; context?: WorkspaceContext; }
        export interface ChatResult { stream_id: string; }

        export interface SuggestParams { partial_input: string; history_context?: string[]; }
        export interface SuggestResult { suggestions: AISuggestion[]; }

        export interface AnalyzeErrorParams { block_id: string; }
        export interface AnalyzeErrorResult extends AIAnalysisResult { }

        // Notifications
        export interface ChunkNotification { stream_id: string; content: string; finish_reason: string | null; }
    }

    // Workflow Service
    export namespace Workflow {
        export interface ListParams { tags?: string[]; search?: string; }
        export interface ListResult { workflows: Workflow[]; }

        export interface RunParams { workflow_id: string; params: Record<string, string>; }
        export interface RunResult { block_id: string; }

        export interface GenerateParams { natural_language_prompt: string; }
        export interface GenerateResult extends Workflow { }
    }

    // Workspace Service
    export namespace Workspace {
        export interface SetEnvParams { key: string; value: string; scope: 'local' | 'shared'; }
        export interface GetContextResult extends WorkspaceContext { }
    }
}
