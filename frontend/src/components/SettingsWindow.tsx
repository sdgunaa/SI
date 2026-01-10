import { motion } from "framer-motion";
import {
  X, Search, User, Palette, Keyboard, Zap,
  Shield, Info, Check, ChevronRight, Plus, Pencil, Trash2, Save
} from "lucide-react";
import { cn } from "@/lib/utils";
import { Switch } from "@/components/ui/switch";
import { Slider } from "@/components/ui/slider";
import { useState } from "react";
import { useToast } from "@/hooks/use-toast";
import { useSettings, AppSettings } from "@/hooks/useSettings";
import { THEMES, FONT_FAMILIES, CURSOR_TYPES, Theme } from "@/lib/themes";

interface SettingsWindowProps {
  isOpen: boolean;
  onClose: () => void;
  initialTab?: string;
}

const SETTINGS_TABS = [
  { id: "appearance", label: "Appearance", icon: Palette },
  { id: "features", label: "Features", icon: Zap },
  { id: "keys", label: "Keyboard", icon: Keyboard },
  { id: "privacy", label: "Privacy", icon: Shield },
  { id: "account", label: "Account", icon: User },
  { id: "about", label: "About", icon: Info },
];

export function SettingsWindow({ isOpen, onClose, initialTab = "appearance" }: SettingsWindowProps) {
  const [currentTab, setCurrentTab] = useState(initialTab);
  const { settings, updateSetting } = useSettings();
  const [searchQuery, setSearchQuery] = useState("");

  if (!isOpen) return null;

  const filteredTabs = searchQuery
    ? SETTINGS_TABS.filter(tab =>
      tab.label.toLowerCase().includes(searchQuery.toLowerCase())
    )
    : SETTINGS_TABS;

  return (
    <motion.div
      initial={{ opacity: 0, scale: 0.96 }}
      animate={{ opacity: 1, scale: 1 }}
      exit={{ opacity: 0, scale: 0.96 }}
      transition={{ duration: 0.15, ease: [0.4, 0, 0.2, 1] }}
      className="fixed inset-6 z-50 flex overflow-hidden rounded-xl border border-[hsl(var(--border))] bg-[hsl(var(--background))] shadow-2xl"
    >
      {/* Settings Navigation - Fixed Width */}
      <div className="w-52 shrink-0 border-r border-[hsl(var(--border))] flex flex-col bg-[hsl(var(--card))]">
        <div className="p-4 border-b border-[hsl(var(--border))] flex items-center justify-between">
          <span className="text-sm font-semibold text-[hsl(var(--foreground))]">Settings</span>
          <button
            onClick={onClose}
            className="h-6 w-6 flex items-center justify-center rounded-md text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))] hover:bg-[hsl(var(--hover))] transition-colors"
          >
            <X className="h-4 w-4" />
          </button>
        </div>
        <div className="p-3 border-b border-[hsl(var(--border))]">
          <div className="relative">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-[hsl(var(--muted-foreground))]" />
            <input
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-[hsl(var(--muted))] border border-[hsl(var(--border))] rounded-lg py-2 pl-10 pr-3 text-[13px] outline-none focus:border-[hsl(var(--primary))] transition-colors"
              placeholder="Search"
            />
          </div>
        </div>
        <div className="flex-1 overflow-y-auto p-2 space-y-0.5">
          {filteredTabs.map(tab => (
            <button
              key={tab.id}
              onClick={() => setCurrentTab(tab.id)}
              className={cn(
                "w-full flex items-center gap-3 px-3 py-2.5 rounded-lg text-[13px] font-medium transition-all",
                currentTab === tab.id
                  ? "bg-[hsl(var(--selection))] text-[hsl(var(--foreground))]"
                  : "text-[hsl(var(--muted-foreground))] hover:bg-[hsl(var(--hover))] hover:text-[hsl(var(--foreground))]"
              )}
            >
              <tab.icon className="h-4 w-4" />
              {tab.label}
              {currentTab === tab.id && (
                <ChevronRight className="h-4 w-4 ml-auto opacity-50" />
              )}
            </button>
          ))}
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col min-w-0 overflow-hidden">
        <div className="h-12 shrink-0 border-b border-[hsl(var(--border))] flex items-center px-6">
          <span className="text-base font-semibold text-[hsl(var(--foreground))]">
            {SETTINGS_TABS.find(t => t.id === currentTab)?.label}
          </span>
        </div>

        <div className="flex-1 overflow-y-auto p-6">
          <div className="max-w-3xl">
            {currentTab === "appearance" && (
              <AppearanceSettings settings={settings} updateSetting={updateSetting} />
            )}
            {currentTab === "keys" && (
              <KeyboardSettings settings={settings} updateSettings={updateSetting} />
            )}
            {currentTab === "features" && (
              <FeaturesSettings settings={settings} updateSettings={updateSetting} />
            )}
            {currentTab === "privacy" && (
              <PrivacySettings settings={settings} updateSettings={updateSetting} />
            )}
            {currentTab === "account" && <AccountSettings />}
            {currentTab === "about" && <AboutSettings />}
          </div>
        </div>
      </div>
    </motion.div>
  );
}

