import { useState, useRef, useEffect, useCallback, useMemo } from "react";
import { TerminalBlock } from "@/components/TerminalBlock";
import { InputPrompt } from "@/components/InputPrompt";
import { Sidebar } from "@/components/Sidebar";
import { SettingsWindow } from "@/components/SettingsWindow";
import { FileEditor } from "@/components/FileEditor";
import { KeyboardShortcutHandler } from "@/components/KeyboardShortcutHandler";
import { useBlocks } from "@/hooks/useBlocks";
import {
  Plus, Settings, PanelLeft,
  ChevronRight, Square, X,
  Command, Copy, Share2, Trash2, Minimize2, ArrowDown
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { cn } from "@/lib/utils";
// ScrollArea removed - using native scroll
import {
  ContextMenu,
  ContextMenuContent,
  ContextMenuItem,
  ContextMenuTrigger,
  ContextMenuSeparator,
} from "@/components/ui/context-menu";
import { AnimatePresence, motion } from "framer-motion";
import { useToast } from "@/hooks/use-toast";

export default function TerminalPage() {
  const {
    getSessionBlocks,
    getSessionCwd,
    executeCommand,
    refreshCwd,
    removeBlock,
    clearBlocks,
    fetchSessionHistory
  } = useBlocks();
  const { toast } = useToast();
  const [activeSessionId, setActiveSessionId] = useState<string | null>(null);
  const [selectedBlockId, setSelectedBlockId] = useState<string | null>(null);
  const [isSidebarOpen, setIsSidebarOpen] = useState(false);
  const [isSettingsOpen, setIsSettingsOpen] = useState(false);
  const [settingsTab, setSettingsTab] = useState("appearance");

  // File Editor State
  const [editorFile, setEditorFile] = useState<string | null>(null);

  // Decoupled State: sessions (Registry) vs tabs (UI Views)
  const [sessions, setSessions] = useState<{ id: string; name: string }[]>([]);
  const [tabs, setTabs] = useState<{ id: string; label: string; active: boolean }[]>([]);

  const [editingTabId, setEditingTabId] = useState<string | null>(null);
  const [editingTabName, setEditingTabName] = useState("");
  const scrollRef = useRef<HTMLDivElement>(null);
  const messagesEndRef = useRef<HTMLDivElement>(null);
  const editInputRef = useRef<HTMLInputElement>(null);
  const [showJumpButton, setShowJumpButton] = useState(false);

  const activeBlocks = activeSessionId ? getSessionBlocks(activeSessionId) : [];
  // Determine current CWD
  const activeCwd = activeSessionId ? getSessionCwd(activeSessionId) : ".";

  // Registry strictly mirrors what the backend considers persistent
  const sessionsList = useMemo(() => {
    return sessions;
  }, [sessions]);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    setShowJumpButton(false);
  };

  // Scroll detection for Jump to Recent button
  useEffect(() => {
    const el = scrollRef.current;
    if (!el) return;

    const handleScroll = () => {
      const distanceFromBottom = el.scrollHeight - el.scrollTop - el.clientHeight;
      setShowJumpButton(distanceFromBottom > 200);
    };

    el.addEventListener('scroll', handleScroll);
    return () => el.removeEventListener('scroll', handleScroll);
  }, []);

  useEffect(() => {
    scrollToBottom();
  }, [activeBlocks.length, activeBlocks]);

  // Fetch sessions registry from backend
  const refreshSessions = useCallback(async () => {
    if (!window.siCore) return;
    try {
      const list = await window.siCore.listSessions();
      setSessions(list);
    } catch (e) {
      console.error("[TerminalPage] Failed to fetch sessions", e);
    }
  }, []);

  // Initialize Registry and Tabs
  useEffect(() => {
    const init = async () => {
      if (!window.siCore) return;
      try {
        const list = await window.siCore.listSessions();
        setSessions(list);

        if (tabs.length === 0) {
          if (list && list.length > 0) {
            // Preload ALL existing sessions as tabs (or just the first one?)
            // Spec says "preload them from storage and make them available"
            // Let's open at least the first one as an active tab
            const initialTabs = list.map((s: any, idx: number) => ({
              id: s.id,
              label: s.name || "Session",
              active: idx === 0
            }));
            setTabs(initialTabs);
            setActiveSessionId(initialTabs[0].id);
            initialTabs.forEach(t => {
              refreshCwd(t.id);
              fetchSessionHistory(t.id);
            });
          } else {
            // One default "New Session"
            addTab();
          }
        }
      } catch (e) {
        console.error("[TerminalPage] Init failed", e);
      }
    };
    init();
  }, [refreshCwd]); // Only run on mount or core ready

  const handleCommand = (cmd: string) => {
    console.log(`[TerminalPage] handleCommand called: '${cmd}'`);
    if (activeSessionId) {
      console.log(`[TerminalPage] Executing on session ${activeSessionId}`);

      // Calculate terminal dimensions
      const containerWidth = scrollRef.current?.clientWidth || 800;
      const padding = 48; // p-6 on each side
      const availableWidth = containerWidth - padding;
      const cols = Math.floor(availableWidth / 8.5);
      const rows = 24; // Default rows

      // ✅ AUTO-RENAME ON FIRST COMMAND (Session Model Spec §4)
      const activeTab = tabs.find(t => t.id === activeSessionId);
      if (activeTab && activeTab.label === "New Session") {
        const skipCommands = ['cd', 'ls', 'pwd', 'clear', 'exit', 'history', 'echo'];
        const baseCmd = cmd.trim().split(' ')[0];

        if (!skipCommands.includes(baseCmd)) {
          console.log(`[TerminalPage] Auto-renaming "New Session" to "${cmd}"`);
          handleRenameSession(activeSessionId, cmd);
        }
      }

      executeCommand(cmd, activeSessionId, cols, rows);

      // Refresh sessions registry after a command 
      setTimeout(() => refreshSessions(), 500);
    } else {
      console.error("[TerminalPage] ERROR: No active session ID. Command dropped.");
    }
  };

  // Window control handlers (Electron)
  const handleMinimize = () => window.siCore?.minimizeWindow();
  const handleMaximize = () => window.siCore?.maximizeWindow();
  const handleClose = () => window.siCore?.closeWindow();

  const addTab = useCallback(async () => {
    if (!window.siCore) return;
    const { sessionId } = await window.siCore.createSession("New Session");
    setTabs(prev => [...prev.map(t => ({ ...t, active: false })), { id: sessionId, label: "New Session", active: true }]);
    setActiveSessionId(sessionId);
    refreshCwd(sessionId);
  }, [refreshCwd]);

  const closeTab = useCallback((id: string, e: React.MouseEvent | { stopPropagation: () => void }) => {
    e.stopPropagation();
    setTabs(prev => {
      const newTabs = prev.filter(t => t.id !== id);

      if (newTabs.length === 0) {
        // If it was the last tab, create a new one automatically
        setTimeout(() => addTab(), 0);
        return [];
      }

      const wasActive = prev.find(t => t.id === id)?.active;
      if (wasActive && newTabs.length > 0) {
        // Switch to the most practical remaining tab
        const nextTab = newTabs[newTabs.length - 1];
        nextTab.active = true;
        setActiveSessionId(nextTab.id);
        refreshCwd(nextTab.id);
      }
      return newTabs;
    });
    // ✅ NO BACKEND DELETE HERE - purely detaching UI view
    console.log(`[TerminalPage] Detached tab for session ${id}`);
  }, [addTab, refreshCwd]);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key === "k") {
        e.preventDefault();
        setIsSettingsOpen(true);
      }
      if ((e.ctrlKey || e.metaKey) && e.key === "t") {
        e.preventDefault();
        addTab();
      }
      if ((e.ctrlKey || e.metaKey) && e.key === "w") {
        e.preventDefault();
        const activeTab = tabs.find(t => t.active);
        if (activeTab) {
          closeTab(activeTab.id, e);
        }
      }
      if (e.key === "Escape") {
        setIsSettingsOpen(false);
        setIsSidebarOpen(false);
        // Also close file editor via UI if needed, but usually specific close button is better
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [addTab, closeTab, tabs]);

  // Auto-focus input when editing starts
  useEffect(() => {
    if (editingTabId && editInputRef.current) {
      editInputRef.current.focus();
      editInputRef.current.select();
    }
  }, [editingTabId]);

  // Inline Tab Editing Handlers
  const startEditingTab = (tabId: string, currentName: string) => {
    setEditingTabId(tabId);
    setEditingTabName(currentName);
  };

  const saveTabEdit = (tabId: string) => {
    if (editingTabName.trim() && editingTabName !== tabs.find(t => t.id === tabId)?.label) {
      handleRenameSession(tabId, editingTabName.trim());
    }
    setEditingTabId(null);
    setEditingTabName("");
  };

  const cancelTabEdit = () => {
    setEditingTabId(null);
    setEditingTabName("");
  };


  // Session Management Handlers
  const handleRenameSession = async (sessionId: string, newName: string) => {
    try {
      if (!newName || newName.trim() === "") return;

      // ✅ OPTIMISTIC UPDATE - update UI immediately
      console.log(`[TerminalPage] Optimistically renaming session ${sessionId} to "${newName}"`);
      setTabs(prev => prev.map(t =>
        t.id === sessionId ? { ...t, label: newName } : t
      ));

      // Send to backend
      await window.siCore.renameSession(sessionId, newName);
      console.log(`[TerminalPage] Session ${sessionId} renamed to "${newName}" (confirmed)`);

    } catch (e) {
      console.error("[TerminalPage] Failed to rename session", e);

      // ✅ ROLLBACK ON ERROR - fetch server state and restore
      try {
        const sessions = await window.siCore.listSessions();
        const serverName = sessions.find((s: any) => s.id === sessionId)?.name;
        if (serverName) {
          console.log(`[TerminalPage] Rolling back to server name: "${serverName}"`);
          setTabs(prev => prev.map(t =>
            t.id === sessionId ? { ...t, label: serverName } : t
          ));
        }
      } catch (rollbackError) {
        console.error("[TerminalPage] Rollback failed:", rollbackError);
      }
    }
  };

  const handleDeleteSession = async (sessionId: string) => {
    try {
      await window.siCore.deleteSession(sessionId);

      // 1. Remove from registry
      setSessions(prev => prev.filter(s => s.id !== sessionId));

      // 2. Close the tab locally
      setTabs(prev => {
        const newTabs = prev.filter(t => t.id !== sessionId);
        const wasActive = prev.find(t => t.id === sessionId)?.active;

        if (wasActive && newTabs.length > 0) {
          const nextTab = newTabs[newTabs.length - 1];
          nextTab.active = true;
          setActiveSessionId(nextTab.id);
          refreshCwd(nextTab.id);
        } else if (newTabs.length === 0) {
          // If zero sessions left, create one New Session (Spec §7)
          setTimeout(() => addTab(), 0);
        }
        return newTabs;
      });

    } catch (e) {
      console.error("[TerminalPage] Failed to delete session", e);
    }
  };

  const handleClearAllSessions = async () => {
    try {
      // Get list of all sessions first
      const sessionList = await window.siCore.listSessions();
      for (const s of sessionList) {
        await window.siCore.deleteSession(s.id);
      }

      // Clear UI
      setTabs([]);
      setActiveSessionId(null);

      // Create exactly one "New Session" (Spec §7)
      addTab();
    } catch (e) {
      console.error("[TerminalPage] Failed to clear all sessions", e);
    }
  };

  const handleSelectSession = async (sessionId: string) => {
    // Check if we already have a tab for this session
    const existingTab = tabs.find(t => t.id === sessionId);
    if (existingTab) {
      // Switch to existing tab
      setTabs(prev => prev.map(t => ({ ...t, active: t.id === sessionId })));
      setActiveSessionId(sessionId);
      refreshCwd(sessionId);
      fetchSessionHistory(sessionId);
    } else {
      // Create new tab for existing session
      try {
        const sessionList = await window.siCore.listSessions();
        const session = sessionList.find((s: any) => s.id === sessionId);
        if (session) {
          setTabs(prev => [
            ...prev.map(t => ({ ...t, active: false })),
            { id: sessionId, label: session.name, active: true }
          ]);
          setActiveSessionId(sessionId);
          refreshCwd(sessionId);
          fetchSessionHistory(sessionId);
        }
      } catch (e) {
        console.error("[TerminalPage] Failed to select session", e);
      }
    }
  };

  // Mock function for editor file saving
  const handleSaveFile = async (path: string, content: string) => {
    try {
      await window.siCore.writeFile(path, content);
      toast({
        title: "File Saved",
        description: `Successfully saved ${path}`,
        className: "bg-[hsl(var(--card))] border-[hsl(var(--border))] text-[hsl(var(--foreground))]",
      });
    } catch (e) {
      console.error("Failed to save file:", e);
      toast({
        variant: "destructive",
        title: "Save Failed",
        description: e instanceof Error ? e.message : "Unknown error occurred",
      });
    }
  };

  const handleShortcutAction = (actionId: string) => {
    switch (actionId) {
      case "session.new":
        addTab();
        break;
      case "session.close":
        if (activeSessionId) closeTab(activeSessionId, { stopPropagation: () => { } } as any);
        break;
      case "app.sidebar.toggle":
        setIsSidebarOpen(!isSidebarOpen);
        break;
      case "terminal.clear":
        if (activeSessionId) {
          clearBlocks(activeSessionId);
        }
        break;
      case "nav.tab.next":
        setTabs(prev => {
          const activeIdx = prev.findIndex(t => t.active);
          const nextIdx = (activeIdx + 1) % prev.length;
          const targetTab = prev[nextIdx];
          setActiveSessionId(targetTab.id);
          refreshCwd(targetTab.id);
          return prev.map((t, i) => ({ ...t, active: i === nextIdx }));
        });
        break;
      case "nav.tab.prev":
        setTabs(prev => {
          const activeIdx = prev.findIndex(t => t.active);
          const prevIdx = (activeIdx - 1 + prev.length) % prev.length;
          const targetTab = prev[prevIdx];
          setActiveSessionId(targetTab.id);
          refreshCwd(targetTab.id);
          return prev.map((t, i) => ({ ...t, active: i === prevIdx }));
        });
        break;
      default:
        console.log("Unhandled shortcut action:", actionId);
    }
  };

  return (
    <>
      {/* Settings Window - rendered outside main container to avoid z-index clipping */}
      <AnimatePresence>
        {isSettingsOpen && (
          <SettingsWindow
            isOpen={isSettingsOpen}
            onClose={() => setIsSettingsOpen(false)}
            initialTab={settingsTab}
          />
        )}
      </AnimatePresence>

      <div className="flex h-screen w-full bg-background text-foreground overflow-hidden font-sans select-none border border-border rounded-lg shadow-2xl relative">
        <KeyboardShortcutHandler onAction={handleShortcutAction} />

        {/* Window Controls & Tabs Header */}
        <div
          className="absolute top-0 left-0 right-0 h-10 flex items-center bg-background border-b border-border px-2 gap-1 z-30"
          style={{ WebkitAppRegion: 'drag' } as any}
        >
          {/* Sidebar Toggle */}
          <Button
            variant="ghost" size="icon"
            className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
            onClick={() => setIsSidebarOpen(!isSidebarOpen)}
            style={{ WebkitAppRegion: 'no-drag' } as any}
          >
            <PanelLeft className="h-4 w-4" />
          </Button>

          <div className="flex-1 flex items-center gap-1 overflow-x-auto no-scrollbar ml-1" style={{ WebkitAppRegion: 'no-drag' } as any}>
            {tabs.map(tab => (
              <ContextMenu key={tab.id}>
                <ContextMenuTrigger>
                  <div
                    onClick={() => {
                      if (editingTabId !== tab.id) {
                        setTabs(prev => prev.map(t => ({ ...t, active: t.id === tab.id })));
                        setActiveSessionId(tab.id);
                        refreshCwd(tab.id);
                      }
                    }}
                    onDoubleClick={() => startEditingTab(tab.id, tab.label)}
                    className={cn(
                      "h-8 px-3 flex items-center gap-2 rounded-lg text-[12px] font-medium transition-all cursor-pointer whitespace-nowrap group relative",
                      tab.active
                        ? "bg-[hsl(var(--primary)/0.1)] text-[hsl(var(--foreground))] border border-[hsl(var(--primary)/0.2)]"
                        : "text-[hsl(var(--muted-foreground))] hover:bg-[hsl(var(--accent))] hover:text-[hsl(var(--foreground))] border border-transparent"
                    )}
                  >
                    <ChevronRight className="h-3 w-3 opacity-50" />

                    {/* Inline editing or static label */}
                    {editingTabId === tab.id ? (
                      <input
                        ref={editInputRef}
                        type="text"
                        value={editingTabName}
                        onChange={(e) => setEditingTabName(e.target.value)}
                        onBlur={() => saveTabEdit(tab.id)}
                        onKeyDown={(e) => {
                          if (e.key === 'Enter') {
                            e.preventDefault();
                            saveTabEdit(tab.id);
                          } else if (e.key === 'Escape') {
                            e.preventDefault();
                            cancelTabEdit();
                          }
                        }}
                        onClick={(e) => e.stopPropagation()}
                        className="bg-accent/50 border border-primary/30 rounded px-1 py-0.5 text-[11px] outline-none focus:border-primary/60 min-w-[80px]"
                      />
                    ) : (
                      <span>{tab.label}</span>
                    )}

                    {/* Close button */}
                    {editingTabId !== tab.id && (
                      <div
                        className={cn(
                          "h-3 w-3 ml-1 flex items-center justify-center rounded-sm transition-all",
                          tab.active ? "opacity-60 hover:opacity-100" : "opacity-0 group-hover:opacity-60 hover:opacity-100"
                        )}
                        onClick={(e) => {
                          e.stopPropagation();
                          closeTab(tab.id, e);
                        }}
                      >
                        <X className="h-2.5 w-2.5" />
                      </div>
                    )}
                  </div>
                </ContextMenuTrigger>
                <ContextMenuContent className="w-52 bg-popover border-border shadow-lg">
                  <ContextMenuItem
                    className="gap-2 text-[13px]"
                    onClick={() => startEditingTab(tab.id, tab.label)}
                  >
                    Rename
                  </ContextMenuItem>
                  <ContextMenuSeparator className="bg-[hsl(var(--border))]" />
                  <ContextMenuItem
                    className="gap-2 text-[13px] text-[hsl(var(--destructive))] focus:text-[hsl(var(--destructive))] focus:bg-[hsl(var(--destructive)/0.1)]"
                    onClick={() => handleDeleteSession(tab.id)}
                  >
                    Delete Session
                  </ContextMenuItem>
                </ContextMenuContent>
              </ContextMenu>
            ))}
            <Button
              variant="ghost"
              size="icon"
              className="h-8 w-8 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
              onClick={addTab}
            >
              <Plus className="h-4 w-4" />
            </Button>
          </div>

          <div className="flex items-center gap-3" style={{ WebkitAppRegion: 'no-drag' } as any}>
            <div className="flex items-center gap-1">
              <div
                className="h-6 w-6 rounded-full bg-primary flex items-center justify-center text-[10px] font-bold text-primary-foreground border border-border shadow-sm cursor-pointer"
                onClick={() => { setSettingsTab("account"); setIsSettingsOpen(true); }}
              >S</div>
              <Button
                variant="ghost" size="icon"
                className={cn("h-7 w-7 transition-colors", isSettingsOpen ? "text-foreground bg-accent" : "text-muted-foreground hover:text-foreground")}
                onClick={() => { setSettingsTab("appearance"); setIsSettingsOpen(!isSettingsOpen); }}
              >
                <Settings className="h-4 w-4" />
              </Button>
            </div>

            <div className="flex items-center gap-1 pl-2 ml-2 border-l border-border">
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-muted-foreground hover:text-foreground hover:bg-accent"
                onClick={handleMinimize}
                style={{ WebkitAppRegion: 'no-drag' } as any}
              >
                <Minimize2 className="h-3.5 w-3.5" />
              </Button>
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))]"
                onClick={handleMaximize}
                style={{ WebkitAppRegion: 'no-drag' } as any}
              >
                <Square className="h-3 w-3" />
              </Button>
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-muted-foreground hover:text-red-500 hover:bg-red-500/10"
                onClick={handleClose}
                style={{ WebkitAppRegion: 'no-drag' } as any}
              >
                <X className="h-3.5 w-3.5" />
              </Button>
            </div>
          </div>
        </div>

        <Sidebar
          key={activeSessionId}  // ✅ Forces re-mount when session changes
          isCollapsed={!isSidebarOpen}

          currentCwd={activeCwd}
          sessions={sessionsList}  // ✅ Using memoized value
          activeSessionId={activeSessionId || undefined}
          onRenameSession={handleRenameSession}
          onDeleteSession={handleDeleteSession}
          onClearAllSessions={handleClearAllSessions}
          onAddSession={addTab}
          onSelectSession={handleSelectSession}
        />

        <div className={cn(
          "flex-1 flex flex-row min-w-0 relative mt-10 transition-all duration-300 ease-in-out",
          isSidebarOpen ? "ml-64" : "ml-0"
        )}>

          {/* Main Terminal Area */}
          <div className="flex-1 flex flex-col min-w-0">
            <div
              className="flex-1 overflow-y-auto scroll-smooth flex flex-col"
              ref={scrollRef}
              onClick={() => setSelectedBlockId(null)}
            >
              <div className="w-full flex flex-col gap-0 mt-auto terminal-blocks-container">
                {activeBlocks.map((block: any) => (
                  <ContextMenu key={block.id}>
                    <ContextMenuTrigger>
                      <TerminalBlock
                        block={block}
                        isSelected={selectedBlockId === block.id}
                        onClick={(e) => {
                          e.stopPropagation();
                          setSelectedBlockId(block.id);
                        }}
                      />
                    </ContextMenuTrigger>
                    <ContextMenuContent className="w-64 bg-popover border-border text-popover-foreground shadow-2xl">
                      <ContextMenuItem
                        className="gap-2 text-xs"
                        onClick={() => {
                          const text = block.output ? block.output.map((l: any) => l.text).join('\n') : "";
                          navigator.clipboard.writeText(text);
                          toast({ description: "Output copied to clipboard" });
                        }}
                      >
                        <Copy className="h-3 w-3" /> Copy Output
                      </ContextMenuItem>
                      <ContextMenuItem
                        className="gap-2 text-xs"
                        onClick={() => {
                          navigator.clipboard.writeText(block.command || "");
                          toast({ description: "Command copied to clipboard" });
                        }}
                      >
                        <Command className="h-3 w-3" /> Copy Command
                      </ContextMenuItem>
                      <ContextMenuItem
                        className="gap-2 text-xs"
                        onClick={() => {
                          toast({ description: "Share link copied (simulated)" });
                        }}
                      >
                        <Share2 className="h-3 w-3" /> Share Block
                      </ContextMenuItem>
                      <ContextMenuSeparator className="bg-[hsl(var(--border))]" />
                      <ContextMenuItem className="gap-2 text-xs text-red-400 focus:text-red-400 focus:bg-red-400/10" onClick={() => removeBlock(block.id)}>
                        <Trash2 className="h-3 w-3" /> Remove Block
                      </ContextMenuItem>
                    </ContextMenuContent>
                  </ContextMenu>
                ))}
                <div ref={messagesEndRef} />
              </div>
            </div>

            {/* Jump to Recent Button */}
            <AnimatePresence>
              {showJumpButton && (
                <motion.button
                  initial={{ opacity: 0, y: 10 }}
                  animate={{ opacity: 1, y: 0 }}
                  exit={{ opacity: 0, y: 10 }}
                  transition={{ duration: 0.15 }}
                  onClick={scrollToBottom}
                  className="absolute bottom-24 right-4 z-30 flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-[hsl(var(--card))] border border-[hsl(var(--border))] text-[12px] text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))] shadow-lg transition-colors"
                >
                  <ArrowDown className="h-3.5 w-3.5" />
                  Jump to Recent
                </motion.button>
              )}
            </AnimatePresence>

            {/* Fixed Input Area */}
            <div className="w-full z-20 bg-background border-t border-border">
              <InputPrompt
                onCommand={handleCommand}
                cwd={activeCwd}
                onOpenFile={(path) => {
                  setEditorFile(path);
                }}
              />
            </div>
          </div>

          {/* Right Side File Editor */}
          <AnimatePresence>
            {editorFile && (
              <motion.div
                initial={{ width: 0, opacity: 0 }}
                animate={{ width: "50%", opacity: 1 }}
                exit={{ width: 0, opacity: 0 }}
                transition={{ type: "spring", damping: 25, stiffness: 200 }}
                className="h-full border-l border-[hsl(var(--border))] overflow-hidden"
              >
                <FileEditor
                  filePath={editorFile}
                  onClose={() => setEditorFile(null)}
                  onSave={handleSaveFile}
                />
              </motion.div>
            )}
          </AnimatePresence>

        </div>
      </div>
    </>
  );
}
