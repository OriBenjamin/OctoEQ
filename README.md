# OctoEQ

An 8-band EQ plug-in (AU + VST3, Apple Silicon) modelled on the workflow of Ableton's EQ Eight, for Live Intro and any other Mac host.

## Install

1. Download `OctoEQ-macOS-arm64.zip` (from the repo's **Releases** page, "latest") and double-click it to unzip.
2. Open **Terminal** and run (drag `install.command` into the Terminal window to fill in the path):
   ```
   bash ~/Downloads/OctoEQ/install.command
   ```
   This copies the plug-in into `~/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/VST3`, and removes the download quarantine flag (the plug-in isn't signed by Apple, so macOS would otherwise block it).
3. In Live: **Settings → Plug-Ins**, turn on **Use Audio Units v2** and/or **Use VST3 Plug-in System Folders**, then click **Rescan**.
4. Find **OctoEQ** under **Plug-Ins** in the browser (AU: *Audio Units → Octo*; VST3: *VST3*) and drop it on a track. Use one format, not both.

## Using it

- **Curve:** drag a numbered dot to set frequency (left/right) and gain (up/down). On cut and notch bands, up/down changes resonance (Q).
  Scroll wheel over a dot changes Q. Double-click a dot turns the band on/off. The coloured fill shows the selected band on its own.
- **Bands:** each column has on/off, filter type (Low Cut 12/48, Low Shelf, Bell, Notch, High Shelf, High Cut 12/48), Freq, Gain and Q. Double-click Gain to reset it.
- **Mode:** *Stereo* (one curve for both sides), *L/R* (separate curves for left and right) or *M/S* (separate curves for mid and side). In L/R or M/S, **Edit** picks which curve you're editing (A = Left/Mid, B = Right/Side).
- **Scale:** scales every bell/shelf gain at once (100% = as set, 0% = flat, −100% = inverted).
- **Output:** final level, ±24 dB.
- **Hi-Q:** 2× oversampling for more accurate boosts and cuts near the top of the spectrum. It adds a few samples of latency, which Live compensates for automatically.
- The grey area behind the curve is a live spectrum of the output.

All controls can be automated and mapped in Live.

## Building from source

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --target OctoEQ_AU OctoEQ_VST3
```
JUCE 8.0.4 is fetched automatically. Every push to GitHub builds, tests and validates the plug-in (`auval`) on a macOS runner.
