import { app, BrowserWindow, ipcMain, shell } from 'electron';
import { join } from 'path';
import { createIPCBridge } from './ipc-bridge';

app.commandLine.appendSwitch('no-sandbox');



let mainWindow: BrowserWindow | null = null;

function createWindow() {
    // electron-vite sets ELECTRON_RENDERER_URL only during dev mode
    const rendererUrl = process.env.ELECTRON_RENDERER_URL;
    const isDev = !!rendererUrl;

    mainWindow = new BrowserWindow({
        width: 1400,
        height: 900,
        minWidth: 800,
        minHeight: 600,
        frame: false,
        titleBarStyle: 'hidden',
        backgroundColor: '#050505', // Exact match for default theme
        show: true, // Show immediately to avoid hidden window issues on Linux
        webPreferences: {
            preload: join(__dirname, '../preload/index.mjs'),
            nodeIntegration: false,
            contextIsolation: true,
            sandbox: false,
        },
    });


    // Load the app
    if (isDev && rendererUrl) {
        mainWindow.loadURL(rendererUrl);
    } else {
        mainWindow.loadFile(join(__dirname, '../renderer/index.html'));
    }

    // Security: Block navigation to external URLs in production
    // Opens external links in system browser instead
    mainWindow.webContents.on('will-navigate', (event, url) => {
        const isLocalUrl = url.startsWith('file://') ||
            (isDev && url.startsWith('http://localhost'));
        if (!isLocalUrl) {
            event.preventDefault();
            shell.openExternal(url);
        }
    });

    // Handle external links opened via window.open or target="_blank"
    mainWindow.webContents.setWindowOpenHandler(({ url }) => {
        shell.openExternal(url);
        return { action: 'deny' };
    });

    mainWindow.on('closed', () => {
        mainWindow = null;
    });
}

// Window control IPC handlers
ipcMain.handle('window:minimize', () => {
    mainWindow?.minimize();
});

ipcMain.handle('window:maximize', () => {
    if (mainWindow?.isMaximized()) {
        mainWindow.unmaximize();
    } else {
        mainWindow?.maximize();
    }
});

ipcMain.handle('window:close', () => {
    mainWindow?.close();
});

ipcMain.handle('window:isMaximized', () => {
    return mainWindow?.isMaximized() ?? false;
});

// App lifecycle
app.whenReady().then(() => {
    createWindow();
    createIPCBridge();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            createWindow();
        }
    });
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
