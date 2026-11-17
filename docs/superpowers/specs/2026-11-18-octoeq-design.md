# OctoEQ — design

## Intent
User has Ableton Live 12 Intro (no EQ Eight, no Max for Live) on an Apple Silicon Mac and wants an EQ that behaves like EQ Eight. Live Intro hosts AU/VST3, so we ship a native plug-in.

## Goals
- 8 bands, each: on/off, type, frequency (10 Hz–22 kHz, log), gain (±15 dB), Q (0.1–18).
- Types: Low Cut 12 dB, Low Cut 48 dB, Low Shelf, Bell, Notch, High Shelf, High Cut 12 dB, High Cut 48 dB.
- Defaults like EQ Eight: band 1 low cut (off), 2 low shelf, 3–6 bells, 7 high shelf, 8 high cut (off); bands 1 and 8 off, 2–7 on.
- Global: mode (Stereo / L-R / Mid-Side) with an edit-channel selector when not Stereo, Scale (-200%..200% of band gains), Output gain (±24 dB), Hi-Q 2× oversampling.
- Display: combined response curve, per-band curves for the selected band, draggable band nodes (drag = freq/gain, wheel = Q, double-click = toggle), live post-EQ spectrum (FFT) behind it.
- Band strip under the display, EQ Eight-style.
- AU + VST3, arm64, built by GitHub Actions on macOS; zip artifact.

## Non-goals
- Audition mode, 64-band adaptive mode, Ableton look/branding.
- Windows/Intel builds, code signing/notarization.

## Approaches
1. **JUCE plug-in, CI-built (chosen)** — full feature set, native in Intro; user needs GitHub + one quarantine-removal command.
2. Local Xcode build — same code, user installs Xcode/CMake. Rejected by user.
3. Use an existing free EQ — no build, but not what was asked.

## Decisions (my calls)
- JUCE 8 via CMake FetchContent; RBJ biquads (cascaded for 48 dB); smoothed parameter updates.
- In L/R and M/S modes each channel has its own full 8-band set (like EQ Eight's Edit A/B).
- Plug-in name "OctoEQ", manufacturer code "Ocqe"/"Octo".
