param(
    [string]$BuildDir = "",
    [string]$Prefix = "$env:LOCALAPPDATA\Programs\StoryBuilderStudio"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $Candidate = Get-ChildItem -Path (Join-Path $RepoRoot "build") -Directory -ErrorAction SilentlyContinue |
                 Where-Object { Test-Path (Join-Path $_.FullName "CMakeCache.txt") } |
                 Select-Object -First 1
    if ($Candidate) {
        $BuildDir = $Candidate.FullName
    } elseif (Test-Path (Join-Path $RepoRoot "build\CMakeCache.txt")) {
        $BuildDir = Join-Path $RepoRoot "build"
    } else {
        Write-Error "No configured CMake build directory found. Pass -BuildDir <path>."
    }
}

Write-Host "[INFO] Installing Story Builder Studio from '$BuildDir' to '$Prefix'..."
cmake --install "$BuildDir" --prefix "$Prefix"

$BinDir = Join-Path $Prefix "bin"
$Windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($Windeployqt) {
    $ExePath = Join-Path $BinDir "appStoryBuilderStudio.exe"
    $QmlDir  = Join-Path $RepoRoot "app"
    Write-Host "[INFO] Running windeployqt on $ExePath..."
    & $Windeployqt.Source --qmldir "$QmlDir" --no-translations "$ExePath"
}

Write-Host "[SUCCESS] Installed to $BinDir"
