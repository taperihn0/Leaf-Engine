# DataGen.ps1

param (
    [string]$ExecutablePath,
    [int]$TotalSessions,
    [int]$GamesPerSession,
    [int]$ThreadCount,
    [int]$Nodes
)

function Log-Msg {
    param([string]$Message)
    $Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$Timestamp] $Message"
}

if (-not (Test-Path $ExecutablePath) -or -not $TotalSessions -or -not $GamesPerSession) {
    Log-Msg "ERROR: Invalid arguments or executable not found."
    Write-Host "Usage: .\DataGen.ps1 <executable_path> <total_sessions> <games_per_session> <thread_count> <nodes>"
    exit 1
}

$DateStr = (Get-Date -Format "MMMdd").ToLower()
$BaseDir = [System.IO.Path]::Combine($PSScriptRoot, "misc", "selfplay_$DateStr")

if (Test-Path $BaseDir) {
    $SuffixCode = 97
    while ($true) {
        $Suffix = [char]$SuffixCode
        $TestDir = "${BaseDir}_${Suffix}"
        if (-not (Test-Path $TestDir)) {
            $BaseDir = $TestDir
            break
        }
        $SuffixCode++
        if ($SuffixCode -gt 122) {
            Log-Msg "ERROR: Too many sessions today!"
            exit 1
        }
    }
}

Log-Msg "Tournament Start"
Write-Host "Storage: $BaseDir"
Write-Host "----------------------------------------------------"

$SessionNum = 1

while ($SessionNum -le $TotalSessions) {
    $SessionDir = [System.IO.Path]::Combine($BaseDir, "session$SessionNum")
    $ErrFile = [System.IO.Path]::Combine($SessionDir, "err.txt") # Dodano rozszerzenie dla pewności

    if (-not (Test-Path $SessionDir)) {
        New-Item -ItemType Directory -Force -Path $SessionDir | Out-Null
    }

    Set-Content -Path $ErrFile -Value $null

    Log-Msg "Launching Session $SessionNum..."

    $NetPathRel = "src/assets/nets/publius_net128_0_h.bin"

    $SafeSessionDir = $SessionDir.Replace('\', '/')
    $SafeErrFile = $ErrFile.Replace('\', '/')

    $InputCmds = @(
        # "export_net $NetPathRel",
        "self_play $GamesPerSession $ThreadCount $SafeSessionDir $SafeErrFile nodes $Nodes"
    )

    $InputCmds | & $ExecutablePath 2>&1 | Write-Host

    $ExitCode = $LASTEXITCODE

    if ($ExitCode -ne 0) {
        Log-Msg "Crash (Code $ExitCode) in Session $SessionNum."
        $ErrTimestamp = Get-Date -Format "HH:mm:ss"
        Add-Content -Path $ErrFile -Value "[$ErrTimestamp] SCRIPT: Process exited with code $ExitCode"
        Log-Msg "Restarting in 5s..."
        Start-Sleep -Seconds 5
    }

    Log-Msg "Progress: $SessionNum / $TotalSessions sessions completed."
    Write-Host "----------------------------------------------------"
    $SessionNum++
}

Log-Msg "Tournament finished."
