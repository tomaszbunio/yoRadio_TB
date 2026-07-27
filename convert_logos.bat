@echo off
cd /d "%~dp0"
if exist "%USERPROFILE%\.platformio\penv\Scripts\python.exe" (
  "%USERPROFILE%\.platformio\penv\Scripts\python.exe" scripts\convert_logos.py --all-resolutions
) else (
  python scripts\convert_logos.py --all-resolutions
)
pause
