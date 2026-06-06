@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0make-dev-cert.ps1" %*
