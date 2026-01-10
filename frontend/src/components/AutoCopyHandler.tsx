import { useEffect } from 'react';
import { useSettings } from '@/hooks/useSettings';
import { useToast } from '@/hooks/use-toast';

export function AutoCopyHandler() {
    const { settings } = useSettings();
    const { toast } = useToast();

    useEffect(() => {
        if (!settings.features.autoCopyOnSelect) return;

        const handleMouseUp = () => {
            const selection = window.getSelection();
            if (selection && selection.toString().trim().length > 0) {
                navigator.clipboard.writeText(selection.toString());
                toast({
                    description: "Copied to clipboard",
                    duration: 1000,
                });
            }
        };

        document.addEventListener('mouseup', handleMouseUp);
        return () => document.removeEventListener('mouseup', handleMouseUp);
    }, [settings.features.autoCopyOnSelect, toast]);

    return null;
}
