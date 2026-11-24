param(
    [string]$BuildDir = "",
    [string]$Prefix = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Prefix)) {
    if ((Split-Path -Leaf $PSScriptRoot) -ieq "bin") {
        $Prefix = Split-Path -Parent $PSScriptRoot
    } else {
        $Prefix = "$env:LOCALAPPDATA\Programs\StoryBuilderStudio"
    }
}

$ParentOfScript = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir) -and (Test-Path (Join-Path $ParentOfScript "build"))) {
    $Candidate = Get-ChildItem -Path (Join-Path $ParentOfScript "build") -Directory -ErrorAction SilentlyContinue |
                 Where-Object { Test-Path (Join-Path $_.FullName "install_manifest.txt") } |
                 Select-Object -First 1
    if ($Candidate) {
        $BuildDir = $Candidate.FullName
    }
}

if ($PWD.Path.StartsWith($Prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    Set-Location $env:TEMP
}

$Manifest = if ($BuildDir) { Join-Path $BuildDir "install_manifest.txt" } else { "" }
if ($Manifest -and (Test-Path $Manifest)) {
    Write-Host "[INFO] Removing files listed in $Manifest..."
    Get-Content $Manifest | ForEach-Object {
        if ($_ -and (Test-Path $_)) {
            Remove-Item -Force $_
            Write-Host "  Removed: $_"
        }
    }
}

if (Test-Path $Prefix) {
    Remove-Item -Recurse -Force $Prefix
    Write-Host "[INFO] Removed installation folder: $Prefix"
}

Write-Host "[SUCCESS] Story Builder Studio uninstalled."
