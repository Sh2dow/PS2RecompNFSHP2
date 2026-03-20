#include "ps2_runtime.h"

#if defined(_MSC_VER)
#include <crtdbg.h>
#include <cstdlib>
#endif
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

void registerAllFunctions(PS2Runtime &runtime);

namespace
{
std::filesystem::path defaultElfPath()
{
    return "F:/Games/ISO/PS2/ALPHA/SLUS_203.62";
}

std::filesystem::path defaultCdRoot()
{
    return "F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002";
}

std::filesystem::path defaultCdImage()
{
    return "F:/Games/ISO/PS2/HP2_A098_PS2_07_MAR_2002 (prototype).iso";
}

std::string makeWindowTitle(const std::filesystem::path &elfPath)
{
    return "PS2-Recomp | Need for Speed: Hot Pursuit 2 | " + elfPath.filename().string();
}

void configureCrashReporting()
{
#if defined(_WIN32)
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}
} // namespace

int main(int argc, char *argv[])
{
    configureCrashReporting();

    const std::filesystem::path elfPath = (argc >= 2) ? std::filesystem::path(argv[1]) : defaultElfPath();
    const std::filesystem::path elfDirectory = elfPath.parent_path();
    const std::filesystem::path bootstrapLogPath = "bootstrap_log.txt";
    std::ofstream bootstrapLog(bootstrapLogPath, std::ios::out | std::ios::trunc);
    auto logStage = [&](const char *stage)
    {
        if (bootstrapLog.is_open())
        {
            bootstrapLog << stage << std::endl;
            bootstrapLog.flush();
        }
    };

    PS2Runtime runtime;
    logStage("before runtime.initialize");
    if (!runtime.initialize(makeWindowTitle(elfPath).c_str()))
    {
        logStage("runtime.initialize failed");
        std::cerr << "Failed to initialize PS2 runtime" << std::endl;
        return 1;
    }
    logStage("after runtime.initialize");

    PS2Runtime::IoPaths paths;
    paths.hostRoot = elfDirectory;
    paths.cdRoot = defaultCdRoot();
    paths.cdImage = defaultCdImage();
    paths.elfPath = elfPath;
    paths.elfDirectory = elfDirectory;
    logStage("before setIoPaths");
    PS2Runtime::setIoPaths(paths);
    logStage("after setIoPaths");

    logStage("before registerAllFunctions");
    registerAllFunctions(runtime);
    runtime.finalizeFunctionTable();
    logStage("after registerAllFunctions");

    logStage("before loadELF");
    if (!runtime.loadELF(elfPath.string()))
    {
        logStage("loadELF failed");
        std::cerr << "Failed to load ELF file: " << elfPath << std::endl;
        return 1;
    }
    logStage("after loadELF");

    logStage("before runtime.run");
    runtime.run();
    logStage("after runtime.run");
    return 0;
}
