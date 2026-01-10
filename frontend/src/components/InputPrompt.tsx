import { useState, useEffect, useRef } from "react";
import {
  Folder, MessageSquare,
  Image as ImageIcon, Mic, AtSign, ChevronDown,
  Terminal, CornerDownLeft, Sparkles
} from "lucide-react";
import { cn } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import { Popover, PopoverContent, PopoverTrigger } from "@/components/ui/popover";
import { useToast } from "@/hooks/use-toast";

interface InputPromptProps {
  onCommand: (cmd: string) => void;
  cwd: string;
  onNavigatePath?: () => void;
  onOpenHistory?: () => void;
  onOpenFile?: (path: string) => void;
}

export function InputPrompt({
  onCommand,
  cwd,
  onOpenFile,
}: InputPromptProps) {
  const [value, setValue] = useState("");
  const [mode, setMode] = useState<'auto' | 'terminal' | 'ai'>('auto');
  const [isFocused, setIsFocused] = useState(false);
  const textareaRef = useRef<HTMLTextAreaElement>(null);
  const { toast } = useToast();

  // Local display CWD for optimistic updates
  const [displayCwd, setDisplayCwd] = useState(cwd);

  // Sync displayCwd when prop cwd changes from backend
  useEffect(() => {
    setDisplayCwd(cwd);
  }, [cwd]);

  // Path Navigator State
  const [isPathOpen, setIsPathOpen] = useState(false);
  const [pathItems, setPathItems] = useState<Array<{ name: string, is_directory: boolean }>>([]);

  // Auto-resize textarea
  useEffect(() => {
    if (textareaRef.current) {
      textareaRef.current.style.height = 'auto';
      textareaRef.current.style.height = textareaRef.current.scrollHeight + 'px';
    }
  }, [value]);

  // Load path items
  useEffect(() => {
    if (isPathOpen && (window as any).siCore) {
      (window as any).siCore.listFiles(displayCwd).then((items: any) => {
        setPathItems(items.sort((a: any, b: any) => {
          if (a.is_directory && !b.is_directory) return -1;
          if (!a.is_directory && b.is_directory) return 1;
          return a.name.localeCompare(b.name);
        }));
      }).catch((e: any) => console.error("Failed to list files:", e));
    }
  }, [isPathOpen, displayCwd]);

  const handleSubmit = (e?: React.FormEvent) => {
    e?.preventDefault();
    if (value.trim()) {
      onCommand(value);
      setValue("");
      if (textareaRef.current) textareaRef.current.style.height = 'auto';
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSubmit();
    }
  };

  const handlePathItemClick = (item: { name: string, is_directory: boolean }) => {
    const fullPath = displayCwd === '/' ? `/${item.name}` : `${displayCwd}/${item.name}`;
    if (item.is_directory) {
      setDisplayCwd(fullPath);
      onCommand(`cd "${fullPath}"`);
      setIsPathOpen(false);
    } else {
      onOpenFile?.(fullPath);
      setIsPathOpen(false);
    }
  };
  const handleMediaClick = (type: string) => {
    toast({
      title: "Coming Soon",
      description: `${type} input support is in development.`,
    });
  };

  const getPlaceholder = () => {
    switch (mode) {
      case 'ai': return "Ask AI anything...";
      case 'terminal': return "Enter command...";
      default: return "Ask AI or type a command...";
    }
  };

  return (
    <div className="w-full">
      <div
        className={cn(
          "relative flex flex-col border-t border-[hsl(var(--border))] bg-[hsl(var(--card))] transition-all duration-300 group",
          isFocused
            ? "border-[hsl(var(--primary)/0.3)] shadow-[0_0_20px_hsl(var(--primary)/0.05)] ring-1 ring-[hsl(var(--primary)/0.1)]"
            : "hover:border-[hsl(var(--border))]"
        )}
      >
        {/* Top Context Indicators */}
        <div className="flex items-center gap-1.5 px-4 pt-3.5 pb-1">
          <Popover open={isPathOpen} onOpenChange={setIsPathOpen}>
            <PopoverTrigger asChild>
              <button className="flex items-center gap-1.5 h-6 px-2 rounded-md bg-[hsl(var(--muted))] border border-[hsl(var(--border))] text-[10.5px] font-mono text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))] transition-all group/path">
                <Folder className="h-3 w-3 opacity-60 group-hover/path:opacity-100 transition-opacity" />
                <span className="truncate max-w-[400px]">{displayCwd}</span>
              </button>
            </PopoverTrigger>
            <PopoverContent
              className="w-72 p-2 bg-[hsl(var(--popover))] border-[hsl(var(--border))] shadow-2xl max-h-[320px] overflow-y-auto"
              align="start"
            >
              <div className="text-[11px] font-medium text-[hsl(var(--muted-foreground))] px-2 py-1.5 mb-1 border-b border-[hsl(var(--border))]">
                {displayCwd}
              </div>
              <div className="flex flex-col gap-0.5">
                <button
                  className="flex items-center gap-2 px-2 py-2 rounded-md text-[13px] text-[hsl(var(--muted-foreground))] hover:bg-[hsl(var(--accent))] hover:text-[hsl(var(--foreground))] transition-colors"
                  onClick={() => {
                    const parentPath = displayCwd.split('/').slice(0, -1).join('/') || '/';
                    setDisplayCwd(parentPath);
                    onCommand("cd ..");
                    setIsPathOpen(false);
                  }}
                >
                  <CornerDownLeft className="h-4 w-4" />
                  <span>..</span>
                </button>
                {pathItems.map((item, idx) => (
                  <button
                    key={idx}
                    className="flex items-center gap-2 px-2 py-2 rounded-md text-[13px] hover:bg-[hsl(var(--accent))] transition-colors truncate text-left"
                    onClick={() => handlePathItemClick(item)}
                  >
                    {item.is_directory ? (
                      <Folder className="h-4 w-4 text-[hsl(var(--primary))] shrink-0" />
                    ) : (
                      <Terminal className="h-4 w-4 text-[hsl(var(--muted-foreground))] shrink-0" />
                    )}
                    <span className={cn(
                      "truncate",
                      item.is_directory ? "text-[hsl(var(--foreground))]" : "text-[hsl(var(--muted-foreground))]"
                    )}>
                      {item.name}
                    </span>
                  </button>
                ))}
              </div>
            </PopoverContent>
          </Popover>
        </div>

        {/* Text Input Area */}
        <div className="px-4 py-1.5">
          <textarea
            ref={textareaRef}
            value={value}
            onChange={(e) => setValue(e.target.value)}
            onFocus={() => setIsFocused(true)}
            onBlur={() => setIsFocused(false)}
            onKeyDown={handleKeyDown}
            rows={1}
            spellCheck={false}
            className={cn(
              "w-full bg-transparent outline-none resize-none",
              "text-[14px] font-[var(--font-mono)] text-[hsl(var(--foreground))]",
              "placeholder:text-[hsl(var(--muted-foreground))]",
              "max-h-[300px] overflow-y-auto leading-relaxed"
            )}
            placeholder={getPlaceholder()}
            autoFocus
          />
        </div>

        {/* Footer Actions */}
        <div className="flex items-center justify-between px-3.5 pb-3.5 pt-1">
          <div className="flex items-center gap-1">
            {/* Mode Toggle Group */}
            <div className="flex items-center p-0.5 rounded-lg bg-[hsl(var(--muted))] border border-[hsl(var(--border))]">
              <button
                onClick={() => setMode('terminal')}
                className={cn(
                  "h-6 w-7 flex items-center justify-center rounded-md transition-all",
                  mode === 'terminal' || mode === 'auto'
                    ? "bg-[hsl(var(--accent))] text-[hsl(var(--foreground))]"
                    : "text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))]"
                )}
              >
                <Terminal className="h-3.5 w-3.5" />
              </button>
              <button
                onClick={() => setMode('ai')}
                className={cn(
                  "h-6 w-7 flex items-center justify-center rounded-md transition-all",
                  mode === 'ai'
                    ? "bg-gradient-to-br from-indigo-500/80 to-purple-500/80 text-[hsl(var(--foreground))]"
                    : "text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))]"
                )}
              >
                <Sparkles className="h-3.5 w-3.5" />
              </button>
            </div>

            {/* Quick Actions */}
            <div className="flex items-center gap-0 px-1 border-r border-[hsl(var(--border))] mx-1">
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-transparent"
                onClick={() => handleMediaClick("Image")}
              >
                <ImageIcon className="h-3.5 w-3.5" />
              </Button>
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-transparent"
                onClick={() => handleMediaClick("Voice")}
              >
                <Mic className="h-3.5 w-3.5" />
              </Button>
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-transparent"
              >
                <AtSign className="h-3.5 w-3.5" />
              </Button>
              <Button
                variant="ghost" size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-transparent"
              >
                <MessageSquare className="h-3.5 w-3.5" />
              </Button>
            </div>

            {/* Model Selector */}
            <button className="flex items-center gap-1.5 h-7 px-2.5 rounded-lg bg-[hsl(var(--muted))] border border-[hsl(var(--border))] text-[11px] text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))] transition-all">
              <span>auto (responsive)</span>
              <ChevronDown className="h-3 w-3 opacity-50" />
            </button>
          </div>

          <div className="flex items-center gap-2">
            <span className="text-[10px] text-[hsl(var(--muted-foreground))] font-[var(--font-mono)]">⌘ ↵ to send</span>
          </div>
        </div>
      </div>
    </div>
  );
}
