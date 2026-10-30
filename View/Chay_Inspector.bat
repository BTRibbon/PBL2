@echo off
chcp 65001 > nul
title PBL2 - Inspector Theo Doi & Giai Ma Du Lieu
cd /d "%~dp0"

taskkill /F /IM inspector.exe >nul 2>&1
timeout /t 1 /nobreak >nul

g++ -std=c++17 -O2 inspector.cpp -o inspector.exe
if %errorlevel% neq 0 (
    echo [-] Loi khi bien dich inspector.cpp!
    pause
    exit /b %errorlevel%
)

inspector.exe
pause
