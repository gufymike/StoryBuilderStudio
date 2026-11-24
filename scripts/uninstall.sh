#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build}"
PREFIX="${PREFIX:-${HOME}/.local}"

if [ -f "${BUILD_DIR}/install_manifest.txt" ]; then
    echo "[INFO] Removing installed files listed in ${BUILD_DIR}/install_manifest.txt..."
    while IFS= read -r file; do
        if [ -n "${file}" ] && [ -e "${file}" ]; then
            rm -f "${file}"
            echo "  Removed: ${file}"
        fi
    done < "${BUILD_DIR}/install_manifest.txt"
else
    echo "[INFO] No install_manifest.txt found; removing standard paths under ${PREFIX}..."
    rm -f "${PREFIX}/bin/appStoryBuilderStudio"
    rm -f "${PREFIX}/bin/run.sh" "${PREFIX}/bin/storybuilderstudio.sh"
    rm -f "${PREFIX}/bin/README.md" "${PREFIX}/bin/LICENSE"
    rm -rf "${PREFIX}/bin/docs" "${PREFIX}/bin/plugins/drivers"
    rm -f "${PREFIX}/share/applications/StoryBuilderStudio.desktop"
    rm -f "${PREFIX}/share/icons/hicolor/256x256/apps/storybuilderstudio.png"
fi

rmdir --ignore-fail-on-non-empty -p "${PREFIX}/bin/plugins/drivers" 2>/dev/null || true

if command -v update-desktop-database >/dev/null 2>&1 && [ -d "${PREFIX}/share/applications" ]; then
    update-desktop-database "${PREFIX}/share/applications" || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1 && [ -d "${PREFIX}/share/icons/hicolor" ]; then
    gtk-update-icon-cache -f -t "${PREFIX}/share/icons/hicolor" || true
fi

echo "[SUCCESS] Story Builder Studio uninstalled."
