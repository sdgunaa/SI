import { createContext, useContext, useState, useEffect, useCallback, ReactNode } from 'react';

import { getThemeById, getDefaultTheme, Theme, FONT_FAMILIES } from "@/lib/themes";

// ============================================================================
// SETTINGS TYPES
// ============================================================================

export interface AppSettings {
    appearance: {
        theme: string;
        syncWithOS: boolean;
        windowOpacity: number;
        fontSize: number;
        fontFamily: string;
        cursorType: 'block' | 'line' | 'underline';
    };
    features: {
        soundEffects: boolean;
        notifications: boolean;
        autoCopyOnSelect: boolean;
    };
    privacy: {
        telemetry: boolean;
        crashReports: boolean;
    };
    keyboard: {
        shortcuts: Array<{
            id: string;
            action: string;
            binding: string;
            category: string;
        }>;
    };
}

// ============================================================================
// DEFAULT SETTINGS
// ============================================================================

const DEFAULT_SETTINGS: AppSettings = {
    appearance: {
        theme: "dark-modern",
        syncWithOS: false,
        windowOpacity: 100,
        fontSize: 14,
        fontFamily: "jetbrains-mono",
        cursorType: "block",
    },
    features: {
        soundEffects: false,
        notifications: true,
        autoCopyOnSelect: false,
    },
    privacy: {
        telemetry: false,
        crashReports: true,
    },
    keyboard: {
        shortcuts: [
            { id: "session.new", action: "New Session", binding: "Ctrl+T", category: "Session" },
            { id: "session.close", action: "Close Session", binding: "Ctrl+W", category: "Session" },
            { id: "app.sidebar.toggle", action: "Toggle Sidebar", binding: "Ctrl+B", category: "App" },
            { id: "terminal.clear", action: "Clear Terminal", binding: "Ctrl+L", category: "Terminal" },
            { id: "terminal.history", action: "Search History", binding: "Ctrl+R", category: "Terminal" },
            { id: "nav.tab.next", action: "Next Tab", binding: "Ctrl+PageDown", category: "Navigation" },
            { id: "nav.tab.prev", action: "Previous Tab", binding: "Ctrl+PageUp", category: "Navigation" },
        ],
    },
};

// ============================================================================
// CONTEXT
// ============================================================================

interface SettingsContextType {
    settings: AppSettings;
    updateSetting: <K extends keyof AppSettings>(category: K, key: keyof AppSettings[K], value: any) => void;
    getCurrentTheme: () => Theme;
    playSound: (type: 'success' | 'error' | 'info') => void;
    showNotification: (title: string, body: string) => void;
    loading: boolean;
}

const SettingsContext = createContext<SettingsContextType | undefined>(undefined);

// ============================================================================
// HELPER: Convert hex to HSL string for CSS variables
// ============================================================================

