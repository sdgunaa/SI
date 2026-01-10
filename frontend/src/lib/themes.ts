// ============================================================================
// SI Terminal Theme Library
// Full VS Code-style theme collection with complete color palettes
// ============================================================================

export interface Theme {
    id: string;
    name: string;
    type: 'dark' | 'light';
    colors: {
        background: string;
        foreground: string;
        accent: string;
        selection: string;
        hover: string;
        border: string;
        sidebar: string;
        sidebarForeground: string;
        card: string;
        muted: string;
        mutedForeground: string;
    };
}

export const THEMES: Theme[] = [
    // === DARK THEMES ===
    {
        id: "dark-modern",
        name: "Dark Modern",
        type: "dark",
        colors: {
            background: "#1e1e1e",
            foreground: "#cccccc",
            accent: "#0078d4",
            selection: "#264f78",
            hover: "#2a2d2e",
            border: "#3c3c3c",
            sidebar: "#252526",
            sidebarForeground: "#cccccc",
            card: "#252526",
            muted: "#3c3c3c",
            mutedForeground: "#808080",
        },
    },
    {
        id: "dark-plus",
        name: "Dark+",
        type: "dark",
        colors: {
            background: "#1e1e1e",
            foreground: "#d4d4d4",
            accent: "#569cd6",
            selection: "#264f78",
            hover: "#2a2d2e",
            border: "#404040",
            sidebar: "#252526",
            sidebarForeground: "#cccccc",
            card: "#252526",
            muted: "#333333",
            mutedForeground: "#6a9955",
        },
    },
    {
        id: "dark-high-contrast",
        name: "Dark High Contrast",
        type: "dark",
        colors: {
            background: "#000000",
            foreground: "#ffffff",
            accent: "#1aebff",
            selection: "#0f4a85",
            hover: "#001f33",
            border: "#6fc3df",
            sidebar: "#000000",
            sidebarForeground: "#ffffff",
            card: "#0a0a0a",
            muted: "#1a1a1a",
            mutedForeground: "#ffffff",
        },
    },
    {
        id: "monokai",
        name: "Monokai",
        type: "dark",
        colors: {
            background: "#272822",
            foreground: "#f8f8f2",
            accent: "#f92672",
            selection: "#49483e",
            hover: "#3e3d32",
            border: "#75715e",
            sidebar: "#1e1f1c",
            sidebarForeground: "#f8f8f2",
            card: "#272822",
            muted: "#3e3d32",
            mutedForeground: "#75715e",
        },
    },
    {
        id: "dracula",
        name: "Dracula",
        type: "dark",
        colors: {
            background: "#282a36",
            foreground: "#f8f8f2",
            accent: "#bd93f9",
            selection: "#44475a",
            hover: "#343746",
            border: "#6272a4",
            sidebar: "#21222c",
            sidebarForeground: "#f8f8f2",
            card: "#282a36",
            muted: "#44475a",
            mutedForeground: "#6272a4",
        },
    },
    {
        id: "nord",
        name: "Nord",
        type: "dark",
        colors: {
            background: "#2e3440",
            foreground: "#eceff4",
            accent: "#88c0d0",
            selection: "#434c5e",
            hover: "#3b4252",
            border: "#4c566a",
            sidebar: "#2e3440",
            sidebarForeground: "#d8dee9",
            card: "#3b4252",
            muted: "#434c5e",
            mutedForeground: "#d8dee9",
        },
    },
    {
        id: "one-dark-pro",
        name: "One Dark Pro",
        type: "dark",
        colors: {
            background: "#282c34",
            foreground: "#abb2bf",
            accent: "#61afef",
            selection: "#3e4451",
            hover: "#2c313c",
            border: "#3e4451",
            sidebar: "#21252b",
            sidebarForeground: "#abb2bf",
            card: "#21252b",
            muted: "#3e4451",
            mutedForeground: "#5c6370",
        },
    },
    {
        id: "abyss",
        name: "Abyss",
        type: "dark",
        colors: {
            background: "#000c18",
            foreground: "#6688cc",
            accent: "#384887",
            selection: "#770811",
            hover: "#082050",
            border: "#384887",
            sidebar: "#000c18",
            sidebarForeground: "#6688cc",
            card: "#000c18",
            muted: "#082050",
            mutedForeground: "#384887",
        },
    },
    {
        id: "solarized-dark",
        name: "Solarized Dark",
        type: "dark",
        colors: {
            background: "#002b36",
            foreground: "#839496",
            accent: "#268bd2",
            selection: "#073642",
            hover: "#073642",
            border: "#073642",
            sidebar: "#002b36",
            sidebarForeground: "#93a1a1",
            card: "#073642",
            muted: "#073642",
            mutedForeground: "#657b83",
        },
    },
    {
        id: "kimbie-dark",
        name: "Kimbie Dark",
        type: "dark",
        colors: {
            background: "#221a0f",
            foreground: "#d3af86",
            accent: "#dc3958",
            selection: "#84613d",
            hover: "#362712",
            border: "#5e452b",
            sidebar: "#221a0f",
            sidebarForeground: "#d3af86",
            card: "#362712",
            muted: "#5e452b",
            mutedForeground: "#a57a4c",
        },
    },
    {
        id: "tomorrow-night",
        name: "Tomorrow Night",
        type: "dark",
        colors: {
            background: "#1d1f21",
            foreground: "#c5c8c6",
            accent: "#81a2be",
            selection: "#373b41",
            hover: "#282a2e",
            border: "#373b41",
            sidebar: "#1d1f21",
            sidebarForeground: "#c5c8c6",
            card: "#282a2e",
            muted: "#373b41",
            mutedForeground: "#969896",
        },
    },
    {
        id: "github-dark",
        name: "GitHub Dark",
        type: "dark",
        colors: {
            background: "#0d1117",
            foreground: "#c9d1d9",
            accent: "#58a6ff",
            selection: "#3fb950",
            hover: "#161b22",
            border: "#30363d",
            sidebar: "#010409",
            sidebarForeground: "#c9d1d9",
            card: "#161b22",
            muted: "#21262d",
            mutedForeground: "#8b949e",
        },
    },

    // === LIGHT THEMES ===
    {
        id: "light-modern",
        name: "Light Modern",
        type: "light",
        colors: {
            background: "#ffffff",
            foreground: "#3b3b3b",
            accent: "#0078d4",
            selection: "#add6ff",
            hover: "#e8e8e8",
            border: "#e5e5e5",
            sidebar: "#f3f3f3",
            sidebarForeground: "#616161",
            card: "#f5f5f5",
            muted: "#f0f0f0",
            mutedForeground: "#616161",
        },
    },
    {
        id: "light-plus",
        name: "Light+",
        type: "light",
        colors: {
            background: "#ffffff",
            foreground: "#000000",
            accent: "#0000ff",
            selection: "#add6ff",
            hover: "#e8e8e8",
            border: "#e7e7e7",
            sidebar: "#f3f3f3",
            sidebarForeground: "#333333",
            card: "#fffffe",
            muted: "#e7e7e7",
            mutedForeground: "#008000",
        },
    },
    {
        id: "light-high-contrast",
        name: "Light High Contrast",
        type: "light",
        colors: {
            background: "#ffffff",
            foreground: "#000000",
            accent: "#0f4a85",
            selection: "#0f4a85",
            hover: "#b5d5ff",
            border: "#292929",
            sidebar: "#ffffff",
            sidebarForeground: "#000000",
            card: "#ffffff",
            muted: "#cecece",
            mutedForeground: "#000000",
        },
    },
    {
        id: "solarized-light",
        name: "Solarized Light",
        type: "light",
        colors: {
            background: "#fdf6e3",
            foreground: "#657b83",
            accent: "#268bd2",
            selection: "#eee8d5",
            hover: "#eee8d5",
            border: "#eee8d5",
            sidebar: "#fdf6e3",
            sidebarForeground: "#586e75",
            card: "#eee8d5",
            muted: "#eee8d5",
            mutedForeground: "#93a1a1",
        },
    },
    {
        id: "quiet-light",
        name: "Quiet Light",
        type: "light",
        colors: {
            background: "#f5f5f5",
            foreground: "#333333",
            accent: "#4b69c6",
            selection: "#c9d0d9",
            hover: "#e4e6e6",
            border: "#e0e0e0",
            sidebar: "#f5f5f5",
            sidebarForeground: "#333333",
            card: "#ffffff",
            muted: "#e0e0e0",
            mutedForeground: "#aaaaaa",
        },
    },
    {
        id: "github-light",
        name: "GitHub Light",
        type: "light",
        colors: {
            background: "#ffffff",
            foreground: "#24292f",
            accent: "#0969da",
            selection: "#ddf4ff",
            hover: "#f6f8fa",
            border: "#d0d7de",
            sidebar: "#f6f8fa",
            sidebarForeground: "#24292f",
            card: "#ffffff",
            muted: "#f6f8fa",
            mutedForeground: "#57606a",
        },
    },
];

