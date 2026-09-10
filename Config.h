#pragma once

// ============================================================================
// VDONinjaPlayer - Configuration Management
// Handles loading and saving application configuration from/to config.json
// 
// Configuration file structure:
// {
//     "url": "current_streaming_url",
//     "default": "default_streaming_url"
// }
// 
// The "url" field is updated by the application when navigating to different
// streams. The "default" field is set once and used for the "Reset URL" feature
// in the system tray, allowing quick return to the default stream.
// 
// This design enables:
// - Personalized application deployment with pre-configured defaults
// - Easy reset to original stream with a tray menu click
// - Standard URL can be changed by users without losing the default
// ============================================================================

#include <string>

class Config
{
public:

    // Load configuration from config.json file
    static bool Load();

    // Get the current streaming URL
    static const std::wstring& Url();

    // Get the default streaming URL (used for Reset URL feature)
    static const std::wstring& DefaultUrl();

    // Show input dialog to get URL from user and save it as both current and default
    static bool GetUrlFromUserAndSave();

    // Show input dialog to set a new default URL
    static bool SetDefaultUrlFromUser();

    // Reset current URL to the default URL
    static bool ResetUrlToDefault();

    // Save current URL only (preserving the default URL)
    static bool SaveUrlOnly(const std::wstring& url);

    // Save configuration to config.json file
    static bool Save(const std::wstring& url, const std::wstring& defaultUrl = L"");

private:

    // Cached streaming URL
    static std::wstring m_url;

    // Cached default streaming URL (for Reset URL feature)
    static std::wstring m_defaultUrl;
};