import { useState } from "react";
import { motion, AnimatePresence } from "framer-motion";
import Ansi from "ansi-to-react";
import {
  Copy,
  Share2,
  MoreHorizontal,
  Sparkles,
  Check,
  Terminal,
  GitBranch
} from "lucide-react";
import { cn } from "@/lib/utils";
import { Block } from "@/data/mockData";

import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { Button } from "@/components/ui/button";
import {
  Tooltip,
  TooltipContent,
  TooltipTrigger,
} from "@/components/ui/tooltip";

interface TerminalBlockProps {
  block: Block;
  isSelected?: boolean;
  onClick?: (e: React.MouseEvent) => void;
  onCopyOutput?: () => void;
  onCopyCommand?: () => void;
}

export function TerminalBlock({
  block,
  isSelected,
  onClick,
  onCopyOutput,
  onCopyCommand
}: TerminalBlockProps) {
  const [copiedState, setCopiedState] = useState<'output' | 'command' | null>(null);

  const handleCopy = (type: 'output' | 'command', e?: React.MouseEvent) => {
    e?.stopPropagation();
    const text = type === 'output'
      ? block.output?.map((l: any) => l.text).join('\n') || ""
      : block.command || "";
    navigator.clipboard.writeText(text);
    setCopiedState(type);
    setTimeout(() => setCopiedState(null), 2000);

    if (type === 'output') onCopyOutput?.();
    else onCopyCommand?.();
  };

  const hasOutput = block.output && block.output.length > 0;
  const isError = block.status === "error";

  return (
    <motion.div
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.2, ease: [0.4, 0, 0.2, 1] }}
      className={cn(
        "group relative w-full transition-all duration-200 border-b border-[hsl(var(--border))]",
        "bg-[hsl(var(--card))]",
        isSelected
          ? "bg-[hsl(var(--selection))] border-b-[hsl(var(--primary)/0.2)]"
          : "hover:bg-[hsl(var(--hover))]"
      )}
      onClick={onClick}
    >
      {/* Block Header */}
      <div className="flex items-center justify-between px-4 py-3">
        <div className="flex items-center gap-3 min-w-0 flex-1">

          {/* Command Info */}
          <div className="flex flex-col gap-0.5 min-w-0 flex-1">
            {/* Directory & Git */}
            <div className="flex items-center gap-2 text-[11px] text-[hsl(var(--muted-foreground))] font-[var(--font-mono)]">
              <span className="truncate max-w-[200px]">{block.directory}</span>
              {block.gitBranch && (
                <span className="flex items-center gap-1 text-[hsl(var(--muted-foreground)/0.7)]">
                  <GitBranch className="h-3 w-3" />
                  <span>{block.gitBranch}</span>
                </span>
              )}
            </div>

            {/* Command */}
            <div className="flex items-center gap-2">
              <Terminal className="h-3.5 w-3.5 text-[hsl(var(--muted-foreground)/0.5)] shrink-0" />
              <code className="font-[var(--font-mono)] text-[13px] text-[hsl(var(--foreground))] truncate select-text">
                {block.command}
              </code>
            </div>
          </div>
        </div>

        {/* Action Buttons */}
        <div className={cn(
          "flex items-center gap-0.5 ml-2 transition-opacity duration-150",
          isSelected ? "opacity-100" : "opacity-0 group-hover:opacity-100"
        )}>
          <Tooltip>
            <TooltipTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
                onClick={(e) => handleCopy('output', e)}
              >
                {copiedState === 'output' ? (
                  <Check className="h-3.5 w-3.5 text-[hsl(var(--success))]" />
                ) : (
                  <Copy className="h-3.5 w-3.5" />
                )}
              </Button>
            </TooltipTrigger>
            <TooltipContent side="bottom" className="text-xs">
              Copy output
            </TooltipContent>
          </Tooltip>

          <Tooltip>
            <TooltipTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
                onClick={(e) => {
                  e.stopPropagation();
                  // Share functionality
                }}
              >
                <Share2 className="h-3.5 w-3.5" />
              </Button>
            </TooltipTrigger>
            <TooltipContent side="bottom" className="text-xs">
              Share block
            </TooltipContent>
          </Tooltip>

          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--accent))]"
              >
                <MoreHorizontal className="h-3.5 w-3.5" />
              </Button>
            </DropdownMenuTrigger>
            <DropdownMenuContent
              align="end"
              className="w-52 bg-[hsl(var(--popover))] border-[hsl(var(--border))] shadow-lg"
            >
              <DropdownMenuItem
                className="gap-2 text-[13px] cursor-pointer"
                onClick={(e) => handleCopy('output', e)}
              >
                <Copy className="h-4 w-4" />
                Copy output
              </DropdownMenuItem>
              <DropdownMenuItem
                className="gap-2 text-[13px] cursor-pointer"
                onClick={(e) => handleCopy('command', e)}
              >
                <Terminal className="h-4 w-4" />
                Copy command
              </DropdownMenuItem>
              <DropdownMenuItem className="gap-2 text-[13px] cursor-pointer">
                <Share2 className="h-4 w-4" />
                Share block...
              </DropdownMenuItem>
              <DropdownMenuSeparator />
              <DropdownMenuItem className="gap-2 text-[13px] cursor-pointer text-[hsl(var(--primary))] focus:text-[hsl(var(--primary))]">
                <Sparkles className="h-4 w-4" />
                Ask AI about this
              </DropdownMenuItem>
            </DropdownMenuContent>
          </DropdownMenu>
        </div>
      </div>

      {/* Output Area */}
      <AnimatePresence initial={false}>
        {hasOutput && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: "auto", opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.2, ease: [0.4, 0, 0.2, 1] }}
            className="overflow-hidden"
          >
            <div className="px-4 pb-4 pt-1 font-[var(--font-mono)] text-[13px] leading-[1.6] overflow-x-auto">
              {block.output.map((line) => (
                <div
                  key={line.id}
                  className={cn(
                    "flex gap-4 whitespace-pre-wrap py-px select-text",
                    line.type === "error" ? "text-[hsl(var(--destructive))]" :
                      line.type === "success" ? "text-[hsl(var(--success))]" :
                        line.type === "info" ? "text-[hsl(var(--primary))]" :
                          "text-[hsl(var(--muted-foreground))]"
                  )}
                >
                  <div className="flex-1">
                    <Ansi>{line.text}</Ansi>
                  </div>
                </div>
              ))}
            </div>
          </motion.div>
        )}
      </AnimatePresence>

      {/* AI Action - For error blocks */}
      {isSelected && isError && (
        <div className="px-4 pb-4 pt-2">
          <Button
            variant="secondary"
            className={cn(
              "w-full justify-start h-9 text-[13px] gap-2 font-medium",
              "bg-[hsl(var(--primary)/0.08)] text-[hsl(var(--primary))]",
              "hover:bg-[hsl(var(--primary)/0.12)] border border-[hsl(var(--primary)/0.2)]",
              "transition-all duration-150"
            )}
            onClick={() => (window as any).siCore?.analyzeError?.(block.id, "default")}
          >
            <Sparkles className="h-4 w-4" />
            Analyze error with AI
          </Button>
        </div>
      )}
    </motion.div>
  );
}
