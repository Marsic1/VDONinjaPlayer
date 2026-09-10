# VDONinjaPlayer

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/marsic1)

A **lightweight, high-priority Windows viewer for [VDO.Ninja](https://vdo.ninja)** streams. Built as a minimal native Win32 application, it keeps your remote webcam smooth even while gaming — no browser tab, no OBS panel, no Electron runtime.

## Why?

VDO.Ninja is an amazing tool for bringing remote cameras into OBS, but playing a stream inside a regular browser tab (or an OBS browser source) while a game is running often leads to lag, stuttering and dropped frames. The browser deprioritizes background tabs, the OS throttles the process, and the stream suffers.

The VDO.Ninja author, [Steve Seguin](https://github.com/steveseguin), already solved part of this with [electroncapture](https://github.com/steveseguin/electroncapture) — but it carries the weight of a full Electron runtime and can be slow to open. VDONinjaPlayer takes a different approach: a tiny, single-purpose native Windows app that is completely optimized for exactly one job — playing a VDO.Ninja stream as smoothly as possible.

## How it stays smooth

The app pulls every performance lever Windows and WebView2 offer:

- **`ABOVE_NORMAL` process priority** — the player outranks normal apps for CPU time
- **MMCSS "Playback" scheduling** — registers the main thread with the Multimedia Class Scheduler Service, the same priority boost media players use
- **Eco QoS disabled** — prevents Windows from power-throttling the process on efficiency cores or when backgrounded
- **WebView2 throttling disabled** — launches the embedded browser with:
  - `--disable-background-timer-throttling`
  - `--disable-renderer-backgrounding`
  - `--disable-backgrounding-occluded-windows`

  So the stream keeps rendering at full speed even when the window is minimized to the tray.
- **Native Win32, no Electron** — a fraction of the memory footprint and near-instant launch

## Features

- 🚀 Minimal native C++ Win32 application — small, fast, no runtime bloat
- 🖥️ Renders with [WebView2](https://developer.microsoft.com/microsoft-edge/webview2/) (the Edge engine, preinstalled on Windows 10/11)
- 📌 **System tray integration** — minimize to tray and keep streaming; double-click to restore, right-click for the menu
- 🔗 **Default URL management** — set a default stream URL and reset back to it with one click from the tray menu. Perfect for pre-configuring copies of the app for friends or a team: they can browse to other streams but always return to the default with a single click
- 💾 Remembers the current URL between sessions (saved on exit)
- 🌙 Dark mode title bar
- 🗂️ Simple JSON configuration (`config.json`) — no registry, no installer

## Getting started

### Requirements

- Windows 10 or 11 (WebView2 runtime, preinstalled on up-to-date systems)
- [Visual Studio](https://visualstudio.microsoft.com/) with C++ desktop development workload
  - The project targets the `v145` platform toolset; if you're on an older Visual Studio, retarget the project to your installed toolset (e.g. `v143`)

### Build

1. Clone the repository
2. Open `VDONinjaPlayer.slnx` (or `VDONinjaPlayer.vcxproj`) in Visual Studio
3. Restore the NuGet packages ([WebView2 SDK](https://www.nuget.org/packages/Microsoft.Web.WebView2), [Windows Implementation Library (WIL)](https://www.nuget.org/packages/Microsoft.Windows.ImplementationLibrary), [nlohmann/json](https://www.nuget.org/packages/nlohmann.json)) — Visual Studio does this automatically on first build
4. Build **Release | x64**

### Usage

1. Run `VDONinjaPlayer.exe`
2. On first launch, paste your VDO.Ninja view URL into the dialog (e.g. the guest invite link you'd normally open in a browser)
3. That's it — the stream opens and runs at high priority

The configuration is stored in `config.json` next to the executable:

```json
{
    "url": "https://vdo.ninja/?view=YOUR_STREAM",
    "default": "https://vdo.ninja/?view=YOUR_STREAM"
}
```

- `url` — the stream currently open (updated automatically on exit)
- `default` — the URL the tray menu's **Go to Default URL** option resets to

See [`config.example.json`](config.example.json) for a template. You can also delete `config.json` to trigger the first-run setup dialog again.

> [!NOTE]
> Your `config.json` may contain room names, hashes and passwords — it is listed in `.gitignore` on purpose. Never commit it.

## Credits

- **[Steve Seguin](https://github.com/steveseguin)** — creator of [VDO.Ninja](https://github.com/steveseguin/vdo.ninja) and [electroncapture](https://github.com/steveseguin/electroncapture). VDO.Ninja is the peer-to-peer video transport that makes this app useful, and electroncapture was the direct inspiration for it. If VDO.Ninja saves your stream, consider [supporting Steve's work](https://github.com/sponsors/steveseguin).
- This is an independent, unofficial companion tool and is not affiliated with VDO.Ninja.

## License

Released under the [MIT License](LICENSE).

## Support

If this little tool saves your stream, you can buy me a coffee:

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/marsic1)
