$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$tracked = @(& git -C $root ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate repository files.' }

$forbiddenNames = @(
    'SkillshotCity.exe',
    'steam_api.dll',
    'steam_api64.dll',
    'steam_appid.txt',
    'openal32_x64.dll',
    'opengl32_system.dll'
)
$forbiddenExtensions = @(
    '.bmp', '.dds', '.depth32f', '.dmp', '.gpr', '.png', '.rbin',
    '.wav', '.ogg', '.mp3', '.obj', '.fbx'
)

$problems = New-Object System.Collections.Generic.List[string]
foreach ($relative in $tracked) {
    $leaf = Split-Path -Leaf $relative
    $extension = [IO.Path]::GetExtension($relative).ToLowerInvariant()
    if ($forbiddenNames -contains $leaf) {
        $problems.Add("forbidden game/system file: $relative")
    }
    if ($forbiddenExtensions -contains $extension) {
        $problems.Add("forbidden asset/capture extension: $relative")
    }
    if ($relative -match '(^|/)(game|captures?|reverse|analysis-ghidra|ghidra-user|tools/RenderDoc)(/|$)') {
        $problems.Add("forbidden development directory: $relative")
    }
}

if ($problems.Count) {
    $problems | ForEach-Object { Write-Error $_ }
    throw 'Public-tree check failed.'
}

Write-Output "Public-tree check passed for $($tracked.Count) tracked files."
