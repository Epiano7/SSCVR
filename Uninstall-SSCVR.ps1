[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'SSCVR')
)

$ErrorActionPreference = 'Stop'
$installRootFull = [IO.Path]::GetFullPath($InstallRoot).TrimEnd('\')
$manifestPath = @(
    (Join-Path $installRootFull 'sscvr-install.json'),
    (Join-Path $installRootFull 'ssvcr-install.json')
) | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1

if (-not $manifestPath) {
    throw "SSCVR's install manifest was not found in $installRootFull. Nothing was removed."
}
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$recordedRoot = [IO.Path]::GetFullPath([string]$manifest.install_root).TrimEnd('\')
if (-not $recordedRoot.Equals($installRootFull, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The install manifest does not match the requested directory. Nothing was removed.'
}
if ($installRootFull.Length -lt 12 -or $installRootFull -eq [IO.Path]::GetPathRoot($installRootFull)) {
    throw 'The install path is too broad to remove safely.'
}

if ($PSCmdlet.ShouldProcess($installRootFull, 'Remove the isolated SSCVR installation')) {
    Remove-Item -LiteralPath $installRootFull -Recurse -Force
    Write-Output "Removed isolated SSCVR installation: $installRootFull"
    Write-Output 'The Steam installation was not changed.'
}
