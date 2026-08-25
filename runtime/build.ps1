param(
    [string]$BuildRootOverride = '',
    [string]$Compiler = '',
    [string]$OpenXRRoot = ''
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$sourceRoot = Join-Path $projectRoot 'src'
$launcherRoot = Join-Path $projectRoot 'launcher'
$testRoot = Join-Path $projectRoot 'tests'
$toolRoot = Join-Path $projectRoot 'tools'
$buildRoot = if ($BuildRootOverride) {
    [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildRootOverride))
} else {
    Join-Path $projectRoot 'build'
}

function Resolve-Compiler {
    param([string]$Requested)

    $candidates = @(
        $Requested,
        $env:SSCVR_GCC,
        'C:\devkitPro\msys2\opt\bin\x86_64-w64-mingw32ucrt-gcc.exe',
        'C:\msys64\ucrt64\bin\gcc.exe',
        'C:\msys64\mingw64\bin\gcc.exe'
    ) | Where-Object { $_ }

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    foreach ($name in @('x86_64-w64-mingw32ucrt-gcc', 'x86_64-w64-mingw32-gcc', 'gcc')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) { return $command.Source }
    }

    throw 'A 64-bit MinGW-w64 GCC compiler was not found. Pass -Compiler or set SSCVR_GCC.'
}

function Resolve-OpenXRFiles {
    param([string]$RequestedRoot)

    $roots = @(
        $RequestedRoot,
        $env:OPENXR_SDK_ROOT,
        (Join-Path $projectRoot '..\OpenXR-SDK'),
        (Join-Path $projectRoot '..\third_party\OpenXR-SDK')
    ) | Where-Object { $_ }

    foreach ($root in $roots) {
        $fullRoot = [IO.Path]::GetFullPath($root)
        $includeCandidates = @(
            (Join-Path $fullRoot 'include'),
            (Join-Path $fullRoot 'prebuilt-1.1.60\include')
        )
        $loaderCandidates = @(
            (Join-Path $fullRoot 'x64\bin\openxr_loader.dll'),
            (Join-Path $fullRoot 'bin\openxr_loader.dll'),
            (Join-Path $fullRoot 'prebuilt-1.1.60\x64\bin\openxr_loader.dll'),
            (Join-Path $fullRoot 'build\src\loader\Release\openxr_loader.dll')
        )
        foreach ($include in $includeCandidates) {
            if (-not (Test-Path -LiteralPath (Join-Path $include 'openxr\openxr.h'))) { continue }
            foreach ($loader in $loaderCandidates) {
                if (Test-Path -LiteralPath $loader -PathType Leaf) {
                    return @((Resolve-Path -LiteralPath $include).Path,
                             (Resolve-Path -LiteralPath $loader).Path)
                }
            }
        }
    }

    throw 'OpenXR headers and a 64-bit openxr_loader.dll were not found. Pass -OpenXRRoot or set OPENXR_SDK_ROOT.'
}

function Invoke-Compiler {
    $privacyFlags = @(
        "-ffile-prefix-map=$projectRoot=."
        "-fdebug-prefix-map=$projectRoot=."
        "-fmacro-prefix-map=$projectRoot=."
    )
    & $script:compilerPath @privacyFlags @args
    if ($LASTEXITCODE -ne 0) {
        throw "Compiler failed with exit code $LASTEXITCODE."
    }
}

$compilerPath = Resolve-Compiler $Compiler
$openXRFiles = Resolve-OpenXRFiles $OpenXRRoot
$openXRInclude = $openXRFiles[0]
$openXRLoader = $openXRFiles[1]
$systemOpenGL = Join-Path ([Environment]::GetFolderPath('System')) 'opengl32.dll'
$definition = Join-Path $projectRoot 'exports.def'

foreach ($required in @($systemOpenGL, $definition)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required file was not found: $required"
    }
}

New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
Copy-Item -LiteralPath $systemOpenGL -Destination (Join-Path $buildRoot 'opengl32_system.dll') -Force
Copy-Item -LiteralPath $openXRLoader -Destination (Join-Path $buildRoot 'openxr_loader.dll') -Force

