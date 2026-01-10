import { ipcMain, BrowserWindow } from 'electron';
import * as net from 'net';

const SOCKET_PATH = process.env.SI_SOCKET_PATH || '/home/guna/Projects/2025/SI/si.sock';

let client: net.Socket | null = null;
let connected = false;
let requestId = 0;
let pendingRequests = new Map<number, { resolve: Function; reject: Function }>();

/**
 * Create the IPC bridge to communicate with the C++ backend via Unix Domain Socket
 */
export function createIPCBridge() {
    connectToCore();

    // Shell execution
    ipcMain.handle('shell:execute', async (_event, { command, sessionId, cols, rows }) => {
        console.log(`[IPC Bridge] shell:execute called. Session: ${sessionId}, Command: ${command}, Size: ${cols}x${rows}`);
        const result = await sendRequest('block.execute', { session_id: sessionId || 'default', command, cols, rows });
        console.log(`[IPC Bridge] block.execute response:`, JSON.stringify(result));
        return result;
    });

    ipcMain.handle('shell:input', async (_event, { blockId, data }) => {
        return sendRequest('block.input', { block_id: blockId, data });
    });

    ipcMain.handle('shell:kill', async (_event, { blockId }) => {
        return sendRequest('block.kill', { block_id: blockId });
    });

    ipcMain.handle('block:list', async (_event, { sessionId }) => {
        return sendRequest('block.list', { session_id: sessionId });
    });

    // Sessions
    ipcMain.handle('session:create', async (_event, { name }) => {
        return sendRequest('session.create', { name, type: 'terminal' });
    });

    ipcMain.handle('session:list', async () => {
        return sendRequest('session.list', {});
    });

    ipcMain.handle('session:delete', async (_event, { sessionId }) => {
        return sendRequest('session.delete', { session_id: sessionId });
    });

    ipcMain.handle('session:rename', async (_event, { sessionId, name }) => {
        return sendRequest('session.rename', { session_id: sessionId, name });
    });

    ipcMain.handle('ai:analyze-error', async (_event, { blockId, sessionId }) => {
        return sendRequest('ai.analyze_error', { block_id: blockId, session_id: sessionId });
    });

    // Session Config
    ipcMain.handle('session:get-config', async (_event, { sessionId }) => {
        return sendRequest('session.get_config', { session_id: sessionId });
    });

    ipcMain.handle('session:set-config', async (_event, { sessionId, cwd, shell }) => {
        return sendRequest('session.set_config', { session_id: sessionId, cwd, shell });
    });

    // FS API
    ipcMain.handle('fs:list', async (_event, { path }) => {
        return sendRequest('fs.list', { path });
    });

    ipcMain.handle('fs:read', async (_event, { path }) => {
        return sendRequest('fs.read', { path });
    });

    ipcMain.handle('fs:write', async (_event, { path, content }) => {
        return sendRequest('fs.write', { path, content });
    });

    // AI
    ipcMain.handle('ai:chat', async (_event, { messages }) => {
        return sendRequest('ai.chat', { messages, context: {} });
    });

    ipcMain.handle('ai:suggest', async (_event, { partial, context }) => {
        return sendRequest('ai.suggest_command', { partial_input: partial, history_context: context });
    });

    // Settings
    ipcMain.handle('settings:get', async (_event, { category }) => {
        return sendRequest('settings.get', { category });
    });

    ipcMain.handle('settings:set', async (_event, { category, data }) => {
        return sendRequest('settings.set', { category, data });
    });

    ipcMain.handle('settings:reset', async (_event, { category }) => {
        return sendRequest('settings.reset', { category });
    });

    // Workflows
    ipcMain.handle('workflow:list', async (_event, { tags }) => {
        return sendRequest('workflow.list', { tags: tags || [], search: '' });
    });

    ipcMain.handle('workflow:run', async (_event, { workflowId, params }) => {
        return sendRequest('workflow.run', { workflow_id: workflowId, params });
    });
}

function connectToCore() {
    if (client) {
        client.destroy();
    }

    console.log('[IPC Bridge] Attempting to connect to:', SOCKET_PATH);
    client = net.createConnection(SOCKET_PATH);
    client.setKeepAlive(true);

    client.on('connect', () => {
        console.log('[IPC Bridge] Successfully connected to SI Core');
        connected = true;
        broadcastConnectionStatus(true);
    });

    client.on('data', (data) => {
        handleIncomingData(data.toString());
    });

    client.on('end', () => {
        console.warn('[IPC Bridge] Connection ended (FIN received)');
        connected = false;
        broadcastConnectionStatus(false);
        client?.destroy();
    });

    client.on('error', (err) => {
        console.error('[IPC Bridge] Connection error:', err);
        connected = false;
        broadcastConnectionStatus(false);
    });

    client.on('close', () => {
        console.warn('[IPC Bridge] Connection closed');
        connected = false;
        broadcastConnectionStatus(false);
        // Retry connection after 2 seconds
        setTimeout(connectToCore, 2000);
    });
}

let inputBuffer = '';

