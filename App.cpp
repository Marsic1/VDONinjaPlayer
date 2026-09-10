// ============================================================================
// VDONinjaPlayer - Main Application Implementation
// Handles window creation, WebView2 integration, priority optimization,
// and system tray functionality for streaming with high CPU priority.
// ============================================================================

#include "App.h"
#include "Config.h"
#include <processthreadsapi.h>
#include <wrl.h>
#include <avrt.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>

#pragma comment(lib,"Avrt.lib")
#pragma comment(lib,"uxtheme.lib")
#pragma comment(lib,"dwmapi.lib")

// ============================================================================
// Multimedia Class Scheduler (MMCSS) handle for priority boost
// ============================================================================
HANDLE g_mmcss = nullptr;

// ============================================================================
// EnableDarkMode - Enable dark mode for the application window
// Applies Windows dark theme and sets appropriate window colors
// This will also affect system menus in Windows 10 1809+ and Windows 11
// ============================================================================
void EnableDarkMode(HWND hwnd)
{
    // Apply dark mode to window title bar and frame
    BOOL isDarkMode = TRUE;
    DwmSetWindowAttribute(
        hwnd,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &isDarkMode,
        sizeof(isDarkMode));

    // Disable the default light theme
    SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);

    // Invalidate window to trigger repaint
    InvalidateRect(hwnd, nullptr, TRUE);
}

// ============================================================================
// Disable Eco QoS to prevent CPU throttling during streaming
// This ensures consistent performance when the system might otherwise
// reduce CPU frequency to save power.
// ============================================================================
void DisableEcoQoS()
{
    PROCESS_POWER_THROTTLING_STATE state{};

    state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;

    state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;

    state.StateMask = 0;

    SetProcessInformation(
        GetCurrentProcess(),
        ProcessPowerThrottling,
        &state,
        sizeof(state));
}

// ============================================================================
// Enable MMCSS (Multimedia Class Scheduler) with Playback priority
// This ensures the application gets consistent, high-priority CPU time
// for real-time media playback.
// ============================================================================
void EnableMMCSS()
{
    DWORD taskIndex = 0;

    g_mmcss = AvSetMmThreadCharacteristicsW(
        L"Playback",
        &taskIndex);
}

App::App()
{
}

App::~App()
{
}

// ============================================================================
// Initialize - Set up the main window, WebView2, and process priority
// Returns: true if successful, false otherwise
// ============================================================================
bool App::Initialize(HINSTANCE instance)
{
	WNDCLASSW wc{};

	wc.lpfnWndProc = App::WndProc;
	wc.hInstance = instance;
	wc.lpszClassName = L"VDONinjaPlayer";

	// Load and set the application icon
	wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));

	RegisterClassW(&wc);

	// Create main window with versioned title
	m_hwnd = CreateWindowExW(
		0,
		wc.lpszClassName,
		APP_TITLE_WITH_VERSION,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		800,
		1000,
		nullptr,
		nullptr,
		instance,
		this);

	if (!m_hwnd)
		return false;

	// Enable dark mode for the application
	EnableDarkMode(m_hwnd);

	ShowWindow(m_hwnd, SW_SHOW);

	UpdateWindow(m_hwnd);

	// Set process priority to ABOVE_NORMAL to prioritize camera streaming
	SetPriorityClass(
		GetCurrentProcess(),
		ABOVE_NORMAL_PRIORITY_CLASS);

	// Disable Eco QoS to prevent CPU throttling for background tabs
	DisableEcoQoS();

	// Enable Multimedia Class Scheduler Service (MMCSS) for optimal playback
	EnableMMCSS();

	// Disable WebView2 background timer throttling
	// This ensures the browser doesn't throttle when in background or minimized
	SetEnvironmentVariableW(
		L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS",
		L"--disable-background-timer-throttling "
		L"--disable-renderer-backgrounding "
		L"--disable-backgrounding-occluded-windows");

	CreateBrowser();

	// Add application icon to system tray on startup to always be accessible
	AddToTray();

	return true;
}

