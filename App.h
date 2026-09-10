#pragma once

// ============================================================================
// VDONinjaPlayer - Main Application Class
// This class manages the main window, WebView2 browser, and system tray
// integration. It handles window messages and lifecycle management.
// ============================================================================

#include <Windows.h>
#include <WebView2.h>
#include <wil/com.h>
#include <string>
#include "Version.h"

// ============================================================================
// Resource IDs
// ============================================================================
#define IDI_APPICON 101

class App
{
public:

    App();

    ~App();

    // Initialize the application window and WebView2
    bool Initialize(HINSTANCE instance);

    // Main message loop
    int Run();

private:

    // Window message procedure - handles all window messages
    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    // Create and initialize WebView2 browser control
    void CreateBrowser();

    // Add the application icon to system tray
    void AddToTray();

    // Remove the application icon from system tray
    void RemoveFromTray();

    // Restore the window from tray
    void RestoreWindow();

    // Display context menu for tray icon
    void ShowTrayMenu();

    // Main window handle
    HWND m_hwnd = nullptr;

    // WebView2 controller for rendering
    wil::com_ptr<ICoreWebView2Controller> m_controller;

    // WebView2 instance
    wil::com_ptr<ICoreWebView2> m_webview;

    // Flag indicating if window is minimized to tray
    bool m_isInTray = false;

    // Flag indicating if tray notification has been shown on first minimize
    bool m_trayNotificationShown = false;

    // Custom window message for tray icon events
    static constexpr UINT WM_TRAYICON = WM_USER + 1;

    // Menu command IDs
    static constexpr UINT ID_TRAY_RESTORE = WM_USER + 2;
    static constexpr UINT ID_TRAY_EXIT = WM_USER + 3;
    static constexpr UINT ID_TRAY_RESET_URL = WM_USER + 4;
    static constexpr UINT ID_TRAY_SET_DEFAULT_URL = WM_USER + 5;
};