// Font family options for terminal and explorer
export const FONT_FAMILIES = [
    { id: "jetbrains-mono", name: "JetBrains Mono", value: "'JetBrains Mono', monospace" },
    { id: "fira-code", name: "Fira Code", value: "'Fira Code', monospace" },
    { id: "sf-mono", name: "SF Mono", value: "'SF Mono', monospace" },
    { id: "droid-sans-mono", name: "Droid Sans Mono", value: "'Droid Sans Mono', monospace" },
    { id: "consolas", name: "Consolas", value: "'Consolas', monospace" },
    { id: "monaco", name: "Monaco", value: "'Monaco', monospace" },
    { id: "source-code-pro", name: "Source Code Pro", value: "'Source Code Pro', monospace" },
    { id: "ubuntu-mono", name: "Ubuntu Mono", value: "'Ubuntu Mono', monospace" },
    { id: "menlo", name: "Menlo", value: "'Menlo', monospace" },
    { id: "cascadia-code", name: "Cascadia Code", value: "'Cascadia Code', monospace" },
];

// Cursor type options
export const CURSOR_TYPES = [
    { id: "block", name: "Block", value: "block" },
    { id: "line", name: "Line", value: "line" },
    { id: "underline", name: "Underline", value: "underline" },
];

// Helper: Get theme by ID
export function getThemeById(id: string): Theme | undefined {
    return THEMES.find(t => t.id === id);
}

// Helper: Get default theme
export function getDefaultTheme(): Theme {
    return THEMES[0];
}

// Legacy export for backward compatibility
export const LEGACY_THEMES = THEMES.map(t => ({
    id: t.id,
    name: t.name,
    bg: t.colors.background,
    fg: t.colors.foreground,
    accent: t.colors.accent,
}));
