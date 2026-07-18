param(
    [switch]$Check
)

$ErrorActionPreference = "Stop"

$projectRoot = $PSScriptRoot
$publications = @(
    [PSCustomObject]@{
        Source = Join-Path $projectRoot "USER_GUIDE.html"
        Output = Join-Path $projectRoot "docs\USER_GUIDE.html"
    },
    [PSCustomObject]@{
        Source = Join-Path $projectRoot "assets\logo.png"
        Output = Join-Path $projectRoot "docs\assets\logo.png"
    }
)

function Test-FilesEqual {
    param(
        [Parameter(Mandatory)]
        [string]$Source,

        [Parameter(Mandatory)]
        [string]$Output
    )

    if (-not (Test-Path -LiteralPath $Source -PathType Leaf) -or
        -not (Test-Path -LiteralPath $Output -PathType Leaf)) {
        return $false
    }

    $sourceHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Source).Hash
    $outputHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Output).Hash
    return $sourceHash -eq $outputHash
}

if ($Check) {
    $staleOutputs = @(
        foreach ($publication in $publications) {
            if (-not (Test-FilesEqual -Source $publication.Source `
                                      -Output $publication.Output)) {
                $publication.Output
            }
        }
    )

    if ($staleOutputs.Count -gt 0) {
        $relativeOutputs = $staleOutputs | ForEach-Object {
            $_.Replace("$projectRoot\", "")
        }
        throw "User Guide publishing is stale: $($relativeOutputs -join ', '). Run .\publish_user_guide.ps1."
    }

    Write-Host "User Guide publishing is up to date."
    return
}

foreach ($publication in $publications) {
    if (-not (Test-Path -LiteralPath $publication.Source -PathType Leaf)) {
        throw "Canonical User Guide input was not found: $($publication.Source)"
    }

    $outputDirectory = Split-Path -Parent $publication.Output
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    Copy-Item -LiteralPath $publication.Source -Destination $publication.Output -Force
}

Write-Host "Published the User Guide to docs/."
