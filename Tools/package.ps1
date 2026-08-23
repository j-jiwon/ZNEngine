# Tools/package.ps1
# Builds a distributable zip from an existing Release|x64 build.
#   PS> .\Tools\package.ps1
#   PS> .\Tools\package.ps1 -Version v1.1
#
# Produces Build\package\ZNEngine_demo_<Version>_x64.zip with the layout GetResourcePath()
# expects when running outside Visual Studio:  TestApp.exe + Resources\ side by side.

param(
    [string]$Config   = "Release",
    [string]$Platform = "x64",
    [string]$Version  = "v1.0"
)

$ErrorActionPreference = "Stop"
$root  = Split-Path $PSScriptRoot -Parent
$bin   = Join-Path $root "Build\${Platform}_${Config}"
$name  = "ZNEngine_demo_${Version}_${Platform}"
$stage = Join-Path $root "Build\package\$name"
$zip   = Join-Path $root "Build\package\$name.zip"

# --- preconditions -------------------------------------------------------
$exe = Join-Path $bin "TestApp.exe"
if (-not (Test-Path $exe)) {
    throw "TestApp.exe not found at $exe. Build $Config|$Platform first."
}
if ($Config -ne "Release") {
    Write-Warning "Packaging a $Config build. Ship Release: the D3D12 debug layer is slow."
}

# --- stage ---------------------------------------------------------------
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

Copy-Item $exe $stage

# Assimp is linked dynamically -- the dll must sit next to the exe.
$dll = Join-Path $root "Library\Bin\x64\assimp-vc143-mt.dll"
if (Test-Path $dll) { Copy-Item $dll $stage }
else { Write-Warning "assimp dll not found at $dll -- skipping (static link?)" }

# Runtime assets. Shaders are compiled at load time, so they must ship.
foreach ($d in @("Shaders", "Models", "Textures")) {
    $src = Join-Path $root "Resources\$d"
    if (-not (Test-Path $src)) { throw "Missing Resources\$d" }
    Copy-Item $src (Join-Path $stage "Resources\$d") -Recurse
}

# Logs\ is created at runtime by WriteLog, but shipping a recorded scenario means
# "Load Log" works on first launch instead of showing an empty timeline.
$logs = Join-Path $stage "Resources\Logs"
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$scenario = Join-Path $root "Resources\Logs\scenario.json"
if (Test-Path $scenario) { Copy-Item $scenario $logs }
else { Write-Warning "No scenario.json -- 'Load Log' will have nothing to play. Record one first." }

# Panel layout. Without this the first launch piles every ImGui window in the corner.
$ini = Join-Path $root "TestApp\imgui.ini"
if (Test-Path $ini) { Copy-Item $ini $stage }
else { Write-Warning "No imgui.ini at $ini -- first launch will use the default layout." }

Copy-Item (Join-Path $root "LICENSE") $stage
Copy-Item (Join-Path $root "Docs\THIRD_PARTY_LICENSES.md") $stage

# --- readme --------------------------------------------------------------
@"
ZNEngine -- automotive surround-view demo ($Version)
====================================================

Run TestApp.exe. Keep it in the same folder as Resources\ -- the app resolves assets
relative to the executable.

Requirements
  Windows 10/11, x64
  A Direct3D 12 capable GPU
  Microsoft Visual C++ Redistributable (x64):
    https://aka.ms/vs/17/release/vc_redist.x64.exe
  (this build links the CRT dynamically)

Controls
  [TODO: fill in -- e.g. right mouse drag to look, WASD to move,
   Speed / Sens sliders in the Debug panel]

What to try
  Scenes            switch between Vehicle / MirrorBall / Test / CCTV
  DataSource        Live (synthetic) vs Load Log (JSON replay, seek + loop)
  Temporal resample interpolation mode, sensor Hz, dropout, jitter
  Instancing        toggle "Instanced draws" and watch the draw-call counter
  Density           x1 / x5 / x20 / x50 traffic multiplier

Source, case study and demo video
  https://github.com/j-jiwon/ZNEngine

Licences
  Engine source: BSD 3-Clause (LICENSE)
  Third-party libraries and assets: THIRD_PARTY_LICENSES.md
"@ | Set-Content (Join-Path $stage "README.txt") -Encoding UTF8

# --- zip -----------------------------------------------------------------
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip

$mb = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host ""
Write-Host "Packaged: $zip  ($mb MB)" -ForegroundColor Green
Write-Host ""
Write-Host "Before publishing, verify from a folder that is NOT the repo:" -ForegroundColor Yellow
Write-Host "  1. extract the zip somewhere else (or another PC)"
Write-Host "  2. double-click TestApp.exe"
Write-Host "  3. check: scene renders, Load Log plays, panel layout is correct"
Write-Host "  Running it in place would hit the dev-layout fallback in GetResourcePath()"
Write-Host "  and prove nothing."