function hexToHslString(hex: string): string {
    hex = hex.replace(/^#/, '');
    const r = parseInt(hex.slice(0, 2), 16) / 255;
    const g = parseInt(hex.slice(2, 4), 16) / 255;
    const b = parseInt(hex.slice(4, 6), 16) / 255;

    const max = Math.max(r, g, b);
    const min = Math.min(r, g, b);
    let h = 0;
    let s = 0;
    const l = (max + min) / 2;

    if (max !== min) {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

        switch (max) {
            case r:
                h = ((g - b) / d + (g < b ? 6 : 0)) / 6;
                break;
            case g:
                h = ((b - r) / d + 2) / 6;
                break;
            case b:
                h = ((r - g) / d + 4) / 6;
                break;
        }
    }

    return `${Math.round(h * 360)} ${Math.round(s * 100)}% ${Math.round(l * 100)}%`;
}

// ============================================================================
// SETTINGS PROVIDER
// ============================================================================

export function SettingsProvider({ children }: { children: ReactNode }) {
    const [settings, setSettings] = useState<AppSettings>(DEFAULT_SETTINGS);
    const [loading, setLoading] = useState(true);

    // Load settings from backend on mount
    useEffect(() => {
        const loadSettings = async () => {
            try {
                if (!(window as any).siCore) {
                    const saved = localStorage.getItem("si-settings");
                    if (saved) {
                        const parsed = JSON.parse(saved);
                        setSettings(prev => ({
                            ...prev,
                            appearance: { ...prev.appearance, ...(parsed.appearance || {}) },
                            features: { ...prev.features, ...(parsed.features || {}) },
                            privacy: { ...prev.privacy, ...(parsed.privacy || {}) },
                            keyboard: { ...prev.keyboard, ...(parsed.keyboard || {}) },
                        }));
                    }
                    setLoading(false);
                    return;
                }

                const [appearance, features, privacy, keyboard] = await Promise.all([
                    (window as any).siCore.settings.get("appearance"),
                    (window as any).siCore.settings.get("features"),
                    (window as any).siCore.settings.get("privacy"),
                    (window as any).siCore.settings.get("keyboard")
                ]);

                setSettings(prev => ({
                    appearance: { ...prev.appearance, ...(appearance || {}) },
                    features: { ...prev.features, ...(features || {}) },
                    privacy: { ...prev.privacy, ...(privacy || {}) },
                    keyboard: { ...prev.keyboard, ...(keyboard || {}) }
                }));
            } catch (error) {
                console.error("Failed to load settings:", error);
            } finally {
                setLoading(false);
            }
        };

        loadSettings();
    }, []);

    // Apply styles when settings change
    useEffect(() => {
        try {
            const root = document.documentElement;
            const appearance = settings.appearance;

            // 1. Get theme colors
            const theme = getThemeById(appearance.theme) || getDefaultTheme();
            const colors = theme.colors;

            // 2. Set CSS variables for colors
            root.style.setProperty('--background', hexToHslString(colors.background));
            root.style.setProperty('--foreground', hexToHslString(colors.foreground));
            root.style.setProperty('--primary', hexToHslString(colors.accent));
            root.style.setProperty('--ring', hexToHslString(colors.accent));
            root.style.setProperty('--selection', hexToHslString(colors.selection));
            root.style.setProperty('--hover', hexToHslString(colors.hover));
            root.style.setProperty('--border', hexToHslString(colors.border));
            root.style.setProperty('--sidebar', hexToHslString(colors.sidebar));
            root.style.setProperty('--sidebar-foreground', hexToHslString(colors.sidebarForeground));
            root.style.setProperty('--card', hexToHslString(colors.card));
            root.style.setProperty('--card-foreground', hexToHslString(colors.foreground));
            root.style.setProperty('--popover', hexToHslString(colors.card));
            root.style.setProperty('--popover-foreground', hexToHslString(colors.foreground));
            root.style.setProperty('--muted', hexToHslString(colors.muted));
            root.style.setProperty('--muted-foreground', hexToHslString(colors.mutedForeground));
            root.style.setProperty('--accent', hexToHslString(colors.hover));
            root.style.setProperty('--accent-foreground', hexToHslString(colors.foreground));
            root.style.setProperty('--destructive', '0 84% 60%');
            root.style.setProperty('--destructive-foreground', '0 0% 100%');

            // 3. Opacity
            root.style.setProperty('--window-opacity', (appearance.windowOpacity / 100).toString());

            // 4. Font Size - apply to base and terminal
            root.style.setProperty('--font-size', `${appearance.fontSize}px`);

            // 5. Font Family
            const fontDef = FONT_FAMILIES.find(f => f.id === appearance.fontFamily);
            root.style.setProperty('--font-mono', fontDef?.value || "'JetBrains Mono', monospace");

            // 6. Cursor Type
            root.style.setProperty('--cursor-type', appearance.cursorType);

            // 7. Theme type for color scheme
            root.style.setProperty('color-scheme', theme.type);

        } catch (error) {
            console.error("Failed to apply theme settings:", error);
        }
    }, [settings.appearance]);

    // Handle OS theme sync
    useEffect(() => {
        if (!settings.appearance.syncWithOS) return;

        const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        const handleChange = (e: MediaQueryListEvent) => {
            const newTheme = e.matches ? "dark-modern" : "light-modern";
            updateSetting("appearance", "theme", newTheme);
        };

        const initialTheme = mediaQuery.matches ? "dark-modern" : "light-modern";
        updateSetting("appearance", "theme", initialTheme);

        mediaQuery.addEventListener('change', handleChange);
        return () => mediaQuery.removeEventListener('change', handleChange);
    }, [settings.appearance.syncWithOS]);

    const updateSetting = useCallback(<K extends keyof AppSettings>(
        category: K,
        key: keyof AppSettings[K],
        value: any
    ) => {
        setSettings(prev => {
            const newSettings = {
                ...prev,
                [category]: {
                    ...prev[category],
                    [key]: value,
                },
            };

            // Persist to backend
            if ((window as any).siCore?.settings) {
                (window as any).siCore.settings.set(category, newSettings[category])
                    .catch((e: any) => console.error(`Failed to save ${category} settings:`, e));
            } else {
                localStorage.setItem("si-settings", JSON.stringify(newSettings));
            }

            return newSettings;
        });
    }, []);

    const getCurrentTheme = useCallback((): Theme => {
        return getThemeById(settings.appearance.theme) || getDefaultTheme();
    }, [settings.appearance.theme]);

    const playSound = useCallback((type: 'success' | 'error' | 'info') => {
        if (!settings.features.soundEffects) return;

        try {
            const ctx = new (window.AudioContext || (window as any).webkitAudioContext)();
            const osc = ctx.createOscillator();
            const gain = ctx.createGain();

            osc.type = 'sine';
            const freq = type === 'success' ? 880 : type === 'error' ? 220 : 440;
            osc.frequency.setValueAtTime(freq, ctx.currentTime);

            gain.gain.setValueAtTime(0.1, ctx.currentTime);
            gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.1);

            osc.connect(gain);
            gain.connect(ctx.destination);

            osc.start();
            osc.stop(ctx.currentTime + 0.1);
        } catch (e) {
            console.error("Failed to play sound:", e);
        }
    }, [settings.features.soundEffects]);

    const showNotification = useCallback((title: string, body: string) => {
        if (!settings.features.notifications) return;

        if (Notification.permission === "granted") {
            new Notification(title, { body });
        } else if (Notification.permission !== "denied") {
            Notification.requestPermission().then(permission => {
                if (permission === "granted") {
                    new Notification(title, { body });
                }
            });
        }
    }, [settings.features.notifications]);

    return (
        <SettingsContext.Provider value={{
            settings,
            updateSetting,
            getCurrentTheme,
            playSound,
            showNotification,
            loading
        }}>
            {children}
        </SettingsContext.Provider>
    );
}

export function useSettings() {
    const context = useContext(SettingsContext);
    if (context === undefined) {
        throw new Error('useSettings must be used within a SettingsProvider');
    }
    return context;
}
