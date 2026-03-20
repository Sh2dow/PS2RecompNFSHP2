param(
    [ValidateSet("menu", "output", "runtime", "run")]
    [string]$Action = "menu",
    [string]$Configuration = "Debug",
    [string]$ElfPath = "F:\Games\ISO\PS2\ALPHA\SLUS_203.62",
    [string]$ExePath = "",
    [switch]$TailLog,
    [switch]$NoWait
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildLogDir = Join-Path $repoRoot "tools\ai\artifacts\build-logs"

if (-not (Test-Path $buildLogDir)) {
    New-Item -ItemType Directory -Path $buildLogDir | Out-Null
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Show-BuildNotification([string]$Title, [string]$Message, [string]$TooltipIcon = "Info") {
    try {
        $notifyIcon = New-Object System.Windows.Forms.NotifyIcon
        $notifyIcon.Icon = [System.Drawing.SystemIcons]::Application
        $notifyIcon.BalloonTipTitle = $Title
        $notifyIcon.BalloonTipText = $Message
        $notifyIcon.BalloonTipIcon = [System.Windows.Forms.ToolTipIcon]::$TooltipIcon
        $notifyIcon.Visible = $true
        $notifyIcon.ShowBalloonTip(5000)
        Start-Sleep -Milliseconds 5500
        $notifyIcon.Dispose()
    }
    catch {
        Write-Warning "Failed to show Windows notification: $($_.Exception.Message)"
    }
}

function Get-VcVarsBat {
    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe not found at $vswhere"
    }

    $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installPath) {
        throw "Could not locate a Visual Studio Build Tools installation."
    }

    $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvars)) {
        throw "vcvars64.bat not found at $vcvars"
    }

    return $vcvars
}

function Invoke-CmakeBuild([string]$BuildDir, [string]$BuildConfig, [string]$Target = "", [bool]$CleanFirst = $false) {
    $vcvarsBat = Get-VcVarsBat
    $quotedVcVars = '"' + $vcvarsBat + '"'
    $targetArg = ""
    if (-not [string]::IsNullOrWhiteSpace($Target)) {
        $targetArg = " --target $Target"
    }
    $cleanFirstArg = ""
    if ($CleanFirst) {
        $cleanFirstArg = " --clean-first"
    }
    $safeTarget = if ([string]::IsNullOrWhiteSpace($Target)) { "all" } else { $Target }
    $logStamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $logPath = Join-Path $buildLogDir ("{0}_{1}_{2}.log" -f $logStamp, $BuildConfig.ToLowerInvariant(), $safeTarget)

    Write-Host "[build] config=$BuildConfig dir=$BuildDir"
    Write-Host "[build] log=$logPath"
    Push-Location $repoRoot
    try {
        $buildOutput = & cmd.exe /c "call $quotedVcVars && cmake --build $BuildDir --config $BuildConfig$targetArg$cleanFirstArg" 2>&1
        $exitCode = $LASTEXITCODE
        $buildOutput | Tee-Object -FilePath $logPath
        return @{
            ExitCode = $exitCode
            LogPath = $logPath
        }
    }
    finally {
        Pop-Location
    }
}

function Invoke-OutputBuild([string]$BuildConfig) {
    $runtimeLib = Join-Path $repoRoot "out\build\ps2xRuntime\$BuildConfig\ps2_runtime.lib"
    $gameExe = Join-Path $repoRoot "output\$BuildConfig\ps2_game.exe"
    $gamePdb = Join-Path $repoRoot "output\$BuildConfig\ps2_game.pdb"
    $gameIlk = Join-Path $repoRoot "output\$BuildConfig\ps2_game.ilk"
    $staleRuntimeHeader = Join-Path $repoRoot "output\ps2_runtime.h"
    $realRuntimeHeader = Join-Path $repoRoot "ps2xRuntime\include\ps2_runtime.h"
    $forceRebuild = $false

    if ((Test-Path $staleRuntimeHeader) -and (Test-Path $realRuntimeHeader)) {
        Write-Host "[build] removing stale output\\ps2_runtime.h so generated sources use ps2xRuntime\\include\\ps2_runtime.h"
        Remove-Item $staleRuntimeHeader -Force
    }

    if (-not (Test-Path $gameExe)) {
        Write-Host "[build] ps2_game.exe is missing, forcing target rebuild"
        $forceRebuild = $true
    }

    if ((Test-Path $runtimeLib) -and (Test-Path $gameExe)) {
        $runtimeLibInfo = Get-Item $runtimeLib
        $gameExeInfo = Get-Item $gameExe
        if ($runtimeLibInfo.LastWriteTime -gt $gameExeInfo.LastWriteTime) {
            Write-Host "[build] runtime lib is newer than ps2_game.exe, forcing relink"
            $forceRebuild = $true
            Remove-Item $gameExe -Force -ErrorAction SilentlyContinue
            Remove-Item $gamePdb -Force -ErrorAction SilentlyContinue
            Remove-Item $gameIlk -Force -ErrorAction SilentlyContinue
        }
    }

    $buildResult = Invoke-CmakeBuild "output\build" $BuildConfig "ps2_game" $forceRebuild
    if ($buildResult.ExitCode -ne 0) {
        return $buildResult.ExitCode
    }

    if (-not (Test-Path $gameExe)) {
        Write-Error "Build completed without producing $gameExe . See log: $($buildResult.LogPath)"
    }

    return 0
}