function handleIncomingData(chunk: string) {
    inputBuffer += chunk;

    let boundary = inputBuffer.indexOf('\n');
    while (boundary !== -1) {
        const line = inputBuffer.substring(0, boundary).trim();
        inputBuffer = inputBuffer.substring(boundary + 1);

        if (line) {
            try {
                const message = JSON.parse(line);

                if (message.id !== undefined) {
                    // This is a response to a request
                    const pending = pendingRequests.get(message.id);
                    if (pending) {
                        pendingRequests.delete(message.id);
                        if (message.error) {
                            pending.reject(new Error(message.error.message));
                        } else {
                            pending.resolve(message.result);
                        }
                    }
                } else if (message.method) {
                    // This is a notification from the server
                    handleNotification(message.method, message.params);
                }
            } catch (e) {
                console.error('[IPC Bridge] Failed to parse message:', e);
            }
        }
        boundary = inputBuffer.indexOf('\n');
    }
}

// Utility to convert snake_case to camelCase
function toCamelCase(obj: any): any {
    if (Array.isArray(obj)) {
        return obj.map(v => toCamelCase(v));
    } else if (obj !== null && obj.constructor === Object) {
        return Object.keys(obj).reduce(
            (result, key) => {
                const camelKey = key.replace(/([-_][a-z])/g, (group) =>
                    group.toUpperCase().replace('-', '').replace('_', '')
                );
                result[camelKey] = toCamelCase(obj[key]);
                return result;
            },
            {} as any
        );
    }
    return obj;
}

function handleNotification(method: string, params: any) {
    const windows = BrowserWindow.getAllWindows();
    // Pre-convert params to camelCase for consistent handling
    const camelParams = toCamelCase(params);

    switch (method) {
        case 'block.output':
            windows.forEach(w => w.webContents.send('block:output', {
                blockId: camelParams.blockId,
                chunk: camelParams.data,
                // Backend uses 'type', frontend expects different handling or type
                // Mapping 'type' back to what frontend expects if needed, or keeping it as is
                // If backend sends "type": "stdout", frontend hook uses it directly?
                // The C++ fix used "type", let's pass it through.
                // Note: The previous code used 'fd', we now have 'type'.
                // If the frontend expects 'fd', we might need to map it, but hooks seem to expect 'chunk'
                // and use 'type' internally if available or default to something?
                // Let's pass the whole object or specific mapped fields.
                // Frontend useBlocks.ts hook uses:
                // data.blockId, data.chunk, and sets type: 'stdout' hardcoded in one place?
                // Let's look at useBlocks.ts again. It sets type: 'stdout'.
                // We should probably just pass 'chunk' as 'data'.
            }));
            break;

        case 'block.complete':
            windows.forEach(w => w.webContents.send('block:complete', {
                blockId: camelParams.blockId,
                sessionId: camelParams.sessionId,
                exitCode: camelParams.exitCode,
                duration: camelParams.durationMs,
            }));
            break;

        case 'ai.chunk':
            windows.forEach(w => w.webContents.send('ai:chunk', {
                streamId: camelParams.streamId,
                content: camelParams.content,
                done: camelParams.finishReason !== null,
            }));
            break;

        default:
            console.log('[IPC Bridge] Unknown notification:', method);
    }
}

function sendRequest(method: string, params: object): Promise<any> {
    return new Promise((resolve, reject) => {
        if (!connected || !client) {
            // If not connected, use mock responses for development
            console.warn('[IPC Bridge] Not connected, using mock response for:', method);
            resolve(toCamelCase(getMockResponse(method, params)));
            return;
        }

        const id = ++requestId;
        const request = {
            jsonrpc: '2.0',
            method,
            params,
            id,
        };

        // Wrap resolve to transform result to camelCase
        const wrappedResolve = (result: any) => resolve(toCamelCase(result));

        pendingRequests.set(id, { resolve: wrappedResolve, reject });
        client.write(JSON.stringify(request) + '\n');

        // Timeout after 30 seconds
        setTimeout(() => {
            if (pendingRequests.has(id)) {
                pendingRequests.delete(id);
                reject(new Error('Request timeout'));
            }
        }, 30000);
    });
}

function broadcastConnectionStatus(status: boolean) {
    BrowserWindow.getAllWindows().forEach(w => {
        w.webContents.send('core:connection', status);
    });
}

/**
 * Mock responses for development without backend
 */
function getMockResponse(method: string, params: any): any {
    switch (method) {
        case 'block.execute':
            const blockId = `block_${Date.now()}`;
            // Simulate output after a delay
            setTimeout(() => {
                BrowserWindow.getAllWindows().forEach(w => {
                    w.webContents.send('block:output', {
                        blockId,
                        chunk: `Executing: ${params.command}\n`,
                    });
                });
                setTimeout(() => {
                    BrowserWindow.getAllWindows().forEach(w => {
                        w.webContents.send('block:complete', {
                            blockId,
                            exitCode: 0,
                            duration: 50,
                        });
                    });
                }, 100);
            }, 50);
            return { block_id: blockId };

        case 'session.list':
            return { sessions: [{ id: 'default', name: '~/Projects', type: 'terminal' }] };

        case 'ai.suggest_command':
            return { suggestions: [{ text: params.partial_input + ' --help', description: 'Show help' }] };

        case 'workflow.list':
            return { workflows: [] };

        default:
            return {};
    }
}
