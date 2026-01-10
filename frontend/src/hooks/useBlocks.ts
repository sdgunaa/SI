import { useState, useEffect, useCallback } from 'react';
import { useSettings } from './useSettings';

export interface OutputChunk {
    id: string;
    text: string;
    type: 'stdout' | 'stderr' | 'info' | 'success' | 'error';
    timestamp?: string;
}

export interface Block {
    id: string;
    sessionId: string;
    command: string;
    timestamp: string;
    directory: string;
    gitBranch?: string;
    output: OutputChunk[];
    status: 'running' | 'success' | 'error';
    duration?: string;
    exitCode?: number;
}

export function useBlocks() {
    const [blocks, setBlocks] = useState<Block[]>([]);
    const [isConnected, setIsConnected] = useState(false);
    const [sessionCwds, setSessionCwds] = useState<Record<string, string>>({});
    const { playSound, showNotification } = useSettings();

    const refreshCwd = useCallback(async (sessionId: string) => {
        if (!window.siCore) return;
        try {
            console.log(`[useBlocks] refreshCwd called for session ${sessionId}`);
            const config = await window.siCore.getSessionConfig(sessionId);
            console.log(`[useBlocks] New CWD for session ${sessionId}:`, config.cwd);
            setSessionCwds(prev => ({ ...prev, [sessionId]: config.cwd }));
        } catch (err) {
            console.error(`Failed to refresh CWD for session ${sessionId}:`, err);
        }
    }, []);

    useEffect(() => {
        // Check if running in Electron
        if (!window.siCore) {
            console.warn('siCore not available - running in browser mode');
            return;
        }

        // Listen for connection status
        const unsubConnection = window.siCore.onConnectionStatus((connected) => {
            setIsConnected(connected);
        });

        // Listen for block output
        const unsubOutput = window.siCore.onBlockOutput((data) => {
            setBlocks(prev => prev.map(block => {
                if (block.id === data.blockId) {
                    return {
                        ...block,
                        output: [
                            ...block.output,
                            {
                                id: `${block.id}_${block.output.length}`,
                                text: data.chunk,
                                type: 'stdout' as const,
                            },
                        ],
                    };
                }
                return block;
            }));
        });

        // Listen for block completion
        const unsubComplete = window.siCore.onBlockComplete((data) => {
            console.log(`[useBlocks] onBlockComplete for blockId: ${data.blockId}, exitCode: ${data.exitCode}`);

            // Play sound and notification
            const isSuccess = data.exitCode === 0;
            if (isSuccess) {
                playSound('success');
            } else {
                playSound('error');
                showNotification("Command Failed", `Block ${data.blockId} exited with code ${data.exitCode}`);
            }

            setBlocks(prev => {
                const updated = prev.map(block => {
                    if (block.id === data.blockId) {
                        return {
                            ...block,
                            status: (data.exitCode === 0 ? 'success' : 'error') as 'success' | 'error',
                            exitCode: data.exitCode,
                            duration: data.duration ? `${(data.duration / 1000).toFixed(3)}s` : '0ms',
                        };
                    }
                    return block;
                });

                // Find the session ID for this block to refresh its CWD
                const block = updated.find(b => b.id === data.blockId);

                // Use explicit sessionId from backend if available, otherwise look up block
                const sessionId = (data as any).sessionId || (block ? block.sessionId : null);

                if (sessionId) {
                    console.log(`[useBlocks] Refreshing CWD for session ${sessionId} after block ${data.blockId}`);
                    refreshCwd(sessionId);
                } else {
                    console.warn(`[useBlocks] Could not determine session ID for block ${data.blockId}`);
                }
                return updated;
            });
        });

        return () => {
            unsubConnection();
            unsubOutput();
            unsubComplete();
        };
    }, [refreshCwd]);

    const executeCommand = useCallback(async (command: string, sessionId: string, cols?: number, rows?: number) => {
        console.log(`executeCommand called with sessionId: ${sessionId}, command: ${command}, size: ${cols}x${rows}`);
        const timestamp = new Date().toLocaleTimeString();

        if (command.trim() === 'clear') {
            console.log(`[useBlocks] Intercepting clear command for session ${sessionId}`);
            clearBlocks(sessionId);
            // Optionally send to backend to record in history, but don't track the block in UI
            if (window.siCore) {
                window.siCore.executeCommand(command, sessionId, cols, rows).catch(err =>
                    console.error("Failed to send clear to backend:", err)
                );
            }
            return;
        }

        // Create optimistic block
        const tempId = `temp_${Date.now()}`;
        const newBlock: Block = {
            id: tempId,
            sessionId,
            command,
            timestamp,
            directory: sessionCwds[sessionId] || '~',
            output: [],
            status: 'running',
        };

        setBlocks(prev => [...prev, newBlock]);

        if (window.siCore) {
            try {
                const result = await window.siCore.executeCommand(command, sessionId, cols, rows);
                console.log('Backend response:', result);
                if (result && result.blockId) {
                    console.log(`Replacing tempId ${tempId} with real blockId: ${result.blockId}`);
                    // Replace temp ID with real block ID
                    setBlocks(prev => prev.map(block =>
                        block.id === tempId ? { ...block, id: result.blockId } : block
                    ));

                    // ✅ PROACTIVE CWD UPDATE - eliminates race condition
                    console.log(`[useBlocks] Checking sessionConfig:`, result.sessionConfig);
                    if (result.sessionConfig?.cwd) {
                        console.log(`[useBlocks] ✅ Proactive CWD update for session ${sessionId}: ${result.sessionConfig.cwd}`);
                        setSessionCwds(prev => ({
                            ...prev,
                            [sessionId]: result.sessionConfig!.cwd
                        }));
                    } else {
                        console.warn(`[useBlocks] ❌ No sessionConfig.cwd in response!`, result);
                    }
                } else {
                    console.error('Backend returned no blockId:', result);
                    throw new Error('No block ID returned from backend');
                }
            } catch (error) {
                console.error('Error executing command:', error);
                // Mark as error
                setBlocks(prev => prev.map(block =>
                    block.id === tempId ? {
                        ...block,
                        status: 'error',
                        output: [{ id: '0', text: `Error: ${error instanceof Error ? error.message : String(error)}`, type: 'error' as const }],
                    } : block
                ));
            }
        } else {
            // Mock mode for browser development
            setTimeout(() => {
                setBlocks(prev => prev.map(block => {
                    if (block.id === tempId) {
                        let output: OutputChunk[] = [];
                        if (command === 'ls') {
                            output = [{ id: '1', text: 'abi.pdf  arms_cache  Desktop  Documents  Downloads  Music  Projects  Public', type: 'stdout' }];
                        } else if (command === 'help') {
                            output = [{ id: '1', text: 'Available commands: ls, help, git status, echo [text]', type: 'info' }];
                        } else if (command.startsWith('echo')) {
                            output = [{ id: '1', text: command.replace('echo ', ''), type: 'stdout' }];
                        } else {
                            output = [{ id: '1', text: `command not found: ${command}`, type: 'error' }];
                        }
                        return {
                            ...block,
                            status: output.find(o => o.type === 'error') ? 'error' : 'success',
                            output,
                            duration: '0.021s',
                        };
                    }
                    return block;
                }));
            }, 300);
        }
    }, [sessionCwds]);

    const fetchSessionHistory = useCallback(async (sessionId: string) => {
        if (!window.siCore) return;
        try {
            console.log(`[useBlocks] Fetching history for session ${sessionId}`);
            const backendBlocks = await window.siCore.listBlocks(sessionId);
            if (!Array.isArray(backendBlocks)) {
                console.warn(`[useBlocks] No history array returned for session ${sessionId}`, backendBlocks);
                refreshCwd(sessionId);
                return;
            }
            console.log(`[useBlocks] Received ${backendBlocks.length} blocks for session ${sessionId}`);

            const mappedBlocks: Block[] = backendBlocks.map((bb: any) => ({
                id: bb.id,
                sessionId: bb.sessionId,
                command: bb.command,
                timestamp: new Date(bb.startTime).toLocaleTimeString(),
                directory: bb.cwd || '~',
                status: bb.state === 1 ? 'success' : (bb.state === 0 ? 'running' : 'error'),
                exitCode: bb.exitCode,
                duration: bb.endTime ? `${((bb.endTime - bb.startTime) / 1000).toFixed(3)}s` : undefined,
                output: (bb.outputChunks || []).map((oc: any, idx: number) => ({
                    id: `${bb.id}_${idx}`,
                    text: oc.data,
                    type: oc.type || 'stdout',
                    timestamp: new Date(oc.ts).toISOString()
                }))
            }));

            setBlocks(prev => {
                // Filter out any existing blocks for this session to avoid duplicates
                const otherBlocks = prev.filter(b => b.sessionId !== sessionId);
                return [...otherBlocks, ...mappedBlocks];
            });

            // Also refresh CWD from history if possible
            if (mappedBlocks.length > 0) {
                const lastBlock = mappedBlocks[mappedBlocks.length - 1];
                setSessionCwds(prev => ({ ...prev, [sessionId]: lastBlock.directory }));
            } else {
                refreshCwd(sessionId);
            }
        } catch (err) {
            console.error(`Failed to fetch history for session ${sessionId}:`, err);
        }
    }, [refreshCwd]);

    const clearBlocks = useCallback((sessionId: string) => {
        setBlocks(prev => prev.filter(b => b.sessionId !== sessionId));
    }, []);

    const removeBlock = useCallback((blockId: string) => {
        setBlocks(prev => prev.filter(b => b.id !== blockId));
    }, []);

    const getSessionBlocks = useCallback((sessionId: string) => {
        return blocks.filter(b => b.sessionId === sessionId);
    }, [blocks]);

    const getSessionCwd = useCallback((sessionId: string) => {
        return sessionCwds[sessionId] || '';
    }, [sessionCwds]);

    return {
        blocks, // Raw blocks if needed
        getSessionBlocks,
        isConnected,
        getSessionCwd,
        executeCommand,
        clearBlocks,
        removeBlock,
        refreshCwd,
        fetchSessionHistory,
    };
}
