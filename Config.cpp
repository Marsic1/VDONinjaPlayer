// ============================================================================
// VDONinjaPlayer - Configuration Implementation
// JSON-based configuration management for application settings
// Supports both current URL and default URL (for Reset URL feature)
// ============================================================================

#include "Config.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <Windows.h>

using json = nlohmann::json;

// ============================================================================
// Static member initialization
// ============================================================================
std::wstring Config::m_url;
std::wstring Config::m_defaultUrl;

// ============================================================================
// Load - Read configuration from config.json
// Loads both current "url" and "default" URL fields from the configuration
// Returns: true if successful, false if file not found or parse error
// ============================================================================
bool Config::Load()
{
	std::ifstream file("config.json");

	if (!file.is_open())
		return false;

	json j;
	file >> j;

	try
	{
		// Extract the current streaming URL from JSON
		std::string url = j.at("url").get<std::string>();
		m_url.assign(url.begin(), url.end());

		// Extract the default streaming URL if it exists
		if (j.contains("default"))
		{
			std::string defaultUrl = j.at("default").get<std::string>();
			m_defaultUrl.assign(defaultUrl.begin(), defaultUrl.end());
		}
		else
		{
			// If no default is set, use current URL as default
			m_defaultUrl = m_url;
		}

		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

// ============================================================================
// Url - Get the current streaming URL
// Returns: Reference to the stored URL string
// ============================================================================
const std::wstring& Config::Url()
{
	return m_url;
}

// ============================================================================
// DefaultUrl - Get the default streaming URL (for Reset URL feature)
// Returns: Reference to the default URL string
// ============================================================================
const std::wstring& Config::DefaultUrl()
{
	return m_defaultUrl;
}

// ============================================================================
// Save - Write configuration to config.json
// Saves both current URL and default URL to the configuration file
// Parameters: url - The current URL to save
//             defaultUrl - The default URL (optional, uses current URL if empty)
// Returns: true if successful, false if write fails
// ============================================================================
bool Config::Save(const std::wstring& url, const std::wstring& defaultUrl)
{
	std::ofstream file("config.json");

	if (!file.is_open())
		return false;

	nlohmann::json j;

	// Convert wide strings to UTF-8 for JSON storage
	std::string utf8Url(url.begin(), url.end());

	j["url"] = utf8Url;
	m_url = url;  // Update the static member with the new URL

	// If no default URL provided, use the current URL as default
	if (defaultUrl.empty())
	{
		j["default"] = utf8Url;
		m_defaultUrl = url;
	}
	else
	{
		std::string utf8Default(defaultUrl.begin(), defaultUrl.end());
		j["default"] = utf8Default;
		m_defaultUrl = defaultUrl;
	}

	// Write JSON with 4-space indentation
	file << j.dump(4);

	return true;
}

// ============================================================================
// ResetUrlToDefault - Reset current URL to the default URL in config
// Updates m_url to match m_defaultUrl and saves the configuration
// Returns: true if successful, false if operation fails
// ============================================================================
bool Config::ResetUrlToDefault()
{
	if (m_defaultUrl.empty())
		return false;

	m_url = m_defaultUrl;
	return Save(m_url, m_defaultUrl);
}

// ============================================================================
// SaveUrlOnly - Save current URL without changing the default URL
// Updates only the "url" field in config.json, preserving the "default" field
// This is called when the application closes to persist the current stream URL
// Parameters: url - The current URL to save
// Returns: true if successful, false if write fails
// ============================================================================
bool Config::SaveUrlOnly(const std::wstring& url)
{
	std::ofstream file("config.json");

	if (!file.is_open())
		return false;

	nlohmann::json j;

	// Convert wide strings to UTF-8 for JSON storage
	std::string utf8Url(url.begin(), url.end());

	j["url"] = utf8Url;
	m_url = url;  // Update the static member with the new URL

	// Keep the existing default URL unchanged
	j["default"] = std::string(m_defaultUrl.begin(), m_defaultUrl.end());

	// Write JSON with 4-space indentation
	file << j.dump(4);

	return true;
}

// ============================================================================
// DialogData - Structure to pass data between WndProc and main function
// Used for both initial configuration and URL modification dialogs
// ============================================================================
struct DialogData
{
	std::wstring url;
	bool confirmed = false;
	bool closed = false;
	HWND hEdit = nullptr;
};

// ============================================================================
// CreateInputDialog - Helper function to create and manage input dialog
// Parameters: title - Window title
//             prompt - Instruction text for user
//             initialText - Pre-filled text in the input field
// Returns: User input string if confirmed, empty string if cancelled
// ============================================================================
static std::wstring CreateInputDialog(const wchar_t* title, const wchar_t* prompt, const wchar_t* initialText = L"")
{
	const wchar_t CLASS_NAME[] = L"VDONinjaPlayerInputDlg";

	DialogData dlgData;

	WNDCLASSW wc = {};
	wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
	{
		DialogData* pData = nullptr;

		if (msg == WM_CREATE)
		{
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			pData = reinterpret_cast<DialogData*>(pCreate->lpCreateParams);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pData);
		}
		else
		{
			pData = reinterpret_cast<DialogData*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		}

		switch (msg)
		{
		case WM_COMMAND:
		{
			int id = LOWORD(wParam);

			if (id == IDOK)
			{
				if (pData && pData->hEdit)
				{
					int len = GetWindowTextLengthW(pData->hEdit);
					if (len > 0)
					{
						wchar_t* buffer = new wchar_t[len + 2];  // Allocate len + 2 to be safe
						int copied = GetWindowTextW(pData->hEdit, buffer, len + 1);
						if (copied > 0)
						{
							pData->url = buffer;
							pData->confirmed = true;
						}
						delete[] buffer;
					}
				}
				if (pData)
					pData->closed = true;
				DestroyWindow(hwnd);
				return 0;
			}
			else if (id == IDCANCEL)
			{
				if (pData)
				{
					pData->confirmed = false;
					pData->closed = true;
				}
				DestroyWindow(hwnd);
				return 0;
			}
			break;
		}

		case WM_CLOSE:
		{
			if (pData)
			{
				pData->confirmed = false;
				pData->closed = true;
			}
			DestroyWindow(hwnd);
			return 0;
		}
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	};

	wc.hInstance = GetModuleHandleW(nullptr);
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = CLASS_NAME;

	if (!RegisterClassW(&wc))
		return L"";

	HWND hwnd = CreateWindowExW(
		WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
		CLASS_NAME,
		title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, 550, 180,
		nullptr, nullptr, GetModuleHandleW(nullptr), &dlgData);

	if (!hwnd)
	{
		UnregisterClassW(CLASS_NAME, GetModuleHandleW(nullptr));
		return L"";
	}

	CreateWindowW(
		L"STATIC",
		prompt,
		WS_CHILD | WS_VISIBLE,
		10, 10, 520, 20,
		hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);

	dlgData.hEdit = CreateWindowW(
		L"EDIT",
		initialText,
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
		10, 35, 520, 30,
		hwnd, (HMENU)1001, GetModuleHandleW(nullptr), nullptr);

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessageW(dlgData.hEdit, WM_SETFONT, (WPARAM)hFont, 0);

	CreateWindowW(
		L"BUTTON",
		L"OK",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		345, 75, 80, 28,
		hwnd, (HMENU)IDOK, GetModuleHandleW(nullptr), nullptr);

	CreateWindowW(
		L"BUTTON",
		L"Cancel",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		435, 75, 80, 28,
		hwnd, (HMENU)IDCANCEL, GetModuleHandleW(nullptr), nullptr);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetFocus(dlgData.hEdit);

	MSG msg;
	while (!dlgData.closed && GetMessageW(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_KEYDOWN)
		{
			if (msg.wParam == VK_RETURN)
			{
				SendMessageW(hwnd, WM_COMMAND, IDOK, 0);
				continue;
			}
			else if (msg.wParam == VK_ESCAPE)
			{
				SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
				continue;
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::wstring result;
	if (dlgData.confirmed)
		result = dlgData.url;

	UnregisterClassW(CLASS_NAME, GetModuleHandleW(nullptr));

	return result;
}

// ============================================================================
// GetUrlFromUserAndSave - Show dialog to get URL from user and save config
// This function is called on first application startup when config.json is missing
// Both URL and default URL are set to the user input on initial configuration
// Returns: true if successful, false if user cancels or operation fails
// ============================================================================
bool Config::GetUrlFromUserAndSave()
{
	std::wstring url = CreateInputDialog(
		L"VDONinjaPlayer - Configuration",
		L"Please paste your VDO Ninja streaming URL:");

	if (!url.empty())
	{
		return Save(url, url);  // Set both current and default to user input
	}

	return false;
}

// ============================================================================
// SetDefaultUrlFromUser - Show dialog to set a new default URL
// Used by the "Set Default URL" tray menu option
// Allows users to update the reset-to URL without losing current configuration
// This is useful for personalizing the application deployment for different users/groups
// Returns: true if successful, false if user cancels or operation fails
// ============================================================================
bool Config::SetDefaultUrlFromUser()
{
	std::wstring newDefault = CreateInputDialog(
		L"VDONinjaPlayer - Set Default URL",
		L"Enter the default URL to use with Reset URL feature:",
		m_defaultUrl.c_str());

	if (!newDefault.empty())
	{
		return Save(m_url, newDefault);  // Keep current URL, update only default
	}

	return false;
}
