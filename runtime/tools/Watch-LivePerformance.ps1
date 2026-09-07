param(
    [Parameter(Mandatory = $true)]
    [string]$GameDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [int]$SampleSeconds = 2
)

$ErrorActionPreference = 'SilentlyContinue'
$gameLog = Join-Path $GameDirectory 'SkillshotVR.log'
$xrLog = Join-Path $GameDirectory 'SkillshotVR-XR.log'
$logicalProcessors = [Math]::Max(1, [Environment]::ProcessorCount)
$missingSamples = 0
$previousCpu = $null
$previousCpuTime = $null

'timestamp,pid,game_fps,cpu_percent,working_set_mb,xr_hz,stereo_pairs_hz,left_eye_hz,right_eye_hz,gpu_percent,encoder_percent,vram_used_mb,vram_total_mb' |
    Set-Content -LiteralPath $OutputPath -Encoding utf8

function Get-RecentGameFps {
    if (-not (Test-Path -LiteralPath $gameLog)) { return $null }
    $samples = @()
    foreach ($line in (Get-Content -LiteralPath $gameLog -Tail 1200)) {
        if ($line -match '^(\d\d):(\d\d):(\d\d)\.(\d\d\d).*Interface load frame=(\d+)') {
            $milliseconds = (([int]$Matches[1] * 3600 +
                [int]$Matches[2] * 60 + [int]$Matches[3]) * 1000) +
                [int]$Matches[4]
            $samples += [pscustomobject]@{
                TimeMs = $milliseconds
                Frame = [long]$Matches[5]
            }
        }
    }
    if ($samples.Count -lt 2) { return $null }
    $newest = $samples[-1]
    $older = $samples | Where-Object {
        $_.Frame -lt $newest.Frame -and ($newest.TimeMs - $_.TimeMs) -ge 1000
    } | Select-Object -Last 1
    if (-not $older) { $older = $samples[-2] }
    $elapsedMs = $newest.TimeMs - $older.TimeMs
    if ($elapsedMs -le 0 -or $newest.Frame -le $older.Frame) { return $null }
    return 1000.0 * ($newest.Frame - $older.Frame) / $elapsedMs
}

function Get-RecentXrRates {
    $result = [ordered]@{ Xr = $null; Pairs = $null; Left = $null; Right = $null }
    if (-not (Test-Path -LiteralPath $xrLog)) { return $result }
    $line = Get-Content -LiteralPath $xrLog -Tail 300 |
        Where-Object { $_ -match 'Cadence .*XR Hz' } |
        Select-Object -Last 1
    if ($line -match 'Cadence ([0-9.]+) XR Hz, ([0-9.]+) coherent pairs/s, eyes ([0-9.]+)/([0-9.]+) renders/s') {
        $result.Xr = [double]$Matches[1]
        $result.Pairs = [double]$Matches[2]
        $result.Left = [double]$Matches[3]
        $result.Right = [double]$Matches[4]
    }
    return $result
}

while ($true) {
    $process = Get-Process -Name SkillshotCity | Where-Object {
        $_.Path -and $_.Path.StartsWith($GameDirectory,
            [StringComparison]::OrdinalIgnoreCase)
    } | Select-Object -First 1
    if (-not $process) {
        $missingSamples++
        if ($missingSamples -ge 3) { break }
        Start-Sleep -Seconds $SampleSeconds
        continue
    }
    $missingSamples = 0
    $now = Get-Date
    $cpuPercent = $null
    if ($null -ne $previousCpu -and $null -ne $previousCpuTime) {
        $elapsed = ($now - $previousCpuTime).TotalSeconds
        if ($elapsed -gt 0) {
            $cpuPercent = 100.0 * ($process.CPU - $previousCpu) /
                ($elapsed * $logicalProcessors)
        }
    }
    $previousCpu = $process.CPU
    $previousCpuTime = $now

    $xr = Get-RecentXrRates
    $gpu = @($null, $null, $null, $null)
    $gpuLine = & nvidia-smi --query-gpu=utilization.gpu,utilization.encoder,memory.used,memory.total --format=csv,noheader,nounits 2>$null |
        Select-Object -First 1
    if ($gpuLine) {
        $parts = $gpuLine -split ',' | ForEach-Object { $_.Trim() }
        if ($parts.Count -ge 4) { $gpu = $parts }
    }

    $values = @(
        $now.ToString('yyyy-MM-ddTHH:mm:ss.fff'),
        $process.Id,
        ('{0:F2}' -f (Get-RecentGameFps)),
        ('{0:F2}' -f $cpuPercent),
        ('{0:F1}' -f ($process.WorkingSet64 / 1MB)),
        ('{0:F1}' -f $xr.Xr),
        ('{0:F1}' -f $xr.Pairs),
        ('{0:F1}' -f $xr.Left),
        ('{0:F1}' -f $xr.Right),
        $gpu[0], $gpu[1], $gpu[2], $gpu[3]
    )
    ($values -join ',') | Add-Content -LiteralPath $OutputPath -Encoding utf8
    Start-Sleep -Seconds $SampleSeconds
}