// ============================================================================
// HELPERS
// ============================================================================

function SectionHeader({ title, description }: { title: string; description?: string }) {
  return (
    <div className="mb-3">
      <h3 className="text-[13px] font-semibold text-[hsl(var(--foreground))] uppercase tracking-wide">{title}</h3>
      {description && (
        <p className="text-[11px] text-[hsl(var(--muted-foreground))] mt-0.5">{description}</p>
      )}
    </div>
  );
}

function SettingsRow({ title, description, children }: { title: string; description?: string; children: React.ReactNode }) {
  return (
    <div className="flex items-center justify-between py-3 border-b border-[hsl(var(--border)/0.3)] last:border-0">
      <div className="space-y-0.5 pr-4 flex-1">
        <div className="text-[13px] font-medium text-[hsl(var(--foreground))]">{title}</div>
        {description && (
          <div className="text-[11px] text-[hsl(var(--muted-foreground))]">{description}</div>
        )}
      </div>
      <div className="shrink-0">{children}</div>
    </div>
  );
}

// ============================================================================
// THEME CARD
// ============================================================================

function ThemeCard({ theme, selected, onClick }: { theme: Theme; selected: boolean; onClick: () => void }) {
  return (
    <button onClick={onClick} className="text-left group">
      <div
        className={cn(
          "aspect-[4/3] rounded-lg border-2 overflow-hidden relative transition-all",
          selected
            ? "border-[hsl(var(--primary))] ring-2 ring-[hsl(var(--primary)/0.2)]"
            : "border-[hsl(var(--border))] hover:border-[hsl(var(--muted-foreground))]"
        )}
        style={{ backgroundColor: theme.colors.background }}
      >
        <div className="absolute inset-0 p-1.5 flex flex-col gap-1">
          <div className="flex gap-0.5">
            <div className="w-1 h-1 rounded-full bg-red-500/60" />
            <div className="w-1 h-1 rounded-full bg-yellow-500/60" />
            <div className="w-1 h-1 rounded-full bg-green-500/60" />
          </div>
          <div className="flex-1 flex flex-col gap-0.5 mt-0.5">
            <div className="h-1 w-4 rounded" style={{ backgroundColor: theme.colors.accent }} />
            <div className="h-1 w-full rounded" style={{ backgroundColor: `${theme.colors.foreground}20` }} />
            <div className="h-1.5 w-3/4 rounded" style={{ backgroundColor: theme.colors.selection }} />
          </div>
        </div>
        {selected && (
          <div className="absolute top-1 right-1 h-3 w-3 rounded-full bg-[hsl(var(--primary))] flex items-center justify-center">
            <Check className="h-2 w-2 text-white" />
          </div>
        )}
      </div>
      <div className={cn(
        "mt-1 text-[10px] font-medium text-center truncate",
        selected ? "text-[hsl(var(--foreground))]" : "text-[hsl(var(--muted-foreground))]"
      )}>
        {theme.name}
      </div>
    </button>
  );
}

// ============================================================================
// APPEARANCE SETTINGS
// ============================================================================

