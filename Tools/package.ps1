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

# Package-specific usage guide. Keep it as a tracked file so controls, links, and
# licence notices are reviewable without editing this script.
Copy-Item (Join-Path $root "Docs\DISTRIBUTION_README.txt") `
          (Join-Path $stage "README.txt")

# --- zip -----------------------------------------------------------------
# Entry names are built by hand instead of using Compress-Archive or
# ZipFile::CreateFromDirectory. Both of those go through .NET Framework, which writes
# Path.DirectorySeparatorChar ('\') into the archive; the zip spec requires '/'. Explorer and
# 7-Zip cope, but unzip on Linux/macOS then produces one flat file per entry with the
# separators baked into the filename instead of a directory tree. ZipArchive::CreateEntry
# takes the entry name verbatim, so normalising it here is the one reliable fix on PS 5.1.
if (Test-Path $zip) { Remove-Item $zip -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$stageParent = Split-Path $stage -Parent
$archive = [System.IO.Compression.ZipFile]::Open(
    $zip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    # Relative to the stage's PARENT, so the archive keeps its top-level folder.
    Get-ChildItem $stage -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($stageParent.Length + 1).Replace('\', '/')
        [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $_.FullName, $rel,
            [System.IO.Compression.CompressionLevel]::Optimal)
    }
}
finally { $archive.Dispose() }

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
