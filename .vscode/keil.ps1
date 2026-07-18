param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Build", "Rebuild", "Flash")]
    [string]$Action
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot "AdressableBalancerLEDPY32F002b.uvprojx"
$target = "Target 1"
$logDirectory = Join-Path $projectRoot "Objects"
$commandLog = Join-Path $logDirectory "vscode-$($Action.ToLowerInvariant()).log"

$uv4Candidates = @(
    $env:KEIL_UV4,
    $(if ($env:LOCALAPPDATA) { Join-Path $env:LOCALAPPDATA "Keil_v5\UV4\UV4.exe" }),
    $(if (${env:ProgramFiles(x86)}) { Join-Path ${env:ProgramFiles(x86)} "Keil_v5\UV4\UV4.exe" }),
    $(if ($env:ProgramFiles) { Join-Path $env:ProgramFiles "Keil_v5\UV4\UV4.exe" })
) | Where-Object { $_ } | Select-Object -Unique

$uv4 = $uv4Candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (-not $uv4) {
    throw "Keil UV4 was not found. Install Keil MDK or set KEIL_UV4 to the full path of UV4.exe."
}

if (-not (Test-Path -LiteralPath $project -PathType Leaf)) {
    throw "Keil project was not found at $project"
}

New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
Remove-Item -LiteralPath $commandLog -Force -ErrorAction SilentlyContinue

$mode = switch ($Action) {
    "Build" { "-b" }
    "Rebuild" { "-r" }
    "Flash" { "-f" }
}

Write-Host "$Action '$target' with $uv4"
$arguments = @(
    $mode,
    "`"$project`"",
    "-t",
    "`"$target`"",
    "-j0",
    "-o",
    "`"$commandLog`""
)
$process = Start-Process -FilePath $uv4 -ArgumentList $arguments -Wait -PassThru
$exitCode = $process.ExitCode

if (Test-Path -LiteralPath $commandLog) {
    Get-Content -LiteralPath $commandLog | Write-Host
}

if ($exitCode -ne 0) {
    throw "Keil $Action failed with exit code $exitCode. See $commandLog"
}

Write-Host "Keil $Action completed successfully."
