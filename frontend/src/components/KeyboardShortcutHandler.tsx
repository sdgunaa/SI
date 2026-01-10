import { useEffect } from 'react';
import { useSettings } from '@/hooks/useSettings';

interface KeyboardShortcutHandlerProps {
    onAction: (actionId: string) => void;
}

export function KeyboardShortcutHandler({ onAction }: KeyboardShortcutHandlerProps) {
    const { settings } = useSettings();

    useEffect(() => {
        const handleKeyDown = (e: KeyboardEvent) => {
            // Don't trigger if user is typing in an input/textarea
            if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) {
                // Allow escape to blur
                if (e.key === 'Escape') {
                    (e.target as HTMLElement).blur();
                }
                return;
            }

            const pressedKeys = [];
            if (e.ctrlKey) pressedKeys.push("Ctrl");
            if (e.altKey) pressedKeys.push("Alt");
            if (e.shiftKey) pressedKeys.push("Shift");
            if (e.metaKey) pressedKeys.push("Meta");

            if (e.key !== "Control" && e.key !== "Alt" && e.key !== "Shift" && e.key !== "Meta") {
                pressedKeys.push(e.key.charAt(0).toUpperCase() + e.key.slice(1));
            }

            const currentBinding = pressedKeys.join("+");
            // console.log("Pressed:", currentBinding);

            const shortcut = settings.keyboard.shortcuts.find(s => s.binding === currentBinding);
            if (shortcut) {
                e.preventDefault();
                onAction(shortcut.id);
            }
        };

        window.addEventListener('keydown', handleKeyDown);
        return () => window.removeEventListener('keydown', handleKeyDown);
    }, [settings.keyboard.shortcuts, onAction]);

    return null;
}
