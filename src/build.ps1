$appName = "app"

$scriptFolder = $PSScriptRoot

Set-Location -Path $scriptFolder

Get-ChildItem -Path $scriptFolder -Recurse -Filter *.cpp | Select-Object -ExpandProperty FullName | ForEach-Object { $_ -replace '\\', '/' } | Out-File -FilePath sources.txt -Encoding ascii

g++ -std=c++20 '@sources.txt' -o $appName

if ($?) { 
    & ".\$appName"
}