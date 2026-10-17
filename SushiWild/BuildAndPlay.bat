@echo off
rem Sushi Wild: find Unreal Engine, match project version, build, then launch the game.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\BuildAndPlay.ps1"
echo.
pause
