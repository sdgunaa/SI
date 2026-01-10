import { useState, useEffect } from "react";
import { X, Save, FileCode } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Checkbox } from "@/components/ui/checkbox";

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

interface FileEditorProps {
    filePath: string;
    initialContent?: string;
    onClose: () => void;
    onSave: (path: string, content: string) => Promise<void>;
}

export function FileEditor({ filePath, initialContent = "", onClose, onSave }: FileEditorProps) {
    const [content, setContent] = useState(initialContent);
    const [originalContent, setOriginalContent] = useState(initialContent);
    const [isDirty, setIsDirty] = useState(false);
    const [isLoading, setIsLoading] = useState(false);
    const [showCloseDialog, setShowCloseDialog] = useState(false);
    const [dontAsk, setDontAsk] = useState(false);

    useEffect(() => {
        loadContent();
    }, [filePath]);

    useEffect(() => {
        setIsDirty(content !== originalContent);
    }, [content, originalContent]);

    const loadContent = async () => {
        setIsLoading(true);
        try {
            const data = await (window as any).siCore.readFile(filePath);
            setContent(data);
            setOriginalContent(data);
        } catch (e) {
            console.error("Failed to load file:", e);
        } finally {
            setIsLoading(false);
        }
    };

    const handleSave = async () => {
        await onSave(filePath, content);
        setOriginalContent(content);
        setIsDirty(false);
    };

    const handleCloseTrigger = () => {
        if (isDirty) {
            if (localStorage.getItem("si-skip-unsaved-confirm") === "true") {
                onClose();
            } else {
                setShowCloseDialog(true);
            }
        } else {
            onClose();
        }
    };

    return (
        <div className="flex flex-col h-full bg-[hsl(var(--background))] text-[hsl(var(--foreground))]">
            {/* Header */}
            <div className="h-10 flex items-center justify-between px-4 border-b border-[hsl(var(--border))] bg-[hsl(var(--card))]">
                <div className="flex items-center gap-2 text-xs font-[var(--font-mono)] text-[hsl(var(--muted-foreground))]">
                    <FileCode className="h-4 w-4 text-[hsl(var(--primary))]" />
                    <span className="truncate max-w-[300px]" title={filePath}>{filePath}</span>
                    {isDirty && <span className="h-1.5 w-1.5 rounded-full bg-yellow-500" />}
                </div>
                <div className="flex items-center gap-1">
                    <Button
                        variant="ghost" size="sm"
                        className="h-7 px-2 text-xs gap-1.5 hover:bg-[hsl(var(--hover))] text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))]"
                        onClick={handleSave}
                        disabled={!isDirty}
                    >
                        <Save className="h-3.5 w-3.5" />
                        Save
                    </Button>
                    <Button
                        variant="ghost" size="icon"
                        className="h-7 w-7 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--destructive)/0.1)] hover:text-[hsl(var(--destructive))]"
                        onClick={handleCloseTrigger}
                    >
                        <X className="h-4 w-4" />
                    </Button>
                </div>
            </div>

            {/* Editor Area */}
            <div className="flex-1 relative">
                {isLoading ? (
                    <div className="absolute inset-0 flex items-center justify-center text-xs text-[hsl(var(--muted-foreground))]">
                        Loading...
                    </div>
                ) : (
                    <textarea
                        value={content}
                        onChange={(e) => setContent(e.target.value)}
                        className="w-full h-full bg-transparent p-4 font-[var(--font-mono)] text-sm leading-relaxed resize-none outline-none text-[hsl(var(--foreground))] placeholder:text-[hsl(var(--muted-foreground))]"
                        placeholder="Type here..."
                        spellCheck={false}
                    />
                )}
            </div>

            {/* Unsaved Changes Alert */}
            <AlertDialog open={showCloseDialog} onOpenChange={setShowCloseDialog}>
                <AlertDialogContent className="bg-[hsl(var(--popover))] border-[hsl(var(--border))] text-[hsl(var(--foreground))]">
                    <AlertDialogHeader>
                        <AlertDialogTitle>Unsaved Changes</AlertDialogTitle>
                        <AlertDialogDescription className="text-[hsl(var(--muted-foreground))]">
                            You have unsaved changes. Closing this file will discard them.
                        </AlertDialogDescription>
                    </AlertDialogHeader>

                    <div className="flex items-center space-x-2 py-2">
                        <Checkbox
                            id="dont-ask"
                            checked={dontAsk}
                            onCheckedChange={(checked) => setDontAsk(checked as boolean)}
                            className="border-[hsl(var(--border))] data-[state=checked]:bg-[hsl(var(--primary))] data-[state=checked]:border-[hsl(var(--primary))]"
                        />
                        <label
                            htmlFor="dont-ask"
                            className="text-xs text-[hsl(var(--muted-foreground))] cursor-pointer select-none"
                        >
                            Don't ask again
                        </label>
                    </div>

                    <AlertDialogFooter>
                        <AlertDialogCancel className="bg-transparent border-[hsl(var(--border))] text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))] hover:text-[hsl(var(--foreground))]">Cancel</AlertDialogCancel>
                        <AlertDialogAction
                            className="bg-[hsl(var(--destructive)/0.1)] text-[hsl(var(--destructive))] hover:bg-[hsl(var(--destructive)/0.2)] border border-[hsl(var(--destructive)/0.2)]"
                            onClick={() => {
                                if (dontAsk) {
                                    localStorage.setItem("si-skip-unsaved-confirm", "true");
                                }
                                setShowCloseDialog(false);
                                onClose();
                            }}
                        >
                            Discard Changes
                        </AlertDialogAction>
                    </AlertDialogFooter>
                </AlertDialogContent>
            </AlertDialog>
        </div>
    );
}
