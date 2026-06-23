@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0start-https-server.ps1" %*
