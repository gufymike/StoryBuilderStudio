#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${REPO_URL:-https://github.com/<YOUR_USER>/StoryBuilderStudio.git}"
REPO_DIR="${REPO_DIR:-StoryBuilderStudio}"
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

ALL_LIBS=("llama.cpp" "audio.cpp" "kokoro" "stable-diffusion")
WITH_LIST=""
WITHOUT_LIST=""
USE_ALL=false

usage() {
    cat <<EOF
Usage: $0 [options]
  --all                    Build all local AI libs (llama.cpp, audio.cpp, kokoro, stable-diffusion)
  --with="a,b"             Build ONLY the specified comma-separated libs
  --without="a,b"          Build ALL libs EXCEPT the specified comma-separated libs
  --build-dir=DIR          CMake build directory (default: build)
  -h, --help               Show this help
EOF
}

normalize_lib() {
    local name
    name="$(echo "$1" | tr '[:upper:]' '[:lower:]' | xargs)"
    case "$name" in
        llama|llama.cpp)                            echo "llama.cpp" ;;
        audio|audio.cpp|audiocpp)                   echo "audio.cpp" ;;
        kokoro|kororo|sherpa-onnx|onnxruntime)      echo "kokoro" ;;
        stable-diffusion|stable-diffusion.cpp|standard-diffusion|sd) echo "stable-diffusion" ;;
        "")                                         echo "" ;;
        *) echo "ERROR: Unknown library '$1'" >&2; exit 1 ;;
    esac
}

for arg in "$@"; do
    case "$arg" in
        --all)          USE_ALL=true ;;
        --with=*)       WITH_LIST="${arg#*=}" ;;
        --without=*)    WITHOUT_LIST="${arg#*=}" ;;
        --build-dir=*)  BUILD_DIR="${arg#*=}" ;;
        -h|--help)      usage; exit 0 ;;
        *)              echo "Unknown option: $arg" >&2; usage; exit 1 ;;
    esac
done

# Resolve final library set (defaults to none)
SELECTED=()
if [ "$USE_ALL" = true ] || { [ -n "$WITHOUT_LIST" ] && [ -z "$WITH_LIST" ]; }; then
    SELECTED=("${ALL_LIBS[@]}")
fi

if [ -n "$WITH_LIST" ]; then
    IFS=',' read -ra RAW <<< "$WITH_LIST"
    for item in "${RAW[@]}"; do
        norm="$(normalize_lib "$item")"
        [ -n "$norm" ] && SELECTED+=("$norm")
    done
fi

if [ -n "$WITHOUT_LIST" ]; then
    IFS=',' read -ra RAW <<< "$WITHOUT_LIST"
    for item in "${RAW[@]}"; do
        norm="$(normalize_lib "$item")"
        FILTERED=()
        for s in "${SELECTED[@]}"; do
            [ "$s" != "$norm" ] && FILTERED+=("$s")
        done
        SELECTED=("${FILTERED[@]}")
    done
fi

# 1. Clone or update StoryBuilderStudio
if [ -f "CMakeLists.txt" ]; then
    echo "==> Updating existing repository..."
    git pull --ff-only || true
else
    [ ! -d "$REPO_DIR" ] && git clone "$REPO_URL" "$REPO_DIR"
    cd "$REPO_DIR"
    git pull --ff-only || true
fi

# 2. Platform & generator detection
GEN_ARGS=()
command -v ninja >/dev/null 2>&1 && GEN_ARGS=(-G Ninja)

OS_NAME="$(uname -s)"
if [ "$OS_NAME" = "Darwin" ]; then
    QT_PREFIX="$(brew --prefix qt@6 2>/dev/null || true)"
    [ -z "$QT_PREFIX" ] && { echo "ERROR: qt@6 not found. Run: brew install qt@6" >&2; exit 1; }
    JOBS="$(sysctl -n hw.ncpu)"