// ============================================================================
// Run - Main application message loop
// Processes window messages until the application is closed
// Returns: exit code from WM_QUIT message
// ============================================================================
int App::Run()
{
	MSG msg;

	// Standard Windows message loop
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);

		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

// ============================================================================
// WndProc - Main window message procedure
// Handles all window messages including resizing, minimize/close,
// and system tray interactions.
// ============================================================================
LRESULT CALLBACK App::WndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam)
{
	App* app = nullptr;

	// Retrieve the App instance pointer from window user data
	if (msg == WM_NCCREATE)
	{
		auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);

		app = static_cast<App*>(cs->lpCreateParams);

		SetWindowLongPtr(
			hwnd,
			GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(app));

		app->m_hwnd = hwnd;
	}
	else
	{
		app = reinterpret_cast<App*>(
			GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}

	switch (msg)
	{
	case WM_SIZE:
		// Handle window resizing - update WebView2 bounds
		if (app && app->m_controller)
		{
			RECT rc;

			GetClientRect(hwnd, &rc);

			app->m_controller->put_Bounds(rc);
		}

		return 0;

	case WM_SYSCOMMAND:
		// Handle minimize button click ("-") to hide window and add to tray
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			if (app)
			{
				ShowWindow(hwnd, SW_HIDE);

				// Show notification on first minimization to tray
				if (!app->m_trayNotificationShown)
				{
					app->m_trayNotificationShown = true;

					// Display balloon tooltip notification
					NOTIFYICONDATAW nid{};
					nid.cbSize = sizeof(NOTIFYICONDATAW);
					nid.hWnd = hwnd;
					nid.uID = 1;
					nid.uFlags = NIF_INFO;
					nid.dwInfoFlags = NIIF_INFO;
					nid.uTimeout = 5000;  // 5 seconds

					wcscpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(wchar_t), L"VDONinjaPlayer");
					wcscpy_s(nid.szInfo, sizeof(nid.szInfo) / sizeof(wchar_t), 
						L"Application minimized to system tray. Right-click the tray icon to access the menu.");

					Shell_NotifyIconW(NIM_MODIFY, &nid);
				}
			}
			return 0;
		}
		break;

	case WM_CLOSE:
	{
		// Request confirmation before closing the application
		int result = MessageBoxW(
			hwnd,
			L"Are you sure you want to close the application?",
			L"Confirm Exit",
			MB_YESNO | MB_ICONQUESTION);

		if (result == IDYES)
		{
			// Save the current URL before closing (preserving the default URL)
			if (app && app->m_webview)
			{
				LPWSTR source = nullptr;

				if (SUCCEEDED(app->m_webview->get_Source(&source)))
				{
					Config::SaveUrlOnly(source);
					CoTaskMemFree(source);
				}
			}

			PostQuitMessage(0);
		}
		return 0;
	}

    case WM_TRAYICON:
    {
        // Handle system tray icon events
        if (app)
        {
            switch (lParam)
            {
            case WM_LBUTTONDBLCLK:
                // Double-click restores the window
                app->RestoreWindow();
                break;

            case WM_RBUTTONUP:
                // Right-click displays context menu
                app->ShowTrayMenu();
                break;
            }
        }
        return 0;
    }

    case WM_DESTROY:
    {
        // Clean up tray icon before window destruction
        if (app && app->m_isInTray)
        {
            app->RemoveFromTray();
        }

        PostQuitMessage(0);

        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================================
// AddToTray - Add application icon to system tray
// Loads the application icon from resources and displays it in the tray
// ============================================================================
void App::AddToTray()
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = m_hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;

    // Load application icon from resources
    HICON hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APPICON));

    // Fallback to application icon if resource loading fails
    if (!hIcon)
        hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    nid.hIcon = hIcon;

    wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t), APP_TITLE_WITH_VERSION);

    Shell_NotifyIconW(NIM_ADD, &nid);
    m_isInTray = true;
}

// ============================================================================
// RemoveFromTray - Remove application icon from system tray
// ============================================================================
void App::RemoveFromTray()
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = m_hwnd;
    nid.uID = 1;

    Shell_NotifyIconW(NIM_DELETE, &nid);
    m_isInTray = false;
}

