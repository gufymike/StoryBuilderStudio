<div align="center">

# 🎬 Story Builder Studio

**Offline-capable, cross-platform generative narrative studio**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![Qt 6](https://img.shields.io/badge/Qt-6.8%2B%20%7C%206.11%2B-41CD52?style=for-the-badge&logo=qt&logoColor=white)](https://www.qt.io/)
[![Platforms](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-6E40C9?style=for-the-badge)](#3-building-from-source)
[![Engines](https://img.shields.io/badge/AI-Local%20Plugins%20%2B%20REST-F59E0B?style=for-the-badge)](#5-core-engine-dynamic-libraries)

Orchestrates local multi-modal engines (LLM text, image diffusion, video DiT, sound effects, and text-to-speech) alongside remote REST APIs, project management, and audit ledger systems.

</div>

---

## 1. Quick Start (Automated Build Scripts)

Use the included automation scripts in `scripts/` to clone/update, configure, build, and run the test suite in one step:

* **Linux & macOS:**
  * ```chmod +x scripts/build.sh  ```
  
  * #### REST-only (default)
  * ```./scripts/build.sh```
  
  * #### Everything
  * ```./scripts/build.sh --all```
  
  * #### Only llama.cpp and audio.cpp
  * ```./scripts/build.sh --with="llama.cpp,audio.cpp"```
  
  * #### Everything except llama.cpp (e.g., if using Fedora's system llama-cpp-devel)
  * ```./scripts/build.sh --without="llama.cpp"```
* **Windows (PowerShell):**
  ```.\scripts\build.ps1 -QtPrefix "C:\Qt\6.11.0\llvm-mingw_64"```
  
  * #### REST-only (default)
  * ```.\scripts\build.ps1 -QtPrefix "C:\Qt\6.11.0\llvm-mingw_64"```
  
  * #### Everything
  * ```.\scripts\build.ps1 -All -QtPrefix "C:\Qt\6.11.0\llvm-mingw_64"```
  
  * #### Only llama.cpp and audio.cpp
  * ```.\scripts\build.ps1 -With "llama.cpp,audio.cpp" -QtPrefix "C:\Qt\6.11.0\llvm-mingw_64"```
  
  * #### Everything except llama.cpp 
  * ```.\scripts\build.ps1 -Without "llama.cpp" -QtPrefix "C:\Qt\6.11.0\llvm-mingw_64"```

> [!TIP]
> Pass `LOCAL_INCLUDE_PATHS` and `LOCAL_DLL_PATHS` (environment variables on Linux/macOS, or `-LocalIncludePaths` / `-LocalDllPaths` on Windows) to enable optional local AI plugins during the build.

---

## 2. System Requirements & Runtimes

### Minimum Qt Version
* **Qt 6.8+** (Qt 6.11 / 6.12 recommended; supported without code changes)
* Install via the [Qt Online Installer](https://doc.qt.io/qt-6/qt-online-installation.html), your Linux distribution's package manager, or Homebrew on macOS.

### Recommended Compiler Targets (C++20)
* **Windows:** `llvm-mingw` or `MinGW 64-bit` (e.g., `C:\Qt\6.11.x\llvm-mingw_64` or `C:\Qt\6.11.x\mingw_64`)
* **Linux:** `GCC 64-bit` or `Clang` (`/opt/Qt/6.11.x/gcc_64` or system Qt 6)
* **macOS:** `Clang 64-bit` (Xcode Command Line Tools + Homebrew `qt@6`)

---

## 3. Building from Source

Story Builder Studio supports two build modes out of the box:

* **Core / REST-Only (Default):** Builds immediately with only Qt 6 and a C++20 compiler. Supports Gemini, OpenAI, and local Ollama REST endpoints with zero extra AI dependencies.
* **Local AI Plugins (Optional):** Automatically enables local inference plugins (`LocalLlmEngine`, `KokoroTtsEngine`, etc.) when their shared libraries and headers are detected by CMake.

### 🐧 Linux (Fedora, Debian/Ubuntu, Arch)

#### 1. Core Dependencies
* **Fedora:**
  ```bash
  sudo dnf install -y git cmake ninja-build gcc-c++ qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtmultimedia-devel qt6-qtsvg-devel
  ```
* **Debian 13 / Ubuntu:**
  ```bash
  sudo apt update && sudo apt install -y git cmake ninja-build g++ qt6-base-dev qt6-declarative-dev qt6-multimedia-dev libqt6svg6-dev
  ```

#### 2. Optional Local AI Libraries
> [!IMPORTANT]
> **Check your distribution's repositories first.** Some distros ship pre-packaged `-devel` libraries (for example, Fedora provides `llama-cpp-devel`, which links ROCm/HIP and falls back to CPU if no AMD GPU is present). If a library is unavailable or outdated in your distro repos, build it from GitHub as a shared library.

Example building `llama.cpp` from source (CPU-only; omit `-DGGML_HIP=OFF -DGGML_CUDA=OFF` or enable `-DGGML_CUDA=ON` / `-DGGML_VULKAN=ON` for GPU acceleration):
```bash
git clone --depth 1 https://github.com/ggml-org/llama.cpp.git
cmake -S llama.cpp -B llama.cpp/build -G Ninja \
  -DBUILD_SHARED_LIBS=ON -DGGML_HIP=OFF -DGGML_CUDA=OFF
cmake --build llama.cpp/build --target llama
```

Repeat the same shared-library build pattern for any other optional local backends (`sherpa-onnx`, `onnxruntime`, `stable-diffusion.cpp`, `audiocpp`), then point CMake to their include and library directories.

#### 3. Configure & Build
```bash
# Core (REST-only) or using system-installed libraries:
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure

# With custom source-built local libraries:
cmake -B build-plugins -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DLOCAL_INCLUDE_PATHS="/path/to/llama.cpp/include;/path/to/other/include" \
  -DLOCAL_DLL_PATHS="/path/to/llama.cpp/build/bin;/path/to/other/lib"
cmake --build build-plugins
```

### 🍎 macOS *(Generic / Community-Tested)*

> [!NOTE]
> macOS builds are currently untested on Apple hardware. Apple Silicon builds of `llama.cpp` enable Metal GPU acceleration by default.

1. Install Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. Install build tools and Qt 6 via Homebrew:
   ```bash
   brew install git cmake ninja qt@6
   ```
3. Configure and build (Homebrew installs `qt@6` keg-only, so `CMAKE_PREFIX_PATH` is required):
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
     -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```

### 🪟 Windows (10 / 11)

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/llvm-mingw_64"
cmake --build build
ctest --test-dir build --output-on-failure
```
Add `-DLOCAL_INCLUDE_PATHS="..."` and `-DLOCAL_DLL_PATHS="..."` to build optional local `.dll` plugins.

---

## 4. Remote API Key Setup

Configure environment variables for any cloud or custom REST providers you use:

### Google Gemini (`gemini_bridge`)
* **Variable:** `GEMINI_API_KEY` (or `GOOGLE_API_KEY`)
* **Windows (PowerShell):**
  ```powershell
  [System.Environment]::SetEnvironmentVariable('GEMINI_API_KEY', 'your_key_here', 'User')
  ```
* **Linux / macOS:**
  ```bash
  echo 'export GEMINI_API_KEY="your_key_here"' >> ~/.bashrc  # or ~/.zshrc
  source ~/.bashrc
  ```

### OpenAI (`openai_bridge`)
* **Variable:** `OPENAI_API_KEY`
* **Windows (PowerShell):**
  ```powershell
  [System.Environment]::SetEnvironmentVariable('OPENAI_API_KEY', 'your_key_here', 'User')
  ```
* **Linux / macOS:**
  ```bash
  echo 'export OPENAI_API_KEY="your_key_here"' >> ~/.bashrc  # or ~/.zshrc
  source ~/.bashrc
  ```

### Custom REST Endpoints
> [!NOTE]
> For security, custom environment variables must end in `_API_KEY` or `_TOKEN` (for example, `MY_CUSTOM_LLM_API_KEY`).

---

## 5. Core Engine Dynamic Libraries

For standalone deployments, place pre-built dynamic shared libraries directly in the root binary directory alongside `appStoryBuilderStudio.exe` (or in your system library path on Linux/macOS):

| Engine | Upstream Release / Source | Required Shared Libraries (Windows / Linux Equivalent) |
| :--- | :--- | :--- |
| **Script / Narrative** | [llama.cpp](https://github.com/ggml-org/llama.cpp) | `llama.dll`, `ggml.dll`, `ggml-base.dll`, `ggml-cpu.dll` (`libllama.so`) |
| **Image & Video** | [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp/releases) | `stable-diffusion.dll` (`libstable-diffusion.so`) |
| **Audio FX / BGM** | [audio.cpp](https://github.com/0xShug0/audio.cpp/releases) | `audiocpp.dll` (`libaudiocpp.so`) |
| **Speech (TTS)** | [Sherpa-ONNX](https://github.com/k2-fsa/sherpa-onnx/releases) | `sherpa-onnx-c-api.dll`, `onnxruntime.dll` (`libsherpa-onnx-c-api.so`) |

---

## 6. Launching & Installing Standalone Binaries

### Installing from a Source Build
* **Windows (PowerShell):** `.\scripts\install.ps1` (removes via `.\scripts\uninstall.ps1` or `<Prefix>\bin\uninstall.ps1`)
* **Linux / macOS:** `./scripts/install.sh` (removes via `./scripts/uninstall.sh`)

### Running Unbundled Developer Builds
If you are running directly from a build directory without embedded Qt libraries:

#### 🪟 Windows
* **Option A — Launcher Script (`scripts\storybuilderstudio.bat`, Recommended):**
  Set `QT_BIN_DIR` (or `PATH`) and optional `LOCAL_DLL_PATH` in your environment, then run `storybuilderstudio.bat`.
* **Option B — Persistent User Environment Variable:**
  1. Press `Win + R`, type `sysdm.cpl`, and press **Enter**.
  2. Navigate to **Advanced → Environment Variables**.
  3. Under **User variables**, edit `Path` and append `C:\Qt\6.11.0\llvm-mingw_64\bin`.

#### 🐧 Linux
```bash
./scripts/storybuilderstudio.sh
# Or directly (not required if built against system Qt 6):
LD_LIBRARY_PATH=/opt/Qt/6.11.0/gcc_64/lib ./appStoryBuilderStudio
  ```
  Make executable with `chmod +x run.sh`. *(Not required if built against system Qt 6.)*

### 🍎 macOS
```bash
export DYLD_FRAMEWORK_PATH=$HOME/Qt/6.11.0/macos/lib:$DYLD_FRAMEWORK_PATH
export DYLD_LIBRARY_PATH=$HOME/Qt/6.11.0/macos/lib:$DYLD_LIBRARY_PATH
./appStoryBuilderStudio
```

---

## 7. Model Directory Architecture

Story Builder Studio automatically queries a standardized model directory in your user application data path:

* **Windows:** `%APPDATA%\StoryBuilderStudio\`
* **Linux:** `~/.local/share/StoryBuilderStudio/`
* **macOS:** `~/Library/Application Support/StoryBuilderStudio/`

```text
StoryBuilderStudio/
├── text/
│   ├── Qwen3-8B-Q4_K_M.gguf
│   └── tokenizer.json
├── images/
│   └── flux/
│       ├── flux1-schnell-q4_k_m.gguf
│       ├── ae.safetensors
│       ├── clip_l.safetensors
│       └── t5-v1_1-xxl-encoder-Q5_K_M.gguf
├── video/
│   └── wan/
│       ├── wan2.1_t2v_1.3b-q4_k_m.gguf
│       ├── wan_2.1_vae.safetensors
│       └── umt5-xxl-encoder-Q5_K_M.gguf
└── audio/
    ├── speech/
    │   └── Kokoro/
    │       ├── model.onnx
    │       ├── voices.bin
    │       ├── tokens.txt
    │       ├── lexicon.txt
    │       └── espeak-ng-data/
    ├── sfx/
    │   └── stable-audio-3.gguf
    └── bgm/
```

---

## 8. HuggingFace Model Manifest & Setup

### A. Shared Base Text Encoder
Universal T5 Transformer encoder providing text conditioning shared across image and video workflows:
* **Repository:** [city96/t5-v1_1-xxl-encoder-gguf](https://huggingface.co/city96/t5-v1_1-xxl-encoder-gguf/tree/main)
* **File:** `t5-v1_1-xxl-encoder-Q5_K_M.gguf`
* **Target:** `images/flux/t5-v1_1-xxl-encoder-Q5_K_M.gguf`

### B. Image Pipeline (FLUX.1 Schnell)
Place all four components inside `images/flux/`:
1. **Diffusion Model:** [city96/FLUX.1-schnell-gguf](https://huggingface.co/city96/FLUX.1-schnell-gguf/tree/main) (`flux1-schnell-q4_k_m.gguf` or similar)
2. **VAE Decoder:** [carlosrozas/ae.safetensors](https://huggingface.co/carlosrozas/ae.safetensors) (`ae.safetensors`)
3. **Text Encoder:** [DanielMarius/clip_l.safetensors](https://huggingface.co/DanielMarius/clip_l.safetensors/tree/main) (`clip_l.safetensors`)
4. **T5 Encoder:** Shared T5 encoder placed here or referenced via Settings.

### C. Video Pipeline (Wan 2.1 DiT)
Place video generation weights inside `video/<model_name>/` (e.g., `video/wan/`):
* **Repository:** [Wan-AI / HuggingFace Mirrors](https://huggingface.co/Wan-AI)
* **Files:** `wan2.1_t2v_1.3b-q4_k_m.gguf`, `wan_2.1_vae.safetensors`, `umt5-xxl-encoder-Q5_K_M.gguf` (or fallback to shared global T5)

### D. Audio Pipeline (`audio.cpp` GGUF Models)
Place audio synthesis and sound effect weights inside `audio/` (or `audio/sfx/`):
* **Repository:** [audio-cpp/audio.cpp-gguf](https://huggingface.co/audio-cpp/audio.cpp-gguf)
* **Files:** Supported GGUF audio weights (e.g., `stable-audio-3.gguf`)
* The application automatically scans the folder, strips the extension, and lists the model family inside the Audio prompt workspace.

### E. Speech Pipeline (Kokoro TTS via Sherpa-ONNX)
Place extracted model assets inside `audio/speech/Kokoro/`:
* **Repository:** [csukuangfj/kokoro-en-v0_19](https://huggingface.co/csukuangfj/kokoro-en-v0_19/tree/main)
* **Required Files:** `model.onnx`, `voices.bin`, `tokens.txt`, `lexicon.txt` (or `lexicon-us-en.txt`), and `espeak-ng-data/`

### F. Script & Narrative Pipeline (Qwen 3 / ChatML)
Place the language model and tokenizer inside `text/`:
* **GGUF Weights:** [Qwen/Qwen3-8B-GGUF](https://huggingface.co/Qwen/Qwen3-8B-GGUF) (`Qwen3-8B-Q4_K_M.gguf`)
* **Tokenizer Metadata:** [Qwen/Qwen3-8B](https://huggingface.co/Qwen/Qwen3-8B) (`tokenizer.json`)

---

## 9. First-Run Health Checklist

1. **Verify Plugins & Manifests:** Open **Help → About** to confirm loaded vs. missing local plugins, then check the **Model Settings / Manifest** tab to ensure required `.gguf`, `.onnx`, and `.safetensors` weights are highlighted green.
2. **Audio Waveform Initialization:** In the Audio workspace, select a track or synthesize a sentence to confirm `QAudioDecoder` loads and renders waveform peaks without runtime errors.
3. **Audit Trail Verification:** Confirm `prompt_history.json` and `think_history.json` are created in active project folders to log prompt dispatches and reasoning streams.

## 10. License & Third-Party Notices

Story Builder Studio is licensed under the **MIT License** — see [LICENSE](LICENSE) for full terms.

Optional and runtime dependencies retain their respective open-source licenses:
* **Qt 6** — GNU LGPL v3 / GPL v2 / GPL v3 (dynamically linked)
* **llama.cpp**, **stable-diffusion.cpp**, **ONNX Runtime** — MIT License
* **sherpa-onnx** — Apache License 2.0
* **LLVM MinGW Runtime** (`libc++`, `libunwind`, `libomp`) — Apache License 2.0 with LLVM Exception
