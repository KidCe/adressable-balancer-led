param(
    [string]$Version = "1.1.0"
)

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$project = Join-Path $projectRoot "AdressableBalancerLEDPY32F002b.uvprojx"
$baseName = "AdressableBalancerLEDPY32F002b"
$objects = Join-Path $projectRoot "Objects"
$buildLog = Join-Path $objects "$baseName.build_log.htm"
$uv4 = Join-Path $env:LOCALAPPDATA "Keil_v5\UV4\UV4.exe"
$fromElf = Join-Path $env:LOCALAPPDATA "Keil_v5\ARM\ARMCLANG\Bin\FromElf.exe"
$releaseDir = Join-Path $projectRoot "release\v$Version"

& (Join-Path $projectRoot "publish_user_guide.ps1") -Check

if (-not (Test-Path -LiteralPath $uv4)) {
    throw "Keil UV4 was not found at $uv4"
}
if (-not (Test-Path -LiteralPath $fromElf)) {
    throw "FromElf was not found at $fromElf"
}

& $uv4 -b $project -j0

if (-not (Test-Path -LiteralPath $buildLog)) {
    throw "Keil did not create the expected build log."
}
$logText = Get-Content -Raw -LiteralPath $buildLog
if ($logText -notmatch '0 Error\(s\), 0 Warning\(s\)') {
    throw "Release build is not clean. Inspect $buildLog"
}

$axf = Join-Path $objects "$baseName.axf"
$hex = Join-Path $objects "$baseName.hex"
$bin = Join-Path $objects "$baseName.bin"

& $fromElf --bin --output $bin $axf

New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
Copy-Item -LiteralPath $axf -Destination (Join-Path $releaseDir "$baseName-v$Version.axf")
Copy-Item -LiteralPath $hex -Destination (Join-Path $releaseDir "$baseName-v$Version.hex")
Copy-Item -LiteralPath $bin -Destination (Join-Path $releaseDir "$baseName-v$Version.bin")
Copy-Item -LiteralPath (Join-Path $projectRoot "README.md") -Destination $releaseDir
Copy-Item -LiteralPath (Join-Path $projectRoot "CHANGELOG.md") -Destination $releaseDir
Copy-Item -LiteralPath (Join-Path $projectRoot "USER_GUIDE.html") -Destination $releaseDir

$checksumFile = Join-Path $releaseDir "SHA256SUMS.txt"
Get-ChildItem -LiteralPath $releaseDir -File |
    Where-Object Name -ne "SHA256SUMS.txt" |
    Sort-Object Name |
    ForEach-Object {
        $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName
        "$($hash.Hash.ToLowerInvariant())  $($_.Name)"
    } | Set-Content -Encoding ascii -LiteralPath $checksumFile

Write-Host "Release v$Version created at $releaseDir"
