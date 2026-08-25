param(
    [string]$CaptureExecutable = ''
)

$ErrorActionPreference = 'Stop'
$launcherRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$runtimeRoot = Split-Path -Parent $launcherRoot
$developmentRoot = Split-Path -Parent $runtimeRoot
$gameRoot = Join-Path $developmentRoot 'game'
$requestPath = Join-Path $gameRoot 'SkillshotVR-headset-capture-request.txt'
$proofRequestPath = Join-Path $gameRoot 'SkillshotCityVR-geometry-proof.txt'
$captureRoot = Join-Path $gameRoot 'headset-captures'
$logPath = Join-Path $captureRoot 'capture-watcher.log'

if (!$CaptureExecutable) {
    $CaptureExecutable = Join-Path $runtimeRoot 'build-ui-alpha-v12\headset_view_capture.exe'
}
if (!(Test-Path -LiteralPath $CaptureExecutable)) {
    throw "Headset capture executable not found: $CaptureExecutable"
}
New-Item -ItemType Directory -Force -Path $captureRoot | Out-Null

function Write-CaptureLog([string]$Message) {
    Add-Content -LiteralPath $logPath -Value (
        (Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff') + ' ' + $Message)
}

Write-CaptureLog 'Shift+1 two-eye capture watcher started.'
while (Get-Process -Name SkillshotCity -ErrorAction SilentlyContinue) {
    if (Test-Path -LiteralPath $requestPath) {
        Remove-Item -LiteralPath $requestPath -Force
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
        $compositorPath = Join-Path $captureRoot ("$stamp-compositor-both-eyes.bmp")
        & $CaptureExecutable $compositorPath 'OculusMirror.exe'
        $captureExit = $LASTEXITCODE

        # Ask the bridge for the pre-compositor eye, HUD-base, and exact-alpha
        # sources too. These isolate mod-layer bugs from the runtime lens warp.
        [IO.File]::WriteAllText($proofRequestPath, "Shift+1 diagnostic capture`n")
        Start-Sleep -Milliseconds 900
        $proofNames = @(
            'SkillshotCityVR-geometry-left.bmp',
            'SkillshotCityVR-geometry-right.bmp',
            'SkillshotCityVR-presented-left.bmp',
            'SkillshotCityVR-presented-right.bmp',
            'SkillshotCityVR-hud-base-left.bmp',
            'SkillshotCityVR-hud-base-right.bmp',
            'SkillshotCityVR-interface-alpha.bmp',
            'SkillshotCityVR-interface-alpha-mask.bmp'
        )
        foreach ($name in $proofNames) {
            $source = Join-Path $gameRoot $name
            if (Test-Path -LiteralPath $source) {
                $suffix = $name -replace '^SkillshotCityVR-', ''
                Copy-Item -LiteralPath $source -Destination (
                    Join-Path $captureRoot ("$stamp-$suffix")) -Force
            }
        }
        Write-CaptureLog "capture=$stamp compositorExit=$captureExit"
    }
    Start-Sleep -Milliseconds 150
}
Write-CaptureLog 'Game exited; capture watcher stopped.'