else
    QT_PREFIX=""
    JOBS="$(nproc)"
fi

# 3. Clone & build selected 3rdParty libraries
THIRD_PARTY="$PWD/3rdParty"
INC_LIST=()
LIB_LIST=()

clone_or_pull() {
    local url="$1" dir="$2"
    mkdir -p "$THIRD_PARTY"
    if [ -d "$dir/.git" ]; then
        git -C "$dir" pull --ff-only || true
    else
        git clone --depth 1 --recursive "$url" "$dir"
    fi
}

for lib in "${SELECTED[@]}"; do
    echo "==> Preparing $lib..."
    case "$lib" in
        llama.cpp)
            DIR="$THIRD_PARTY/llama.cpp"
            clone_or_pull "https://github.com/ggml-org/llama.cpp.git" "$DIR"
            cmake -S "$DIR" -B "$DIR/build" "${GEN_ARGS[@]}" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_SHARED_LIBS=ON
            cmake --build "$DIR/build" --target llama -j"$JOBS"
            INC_LIST+=("$DIR/include" "$DIR/ggml/include")
            LIB_LIST+=("$DIR/build/bin" "$DIR/build/src")
            ;;
        audio.cpp)
            DIR="$THIRD_PARTY/audio.cpp"
            clone_or_pull "https://github.com/0xShug0/audio.cpp.git" "$DIR"
            cmake -S "$DIR" -B "$DIR/build" "${GEN_ARGS[@]}" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_SHARED_LIBS=ON
            cmake --build "$DIR/build" -j"$JOBS"
            INC_LIST+=("$DIR/include")
            LIB_LIST+=("$DIR/build/bin" "$DIR/build/lib" "$DIR/build")
            ;;
        kokoro)
            DIR="$THIRD_PARTY/sherpa-onnx"
            clone_or_pull "https://github.com/k2-fsa/sherpa-onnx.git" "$DIR"
            cmake -S "$DIR" -B "$DIR/build" "${GEN_ARGS[@]}" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
                -DBUILD_SHARED_LIBS=ON \
                -DSHERPA_ONNX_ENABLE_BINARY=OFF \
                -DCMAKE_INSTALL_PREFIX="$DIR/build/install"
            cmake --build "$DIR/build" --target install -j"$JOBS"
            INC_LIST+=("$DIR/build/install/include")
            LIB_LIST+=("$DIR/build/install/lib")
            ;;
        stable-diffusion)
            DIR="$THIRD_PARTY/stable-diffusion.cpp"
            clone_or_pull "https://github.com/leejet/stable-diffusion.cpp.git" "$DIR"
            cmake -S "$DIR" -B "$DIR/build" "${GEN_ARGS[@]}" \
                -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_SHARED_LIBS=ON
            cmake --build "$DIR/build" -j"$JOBS"
            INC_LIST+=("$DIR")
            LIB_LIST+=("$DIR/build/bin" "$DIR/build")
            ;;
    esac
done

# 4. Configure, build, and test StoryBuilderStudio
CMAKE_ARGS=(-B "$BUILD_DIR" "${GEN_ARGS[@]}" -DCMAKE_BUILD_TYPE="$BUILD_TYPE")
[ -n "$QT_PREFIX" ] && CMAKE_ARGS+=(-DCMAKE_PREFIX_PATH="$QT_PREFIX")

if [ ${#INC_LIST[@]} -gt 0 ]; then
    CMAKE_ARGS+=(-DLOCAL_INCLUDE_PATHS="$(IFS=';'; echo "${INC_LIST[*]}")")
    CMAKE_ARGS+=(-DLOCAL_DLL_PATHS="$(IFS=';'; echo "${LIB_LIST[*]}")")
fi

echo "==> Configuring StoryBuilderStudio (libs: ${SELECTED[*]:-none})..."
cmake "${CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" -j"$JOBS"
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "==> Done! Binary is in: $PWD/$BUILD_DIR"
