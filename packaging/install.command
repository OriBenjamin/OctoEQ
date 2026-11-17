#!/bin/bash
# Installs OctoEQ for the current user and clears the macOS download quarantine flag.
set -e
cd "$(dirname "$0")"
AU="$HOME/Library/Audio/Plug-Ins/Components"
VST3="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$AU" "$VST3"
rm -rf "$AU/OctoEQ.component" "$VST3/OctoEQ.vst3"
cp -R OctoEQ.component "$AU/"
cp -R OctoEQ.vst3 "$VST3/"
xattr -dr com.apple.quarantine "$AU/OctoEQ.component" "$VST3/OctoEQ.vst3" 2>/dev/null || true
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "OctoEQ installed:"
echo "  $AU/OctoEQ.component"
echo "  $VST3/OctoEQ.vst3"
echo "Restart Ableton Live (or Rescan plug-ins) to see it."
