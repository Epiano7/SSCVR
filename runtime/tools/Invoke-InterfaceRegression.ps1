param(
    [Parameter(Mandatory)][string]$BuildRoot,
    [Parameter(Mandatory)][string]$RunRoot,
    [Parameter(Mandatory)][string]$RuntimeJson,
    [Parameter(Mandatory)][string]$SimulatorConfig,
    [string]$ReplayDirectory = '',
    [ValidateRange(2400,120000)][int]$Frames = 2400,
    [ValidateRange(60,3600)][int]$TimeoutSeconds = 600,
    [string]$Python = 'python'
)

$ErrorActionPreference = 'Stop'
$RunRoot = [IO.Path]::GetFullPath($RunRoot)
if (Test-Path -LiteralPath $RunRoot) { throw 'Use a new run directory to exclude stale evidence.' }
if (Get-Process -Name openxr_bridge_integration_test -ErrorAction SilentlyContinue |
        Where-Object { !$_.HasExited }) {
    throw 'An integration test is already running; do not stack simulator processes.'
}
$RuntimeJson = (Resolve-Path -LiteralPath $RuntimeJson).Path
$SimulatorConfig = (Resolve-Path -LiteralPath $SimulatorConfig).Path
if ($ReplayDirectory) { $ReplayDirectory = (Resolve-Path -LiteralPath $ReplayDirectory).Path }
$config = Get-Content -LiteralPath $SimulatorConfig -Raw | ConvertFrom-Json
if (!$config.use_batch_mode -or !$config.disable_compositor) {
    throw 'Unattended tests require batch mode and a disabled simulator compositor.'
}
New-Item -ItemType Directory -Path $RunRoot | Out-Null
foreach ($file in @('openxr_bridge_integration_test.exe','openxr_loader.dll','opengl32_system.dll')) {
    Copy-Item -LiteralPath (Join-Path $BuildRoot "integration\$file") -Destination $RunRoot
}
$settings = @{
    XR_RUNTIME_JSON = $RuntimeJson
    META_XRSIM_CONFIG_JSON = $SimulatorConfig
    SKILLSHOTVR_INTEGRATION_HIDDEN = '1'
    SKILLSHOTVR_INTEGRATION_FRAMES = "$Frames"
    SKILLSHOTVR_INTEGRATION_UI_REPLAY = $ReplayDirectory
}
$saved = @{}
$testProcess = $null
try {
    foreach ($key in $settings.Keys) {
        $saved[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $settings[$key], 'Process')
    }
    $started = Get-Date
    $testProcess = Start-Process -FilePath (Join-Path $RunRoot 'openxr_bridge_integration_test.exe') `
        -WorkingDirectory $RunRoot -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $RunRoot 'stdout.txt') `
        -RedirectStandardError (Join-Path $RunRoot 'stderr.txt')
    $resultPath = Join-Path $RunRoot 'integration-result.txt'
    while (!(Test-Path -LiteralPath $resultPath)) {
        if ($testProcess.HasExited) { throw "Test exited before writing a result: $($testProcess.ExitCode)" }
        if (((Get-Date) - $started).TotalSeconds -gt $TimeoutSeconds) { throw 'Integration deadline exceeded.' }
        Start-Sleep -Milliseconds 500
    }
    # The result is written only after the bridge completes its idempotent
    # shutdown checks. Give the simulator a bounded process-exit window too.
    $exited = $testProcess.WaitForExit(5000)
    $resultText = Get-Content -LiteralPath $resultPath -Raw
    [pscustomobject]@{
        Started = $started.ToUniversalTime().ToString('o')
        Frames = $Frames
        ReplayDirectory = $ReplayDirectory
        TestSHA256 = (Get-FileHash -LiteralPath (Join-Path $RunRoot 'openxr_bridge_integration_test.exe')).Hash
        TeardownCompleted = $exited
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $RunRoot 'run.json')
    if ($resultText -notmatch '\bpassed=1\b') { throw "Integration failed: $resultText" }
    if ($resultText -notmatch '\bshutdown_once=1\b' -or
        $resultText -notmatch '\bshutdown_twice=1\b' -or
        $resultText -notmatch '\bstatus=stopped cleanly\b') {
        throw "OpenXR teardown did not complete cleanly: $resultText"
    }
    $xrLog = Get-Content -LiteralPath (Join-Path $RunRoot 'SkillshotVR-XR.log') -Raw
    $pairRates = @([regex]::Matches($xrLog, 'Cadence [\d.]+ XR Hz, ([\d.]+) coherent pairs/s') |
        ForEach-Object { [double]::Parse($_.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture) })
    $minPairs = ($pairRates | Measure-Object -Minimum).Minimum
    if ($pairRates.Count -lt 2 -or $minPairs -le 0) { throw 'Missing or stalled fresh-pair cadence.' }
    if ($xrLog -match '(?:Menu interop (?:lock|unlock)|Headset frame copy) failed') {
        throw 'Interop/frame-copy failure in the fresh test log.'
    }
    if ($xrLog -notmatch 'Gameplay-to-menu handoff: cleared stereo projection' -or
        $xrLog -notmatch 'Presentation mode: flat menu quad') {
        throw 'Round-complete monoscopic handoff was not observed.'
    }
    $validator = if ($ReplayDirectory) { 'verify_interface_replay.py' } else { 'verify_interface_proofs.py' }
    $validatorArgs = @('-B', (Join-Path $PSScriptRoot $validator), $RunRoot)
    if ($ReplayDirectory) { $validatorArgs += $ReplayDirectory }
    $pixels = & $Python @validatorArgs
    $pixelExit = $LASTEXITCODE
    $pixels | Set-Content -LiteralPath (Join-Path $RunRoot 'pixel-verification.json')
    if ($pixelExit -ne 0) { throw 'Submitted pixel verification failed; see pixel-verification.json.' }
    Write-Output $resultText.Trim()
    Write-Output "Minimum logged fresh-pair rate: $minPairs pairs/s"
    Write-Output "PASS submitted pixels; evidence: $RunRoot"
}
finally {
    if ($testProcess -and !$testProcess.HasExited) {
        $testProcess.Kill()
        $testProcess.WaitForExit(5000) | Out-Null
    }
    foreach ($key in $saved.Keys) {
        [Environment]::SetEnvironmentVariable($key, $saved[$key], 'Process')
    }
}
