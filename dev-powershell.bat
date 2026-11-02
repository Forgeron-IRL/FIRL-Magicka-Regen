@echo off
set "DEVSHELL=C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\Launch-VsDevShell.ps1"
set "PROJECT=D:\Modding\FIRL's mods\FIRL Magicka Regen"
powershell.exe -NoExit -ExecutionPolicy Bypass -Command "& $env:DEVSHELL -Arch amd64 -HostArch amd64; Set-Location -LiteralPath $env:PROJECT"