function Invoke-RuntimeAndOutputBuild([string]$BuildConfig) {
    $runtimeResult = Invoke-CmakeBuild "out\build" $BuildConfig "ps2_runtime"
    if ($runtimeResult.ExitCode -ne 0) {
        return $runtimeResult.ExitCode
    }

    return (Invoke-OutputBuild $BuildConfig)
}

function Invoke-GameRun([string]$RunElfPath, [string]$RunExePath, [bool]$ShouldTailLog, [bool]$ShouldNoWait) {
    if ([string]::IsNullOrWhiteSpace($RunExePath)) {
        $RunExePath = Join-Path $repoRoot "output\Debug\ps2_game.exe"
    }

    $logPath = Join-Path (Split-Path -Parent $RunExePath) "ps2_log.txt"

    if (-not (Test-Path $RunExePath)) {
        throw "Missing executable: $RunExePath"
    }

    Write-Host "[run] exe=$RunExePath"
    Write-Host "[run] elf=$RunElfPath"
    Write-Host "[run] ps2Log=$logPath"

    if (Test-Path $logPath) {
        Remove-Item $logPath -Force
    }

    $proc = Start-Process -FilePath $RunExePath -ArgumentList $RunElfPath -PassThru
    Write-Host "[run] pid=$($proc.Id)"

    if ($ShouldTailLog) {
        $deadline = (Get-Date).AddSeconds(15)
        while ((Get-Date) -lt $deadline -and -not (Test-Path $logPath) -and -not $proc.HasExited) {
            Start-Sleep -Milliseconds 250
            $proc.Refresh()
        }

        if (Test-Path $logPath) {
            Write-Host "[run] tailing $logPath"
            Get-Content $logPath -Wait
            return 0
        }

        Write-Warning "ps2_log.txt was not created before timeout."
    }

    if (-not $ShouldNoWait) {
        $proc.WaitForExit()
        return $proc.ExitCode
    }

    return 0
}

if ($Action -eq "menu") {
    Write-Host "build.ps1"
    Write-Host "1. Build output only"
    Write-Host "2. Build ps2xRuntime and output"
    Write-Host "3. Run game"
    $choice = Read-Host "Choose action"

    switch ($choice) {
        "1" { $Action = "output" }
        "2" { $Action = "runtime" }
        "3" { $Action = "run" }
        default { throw "Unknown selection: $choice" }
    }
}

switch ($Action) {
    "output" {
        try {
            $exitCode = Invoke-OutputBuild $Configuration
            if ($exitCode -eq 0) {
                Show-BuildNotification "PS2Recomp Build" "Output build finished successfully." "Info"
            }
            else {
                Show-BuildNotification "PS2Recomp Build" "Output build failed with exit code $exitCode." "Error"
            }
            exit $exitCode
        }
        catch {
            Show-BuildNotification "PS2Recomp Build" "Output build failed: $($_.Exception.Message)" "Error"
            throw
        }
    }
    "runtime" {
        try {
            $exitCode = Invoke-RuntimeAndOutputBuild $Configuration
            if ($exitCode -eq 0) {
                Show-BuildNotification "PS2Recomp Build" "Runtime and output build finished successfully." "Info"
            }
            else {
                Show-BuildNotification "PS2Recomp Build" "Runtime/output build failed with exit code $exitCode." "Error"
            }
            exit $exitCode
        }
        catch {
            Show-BuildNotification "PS2Recomp Build" "Runtime/output build failed: $($_.Exception.Message)" "Error"
            throw
        }
    }
    "run" {
        exit (Invoke-GameRun $ElfPath $ExePath $TailLog.IsPresent $NoWait.IsPresent)
    }
    default {
        throw "Unsupported action: $Action"
    }
}
