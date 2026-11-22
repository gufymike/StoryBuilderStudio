param(
    [switch]$All,
    [string]$With = "",
    [string]$Without = "",
    [string]$RepoUrl = "https://github.com/<YOUR_USER>/StoryBuilderStudio.git",
    [string]$RepoDir = "StoryBuilderStudio",
    [string]$BuildDir = "build",
    [string]$BuildType = "Release",
    [string]$QtPrefix = ""
)

$ErrorActionPreference = "Stop"

$AllLibs = @("llama.cpp", "audio.cpp", "kokoro", "stable-diffusion")

function Normalize-Lib([string]$Name) {
    switch ($Name.Trim().ToLower()) {
        { $_ -in "llama", "llama.cpp" }                                          { return "llama.cpp" }
        { $_ -in "audio", "audio.cpp", "audiocpp" }                              { return "audio.cpp" }
        { $_ -in "kokoro", "kororo", "sherpa-onnx", "onnxruntime" }              { return "kokoro" }
        { $_ -in "stable-diffusion", "stable-diffusion.cpp", "standard-diffusion", "sd" } { return "stable-diffusion" }
        ""                                                                       { return "" }
        default { throw "Unknown library: '$Name'" }
    }
}

# Resolve final library set (defaults to none)
$Selected = [System.Collections.Generic.List[string]]::new()
if ($All -or ($Without -and -not $With)) {
    foreach ($l in $AllLibs) { $Selected.Add($l) }
}
if ($With) {
    foreach ($item in $With.Split(",")) {
        $norm = Normalize-Lib $item
        if ($norm -and -not $Selected.Contains($norm)) { $Selected.Add($norm) }
    }
}
if ($Without) {
    foreach ($item in $Without.Split(",")) {
        $norm = Normalize-Lib $item
        if ($norm) { [void]$Selected.Remove($norm) }
    }
}

# 1. Clone or update StoryBuilderStudio
if (Test-Path "CMakeLists.txt") {
    Write-Host "==> Updating existing repository..."
    git pull --ff-only
} else {
    if (-not (Test-Path $RepoDir)) { git clone $RepoUrl $RepoDir }
    Set-Location $RepoDir
    git pull --ff-only
}

$GenArgs = @()
if (Get-Command ninja -ErrorAction SilentlyContinue) { $GenArgs = @("-G", "Ninja") }

# 2. Clone & build selected 3rdParty libraries
$ThirdParty = Join-Path (Get-Location).Path "3rdParty"
$IncList = [System.Collections.Generic.List[string]]::new()
$LibList = [System.Collections.Generic.List[string]]::new()

function Clone-Or-Pull([string]$Url, [string]$Dir) {
    New-Item -ItemType Directory -Force -Path $ThirdParty | Out-Null
    if (Test-Path (Join-Path $Dir ".git")) {
        git -C $Dir pull --ff-only
    } else {
        git clone --depth 1 --recursive $Url $Dir
    }
}

foreach ($lib in $Selected) {
    Write-Host "==> Preparing $lib..."
    switch ($lib) {
        "llama.cpp" {
            $dir = Join-Path $ThirdParty "llama.cpp"
            Clone-Or-Pull "https://github.com/ggml-org/llama.cpp.git" $dir
            cmake -S $dir -B "$dir\build" @GenArgs -DCMAKE_BUILD_TYPE=$BuildType -DBUILD_SHARED_LIBS=ON
            cmake --build "$dir\build" --target llama --parallel
            $IncList.Add("$dir\include"); $IncList.Add("$dir\ggml\include")
            $LibList.Add("$dir\build\bin"); $LibList.Add("$dir\build\src")
        }
        "audio.cpp" {
            $dir = Join-Path $ThirdParty "audio.cpp"
            Clone-Or-Pull "https://github.com/0xShug0/audio.cpp.git" $dir
            cmake -S $dir -B "$dir\build" @GenArgs -DCMAKE_BUILD_TYPE=$BuildType -DBUILD_SHARED_LIBS=ON
            cmake --build "$dir\build" --parallel
            $IncList.Add("$dir\include")
            $LibList.Add("$dir\build\bin"); $LibList.Add("$dir\build\lib"); $LibList.Add("$dir\build")
        }
        "kokoro" {
            $dir = Join-Path $ThirdParty "sherpa-onnx"
            Clone-Or-Pull "https://github.com/k2-fsa/sherpa-onnx.git" $dir
            cmake -S $dir -B "$dir\build" @GenArgs `
                -DCMAKE_BUILD_TYPE=$BuildType `
                -DBUILD_SHARED_LIBS=ON `
                -DSHERPA_ONNX_ENABLE_BINARY=OFF `
                -DCMAKE_INSTALL_PREFIX="$dir\build\install"
            cmake --build "$dir\build" --target install --parallel
            $IncList.Add("$dir\build\install\include")
            $LibList.Add("$dir\build\install\lib")
        }
        "stable-diffusion" {
            $dir = Join-Path $ThirdParty "stable-diffusion.cpp"
            Clone-Or-Pull "https://github.com/leejet/stable-diffusion.cpp.git" $dir
            cmake -S $dir -B "$dir\build" @GenArgs -DCMAKE_BUILD_TYPE=$BuildType -DBUILD_SHARED_LIBS=ON
            cmake --build "$dir\build" --parallel
            $IncList.Add($dir)
            $LibList.Add("$dir\build\bin"); $LibList.Add("$dir\build")
        }
    }
}

# 3. Configure, build, and test StoryBuilderStudio
$CmakeArgs = @("-B", $BuildDir) + $GenArgs + @("-DCMAKE_BUILD_TYPE=$BuildType")
if ($QtPrefix) { $CmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPrefix" }
if ($IncList.Count -gt 0) {
    $CmakeArgs += "-DLOCAL_INCLUDE_PATHS=$($IncList -join ';')"
    $CmakeArgs += "-DLOCAL_DLL_PATHS=$($LibList -join ';')"
}

$Summary = if ($Selected.Count -gt 0) { $Selected -join ", " } else { "none" }
Write-Host "==> Configuring StoryBuilderStudio (libs: $Summary)..."
cmake @CmakeArgs
cmake --build $BuildDir --parallel
ctest --test-dir $BuildDir --output-on-failure

Write-Host "==> Done! Binary is in: $((Get-Location).Path)\$BuildDir"