function AppearanceSettings({
  settings,
  updateSetting
}: {
  settings: AppSettings;
  updateSetting: any;
}) {
  const darkThemes = THEMES.filter(t => t.type === 'dark');
  const lightThemes = THEMES.filter(t => t.type === 'light');

  return (
    <div className="space-y-6">
      {/* Themes */}
      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <SectionHeader title="Theme" />

        <div className="mb-3">
          <div className="text-[11px] text-[hsl(var(--muted-foreground))] mb-2">Dark</div>
          <div className="grid grid-cols-6 gap-2">
            {darkThemes.map(theme => (
              <ThemeCard
                key={theme.id}
                theme={theme}
                selected={settings.appearance.theme === theme.id}
                onClick={() => updateSetting("appearance", "theme", theme.id)}
              />
            ))}
          </div>
        </div>

        <div>
          <div className="text-[11px] text-[hsl(var(--muted-foreground))] mb-2">Light</div>
          <div className="grid grid-cols-6 gap-2">
            {lightThemes.map(theme => (
              <ThemeCard
                key={theme.id}
                theme={theme}
                selected={settings.appearance.theme === theme.id}
                onClick={() => updateSetting("appearance", "theme", theme.id)}
              />
            ))}
          </div>
        </div>

        <div className="mt-3 pt-3 border-t border-[hsl(var(--border))]">
          <SettingsRow title="Sync with OS" description="Follow system preference">
            <Switch
              checked={settings.appearance.syncWithOS}
              onCheckedChange={(v) => updateSetting("appearance", "syncWithOS", v)}
            />
          </SettingsRow>
        </div>
      </section>

      {/* Typography */}
      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <SectionHeader title="Typography" />

        <SettingsRow title="Font Family" description="Terminal and UI text">
          <select
            value={settings.appearance.fontFamily}
            onChange={(e) => updateSetting("appearance", "fontFamily", e.target.value)}
            className="bg-[hsl(var(--muted))] border border-[hsl(var(--border))] rounded-lg px-3 py-1.5 text-[12px] outline-none"
          >
            {FONT_FAMILIES.map(font => (
              <option key={font.id} value={font.id}>{font.name}</option>
            ))}
          </select>
        </SettingsRow>

        <SettingsRow title="Font Size" description="Base text size">
          <div className="flex items-center gap-2">
            <Slider
              value={[settings.appearance.fontSize]}
              onValueChange={(v) => updateSetting("appearance", "fontSize", v[0])}
              min={10}
              max={20}
              step={1}
              className="w-24"
            />
            <span className="text-[12px] text-[hsl(var(--muted-foreground))] w-8 text-right">
              {settings.appearance.fontSize}px
            </span>
          </div>
        </SettingsRow>
      </section>

      {/* Cursor */}
      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <SectionHeader title="Cursor" />
        <SettingsRow title="Cursor Type" description="Terminal cursor style">
          <div className="flex gap-1">
            {CURSOR_TYPES.map(cursor => (
              <button
                key={cursor.id}
                onClick={() => updateSetting("appearance", "cursorType", cursor.value)}
                className={cn(
                  "px-2.5 py-1 rounded-md text-[11px] font-medium transition-all",
                  settings.appearance.cursorType === cursor.value
                    ? "bg-[hsl(var(--primary))] text-white"
                    : "bg-[hsl(var(--muted))] text-[hsl(var(--muted-foreground))] hover:bg-[hsl(var(--hover))]"
                )}
              >
                {cursor.name}
              </button>
            ))}
          </div>
        </SettingsRow>
      </section>

      {/* Window */}
      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <SectionHeader title="Window" />
        <SettingsRow title="Opacity" description="Window transparency">
          <div className="flex items-center gap-2">
            <Slider
              value={[settings.appearance.windowOpacity]}
              onValueChange={(v) => updateSetting("appearance", "windowOpacity", v[0])}
              min={50}
              max={100}
              step={5}
              className="w-24"
            />
            <span className="text-[12px] text-[hsl(var(--muted-foreground))] w-10 text-right">
              {settings.appearance.windowOpacity}%
            </span>
          </div>
        </SettingsRow>
      </section>
    </div>
  );
}

// ============================================================================
// KEYBOARD SETTINGS
// ============================================================================

