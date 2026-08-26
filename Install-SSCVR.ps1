[CmdletBinding()]
param(
    [string]$GamePath = '',
    [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'SSCVR'),
    [string]$RuntimePath = ''
)

$ErrorActionPreference = 'Stop'
$supportedBuilds = @(
    [pscustomobject]@{
        Name = 'frozen v17 test build'
        Size = 15170560L
        Sha256 = '40FAFE9AD3AFFA0A7C70254A757F495F613C2C6F73B9EDB609D9B62B1C65BD6D'
    },
    [pscustomobject]@{
        Name = 'v1.990 (revision 5)'
        Size = 15171072L
        Sha256 = '3FE88430ADFD97C5F32ED2BFC4612FF918C2FE856BB57A3BB72B3BAB7C0CDA1E'
    },
    [pscustomobject]@{
        Name = 'v1.990 (revision 8)'
        Size = 15173632L
        Sha256 = 'B5AFB4081D8F13733DA55BBB10DC0751E1D4F4F1C4F3B15EC11308A6CDE4C3A2'
    }
)

function Get-FullPath {
    param([string]$Path)
    return [IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Get-SteamLibraries {
    $roots = New-Object System.Collections.Generic.List[string]
    foreach ($key in @(
        'HKCU:\Software\Valve\Steam',
        'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam'
    )) {
        if (-not (Test-Path -LiteralPath $key)) { continue }
        $properties = Get-ItemProperty -LiteralPath $key
        foreach ($name in @('SteamPath', 'InstallPath')) {
            $value = $properties.$name
            if ($value) { $roots.Add((Get-FullPath $value)) }
        }
    }
    foreach ($candidate in @('C:\Program Files (x86)\Steam', 'C:\Program Files\Steam')) {
        if (Test-Path -LiteralPath $candidate) { $roots.Add((Get-FullPath $candidate)) }
    }

    foreach ($root in @($roots)) {
        $libraryFile = Join-Path $root 'steamapps\libraryfolders.vdf'
        if (-not (Test-Path -LiteralPath $libraryFile -PathType Leaf)) { continue }
        foreach ($line in Get-Content -LiteralPath $libraryFile) {
            if ($line -match '^\s*"path"\s+"([^"]+)"') {
                $library = $Matches[1] -replace '\\\\', '\'
                $roots.Add((Get-FullPath $library))
            }
        }
    }

    return $roots | Sort-Object -Unique
}

function Find-SkillshotCity {
    foreach ($library in Get-SteamLibraries) {
        $manifest = Join-Path $library 'steamapps\appmanifest_308600.acf'
        if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { continue }
        $installDirectory = $null
        foreach ($line in Get-Content -LiteralPath $manifest) {
            if ($line -match '^\s*"installdir"\s+"([^"]+)"') {
                $installDirectory = $Matches[1]
                break
            }
        }
        if ($installDirectory) {
            $candidate = Join-Path $library "steamapps\common\$installDirectory"
            if (Test-Path -LiteralPath (Join-Path $candidate 'SkillshotCity.exe') -PathType Leaf) {
                return (Get-FullPath $candidate)
            }
        }
    }
    throw 'Skillshot City (Steam app 308600) was not found. Pass -GamePath explicitly.'
}

function Resolve-RuntimePath {
    param([string]$Requested)
    $candidates = @(
        $Requested,
        $PSScriptRoot,
        (Join-Path $PSScriptRoot 'runtime\build')
    ) | Where-Object { $_ }
    foreach ($candidate in $candidates) {
        $full = Get-FullPath $candidate
        $required = @('SkillshotCityVRLauncher.exe', 'opengl32.dll', 'openxr_loader.dll')
        if (($required | Where-Object { -not (Test-Path -LiteralPath (Join-Path $full $_) -PathType Leaf) }).Count -eq 0) {
            return $full
        }
    }
    throw 'Built SSCVR runtime files were not found. Pass -RuntimePath or run runtime/build.ps1 first.'
}

$sourceGame = if ($GamePath) { Get-FullPath $GamePath } else { Find-SkillshotCity }
$installRootFull = Get-FullPath $InstallRoot
$runtimeRoot = Resolve-RuntimePath $RuntimePath
$sourceExe = Join-Path $sourceGame 'SkillshotCity.exe'

if (-not (Test-Path -LiteralPath $sourceExe -PathType Leaf)) {
    throw "SkillshotCity.exe was not found in $sourceGame"
}
$sourceFile = Get-Item -LiteralPath $sourceExe
$sourceHash = (Get-FileHash -LiteralPath $sourceExe -Algorithm SHA256).Hash
$selectedBuild = $supportedBuilds | Where-Object {
    $_.Size -eq $sourceFile.Length -and $_.Sha256 -eq $sourceHash
} | Select-Object -First 1
if (-not $selectedBuild) {
    $supported = ($supportedBuilds | ForEach-Object {
        "$($_.Name): $($_.Sha256) ($($_.Size) bytes)"
    }) -join '; '
    throw "Unsupported Skillshot City build. Found $sourceHash ($($sourceFile.Length) bytes). Supported: $supported"
}

$sourcePrefix = $sourceGame + '\'
$installPrefix = $installRootFull + '\'
if ($installRootFull.Equals($sourceGame, [StringComparison]::OrdinalIgnoreCase) -or
    $installPrefix.StartsWith($sourcePrefix, [StringComparison]::OrdinalIgnoreCase) -or
    $sourcePrefix.StartsWith($installPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The install directory must be separate from the Steam game directory.'
}

$gameDestination = Join-Path $installRootFull 'game'
if (Test-Path -LiteralPath $gameDestination) {
    throw "An SSCVR game copy already exists at $gameDestination. Uninstall it or choose another -InstallRoot."
}

$installRootExisted = Test-Path -LiteralPath $installRootFull
New-Item -ItemType Directory -Force -Path $installRootFull | Out-Null
$staging = Join-Path $installRootFull ('.installing-' + [guid]::NewGuid().ToString('N'))
$installedRuntimeFiles = @(
    'SkillshotCityVRLauncher.exe',
    'opengl32.dll',
    'openxr_loader.dll',
    'headset_view_capture.exe',
    'watch_headset_capture.ps1'
)
$installedSupportFiles = @(
    'Uninstall-SSCVR.ps1',
    'Uninstall SSCVR.cmd'
)
$createdRootFiles = New-Object System.Collections.Generic.List[string]
$gameMoved = $false

try {
    Write-Output "Copying the verified Steam installation from $sourceGame"
    Write-Output "Matched $($selectedBuild.Name)"
    Copy-Item -LiteralPath $sourceGame -Destination $staging -Recurse

    foreach ($name in $installedRuntimeFiles) {
        $source = Join-Path $runtimeRoot $name
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            $destination = Join-Path $installRootFull $name
            Copy-Item -LiteralPath $source -Destination $destination -Force
            $createdRootFiles.Add($destination)
        }
    }
    foreach ($name in $installedSupportFiles) {
        $source = Join-Path $PSScriptRoot $name
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Installer support file was not found: $name"
        }
        $destination = Join-Path $installRootFull $name
        Copy-Item -LiteralPath $source -Destination $destination -Force
        $createdRootFiles.Add($destination)
    }

    $launcher = Join-Path $installRootFull 'SkillshotCityVRLauncher.exe'
    $savedGameDirectory = $env:SSCVR_GAME_DIR
    $env:SSCVR_GAME_DIR = $staging
    try {
        & $launcher --prepare
        if ($LASTEXITCODE -ne 0) { throw "The SSCVR launcher rejected the staged installation (exit $LASTEXITCODE)." }
    } finally {
        if ($null -eq $savedGameDirectory) {
            Remove-Item Env:SSCVR_GAME_DIR -ErrorAction SilentlyContinue
        } else {
            $env:SSCVR_GAME_DIR = $savedGameDirectory
        }
    }

    Move-Item -LiteralPath $staging -Destination $gameDestination
    $gameMoved = $true

    $manifest = [ordered]@{
        format = 1
        installed_at_utc = [datetime]::UtcNow.ToString('o')
        install_root = $installRootFull
        game_build = $selectedBuild.Name
        game_executable_size = $selectedBuild.Size
        game_executable_sha256 = $selectedBuild.Sha256
        runtime = [ordered]@{}
    }
    foreach ($name in @($installedRuntimeFiles + $installedSupportFiles)) {
        $path = Join-Path $installRootFull $name
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $manifest.runtime[$name] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        }
    }
    $manifestPath = Join-Path $installRootFull 'sscvr-install.json'
    $manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    $createdRootFiles.Add($manifestPath)

    $launchCommand = Join-Path $installRootFull 'Launch SSCVR.cmd'
    '@echo off
set "SSCVR_GAME_DIR=%~dp0game"
"%~dp0SkillshotCityVRLauncher.exe" --launch-vr
' | Set-Content -LiteralPath $launchCommand -Encoding ascii
    $createdRootFiles.Add($launchCommand)
    $verifyCommand = Join-Path $installRootFull 'Verify SSCVR.cmd'
    '@echo off
set "SSCVR_GAME_DIR=%~dp0game"
"%~dp0SkillshotCityVRLauncher.exe" --verify
pause
' | Set-Content -LiteralPath $verifyCommand -Encoding ascii
    $createdRootFiles.Add($verifyCommand)

    Write-Output "SSCVR installed to $installRootFull"
    Write-Output 'The Steam installation was read only and was not modified.'
    Write-Output "Start VR with: $launchCommand"
    Write-Output "Remove SSCVR with: $(Join-Path $installRootFull 'Uninstall SSCVR.cmd')"
} catch {
    if (Test-Path -LiteralPath $staging) {
        $resolvedStaging = Get-FullPath $staging
        if ($resolvedStaging.StartsWith($installPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $resolvedStaging -Recurse -Force
        }
    }
    if ($gameMoved -and (Test-Path -LiteralPath $gameDestination)) {
        $resolvedGame = Get-FullPath $gameDestination
        if ($resolvedGame.Equals((Join-Path $installRootFull 'game'),
                                 [StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $resolvedGame -Recurse -Force
        }
    }
    foreach ($path in $createdRootFiles) {
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            $resolvedFile = Get-FullPath $path
            if ((Split-Path -Parent $resolvedFile).Equals(
                    $installRootFull, [StringComparison]::OrdinalIgnoreCase)) {
                Remove-Item -LiteralPath $resolvedFile -Force
            }
        }
    }
    if (-not $installRootExisted -and (Test-Path -LiteralPath $installRootFull) -and
        -not (Get-ChildItem -LiteralPath $installRootFull -Force | Select-Object -First 1)) {
        Remove-Item -LiteralPath $installRootFull -Force
    }
    throw
}
