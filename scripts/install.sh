#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build}"
PREFIX="${PREFIX:-${HOME}/.local}"

if [ ! -d "${BUILD_DIR}" ]; then
    echo "[ERROR] Build directory not found: ${BUILD_DIR}"
    echo "        Run ./scripts/build.sh first or set BUILD_DIR=/path/to/build."
    exit 1
fi

echo "[INFO] Installing Story Builder Studio from ${BUILD_DIR} to ${PREFIX}..."
cmake --install "${BUILD_DIR}" --prefix "${PREFIX}"

if command -v update-desktop-database >/dev/null 2>&1 && [ -d "${PREFIX}/share/applications" ]; then
    update-desktop-database "${PREFIX}/share/applications" || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1 && [ -d "${PREFIX}/share/icons/hicolor" ]; then
    gtk-update-icon-cache -f -t "${PREFIX}/share/icons/hicolor" || true
fi

echo "[SUCCESS] Installed to ${PREFIX}."
echo "          Ensure ${PREFIX}/bin is in your PATH."
