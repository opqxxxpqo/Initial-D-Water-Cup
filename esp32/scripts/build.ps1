param([ValidateSet('build','upload')][string]$Action='build')
$ErrorActionPreference='Stop'
$python=Join-Path $env:USERPROFILE '.platformio/penv/Scripts/python.exe'
if(!(Test-Path $python)){throw 'PlatformIO Python not found'}
if(Test-Path 'P:\'){throw 'P: is occupied'}
$project=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
subst P: $project
if($LASTEXITCODE -ne 0){throw 'Cannot create ASCII project path'}
try {
 Push-Location 'P:\'
 try {
  if($Action -eq 'build'){& $python -m platformio run -e water_cup}
  else {& $python -m platformio run -e water_cup -t upload --upload-port COM3}
  if($LASTEXITCODE -ne 0){throw "PlatformIO $Action failed: $LASTEXITCODE"}
 } finally {Pop-Location}
} finally {subst P: /D}
