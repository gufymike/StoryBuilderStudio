#!/bin/bash

# ==========================================
# CUSTOM CONFIGURATION
# ==========================================
# Edit these paths if your dependencies are not in standard locations
CUSTOM_QT_PATH=""
LOCAL_DLL_PATH=""

# Add custom local DLLs to LD_LIBRARY_PATH right away if defined
if [ -n "$LOCAL_DLL_PATH" ] && [ -d "$LOCAL_DLL_PATH" ]; then
    echo "[INFO] Injecting local DLL path into environment: $LOCAL_DLL_PATH"
    export LD_LIBRARY_PATH="$LOCAL_DLL_PATH:$LD_LIBRARY_PATH"
fi

# Check if custom Qt path is set and valid
if [ -n "$CUSTOM_QT_PATH" ] && [ -d "$CUSTOM_QT_PATH" ]; then
    echo "[INFO] Using custom Qt path: $CUSTOM_QT_PATH"
    export LD_LIBRARY_PATH="$CUSTOM_QT_PATH:$LD_LIBRARY_PATH"
    ./appStoryBuilderStudio
    exit 0
fi

echo "[INFO] Scanning for local Qt 6.11+ installations..."

# Search order: Common /opt/Qt paths, then /usr/local
SEARCH_PATHS=(
    "/opt/Qt/6."*"/gcc_64/lib"
    "/usr/local/Qt/6."*"/gcc_64/lib"
)

for pattern in "${SEARCH_PATHS[@]}"; do
    # Expand wildcards safely
    for dir in $pattern; do
        if [ -d "$dir" ] && [ -f "$dir/libQt6Core.so" ]; then
            # Extract minor version number from the path (e.g., 6.11.0 -> 11)
            if [[ "$dir" =~ 6\.([0-9]+) ]]; then
                minor_version="${BASH_REMATCH[1]}"
                if [ "$minor_version" -ge 11 ]; then
                    echo "[SUCCESS] Found Qt 6.11+ libraries at: $dir"
                    export LD_LIBRARY_PATH="$dir:$LD_LIBRARY_PATH"
                    ./appStoryBuilderStudio
                    exit 0
                fi
            fi
        fi
    done
done

# Last ditch attempt: check if system can find Qt globally
if ldconfig -p | grep -qE 'libQt6Core\.so'; then
    echo "[INFO] Qt detected in global system cache. Attempting launch..."
    ./appStoryBuilderStudio
    exit 0
fi

echo "[ERROR] Could not automatically locate Qt 6.11+ libraries."
echo "[ERROR] Please open 'run.sh' in a text editor and set your CUSTOM_QT_PATH."
exit 1