function KeyboardSettings({ settings, updateSettings }: { settings: AppSettings; updateSettings: any }) {
  const [searchQuery, setSearchQuery] = useState("");
  const [editingId, setEditingId] = useState<string | null>(null);
  const [editAction, setEditAction] = useState("");
  const [editBinding, setEditBinding] = useState("");
  const { toast } = useToast();

  const handleEdit = (shortcut: any) => {
    setEditingId(shortcut.id);
    setEditAction(shortcut.action);
    setEditBinding(shortcut.binding);
  };

  const handleSave = () => {
    if (!editAction.trim() || !editBinding.trim()) {
      toast({ description: "Action and binding are required", variant: "destructive" });
      return;
    }

    const newShortcuts = [...settings.keyboard.shortcuts];
    const index = newShortcuts.findIndex(s => s.id === editingId);

    if (index >= 0) {
      newShortcuts[index] = { ...newShortcuts[index], action: editAction, binding: editBinding };
    } else {
      newShortcuts.push({
        id: `custom.${Date.now()}`,
        action: editAction,
        binding: editBinding,
        category: "Custom"
      });
    }

    updateSettings("keyboard", "shortcuts", newShortcuts);
    setEditingId(null);
    toast({ description: "Shortcut saved" });
  };

  const handleDelete = (id: string) => {
    const newShortcuts = settings.keyboard.shortcuts.filter(s => s.id !== id);
    updateSettings("keyboard", "shortcuts", newShortcuts);
    toast({ description: "Shortcut removed" });
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (editingId) {
      e.preventDefault();
      const keys = [];
      if (e.ctrlKey) keys.push("Ctrl");
      if (e.altKey) keys.push("Alt");
      if (e.shiftKey) keys.push("Shift");
      if (e.metaKey) keys.push("Meta");
      const key = e.key === "Control" || e.key === "Alt" || e.key === "Shift" || e.key === "Meta"
        ? ""
        : e.key.charAt(0).toUpperCase() + e.key.slice(1);
      if (key) keys.push(key);
      setEditBinding(keys.join("+"));
    }
  };

  const filtered = searchQuery
    ? settings.keyboard.shortcuts.filter(s =>
      s.action.toLowerCase().includes(searchQuery.toLowerCase()) ||
      s.binding.toLowerCase().includes(searchQuery.toLowerCase())
    )
    : settings.keyboard.shortcuts;

  const categories = [...new Set(filtered.map(s => s.category))];

  return (
    <div className="space-y-4">
      <div className="flex items-center gap-3">
        <div className="relative flex-1">
          <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-[hsl(var(--muted-foreground))]" />
          <input
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full bg-[hsl(var(--muted))] border border-[hsl(var(--border))] rounded-lg py-2 pl-10 pr-3 text-[12px] outline-none"
            placeholder="Search shortcuts"
          />
        </div>
        <button
          onClick={() => { setEditingId("new"); setEditAction(""); setEditBinding(""); }}
          className="h-9 px-3 rounded-lg bg-[hsl(var(--primary))] text-white text-[12px] font-medium flex items-center gap-1.5"
        >
          <Plus className="h-3.5 w-3.5" />
          Add
        </button>
      </div>

      {editingId && (
        <section className="p-4 rounded-lg border-2 border-[hsl(var(--primary))] bg-[hsl(var(--card))] space-y-3">
          <div className="grid grid-cols-2 gap-3">
            <div className="space-y-1">
              <label className="text-[11px] text-[hsl(var(--muted-foreground))]">Action</label>
              <input
                value={editAction}
                onChange={(e) => setEditAction(e.target.value)}
                className="w-full bg-[hsl(var(--background))] border border-[hsl(var(--border))] rounded-lg px-3 py-1.5 text-[12px] outline-none"
                placeholder="e.g. Save File"
              />
            </div>
            <div className="space-y-1">
              <label className="text-[11px] text-[hsl(var(--muted-foreground))]">Binding (press keys)</label>
              <input
                value={editBinding}
                onKeyDown={handleKeyDown}
                readOnly
                className="w-full bg-[hsl(var(--background))] border border-[hsl(var(--border))] rounded-lg px-3 py-1.5 text-[12px] font-mono outline-none cursor-pointer"
                placeholder="Ctrl+S"
              />
            </div>
          </div>
          <div className="flex justify-end gap-2">
            <button onClick={() => setEditingId(null)} className="px-3 py-1.5 text-[12px] text-[hsl(var(--muted-foreground))]">
              Cancel
            </button>
            <button onClick={handleSave} className="px-3 py-1.5 rounded-lg bg-[hsl(var(--primary))] text-white text-[12px] flex items-center gap-1">
              <Save className="h-3 w-3" />
              Save
            </button>
          </div>
        </section>
      )}

      {categories.map(cat => (
        <section key={cat} className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
          <SectionHeader title={cat} />
          <div className="space-y-1">
            {filtered.filter(s => s.category === cat).map(shortcut => (
              <div key={shortcut.id} className="group flex items-center justify-between py-1.5 px-2 -mx-2 rounded-lg hover:bg-[hsl(var(--hover))] transition-colors">
                <span className="text-[12px] text-[hsl(var(--foreground))]">{shortcut.action}</span>
                <div className="flex items-center gap-2">
                  <kbd className="px-2 py-0.5 rounded bg-[hsl(var(--muted))] border border-[hsl(var(--border))] text-[11px] font-mono text-[hsl(var(--muted-foreground))]">
                    {shortcut.binding}
                  </kbd>
                  <div className="flex opacity-0 group-hover:opacity-100 transition-opacity">
                    <button onClick={() => handleEdit(shortcut)} className="p-1 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--foreground))]">
                      <Pencil className="h-3 w-3" />
                    </button>
                    <button onClick={() => handleDelete(shortcut.id)} className="p-1 text-[hsl(var(--muted-foreground))] hover:text-[hsl(var(--destructive))]">
                      <Trash2 className="h-3 w-3" />
                    </button>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </section>
      ))}
    </div>
  );
}

// ============================================================================
// OTHER TABS
// ============================================================================

function FeaturesSettings({ settings, updateSettings }: { settings: AppSettings; updateSettings: any }) {
  return (
    <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
      <SectionHeader title="General" />
      <SettingsRow title="Sound Effects" description="Play sounds for notifications">
        <Switch checked={settings.features.soundEffects} onCheckedChange={(v) => updateSettings("features", "soundEffects", v)} />
      </SettingsRow>
      <SettingsRow title="Notifications" description="Show system notifications">
        <Switch checked={settings.features.notifications} onCheckedChange={(v) => updateSettings("features", "notifications", v)} />
      </SettingsRow>
      <SettingsRow title="Auto Copy on Select" description="Copy selected text automatically">
        <Switch checked={settings.features.autoCopyOnSelect} onCheckedChange={(v) => updateSettings("features", "autoCopyOnSelect", v)} />
      </SettingsRow>
    </section>
  );
}

function PrivacySettings({ settings, updateSettings }: { settings: AppSettings; updateSettings: any }) {
  return (
    <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
      <SectionHeader title="Data Collection" />
      <SettingsRow title="Telemetry" description="Help improve SI with anonymous usage data">
        <Switch checked={settings.privacy.telemetry} onCheckedChange={(v) => updateSettings("privacy", "telemetry", v)} />
      </SettingsRow>
      <SettingsRow title="Crash Reports" description="Automatically send crash reports">
        <Switch checked={settings.privacy.crashReports} onCheckedChange={(v) => updateSettings("privacy", "crashReports", v)} />
      </SettingsRow>
    </section>
  );
}

function AccountSettings() {
  return (
    <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
      <SectionHeader title="Profile" />
      <div className="flex items-center gap-3 py-3">
        <div className="h-12 w-12 rounded-full bg-[hsl(var(--primary))] flex items-center justify-center text-lg font-bold text-white">
          S
        </div>
        <div>
          <div className="text-[13px] font-semibold text-[hsl(var(--foreground))]">SI User</div>
          <div className="text-[11px] text-[hsl(var(--muted-foreground))]">Local account</div>
        </div>
      </div>
    </section>
  );
}

function AboutSettings() {
  return (
    <div className="space-y-4">
      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <div className="flex items-center gap-3">
          <div className="h-12 w-12 rounded-xl bg-gradient-to-br from-cyan-500 to-blue-600 flex items-center justify-center text-white font-bold text-lg">
            SI
          </div>
          <div>
            <div className="text-[14px] font-semibold text-[hsl(var(--foreground))]">Shell Intelligence</div>
            <div className="text-[11px] text-[hsl(var(--muted-foreground))]">Version 0.1.0 (dev)</div>
          </div>
        </div>
      </section>

      <section className="p-4 rounded-lg border border-[hsl(var(--border))] bg-[hsl(var(--card))]">
        <SectionHeader title="Links" />
        <div className="space-y-1.5">
          <a href="#" className="block text-[12px] text-[hsl(var(--primary))] hover:underline">Documentation</a>
          <a href="#" className="block text-[12px] text-[hsl(var(--primary))] hover:underline">GitHub Repository</a>
          <a href="#" className="block text-[12px] text-[hsl(var(--primary))] hover:underline">Report an Issue</a>
        </div>
      </section>
    </div>
  );
}
