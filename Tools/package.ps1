# Tools/package.ps1
# Builds the distributable zip from an existing Release|x64 build.
#   PS> .\Tools\package.ps1
#
# Produces Build\package\ZNEngine_demo_x64.zip with the stable asset name used by the
# releases/latest download link. The archive layout keeps TestApp.exe and Resources\ side by side.

$ErrorActionPreference = "Stop"
$config  = "Release"
$platform = "x64"
$root     = Split-Path $PSScriptRoot -Parent
$packageAssets = Join-Path $root "Tools\PackageAssets"
$bin      = Join-Path $root "Build\${platform}_${config}"
$name     = "ZNEngine_demo_${platform}"
$stage = Join-Path $root "Build\package\$name"
$zip   = Join-Path $root "Build\package\$name.zip"

# --- preconditions -------------------------------------------------------
$exe = Join-Path $bin "TestApp.exe"
if (-not (Test-Path $exe)) {
    throw "TestApp.exe not found at $exe. Build $config|$platform first."
}

# --- stage ---------------------------------------------------------------
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage -Force | Out-Null

Copy-Item $exe $stage

# Assimp is linked dynamically -- the dll must sit next to the exe.
$dll = Join-Path $root "Library\Bin\x64\assimp-vc143-mt.dll"
if (-not (Test-Path $dll)) { throw "Required runtime DLL not found at $dll" }
Copy-Item $dll $stage

# Runtime assets. Shaders are compiled at load time, so they must ship.
foreach ($d in @("Shaders", "Models", "Textures")) {
    $src = Join-Path $root "Resources\$d"
    if (-not (Test-Path $src)) { throw "Missing Resources\$d" }
    Copy-Item $src (Join-Path $stage "Resources\$d") -Recurse
}

# Runtime recordings stay ignored under Resources\Logs. PackageAssets holds a tracked default so
# every checkout ships the same recording and "Load Log" works on first launch.
$logs = Join-Path $stage "Resources\Logs"
New-Item -ItemType Directory -Path $logs -Force | Out-Null
$scenario = Join-Path $packageAssets "default_scenario.json"
if (-not (Test-Path $scenario)) {
    throw "Tracked sample log not found at $scenario"
}
Copy-Item $scenario (Join-Path $logs "scenario.json")

# Runtime layout changes stay ignored. Ship the tracked default instead.
$ini = Join-Path $packageAssets "default_imgui.ini"
if (-not (Test-Path $ini)) { throw "Tracked UI layout not found at $ini" }
Copy-Item $ini (Join-Path $stage "imgui.ini")

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
