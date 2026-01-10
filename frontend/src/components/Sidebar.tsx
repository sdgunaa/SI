import { cn } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";
import { useState, useEffect } from "react";
import {
  ChevronDown,
  Plus,
  FileText,
  ChevronRight,
  Folder,
  History as HistoryIcon,
  Workflow
} from "lucide-react";
import {
  ContextMenu,
  ContextMenuContent,
  ContextMenuItem,
  ContextMenuTrigger,
} from "@/components/ui/context-menu";

import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
} from "@/components/ui/alert-dialog";
import { Checkbox } from "@/components/ui/checkbox";
import { useToast } from "@/hooks/use-toast";

interface SidebarProps {
  isCollapsed: boolean;
  sessions: { id: string; name: string }[];
  currentCwd?: string;
  activeSessionId?: string;
  onRenameSession?: (sessionId: string, newName: string) => void;
  onDeleteSession?: (sessionId: string) => void;
  onClearAllSessions?: () => void;
  onAddSession?: () => void;
  onSelectSession?: (sessionId: string) => void;
}

interface FileEntry {
  name: string;
  isDirectory: boolean;
  size: number;
  mtime: number;
}

interface WorkflowEntry {
  id: string;
  name: string;
  command?: string;
}

export function Sidebar({
  isCollapsed,
  currentCwd = ".",
  sessions,
  activeSessionId,
  onRenameSession,
  onDeleteSession,
  onClearAllSessions,
  onAddSession,
  onSelectSession
}: SidebarProps) {
  const [files, setFiles] = useState<FileEntry[]>([]);
  const [loading, setLoading] = useState(false);
  const [isProjectExpanded, setIsProjectExpanded] = useState(true);
  const [isSessionsExpanded, setIsSessionsExpanded] = useState(true);
  const [isWorkflowsExpanded, setIsWorkflowsExpanded] = useState(true);
  const { toast } = useToast();
  const [editingSessionId, setEditingSessionId] = useState<string | null>(null);
  const [editingSessionName, setEditingSessionName] = useState("");

  // Workflows with localStorage persistence
  const [workflows, _setWorkflows] = useState<WorkflowEntry[]>(() => {
    const saved = localStorage.getItem("si-workflows");
    return saved ? JSON.parse(saved) : [];
  });

  useEffect(() => {
    localStorage.setItem("si-workflows", JSON.stringify(workflows));
  }, [workflows]);

  // Alert States
  const [sessionToDelete, setSessionToDelete] = useState<string | null>(null);
  const [showClearAllDialog, setShowClearAllDialog] = useState(false);
  const [dontAskDelete, setDontAskDelete] = useState(false);
  const [dontAskClear, setDontAskClear] = useState(false);

  useEffect(() => {
    fetchFiles(currentCwd);
  }, [currentCwd]);

  const startEditingSession = (sessionId: string, currentName: string) => {
    setEditingSessionId(sessionId);
    setEditingSessionName(currentName);
  };

  const saveSessionEdit = (sessionId: string) => {
    if (editingSessionName.trim() && editingSessionName !== sessions.find(s => s.id === sessionId)?.name && onRenameSession) {
      onRenameSession(sessionId, editingSessionName.trim());
    }
    setEditingSessionId(null);
    setEditingSessionName("");
  };

  const cancelSessionEdit = () => {
    setEditingSessionId(null);
    setEditingSessionName("");
  };

  const handleDeleteClick = (sessionId: string) => {
    if (localStorage.getItem("si-skip-delete-confirm") === "true") {
      if (onDeleteSession) onDeleteSession(sessionId);
    } else {
      setSessionToDelete(sessionId);
    }
  };

  const fetchFiles = async (path: string) => {
    if (!path || path === "~") return; // Basic guard, avoid ~ until resolved
    try {
      setLoading(true);
      const result = await (window as any).siCore.listFiles(path);
      setFiles(result);
    } catch (err) {
      console.error("Failed to list files:", err);
    } finally {
      setLoading(false);
    }
  };



  return (
    <div
      className={cn(
        "flex flex-col border-r border-[hsl(var(--border))] bg-[hsl(var(--sidebar))] transition-all duration-200 ease-out z-50 fixed bottom-0 top-10 left-0 w-64 shadow-xl",
        !isCollapsed ? "translate-x-0" : "-translate-x-full"
      )}
    >
      <ScrollArea className="flex-1">
        <div className="py-3 space-y-1">

          {/* Project Explorer - Dynamic */}
          <div className="space-y-0.5">
            <div
              className="px-4 py-2 flex items-center gap-2 text-[11px] font-semibold text-[hsl(var(--muted-foreground))] uppercase tracking-wide cursor-pointer hover:bg-[hsl(var(--accent))] transition-colors rounded-md mx-2"
              onClick={() => setIsProjectExpanded(!isProjectExpanded)}
            >
              {isProjectExpanded ? <ChevronDown className="h-4 w-4" /> : <ChevronRight className="h-4 w-4" />}
              <Folder className="h-4 w-4 text-[hsl(var(--primary))]" />
              <span className="truncate">{currentCwd.split('/').pop() || "project"}</span>
            </div>
            {isProjectExpanded && (
              <div className="ml-6 pl-3 border-l border-[hsl(var(--border))] space-y-0.5">
                {files.map((file) => (
                  <FileTreeItem
                    key={file.name}
                    file={file}
                    path={`${currentCwd}/${file.name}`}
                    depth={1}
                  />
                ))}
                {loading && files.length === 0 && (
                  <div className="px-4 py-2 text-[10px] text-muted-foreground animate-pulse">Loading files...</div>
                )}
              </div>
            )}
          </div>

          {/* Workflows Section */}
          <div className="space-y-0.5">
            <div
              className="px-4 py-2 flex items-center justify-between text-[11px] font-semibold text-[hsl(var(--muted-foreground))] uppercase tracking-wide cursor-pointer hover:bg-[hsl(var(--accent))] transition-colors rounded-md mx-2 group"
              onClick={() => setIsWorkflowsExpanded(!isWorkflowsExpanded)}
            >
              <div className="flex items-center gap-2">
                {isWorkflowsExpanded ? <ChevronDown className="h-4 w-4" /> : <ChevronRight className="h-4 w-4" />}
                <Workflow className="h-4 w-4" />
                <span>Workflows</span>
              </div>
              <Plus
                className="h-4 w-4 cursor-pointer hover:text-[hsl(var(--foreground))] opacity-0 group-hover:opacity-100 transition-opacity"
                onClick={(e) => {
                  e.stopPropagation();
                  toast({
                    title: "Coming Soon",
                    description: "Workflow creation is in development.",
                  });
                }}
              />
            </div>
            {isWorkflowsExpanded && (
              <div className="ml-6 pl-3 border-l border-[hsl(var(--border))] space-y-0.5">
                {workflows.length === 0 ? (
                  <div className="py-2 px-2 text-[11px] text-[hsl(var(--muted-foreground))] italic">
                    No workflows yet
                  </div>
                ) : (
                  workflows.map((wf) => (
                    <div
                      key={wf.id}
                      className="h-8 px-3 flex items-center gap-2 rounded-md text-[12px] text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))] cursor-pointer transition-all"
                      onClick={() => {
                        toast({
                          title: "Workflow",
                          description: `Running: ${wf.name}`,
                        });
                      }}
                    >
                      <Workflow className="h-3.5 w-3.5" />
                      <span className="truncate">{wf.name}</span>
                    </div>
                  ))
                )}
              </div>
            )}
          </div>

          {/* Sessions Section */}
          <div className="space-y-0.5">
            <div
              className="px-4 py-2 flex items-center justify-between text-[11px] font-semibold text-[hsl(var(--muted-foreground))] uppercase tracking-wide cursor-pointer hover:bg-[hsl(var(--accent))] transition-colors rounded-md mx-2 group"
              onClick={() => setIsSessionsExpanded(!isSessionsExpanded)}
            >
              <div className="flex items-center gap-2">
                {isSessionsExpanded ? <ChevronDown className="h-4 w-4" /> : <ChevronRight className="h-4 w-4" />}
                <HistoryIcon className="h-4 w-4" />
                <span>History</span>
              </div>
              <div className="flex items-center gap-2 opacity-0 group-hover:opacity-100 transition-opacity">
                <Plus
                  className="h-3 w-3 cursor-pointer hover:text-[hsl(var(--foreground))]"
                  onClick={(e) => {
                    e.stopPropagation();
                    if (onAddSession) onAddSession();
                  }}
                />
                <span
                  className="hover:text-[hsl(var(--destructive))] cursor-pointer text-[8px] border border-[hsl(var(--border))] px-1 rounded hover:border-[hsl(var(--destructive)/0.5)]"
                  title="Permamently delete all sessions"
                  onClick={(e) => {
                    e.stopPropagation();
                    if (localStorage.getItem("si-skip-clear-confirm") === "true") {
                      if (onClearAllSessions) onClearAllSessions();
                    } else {
                      setShowClearAllDialog(true);
                    }
                  }}
                >
                  Clear All
                </span>
              </div>
            </div>
            {isSessionsExpanded && (
              <div className="ml-6 pl-3 border-l border-[hsl(var(--border))] space-y-0.5">
                {sessions.map((s) => (
                  <ContextMenu key={s.id}>
                    <ContextMenuTrigger>
                      <SessionItem
                        sessionId={s.id}
                        name={s.name}
                        onClick={() => onSelectSession?.(s.id)}
                        active={s.id === activeSessionId}
                        isEditing={editingSessionId === s.id}
                        editingName={editingSessionName}
                        onStartEdit={() => startEditingSession(s.id, s.name)}
                        onNameChange={setEditingSessionName}
                        onSaveEdit={() => saveSessionEdit(s.id)}
                        onCancelEdit={cancelSessionEdit}
                      />
                    </ContextMenuTrigger>
                    <ContextMenuContent className="bg-[hsl(var(--popover))] border-[hsl(var(--border))] text-[hsl(var(--foreground))]">
                      <ContextMenuItem onClick={() => startEditingSession(s.id, s.name)} className="text-[13px]">
                        Rename
                      </ContextMenuItem>
                      <ContextMenuItem onClick={() => handleDeleteClick(s.id)} className="text-[13px] text-[hsl(var(--destructive))] focus:text-[hsl(var(--destructive))]">
                        Delete
                      </ContextMenuItem>
                    </ContextMenuContent>
                  </ContextMenu>
                ))}
                {sessions.length === 0 && (
                  <div className="px-5 py-1 text-[10px] text-muted-foreground/50 italic">No active sessions</div>
                )}
              </div>
            )}
          </div>
        </div>




      </ScrollArea >

      {/* Delete Single Session Alert */}
      < AlertDialog open={!!sessionToDelete
      } onOpenChange={(open) => !open && setSessionToDelete(null)}>
        <AlertDialogContent className="bg-popover border-border text-popover-foreground">
          <AlertDialogHeader>
            <AlertDialogTitle>Delete Session?</AlertDialogTitle>
            <AlertDialogDescription className="text-muted-foreground">
              Are you sure you want to delete this session? This action cannot be undone.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <div className="flex items-center space-x-2 py-2">
            <Checkbox
              id="dont-ask-delete"
              checked={dontAskDelete}
              onCheckedChange={(checked) => setDontAskDelete(checked as boolean)}
              className="border-border data-[state=checked]:bg-primary data-[state=checked]:border-primary"
            />
            <label
              htmlFor="dont-ask-delete"
              className="text-xs text-muted-foreground cursor-pointer select-none"
            >
              Don't ask again
            </label>
          </div>
          <AlertDialogFooter>
            <AlertDialogCancel className="bg-transparent border-border text-foreground hover:bg-accent hover:text-foreground">Cancel</AlertDialogCancel>
            <AlertDialogAction
              className="bg-destructive/10 text-destructive hover:bg-destructive/20 border border-destructive/20"
              onClick={() => {
                if (sessionToDelete && onDeleteSession) {
                  onDeleteSession(sessionToDelete);
                }
                if (dontAskDelete) {
                  localStorage.setItem("si-skip-delete-confirm", "true");
                }
                setSessionToDelete(null);
              }}
            >
              Delete
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog >

      {/* Clear All Sessions Alert */}
      < AlertDialog open={showClearAllDialog} onOpenChange={setShowClearAllDialog} >
        <AlertDialogContent className="bg-popover border-border text-popover-foreground">
          <AlertDialogHeader>
            <AlertDialogTitle>Clear All Sessions?</AlertDialogTitle>
            <AlertDialogDescription className="text-muted-foreground">
              This will permanently delete ALL sessions and command history. This action cannot be undone.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <div className="flex items-center space-x-2 py-2">
            <Checkbox
              id="dont-ask-clear"
              checked={dontAskClear}
              onCheckedChange={(checked) => setDontAskClear(checked as boolean)}
              className="border-border data-[state=checked]:bg-primary data-[state=checked]:border-primary"
            />
            <label
              htmlFor="dont-ask-clear"
              className="text-xs text-muted-foreground cursor-pointer select-none"
            >
              Don't ask again
            </label>
          </div>
          <AlertDialogFooter>
            <AlertDialogCancel className="bg-transparent border-border text-foreground hover:bg-accent hover:text-foreground">Cancel</AlertDialogCancel>
            <AlertDialogAction
              className="bg-destructive/10 text-destructive hover:bg-destructive/20 border border-destructive/20"
              onClick={() => {
                if (onClearAllSessions) {
                  onClearAllSessions();
                }
                if (dontAskClear) {
                  localStorage.setItem("si-skip-clear-confirm", "true");
                }
                setShowClearAllDialog(false);
              }}
            >
              Clear All
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog >
    </div >
  );
}



