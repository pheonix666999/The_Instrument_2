#!/usr/bin/env bash
set -euo pipefail

DIST_DIR="${1:?dist dir required}"
OUT_PKG="${2:?output pkg path required}"

TMP_ROOT="$(mktemp -d)"
cleanup() { rm -rf "${TMP_ROOT}"; }
trap cleanup EXIT

mkdir -p "${TMP_ROOT}/Library/Audio/Plug-Ins/VST3"
mkdir -p "${TMP_ROOT}/Library/Audio/Plug-Ins/Components"
mkdir -p "${TMP_ROOT}/Applications"

cp -R "${DIST_DIR}/MelodyForgePro.vst3" "${TMP_ROOT}/Library/Audio/Plug-Ins/VST3/"

if [[ -d "${DIST_DIR}/MelodyForgePro.component" ]]; then
  cp -R "${DIST_DIR}/MelodyForgePro.component" "${TMP_ROOT}/Library/Audio/Plug-Ins/Components/"
fi

cp -R "${DIST_DIR}/MelodyForgePro.app" "${TMP_ROOT}/Applications/"

pkgbuild \
  --root "${TMP_ROOT}" \
  --install-location "/" \
  --identifier "com.xai.musictools.melodyforgepro" \
  --version "1.0.0" \
  "${OUT_PKG}"

echo "Built pkg: ${OUT_PKG}"