// ============================================================================
// RestoreWindow - Restore the application window from tray
// The tray icon remains visible so users can access the menu at any time
// ============================================================================
void App::RestoreWindow()
{
    ShowWindow(m_hwnd, SW_RESTORE);
    SetForegroundWindow(m_hwnd);
    // Tray icon intentionally NOT removed - remains always visible for quick access
}

// ============================================================================
// ShowTrayMenu - Display context menu for tray icon
// Provides options: 
//   - Go to Default URL: Reverts the current streaming URL to the configured default
//   - Set Default URL: Opens dialog to update the default reset-to URL
//   - Restore Window: Brings the window back from tray minimization
//   - Exit: Terminates the application
// ============================================================================
void App::ShowTrayMenu()
{
    HMENU hMenu = CreatePopupMenu();

    // Application title (non-clickable header)
    AppendMenuW(hMenu, MF_STRING, 0, APP_TITLE_WITH_VERSION);
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // Menu options for URL management and configuration
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESET_URL, L"Go to Default URL");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SET_DEFAULT_URL, L"Set Default URL");

    // Separator before window control options
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

    // Window management options
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESTORE, L"Restore Window");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(m_hwnd);

    int cmd = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_LEFTALIGN,
        pt.x,
        pt.y,
        0,
        m_hwnd,
        nullptr);

    DestroyMenu(hMenu);

    // Handle menu selection
    if (cmd == ID_TRAY_RESTORE)
    {
        RestoreWindow();
    }
    else if (cmd == ID_TRAY_RESET_URL)
    {
        // Reset current URL to the default and reload in browser
        if (Config::ResetUrlToDefault())
        {
            m_webview->Navigate(Config::Url().c_str());
        }
    }
    else if (cmd == ID_TRAY_SET_DEFAULT_URL)
    {
        // Open dialog to set a new default URL
        Config::SetDefaultUrlFromUser();
    }
    else if (cmd == ID_TRAY_EXIT)
    {
        PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
    }
}

// ============================================================================
// CreateBrowser - Initialize WebView2 browser control
// Sets up the browser with optimal settings for live streaming
// (DevTools disabled, context menus disabled, zoom disabled)
// ============================================================================
void App::CreateBrowser()
{
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,

        Microsoft::WRL::Callback<
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](
                HRESULT hr,
                ICoreWebView2Environment* env)
            -> HRESULT
            {
                if (!env)
                    return E_FAIL;

                env->CreateCoreWebView2Controller(
                    m_hwnd,

                    Microsoft::WRL::Callback<
                    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](
                            HRESULT hr,
                            ICoreWebView2Controller* controller)
                        -> HRESULT
                        {
                            if (!controller)
                                return E_FAIL;

                            m_controller = controller;

                            controller->get_CoreWebView2(
                                &m_webview);

                            wil::com_ptr<ICoreWebView2Settings> settings;

                            m_webview->get_Settings(&settings);

                            settings->put_AreDevToolsEnabled(FALSE);

                            settings->put_AreDefaultContextMenusEnabled(FALSE);

                            settings->put_IsZoomControlEnabled(FALSE);

                            RECT rc;

                            GetClientRect(
                                m_hwnd,
                                &rc);

                            controller->put_Bounds(rc);

                            // Navigate to the configured URL
                            // Only navigate if URL is not empty to avoid navigation to about:blank
                            const std::wstring& url = Config::Url();
                            if (!url.empty())
                            {
                                m_webview->Navigate(url.c_str());
                            }
                            else
                            {
                                // URL should have been set before Initialize() was called
                                // If this occurs, display an error and close the application
                                MessageBoxW(
                                    m_hwnd,
                                    L"No URL configured. Please restart the application and enter a valid URL.",
                                    L"Configuration Error",
                                    MB_ICONERROR);
                                PostMessageW(m_hwnd, WM_CLOSE, 0, 0);
                            }

                            return S_OK;
                        }).Get());

                return S_OK;
            }).Get());
}