function FileTreeItem({ file, path, depth }: { file: FileEntry, path: string, depth: number }) {
  const [isOpen, setIsOpen] = useState(false);
  const [subFiles, setSubFiles] = useState<FileEntry[]>([]);

  const toggle = async () => {
    if (!file.isDirectory) return;

    if (!isOpen && subFiles.length === 0) {
      if (!path) return;
      try {
        const result = await (window as any).siCore.listFiles(path);
        setSubFiles(result);
      } catch (err) {
        console.error("Failed to list sub-files:", err);
      }
    }
    setIsOpen(!isOpen);
  };

  return (
    <div>
      <div
        className={cn(
          "flex items-center gap-1.5 px-2 py-0.5 cursor-pointer text-[13px] transition-colors",
          "hover:bg-[hsl(var(--hover))]",
          "text-[hsl(var(--sidebar-foreground))]",
          "font-[var(--font-mono)]"
        )}
        style={{ paddingLeft: `${depth * 12 + 8}px` }}
        onClick={toggle}
      >
        {file.isDirectory ? (
          isOpen ? <ChevronDown className="h-3.5 w-3.5 shrink-0 text-[hsl(var(--muted-foreground))]" /> : <ChevronRight className="h-3.5 w-3.5 shrink-0 text-[hsl(var(--muted-foreground))]" />
        ) : (
          <div className="w-3.5" />
        )}
        {file.isDirectory ? (
          <Folder className="h-4 w-4 shrink-0 text-amber-500" />
        ) : (
          <FileText className="h-4 w-4 shrink-0 text-[hsl(var(--muted-foreground))]" />
        )}
        <span className="truncate">{file.name}</span>
      </div>

      {isOpen && file.isDirectory && (
        <div>
          {subFiles.map((subFile) => (
            <FileTreeItem
              key={subFile.name}
              file={subFile}
              path={`${path}/${subFile.name}`}
              depth={depth + 1}
            />
          ))}
        </div>
      )}
    </div>
  );
}



