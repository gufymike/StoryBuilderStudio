#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -z "${PREFIX:-}" ]; then
    if [ "$(basename "${SCRIPT_DIR}")" = "bin" ]; then
        PREFIX="$(dirname "${SCRIPT_DIR}")"
    elif [ "$(basename "${SCRIPT_DIR}")" = "StoryBuilderStudio" ] && [ "$(basename "$(dirname "${SCRIPT_DIR}")")" = "share" ]; then
        PREFIX="$(dirname "$(dirname "${SCRIPT_DIR}")")"
    else
        PREFIX="${HOME}/.local"
    fi
fi

REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build}"

cd /tmp

if [ -f "${BUILD_DIR}/install_manifest.txt" ]; then
    echo "[INFO] Removing installed files listed in ${BUILD_DIR}/install_manifest.txt..."
    while IFS= read -r file; do
        if [ -n "${file}" ] && [ -e "${file}" ]; then
            rm -f "${file}"
            echo "  Removed: ${file}"
        fi
    done < "${BUILD_DIR}/install_manifest.txt"
else
    echo "[INFO] Removing Story Builder Studio files under ${PREFIX}..."
    rm -f "${PREFIX}/bin/appStoryBuilderStudio"
    rm -f "${PREFIX}/bin/run.sh" "${PREFIX}/bin/storybuilderstudio.sh"
    rm -f "${PREFIX}/bin/uninstall.sh"
    rm -f "${PREFIX}/bin/README.md" "${PREFIX}/bin/LICENSE"
    rm -rf "${PREFIX}/bin/docs" "${PREFIX}/bin/plugins/drivers"
    rm -f "${PREFIX}/share/applications/StoryBuilderStudio.desktop"
    rm -f "${PREFIX}/share/icons/hicolor/256x256/apps/storybuilderstudio.png"
    rm -rf "${PREFIX}/share/doc/StoryBuilderStudio"
    rm -rf "${PREFIX}/share/StoryBuilderStudio"
fi

rmdir --ignore-fail-on-non-empty "${PREFIX}/bin/plugins/drivers" 2>/dev/null || true
rmdir --ignore-fail-on-non-empty "${PREFIX}/bin/plugins" 2>/dev/null || true
rmdir --ignore-fail-on-non-empty "${PREFIX}/bin" 2>/dev/null || true

if command -v update-desktop-database >/dev/null 2>&1 && [ -d "${PREFIX}/share/applications" ]; then
    update-desktop-database "${PREFIX}/share/applications" || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1 && [ -d "${PREFIX}/share/icons/hicolor" ]; then
    gtk-update-icon-cache -f -t "${PREFIX}/share/icons/hicolor" || true
fi

echo "[SUCCESS] Story Builder Studio uninstalled."
