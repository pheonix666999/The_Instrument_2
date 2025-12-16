#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build}"
CONFIG="${2:-Release}"
DIST_DIR="${3:-dist/macos}"

ARTEFACTS_DIR="${BUILD_DIR}/MelodyForgePro_artefacts/${CONFIG}"

mkdir -p "${DIST_DIR}"
rm -rf "${DIST_DIR}/MelodyForgePro.vst3" "${DIST_DIR}/MelodyForgePro.component" "${DIST_DIR}/MelodyForgePro.app"

cp -R "${ARTEFACTS_DIR}/VST3/MelodyForgePro.vst3" "${DIST_DIR}/MelodyForgePro.vst3"

if [[ -d "${ARTEFACTS_DIR}/AU/MelodyForgePro.component" ]]; then
  cp -R "${ARTEFACTS_DIR}/AU/MelodyForgePro.component" "${DIST_DIR}/MelodyForgePro.component"
fi

cp -R "${ARTEFACTS_DIR}/Standalone/MelodyForgePro.app" "${DIST_DIR}/MelodyForgePro.app"

IDENTITY="${CODESIGN_IDENTITY:--}"
echo "codesign identity: ${IDENTITY} (placeholder signing when '-')"

codesign --force --deep --sign "${IDENTITY}" "${DIST_DIR}/MelodyForgePro.app" || true
codesign --force --deep --sign "${IDENTITY}" "${DIST_DIR}/MelodyForgePro.vst3" || true
if [[ -d "${DIST_DIR}/MelodyForgePro.component" ]]; then
  codesign --force --deep --sign "${IDENTITY}" "${DIST_DIR}/MelodyForgePro.component" || true
fi

bash scripts/macos/build_pkg.sh "${DIST_DIR}" "${DIST_DIR}/MelodyForgePro.pkg"

echo "Packaged macOS outputs to ${DIST_DIR}"

