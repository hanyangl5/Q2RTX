@echo off
setlocal

set "APK=.\build\androidapprelease\app\build\outputs\apk\release\app-release.apk"
set "DATA_DIR=%~dp0baseq2"

echo Validating local game data before install...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "$base = [System.IO.Path]::GetFullPath('%DATA_DIR%');" ^
  "if (-not (Test-Path -LiteralPath $base)) { throw 'Missing baseq2 directory.' }" ^
  "$paks = Get-ChildItem -LiteralPath $base -Filter 'pak*.pak' -File;" ^
  "if (-not $paks) { throw 'No pak*.pak files were found in baseq2.' }" ^
  "foreach ($pak in $paks) {" ^
  "  $stream = [System.IO.File]::OpenRead($pak.FullName);" ^
  "  try {" ^
  "    $header = New-Object byte[] 4;" ^
  "    if ($stream.Read($header, 0, 4) -ne 4) { throw ('Failed to read header from ' + $pak.Name) }" ^
  "  } finally {" ^
  "    $stream.Dispose();" ^
  "  }" ^
  "  if ([System.Text.Encoding]::ASCII.GetString($header) -ne 'PACK') {" ^
  "    throw ('Invalid Quake PAK header in ' + $pak.FullName + '. Expected PACK, got ' + ([BitConverter]::ToString($header)))" ^
  "  }" ^
  "}" ^
  "$zipChecks = @('q2rtx_media.pkz', 'blue_noise.pkz');" ^
  "foreach ($name in $zipChecks) {" ^
  "  $path = Join-Path $base $name;" ^
  "  if (-not (Test-Path -LiteralPath $path)) { continue }" ^
  "  $stream = [System.IO.File]::OpenRead($path);" ^
  "  try {" ^
  "    $header = New-Object byte[] 2;" ^
  "    if ($stream.Read($header, 0, 2) -ne 2) { throw ('Failed to read header from ' + $name) }" ^
  "  } finally {" ^
  "    $stream.Dispose();" ^
  "  }" ^
  "  if ([System.Text.Encoding]::ASCII.GetString($header) -ne 'PK') {" ^
  "    throw ('Invalid ZIP header in ' + $path + '. Expected PK, got ' + ([BitConverter]::ToString($header)))" ^
  "  }" ^
  "}" ^
  "Write-Host 'Game data validation passed.'"
if errorlevel 1 exit /b 1

adb  -s 0182200148488540 install "%APK%"
adb  -s 0182200148488540 shell ls /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb  -s 0182200148488540 shell mkdir -p /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb  -s 0182200148488540 push baseq2 /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/
adb  -s 0182200148488540 shell chmod -R 777 /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx
adb  -s 0182200148488540 shell ls /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2
adb  -s 0182200148488540 shell cat /storage/emulated/0/Android/data/com.nvidia.q2rtx/files/q2rtx/baseq2/logs/console.log
