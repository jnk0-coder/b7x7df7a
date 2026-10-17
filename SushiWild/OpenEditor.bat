@echo off
rem Sushi Wild: same as BuildAndPlay.bat, but opens the Unreal Editor instead of the game.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\BuildAndPlay.ps1" -Editor
echo.
pause
