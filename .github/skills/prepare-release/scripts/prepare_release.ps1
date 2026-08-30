<#
.SYNOPSIS
    Prepare a miskeyed-workbench release: bump the package version and stamp the
    changelog. Edits files only — it never commits, tags, or pushes.

.DESCRIPTION
    Applies the mechanical release edits so a human can review the diff, then commit:
      - pyproject.toml      version = "X.Y.Z"
      - CHANGELOG.md        turns "## [Unreleased]" into a dated "## [X.Y.Z]" section
                            and reopens a fresh empty "## [Unreleased]".

.PARAMETER Version
    The release version, e.g. 0.2.0 (must be X.Y.Z).

.EXAMPLE
    pwsh .github/skills/prepare-release/scripts/prepare_release.ps1 -Version 0.2.0
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version
)

$ErrorActionPreference = 'Stop'

# Locate the repo root by walking up from this script until pyproject.toml appears.
$dir = $PSScriptRoot
while ($dir -and -not (Test-Path (Join-Path $dir 'pyproject.toml'))) {
    $dir = Split-Path $dir -Parent
}
if (-not $dir) { throw 'Could not locate repo root (no pyproject.toml found above this script).' }
$root = $dir

# Write UTF-8 WITHOUT a BOM (Windows PowerShell's -Encoding UTF8 adds one, which would
# corrupt pyproject.toml).
function Write-Utf8NoBom($path, $text) {
    [System.IO.File]::WriteAllText($path, $text, (New-Object System.Text.UTF8Encoding $false))
}

$today = Get-Date -Format 'yyyy-MM-dd'
$emdash = [char]0x2014

# pyproject.toml
$pp = Join-Path $root 'pyproject.toml'
$t = [System.IO.File]::ReadAllText($pp)
$t = [regex]::new('(?m)^version = "[^"]*"').Replace($t, "version = `"$Version`"", 1)
Write-Utf8NoBom $pp $t
Write-Host "pyproject.toml  -> version = $Version"

# CHANGELOG.md: stamp the top [Unreleased] block and reopen a fresh one above it.
$cl = Join-Path $root 'CHANGELOG.md'
$t = [System.IO.File]::ReadAllText($cl)
if ($t -notmatch '## \[Unreleased\]') { throw 'CHANGELOG.md has no "## [Unreleased]" section to stamp.' }
$stamp = "## [Unreleased]`n`n## [$Version] $emdash $today"
$t = [regex]::new('## \[Unreleased\]').Replace($t, $stamp, 1)
Write-Utf8NoBom $cl $t
Write-Host "CHANGELOG.md    -> [$Version] $emdash $today"

Write-Host ''
Write-Host "Prepared $Version. Review the diff, then commit. Tagging/publishing stays manual:"
Write-Host "  git add pyproject.toml CHANGELOG.md"
Write-Host "  git commit -m `"release: prepare $Version`""
Write-Host "  # after merge to main: git tag v$Version; git push origin v$Version"