function SessionItem({
  name,
  branch,
  active,
  isEditing,
  editingName,
  onStartEdit,
  onNameChange,
  onSaveEdit,
  onCancelEdit,
  onClick
}: {
  sessionId: string;
  name: string;
  branch?: string;
  active?: boolean;
  isEditing?: boolean;
  editingName?: string;
  onStartEdit?: () => void;
  onNameChange?: (name: string) => void;
  onSaveEdit?: () => void;
  onCancelEdit?: () => void;
  onClick?: () => void;
}) {
  return (
    <Button
      variant="ghost"
      onClick={onClick}
      onDoubleClick={onStartEdit}
      className={cn(
        "w-full h-8 flex items-center gap-2 justify-start px-3 mb-0.5 font-normal rounded-lg transition-all duration-150",
        active
          ? "bg-[hsl(var(--primary)/0.12)] text-[hsl(var(--foreground))] shadow-[inset_0_0_0_1px_hsl(var(--primary)/0.2)]"
          : "text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
      )}
    >
      {/* Active indicator dot */}
      {active && (
        <span className="w-1.5 h-1.5 rounded-full bg-[hsl(var(--primary))] shrink-0" />
      )}

      {isEditing ? (
        <input
          type="text"
          value={editingName}
          onChange={(e) => onNameChange?.(e.target.value)}
          onBlur={onSaveEdit}
          onKeyDown={(e) => {
            if (e.key === 'Enter') {
              e.preventDefault();
              onSaveEdit?.();
            } else if (e.key === 'Escape') {
              e.preventDefault();
              onCancelEdit?.();
            }
          }}
          onClick={(e) => e.stopPropagation()}
          autoFocus
          className="flex-1 bg-[hsl(var(--muted))] border border-[hsl(var(--primary)/0.3)] rounded px-2 py-0.5 text-[12px] font-mono outline-none focus:border-[hsl(var(--primary)/0.6)]"
        />
      ) : (
        <span className="truncate flex-1 text-left text-[12px]">{name}</span>
      )}

      {branch && <span className="text-[9px] px-1.5 py-0.5 rounded-full bg-[hsl(var(--muted))] text-[hsl(var(--muted-foreground))] flex items-center gap-1">
        <span className="w-1 h-1 rounded-full bg-green-500"></span>
        {branch}
      </span>}
    </Button>
  );
}
