param(
    [ValidateSet("menu", "smart", "output", "runtime", "run")]
    [string]$Action = "menu",
    [string]$Configuration = "Debug",
    [string]$ElfPath = "F:\Games\ISO\PS2\ALPHA\SLUS_203.62",
    [string]$ExePath = "",
    [ValidateSet("auto", "vs", "ninja")]
    [string]$OutputBackend = "auto",
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

function Get-PreferredOutputBackend([string]$RequestedBackend) {
    if ($RequestedBackend -ne "auto") {
        return $RequestedBackend
    }

    if (Get-Command ninja -ErrorAction SilentlyContinue) {
        return "ninja"
    }

    return "vs"
}

function Get-NinjaPath {
    $vsBundledNinja = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
    if (Test-Path $vsBundledNinja) {
        return $vsBundledNinja
    }

    $ninjaCmd = Get-Command ninja -ErrorAction SilentlyContinue
    if ($null -eq $ninjaCmd) {
        throw "ninja was requested but not found on PATH"
    }

    return $ninjaCmd.Source
}

function Get-OutputBuildDir([string]$Backend, [string]$BuildConfig) {
    switch ($Backend) {
        "ninja" {
            $configSuffix = $BuildConfig.ToLowerInvariant()
            return "output\build-ninja-$configSuffix"
        }
        default { return "output\build" }
    }
}

function Ensure-CmakeConfigured([string]$SourceDir, [string]$BuildDir, [string]$Generator, [string]$BuildConfig = "") {
    $buildPath = Join-Path $repoRoot $BuildDir
    $cachePath = Join-Path $buildPath "CMakeCache.txt"
    $expectedManifest = if ($Generator -like "Ninja*") {
        Join-Path $buildPath "build.ninja"
    }
    else {
        $null
    }

    $isFullyConfigured = (Test-Path $cachePath)
    if ($expectedManifest) {
        $isFullyConfigured = $isFullyConfigured -and (Test-Path $expectedManifest)
    }

    if ($isFullyConfigured) {
        return
    }

    $vcvarsBat = Get-VcVarsBat
    $quotedVcVars = '"' + $vcvarsBat + '"'
    $quotedGenerator = '"' + $Generator + '"'
    $extraArgs = ""

    if ($Generator -like "Ninja*") {
        $ninjaPath = Get-NinjaPath
        $quotedNinjaPath = '"' + $ninjaPath + '"'
        $extraArgs = " -DCMAKE_MAKE_PROGRAM=$quotedNinjaPath"
        if (-not [string]::IsNullOrWhiteSpace($BuildConfig)) {
            $extraArgs += " -DCMAKE_BUILD_TYPE=$BuildConfig"
        }
    }

    if ((Test-Path $cachePath) -and -not $isFullyConfigured) {
        Write-Host "[configure] detected incomplete CMake state in $BuildDir, re-running configure"
    }

    Write-Host "[configure] source=$SourceDir build=$BuildDir generator=$Generator"
    Push-Location $repoRoot
    try {
        & cmd.exe /c "call $quotedVcVars && cmake -S $SourceDir -B $BuildDir -G $quotedGenerator$extraArgs"
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configure failed for $BuildDir"
        }
    }
    finally {
        Pop-Location
    }
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

function Get-LatestWriteTime([string[]]$Paths, [string[]]$IncludePatterns = @("*"), [string[]]$ExcludeDirectories = @()) {
    $latest = [datetime]::MinValue

    foreach ($path in $Paths) {
        if (-not (Test-Path $path)) {
            continue
        }

        $item = Get-Item $path
        if ($item.PSIsContainer) {
            $files = Get-ChildItem $path -Recurse -File -Include $IncludePatterns
            foreach ($file in $files) {
                $skip = $false
                foreach ($excludeDir in $ExcludeDirectories) {
                    if ($file.FullName.StartsWith($excludeDir, [System.StringComparison]::OrdinalIgnoreCase)) {
                        $skip = $true
                        break
                    }
                }

                if ($skip) {
                    continue
                }

                if ($file.LastWriteTime -gt $latest) {
                    $latest = $file.LastWriteTime
                }
            }
        }
        elseif ($item.LastWriteTime -gt $latest) {
            $latest = $item.LastWriteTime
        }
    }

    return $latest
}

function Get-SmartBuildPlan([string]$BuildConfig) {
    $runtimeLib = Join-Path $repoRoot "out\build\ps2xRuntime\$BuildConfig\ps2_runtime.lib"
    $gameExe = Join-Path $repoRoot "output\$BuildConfig\ps2_game.exe"

    $runtimeSources = @(
        (Join-Path $repoRoot "ps2xRuntime"),
        (Join-Path $repoRoot "CMakeLists.txt")
    )

    $outputSources = @(
        (Join-Path $repoRoot "output\CMakeLists.txt"),
        (Join-Path $repoRoot "output\main.cpp")
    )

    $generatedOutputDir = Join-Path $repoRoot "output"
    $excludeOutputDirs = @(
        (Join-Path $repoRoot "output\build"),
        (Join-Path $repoRoot "output\build-ninja"),
        (Join-Path $repoRoot "output\Debug"),
        (Join-Path $repoRoot "output\Release"),
        (Join-Path $repoRoot "output\RelWithDebInfo"),
        (Join-Path $repoRoot "output\MinSizeRel")
    )

    $runtimeLatest = Get-LatestWriteTime -Paths $runtimeSources -IncludePatterns @("*.cpp", "*.cc", "*.c", "*.h", "*.hpp", "*.inl", "*.ipp", "*.cmake", "CMakeLists.txt")
    $outputLatest = Get-LatestWriteTime -Paths ($outputSources + $generatedOutputDir) -IncludePatterns @("*.cpp", "*.h", "*.hpp", "*.inl", "*.toml", "CMakeLists.txt") -ExcludeDirectories $excludeOutputDirs

    $runtimeLibTime = if (Test-Path $runtimeLib) { (Get-Item $runtimeLib).LastWriteTime } else { [datetime]::MinValue }
    $gameExeTime = if (Test-Path $gameExe) { (Get-Item $gameExe).LastWriteTime } else { [datetime]::MinValue }

    $needsRuntime = (-not (Test-Path $runtimeLib)) -or ($runtimeLatest -gt $runtimeLibTime)
    $needsOutput = (-not (Test-Path $gameExe)) -or ($outputLatest -gt $gameExeTime)

    if ($needsRuntime) {
        $needsOutput = $true
    }

    $mode = if ($needsRuntime) {
        "runtime"
    }
    elseif ($needsOutput) {
        "output"
    }
    else {
        "none"
    }

    return @{
        Mode = $mode
        RuntimeLatest = $runtimeLatest
        RuntimeLibTime = $runtimeLibTime
        OutputLatest = $outputLatest
        GameExeTime = $gameExeTime
        RuntimeLib = $runtimeLib
        GameExe = $gameExe
    }
}

function Invoke-OutputBuild([string]$BuildConfig) {
    $backend = Get-PreferredOutputBackend $OutputBackend
    $outputBuildDir = Get-OutputBuildDir $backend $BuildConfig
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

    if ($backend -eq "ninja") {
        Ensure-CmakeConfigured "output" $outputBuildDir "Ninja" $BuildConfig
    }

    Write-Host "[build] output-backend=$backend"
    $buildResult = Invoke-CmakeBuild $outputBuildDir $BuildConfig "ps2_game" $forceRebuild
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

function Invoke-SmartBuild([string]$BuildConfig) {
    $plan = Get-SmartBuildPlan $BuildConfig

    Write-Host "[smart] runtimeLatest=$($plan.RuntimeLatest.ToString('yyyy-MM-dd HH:mm:ss')) runtimeLib=$($plan.RuntimeLibTime.ToString('yyyy-MM-dd HH:mm:ss'))"
    Write-Host "[smart] outputLatest=$($plan.OutputLatest.ToString('yyyy-MM-dd HH:mm:ss')) gameExe=$($plan.GameExeTime.ToString('yyyy-MM-dd HH:mm:ss'))"
    Write-Host "[smart] decision=$($plan.Mode)"
    Write-Host "[smart] output-backend=$(Get-PreferredOutputBackend $OutputBackend)"

    switch ($plan.Mode) {
        "runtime" {
            return (Invoke-RuntimeAndOutputBuild $BuildConfig)
        }
        "output" {
            return (Invoke-OutputBuild $BuildConfig)
        }
        "none" {
            Write-Host "[smart] no build needed"
            return 0
        }
        default {
            throw "Unknown smart-build mode: $($plan.Mode)"
        }
    }
}

function Invoke-GameRun([string]$RunElfPath, [string]$RunExePath, [bool]$ShouldTailLog, [bool]$ShouldNoWait) {
    if ([string]::IsNullOrWhiteSpace($RunExePath)) {
        $RunExePath = Join-Path $repoRoot "output\Debug\ps2_game.exe"
    }

    $exeDir = Split-Path -Parent $RunExePath
    $logPath = Join-Path $exeDir "ps2_log.txt"
    $rawLogPath = Join-Path $exeDir "raw_log.txt"

    if (-not (Test-Path $RunExePath)) {
        throw "Missing executable: $RunExePath"
    }

    Write-Host "[run] exe=$RunExePath"
    Write-Host "[run] elf=$RunElfPath"
    Write-Host "[run] ps2Log=$logPath"
    Write-Host "[run] rawLog=$rawLogPath"

    if (Test-Path $logPath) {
        Remove-Item $logPath -Force
    }

    if (Test-Path $rawLogPath) {
        Remove-Item $rawLogPath -Force
    }

    if (-not $ShouldNoWait -and -not $ShouldTailLog) {
        $stdoutPath = Join-Path $exeDir "stdout.log"
        $stderrPath = Join-Path $exeDir "stderr.log"
        if (Test-Path $stdoutPath) {
            Remove-Item $stdoutPath -Force
        }
        if (Test-Path $stderrPath) {
            Remove-Item $stderrPath -Force
        }

        Push-Location $exeDir
        try {
            $proc = Start-Process -FilePath $RunExePath `
                                  -ArgumentList $RunElfPath `
                                  -WorkingDirectory $exeDir `
                                  -RedirectStandardOutput $stdoutPath `
                                  -RedirectStandardError $stderrPath `
                                  -PassThru `
                                  -Wait

            $stdoutLines = if (Test-Path $stdoutPath) { Get-Content $stdoutPath } else { @() }
            $stderrLines = if (Test-Path $stderrPath) { Get-Content $stderrPath } else { @() }
            @($stdoutLines + $stderrLines) | Set-Content -Path $rawLogPath
            return $proc.ExitCode
        }
        finally {
            Pop-Location
        }
    }

    $proc = Start-Process -FilePath $RunExePath -ArgumentList $RunElfPath -WorkingDirectory $exeDir -PassThru
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
    Write-Host "1. Smart incremental build"
    Write-Host "2. Build output only"
    Write-Host "3. Build ps2xRuntime and output"
    Write-Host "4. Run game"
    Write-Host "backend: $(Get-PreferredOutputBackend $OutputBackend)"
    $choice = Read-Host "Choose action"

    switch ($choice) {
        "1" { $Action = "smart" }
        "2" { $Action = "output" }
        "3" { $Action = "runtime" }
        "4" { $Action = "run" }
        default { throw "Unknown selection: $choice" }
    }
}

switch ($Action) {
    "smart" {
        try {
            $exitCode = Invoke-SmartBuild $Configuration
            if ($exitCode -eq 0) {
                Show-BuildNotification "PS2Recomp Build" "Smart incremental build finished successfully." "Info"
            }
            else {
                Show-BuildNotification "PS2Recomp Build" "Smart incremental build failed with exit code $exitCode." "Error"
            }
            exit $exitCode
        }
        catch {
            Show-BuildNotification "PS2Recomp Build" "Smart incremental build failed: $($_.Exception.Message)" "Error"
            throw
        }
    }
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
