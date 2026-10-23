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

$dataFolder = Join-Path $scriptFolder "data"
$accountSource = Join-Path $dataFolder "accounts.txt"
$accountDatabase = Join-Path $dataFolder "accounts.dat"
if (-not (Test-Path $accountSource)) {
    throw "Account source data was not found at $accountSource"
}

function Get-PasswordHash([string]$password, [string]$salt) {
    Add-Type -AssemblyName System.Numerics
    $text = [System.Text.Encoding]::UTF8.GetBytes($salt + ":" + $password)
    $mask = ([System.Numerics.BigInteger]::One -shl 64) - 1
    $hash = [System.Numerics.BigInteger]::Parse("14695981039346656037")
    foreach ($byte in $text) {
        $hash = $hash -bxor [UInt64]$byte
        $hash = ($hash * 1099511628211) -band $mask
    }
    $hex = $hash.ToString("x")
    if ($hex.Length -gt 16) {
        $hex = $hex.Substring($hex.Length - 16)
    }
    return $hex.PadLeft(16, "0")
}

$encodedAccounts = New-Object System.Collections.Generic.List[byte]
foreach ($line in [System.IO.File]::ReadAllLines($accountSource)) {
    if ([string]::IsNullOrWhiteSpace($line)) { continue }
    $fields = $line.Split("|")
    if ($fields.Count -ne 6) {
        throw "Invalid account record: $line"
    }
    $salt = [guid]::NewGuid().ToString("N")
    $passwordHash = Get-PasswordHash $fields[5] $salt
    $record = ($fields[0..4] -join "|") + "|" + $salt + "|" + $passwordHash + "`n"
    foreach ($byte in [System.Text.Encoding]::UTF8.GetBytes($record)) {
        $encodedAccounts.Add([byte]($byte -bxor 0x5A))
    }
}
[System.IO.File]::WriteAllBytes($accountDatabase, $encodedAccounts.ToArray())
Write-Host "Generated encrypted account database: $accountDatabase"

$sources = @(Get-ChildItem -Path $scriptFolder -Recurse -Filter *.cpp -File |
    Where-Object { $_.FullName -notmatch "[\\/]Utils[\\/]raylib-" } |
    ForEach-Object {
        $prefix = $scriptFolder.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
        if ($_.FullName.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
            $rel = $_.FullName.Substring($prefix.Length)
        } else {
            $rel = $_.Name
        }
        $rel -replace '\\', '/'
    } | Sort-Object)

if ($sources.Count -eq 0) {
    throw "No C++ source files were found."
}

$sources | Out-File -FilePath $sourceList -Encoding ascii

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