#!/usr/bin/env bash
# Copyright (c) 2026 Mike Ramirez
# SPDX-License-Identifier: MIT

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_BIN="${SCRIPT_DIR}/appStoryBuilderStudio"

# ==========================================
# CUSTOM CONFIGURATION
# ==========================================
# Set CUSTOM_QT_PATH or LOCAL_DLL_PATH in your environment,
# or override them below if dependencies are in non-standard locations.
CUSTOM_QT_PATH="${CUSTOM_QT_PATH:-}"
LOCAL_DLL_PATH="${LOCAL_DLL_PATH:-}"

# Add custom local shared libraries to LD_LIBRARY_PATH if defined
if [ -n "$LOCAL_DLL_PATH" ]; then
    if [ -d "$LOCAL_DLL_PATH" ]; then
        echo "[INFO] Injecting local library path into environment: $LOCAL_DLL_PATH"
        export LD_LIBRARY_PATH="$LOCAL_DLL_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    else
        echo "[WARN] LOCAL_DLL_PATH does not exist: $LOCAL_DLL_PATH"
    fi
fi

# Check if custom Qt path is set and valid
if [ -n "$CUSTOM_QT_PATH" ] && [ -d "$CUSTOM_QT_PATH" ]; then
    echo "[INFO] Using custom Qt path: $CUSTOM_QT_PATH"
    export LD_LIBRARY_PATH="$CUSTOM_QT_PATH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    exec "$APP_BIN" "$@"
fi

echo "[INFO] Scanning for local Qt 6.9+ installations..."

SEARCH_PATHS=(
    "/opt/Qt/6."*"/gcc_64/lib"
    "/usr/local/Qt/6."*"/gcc_64/lib"
)

for pattern in "${SEARCH_PATHS[@]}"; do
    for dir in $pattern; do
        if [ -d "$dir" ] && [ -f "$dir/libQt6Core.so" ]; then
            if [[ "$dir" =~ 6\.([0-9]+) ]]; then
                minor_version="${BASH_REMATCH[1]}"
                if [ "$minor_version" -ge 9 ]; then
                    echo "[SUCCESS] Found Qt 6.9+ libraries at: $dir"
                    export LD_LIBRARY_PATH="$dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
                    exec "$APP_BIN" "$@"
                fi
            fi
        fi
    done
done

# Fall back to global system Qt libraries (standard on Fedora / Debian packages)
if ldconfig -p 2>/dev/null | grep -qE 'libQt6Core\.so'; then
    echo "[INFO] Qt detected in global system cache. Attempting launch..."
    exec "$APP_BIN" "$@"
fi

echo "[ERROR] Could not automatically locate Qt 6.9+ libraries."
echo "[ERROR] Set CUSTOM_QT_PATH in your environment or edit 'storybuilderstudio.sh'."
exit 1
