# Monster Hunter Stories -> PS Vita: stage the game data for ux0:data/mhs/
#
# Takes your APK and the OBB folder and produces a "mhs" folder laid out the
# way the loader expects. Copy that folder to ux0:data/ on the Vita (VitaShell
# FTP or USB).
#
#   powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
#       -Apk "E:\downloads\MONSTER-HUNTER-STORIES-v1.0.5-full-apkvision.apk" `
#       -ObbDir "E:\downloads\MONSTER-HUNTER-STORIES-v1-0-5-cache\jp.co.capcom.mhsspen" `
#       -Out "E:\downloads\mhs_vita_data"

param(
    [Parameter(Mandatory)] [string] $Apk,
    [Parameter(Mandatory)] [string] $ObbDir,
    [Parameter(Mandatory)] [string] $Out
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem

$root = Join-Path $Out 'mhs'
$obbOut = Join-Path $root 'Android\obb\jp.co.capcom.mhsspen'
New-Item -ItemType Directory -Force $root, $obbOut, (Join-Path $root 'files') | Out-Null

$zip = [IO.Compression.ZipFile]::OpenRead($Apk)
try {
    $lib = $zip.Entries | Where-Object FullName -eq 'lib/armeabi-v7a/libMHR.so'
    if (-not $lib) { throw "APK has no lib/armeabi-v7a/libMHR.so (32-bit build required)" }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($lib, (Join-Path $root 'libMHR.so'), $true)

    # assets/ is read through AAssetManager; the boilerplate serves it from DATA_PATH/assets
    foreach ($e in $zip.Entries) {
        if ($e.FullName -notlike 'assets/*' -or $e.FullName.EndsWith('/')) { continue }
        if ($e.FullName -eq 'assets/apkvision.config') { continue }  # repacker junk, not game data
        $dest = Join-Path $root ($e.FullName -replace '/', '\')
        New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($e, $dest, $true)
    }
} finally {
    $zip.Dispose()
}

foreach ($obb in Get-ChildItem $ObbDir -Filter '*.obb') {
    Write-Host "Copying $($obb.Name) ($([math]::Round($obb.Length / 1GB, 2)) GB)..."
    Copy-Item $obb.FullName $obbOut -Force
}

$size = (Get-ChildItem $root -Recurse -File | Measure-Object Length -Sum).Sum
Write-Host ""
Write-Host "Done: $root ($([math]::Round($size / 1GB, 2)) GB)"
Write-Host "Copy the 'mhs' folder to ux0:data/ on your Vita."
