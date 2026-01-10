import { contextBridge, ipcRenderer } from 'electron';

// Expose protected methods that allow the renderer process to use
// ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('siCore', {
    // Window controls
    minimizeWindow: () => ipcRenderer.invoke('window:minimize'),
    maximizeWindow: () => ipcRenderer.invoke('window:maximize'),
    closeWindow: () => ipcRenderer.invoke('window:close'),
    isMaximized: () => ipcRenderer.invoke('window:isMaximized'),

    // Shell commands
    executeCommand: (command: string, sessionId?: string, cols?: number, rows?: number) =>
        ipcRenderer.invoke('shell:execute', { command, sessionId, cols, rows }),

    sendInput: (blockId: string, data: string) =>
        ipcRenderer.invoke('shell:input', { blockId, data }),

    killBlock: (blockId: string) =>
        ipcRenderer.invoke('shell:kill', { blockId }),

    listBlocks: (sessionId: string) =>
        ipcRenderer.invoke('block:list', { sessionId }),

    // Sessions
    createSession: (name: string) =>
        ipcRenderer.invoke('session:create', { name }),

    listSessions: () =>
        ipcRenderer.invoke('session:list'),

    restoreSession: (sessionId: string) =>
        ipcRenderer.invoke('session:restore', { sessionId }),

    deleteSession: (sessionId: string) =>
        ipcRenderer.invoke('session:delete', { sessionId }),

    renameSession: (sessionId: string, name: string) =>
        ipcRenderer.invoke('session:rename', { sessionId, name }),

    getSessionConfig: (sessionId: string) =>
        ipcRenderer.invoke('session:get-config', { sessionId }),

    setSessionConfig: (sessionId: string, cwd?: string, shell?: string) =>
        ipcRenderer.invoke('session:set-config', { sessionId, cwd, shell }),

    // FS
    listFiles: (path: string) =>
        ipcRenderer.invoke('fs:list', { path }),

    readFile: (path: string) =>
        ipcRenderer.invoke('fs:read', { path }),

    writeFile: (path: string, content: string) =>
        ipcRenderer.invoke('fs:write', { path, content }),

    // AI
    aiChat: (messages: Array<{ role: string; content: string }>) =>
        ipcRenderer.invoke('ai:chat', { messages }),

    analyzeError: (blockId: string, sessionId: string) =>
        ipcRenderer.invoke('ai:analyze-error', { blockId, sessionId }),

    aiSuggest: (partial: string, context: object) =>
        ipcRenderer.invoke('ai:suggest', { partial, context }),

    // Settings
    settings: {
        get: (category: string) => ipcRenderer.invoke('settings:get', { category }),
        set: (category: string, data: any) => ipcRenderer.invoke('settings:set', { category, data }),
        reset: (category: string) => ipcRenderer.invoke('settings:reset', { category }),
    },

    // Workflows
    listWorkflows: (tags?: string[]) =>
        ipcRenderer.invoke('workflow:list', { tags }),

    runWorkflow: (workflowId: string, params: object) =>
        ipcRenderer.invoke('workflow:run', { workflowId, params }),

    // Event listeners
    onBlockOutput: (callback: (data: { blockId: string; chunk: string }) => void) => {
        const handler = (_event: Electron.IpcRendererEvent, data: any) => callback(data);
        ipcRenderer.on('block:output', handler);
        return () => ipcRenderer.removeListener('block:output', handler);
    },

    onBlockComplete: (callback: (data: { blockId: string; exitCode: number; duration: number }) => void) => {
        const handler = (_event: Electron.IpcRendererEvent, data: any) => callback(data);
        ipcRenderer.on('block:complete', handler);
        return () => ipcRenderer.removeListener('block:complete', handler);
    },

    onConnectionStatus: (callback: (connected: boolean) => void) => {
        const handler = (_event: Electron.IpcRendererEvent, connected: boolean) => callback(connected);
        ipcRenderer.on('core:connection', handler);
        return () => ipcRenderer.removeListener('core:connection', handler);
    },

    onAIChunk: (callback: (data: { streamId: string; content: string; done: boolean }) => void) => {
        const handler = (_event: Electron.IpcRendererEvent, data: any) => callback(data);
        ipcRenderer.on('ai:chunk', handler);
        return () => ipcRenderer.removeListener('ai:chunk', handler);
    },
});

// TypeScript declaration for renderer process
declare global {
    interface Window {
        siCore: {
            minimizeWindow: () => Promise<void>;
            maximizeWindow: () => Promise<void>;
            closeWindow: () => Promise<void>;
            isMaximized: () => Promise<boolean>;
            executeCommand: (command: string, sessionId?: string, cols?: number, rows?: number) => Promise<{
                blockId: string;
                sessionConfig?: { cwd: string; shell: string };
            }>;
            sendInput: (blockId: string, data: string) => Promise<void>;
            killBlock: (blockId: string) => Promise<void>;
            listBlocks: (sessionId: string) => Promise<any[]>;
            createSession: (name: string) => Promise<{ sessionId: string }>;
            listSessions: () => Promise<Array<{ id: string; name: string }>>;
            restoreSession: (sessionId: string) => Promise<void>;
            deleteSession: (sessionId: string) => Promise<void>;
            renameSession: (sessionId: string, name: string) => Promise<void>;
            getSessionConfig: (sessionId: string) => Promise<{ cwd: string, shell: string }>;
            setSessionConfig: (sessionId: string, cwd?: string, shell?: string) => Promise<void>;
            listFiles: (path: string) => Promise<Array<{ name: string, is_directory: boolean, size: number, mtime: number }>>;
            readFile: (path: string) => Promise<string>;
            writeFile: (path: string, content: string) => Promise<void>;
            aiChat: (messages: Array<{ role: string; content: string }>) => Promise<{ streamId: string }>;
            analyzeError: (blockId: string, sessionId: string) => Promise<any>;
            aiSuggest: (partial: string, context: object) => Promise<{ suggestions: string[] }>;
            settings: {
                get: (category: string) => Promise<any>;
                set: (category: string, data: any) => Promise<void>;
                reset: (category: string) => Promise<void>;
            };
            listWorkflows: (tags?: string[]) => Promise<Array<{ id: string; name: string }>>;
            runWorkflow: (workflowId: string, params: object) => Promise<{ blockId: string }>;
            onBlockOutput: (callback: (data: { blockId: string; chunk: string }) => void) => () => void;
            onBlockComplete: (callback: (data: { blockId: string; exitCode: number; duration: number }) => void) => () => void;
            onConnectionStatus: (callback: (connected: boolean) => void) => () => void;
            onAIChunk: (callback: (data: { streamId: string; content: string; done: boolean }) => void) => () => void;
            invoke: (channel: string, data?: any) => Promise<any>;
        };
    }
}
