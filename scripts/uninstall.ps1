param(
    [string]$BuildDir = "",
    [string]$Prefix = "$env:LOCALAPPDATA\Programs\StoryBuilderStudio"
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $Candidate = Get-ChildItem -Path (Join-Path $RepoRoot "build") -Directory -ErrorAction SilentlyContinue |
                 Where-Object { Test-Path (Join-Path $_.FullName "install_manifest.txt") } |
                 Select-Object -First 1
    if ($Candidate) {
        $BuildDir = $Candidate.FullName
    }
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
