[CmdletBinding()]
param(
    [string]$Version = '0.1.0-alpha',
    [string]$BuildRoot = '',
    [string]$OutputRoot = ''
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildRoot) { $BuildRoot = Join-Path $repositoryRoot 'runtime\build' }
if (-not $OutputRoot) { $OutputRoot = Join-Path $repositoryRoot 'dist' }
$BuildRoot = [IO.Path]::GetFullPath($BuildRoot)
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
$packageName = "SSCVR-$Version"
$stage = Join-Path $OutputRoot $packageName
$archive = Join-Path $OutputRoot "$packageName.zip"

$required = @('SkillshotCityVRLauncher.exe', 'opengl32.dll', 'openxr_loader.dll')
foreach ($name in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $BuildRoot $name) -PathType Leaf)) {
        throw "Missing build output: $name. Run runtime/build.ps1 first."
    }
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
if (Test-Path -LiteralPath $archive) { Remove-Item -LiteralPath $archive -Force }
New-Item -ItemType Directory -Path $stage | Out-Null

foreach ($name in @(
    'SkillshotCityVRLauncher.exe', 'opengl32.dll', 'openxr_loader.dll',
    'headset_view_capture.exe', 'watch_headset_capture.ps1'
)) {
    $source = Join-Path $BuildRoot $name
    if (Test-Path -LiteralPath $source -PathType Leaf) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $stage $name)
    }
}
foreach ($name in @(
    'Install SSCVR.cmd', 'Install-SSCVR.ps1',
    'Uninstall SSCVR.cmd', 'Uninstall-SSCVR.ps1',
    'README.md', 'LICENSE', 'NOTICE.txt'
)) {
    Copy-Item -LiteralPath (Join-Path $repositoryRoot $name) -Destination (Join-Path $stage $name)
}

if (Test-Path -LiteralPath (Join-Path $stage 'opengl32_system.dll')) {
    throw 'A Windows system DLL entered the release stage.'
}
if (Get-ChildItem -LiteralPath $stage -File -Recurse | Where-Object { $_.Name -in @('SkillshotCity.exe', 'steam_api.dll', 'steam_api64.dll', 'steam_appid.txt', 'openal32_x64.dll') }) {
    throw 'A game file entered the release stage.'
}

Compress-Archive -LiteralPath $stage -DestinationPath $archive -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
Write-Output "Created $archive"
Write-Output "SHA256 $hash"
