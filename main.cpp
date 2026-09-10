// ============================================================================
// VDONinjaPlayer - Main Entry Point
// Initializes configuration and starts the application
// ============================================================================

#include "App.h"
#include "Config.h"

// ============================================================================
// WinMain - Application entry point
// Initializes the configuration and creates the main application instance
// ============================================================================
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int)
{   
    // Try to load configuration from config.json
    if (!Config::Load())
    {
        // If config.json doesn't exist, ask user for URL
        if (!Config::GetUrlFromUserAndSave())
        {
            MessageBoxW(
                nullptr,
                L"No URL provided. Application will exit.",
                L"Configuration Required",
                MB_ICONWARNING);

            return -1;
        }
    }

    // Create and initialize the application
    App app;

    if (!app.Initialize(hInstance))
        return -1;

    // Run the main message loop
    return app.Run();
}