Invoke-Compiler -std=c11 -O2 -Wall -Wextra -shared `
    (Join-Path $sourceRoot 'proxy.c') `
    (Join-Path $sourceRoot 'openxr_bridge.c') `
    $definition -I $sourceRoot -I $openXRInclude `
    -o (Join-Path $buildRoot 'opengl32.dll') `
    -lgdi32 -luser32 -lws2_32 -ld3d11 -ld3dcompiler -ldxgi -ldxguid -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra -mwindows `
    (Join-Path $testRoot 'test_gl.c') `
    -o (Join-Path $buildRoot 'test_gl.exe') `
    -lopengl32 -lgdi32 -luser32 -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $launcherRoot 'launcher.c') -I $sourceRoot `
    -o (Join-Path $buildRoot 'SkillshotCityVRLauncher.exe') `
    -lbcrypt -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $testRoot 'guard_test.c') `
    -o (Join-Path $buildRoot 'guard_test.exe') `
    -lws2_32 -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $testRoot 'geometry_math_test.c') -I $sourceRoot `
    -o (Join-Path $buildRoot 'geometry_math_test.exe') -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $testRoot 'game_build_layout_test.c') -I $sourceRoot `
    -o (Join-Path $buildRoot 'game_build_layout_test.exe') -static-libgcc

Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $toolRoot 'headset_view_capture.c') `
    -o (Join-Path $buildRoot 'headset_view_capture.exe') `
    -lgdi32 -luser32 -static-libgcc

$integrationRoot = Join-Path $buildRoot 'integration'
New-Item -ItemType Directory -Force -Path $integrationRoot | Out-Null
Invoke-Compiler -std=c11 -O2 -Wall -Wextra `
    (Join-Path $testRoot 'openxr_bridge_integration_test.c') `
    (Join-Path $sourceRoot 'openxr_bridge.c') `
    -I $sourceRoot -I $openXRInclude `
    -o (Join-Path $integrationRoot 'openxr_bridge_integration_test.exe') `
    -lgdi32 -luser32 -ld3d11 -ld3dcompiler -ldxgi -ldxguid -static-libgcc
Copy-Item -LiteralPath $openXRLoader -Destination (Join-Path $integrationRoot 'openxr_loader.dll') -Force
Copy-Item -LiteralPath $systemOpenGL -Destination (Join-Path $integrationRoot 'opengl32_system.dll') -Force
Copy-Item -LiteralPath (Join-Path $toolRoot 'watch_headset_capture.ps1') -Destination $buildRoot -Force
Copy-Item -LiteralPath (Join-Path $toolRoot 'analyze_pose_trace.py') -Destination $buildRoot -Force

foreach ($binaryName in @(
    'opengl32.dll', 'test_gl.exe', 'SkillshotCityVRLauncher.exe',
    'guard_test.exe', 'geometry_math_test.exe', 'game_build_layout_test.exe',
    'headset_view_capture.exe'
)) {
    $binaryPath = Join-Path $buildRoot $binaryName
    $normalizedPath = "$binaryPath.normalized"
    Copy-Item -LiteralPath $binaryPath -Destination $normalizedPath -Force
    Move-Item -LiteralPath $normalizedPath -Destination $binaryPath -Force
}

$integrationBinary = Join-Path $integrationRoot 'openxr_bridge_integration_test.exe'
$normalizedIntegration = "$integrationBinary.normalized"
Copy-Item -LiteralPath $integrationBinary -Destination $normalizedIntegration -Force
Move-Item -LiteralPath $normalizedIntegration -Destination $integrationBinary -Force

& (Join-Path $buildRoot 'geometry_math_test.exe')
if ($LASTEXITCODE -ne 0) { throw 'Geometry math test failed.' }

& (Join-Path $buildRoot 'game_build_layout_test.exe')
if ($LASTEXITCODE -ne 0) { throw 'Game build layout test failed.' }

Write-Output "Built SSCVR runtime, tests, launcher, and tools in $buildRoot"
