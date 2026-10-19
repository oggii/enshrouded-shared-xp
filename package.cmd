@echo off
rem Builds, tests and writes the release zip dist\EnshroudedSharedXP-<version>.zip.
cd /d "%~dp0"
call "%~dp0build.cmd" || exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference = 'Stop';" ^
  "$v = [regex]::Match((Get-Content CMakeLists.txt -Raw), 'VERSION (\d+\.\d+\.\d+)').Groups[1].Value;" ^
  "$stage = 'dist\stage'; Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue; New-Item -ItemType Directory $stage | Out-Null;" ^
  "Copy-Item build\powrprof.dll, package\xpshare.ini, package\README.txt, LICENSE, THIRD_PARTY_NOTICES.md $stage;" ^
  "$zip = \"dist\EnshroudedSharedXP-$v.zip\"; Remove-Item $zip -ErrorAction SilentlyContinue;" ^
  "Compress-Archive -Path \"$stage\*\" -DestinationPath $zip; Remove-Item $stage -Recurse -Force;" ^
  "Write-Output \"$zip\"; Get-FileHash build\powrprof.dll -Algorithm SHA256 | ForEach-Object { 'powrprof.dll sha256 ' + $_.Hash.ToLower() }" || exit /b 1
