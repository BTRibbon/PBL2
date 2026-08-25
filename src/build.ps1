param(
    [switch]$NoRun,
    [string]$OutputName = "app.exe"
)

$ErrorActionPreference = "Stop"

$appName = $OutputName
$scriptFolder = $PSScriptRoot
$raylibFolder = Join-Path $scriptFolder "Utils\raylib-6.0_win64_mingw-w64"
$includeFolder = Join-Path $raylibFolder "include"
$libraryFolder = Join-Path $raylibFolder "lib"
$sourceList = Join-Path $scriptFolder "sources.txt"
$appPath = Join-Path $scriptFolder $appName

if (Test-Path $appPath) {
    try {
        $probe = [System.IO.File]::Open($appPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Write, [System.IO.FileShare]::Read)
        $probe.Dispose()
    } catch {
        $appName = "app-latest.exe"
        $appPath = Join-Path $scriptFolder $appName
        Write-Host "app.exe is in use; building and running $appName instead."
    }
}

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw "g++ was not found in PATH. Install MinGW-w64 or add its bin folder to PATH."
}

if (-not (Test-Path (Join-Path $includeFolder "raylib.h"))) {
    throw "raylib.h was not found at $includeFolder"
}

if (-not (Test-Path (Join-Path $libraryFolder "libraylib.a"))) {
    throw "libraylib.a was not found at $libraryFolder"
}

Set-Location -Path $scriptFolder

$sources = @(Get-ChildItem -Path $scriptFolder -Recurse -Filter *.cpp -File |
    Where-Object { $_.FullName -notmatch "[\\/]Utils[\\/]raylib-" } |
    Select-Object -ExpandProperty FullName)

if ($sources.Count -eq 0) {
    throw "No C++ source files were found."
}

$sources | ForEach-Object { $_ -replace '\\', '/' } | Out-File -FilePath $sourceList -Encoding ascii

$compileArgs = @(
    "-std=c++20",
    "-Wall",
    "-Wextra",
    "-I$includeFolder",
    "-L$libraryFolder",
    "@$sourceList",
    "-o",
    $appPath,
    "-lraylib",
    "-lopengl32",
    "-lgdi32",
    "-lwinmm"
)

& g++ @compileArgs
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

if (-not $NoRun) {
    Write-Host "Running: $appPath"
    & $appPath
}