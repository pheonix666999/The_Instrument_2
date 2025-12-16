#!/usr/bin/env bash
set -euo pipefail

DIST_DIR="${1:-dist/macos}"

VST3_SRC="${DIST_DIR}/MelodyForgePro.vst3"
AU_SRC="${DIST_DIR}/MelodyForgePro.component"
APP_SRC="${DIST_DIR}/MelodyForgePro.app"

if [[ ! -d "${VST3_SRC}" ]]; then
  echo "Missing VST3 at ${VST3_SRC}"
  exit 1
fi
if [[ ! -d "${APP_SRC}" ]]; then
  echo "Missing app at ${APP_SRC}"
  exit 1
fi

echo "Installing MelodyForge Pro (requires sudo)..."
sudo mkdir -p "/Library/Audio/Plug-Ins/VST3" "/Applications"
sudo cp -R "${VST3_SRC}" "/Library/Audio/Plug-Ins/VST3/"
if [[ -d "${AU_SRC}" ]]; then
  sudo mkdir -p "/Library/Audio/Plug-Ins/Components"
  sudo cp -R "${AU_SRC}" "/Library/Audio/Plug-Ins/Components/"
fi
sudo cp -R "${APP_SRC}" "/Applications/"

echo "Done."

