#include "ps2_stubs.h"
#include "ps2_runtime.h"
#include "ps2_runtime_macros.h"
#include "ps2_syscalls.h"
#include <iostream>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <limits>
#include <algorithm>

#include "raylib.h"

namespace ps2_stubs
{
    namespace
    {
        constexpr uint8_t kPadModeDualShock = 0x73;
        constexpr uint8_t kPadAnalogCenter = 0x80;

        constexpr uint16_t kPadBtnSelect = 1u << 0;
        constexpr uint16_t kPadBtnL3 = 1u << 1;
        constexpr uint16_t kPadBtnR3 = 1u << 2;
        constexpr uint16_t kPadBtnStart = 1u << 3;
        constexpr uint16_t kPadBtnUp = 1u << 4;
        constexpr uint16_t kPadBtnRight = 1u << 5;
        constexpr uint16_t kPadBtnDown = 1u << 6;
        constexpr uint16_t kPadBtnLeft = 1u << 7;
        constexpr uint16_t kPadBtnL2 = 1u << 8;
        constexpr uint16_t kPadBtnR2 = 1u << 9;
        constexpr uint16_t kPadBtnL1 = 1u << 10;
        constexpr uint16_t kPadBtnR1 = 1u << 11;
        constexpr uint16_t kPadBtnTriangle = 1u << 12;
        constexpr uint16_t kPadBtnCircle = 1u << 13;
        constexpr uint16_t kPadBtnCross = 1u << 14;
        constexpr uint16_t kPadBtnSquare = 1u << 15;

        struct PadInputState
        {
            uint16_t buttons = 0xFFFF; // active-low
            uint8_t rx = kPadAnalogCenter;
            uint8_t ry = kPadAnalogCenter;
            uint8_t lx = kPadAnalogCenter;
            uint8_t ly = kPadAnalogCenter;
        };

        std::mutex g_padOverrideMutex;
        bool g_padOverrideEnabled = false;
        PadInputState g_padOverrideState{};
        bool g_padDebugCached = false;
        bool g_padDebugEnabled = false;

        uint8_t axisToByte(float axis)
        {
            axis = std::clamp(axis, -1.0f, 1.0f);
            const float mapped = (axis + 1.0f) * 127.5f;
            return static_cast<uint8_t>(std::lround(mapped));
        }

        bool padDebugEnabled()
        {
            if (!g_padDebugCached)
            {
                const char *env = std::getenv("PS2_PAD_DEBUG");
                g_padDebugEnabled = (env && *env && std::strcmp(env, "0") != 0);
                g_padDebugCached = true;
            }
            return g_padDebugEnabled;
        }

        void setButton(PadInputState &state, uint16_t mask, bool pressed)
        {
            if (pressed)
            {
                state.buttons = static_cast<uint16_t>(state.buttons & ~mask);
            }
        }

        int findFirstGamepad()
        {
            for (int i = 0; i < 4; ++i)
            {
                if (IsGamepadAvailable(i))
                {
                    return i;
                }
            }
            return -1;
        }

        void applyGamepadState(PadInputState &state)
        {
            if (!IsWindowReady())
            {
                return;
            }

            const int gamepad = findFirstGamepad();
            if (gamepad < 0)
            {
                return;
            }

            // Raylib mapping (PS2 -> raylib buttons/axes):
            // D-Pad -> LEFT_FACE_*, Cross/Circle/Square/Triangle -> RIGHT_FACE_*
            // L1/R1 -> TRIGGER_1, L2/R2 -> TRIGGER_2, L3/R3 -> THUMB
            // Select/Start -> MIDDLE_LEFT/MIDDLE_RIGHT
            state.lx = axisToByte(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X));
            state.ly = axisToByte(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y));
            state.rx = axisToByte(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X));
            state.ry = axisToByte(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y));

            setButton(state, kPadBtnUp, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP));
            setButton(state, kPadBtnDown, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN));
            setButton(state, kPadBtnLeft, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
            setButton(state, kPadBtnRight, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));

            setButton(state, kPadBtnCross, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
            setButton(state, kPadBtnCircle, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
            setButton(state, kPadBtnSquare, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT));
            setButton(state, kPadBtnTriangle, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP));

            setButton(state, kPadBtnL1, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1));
            setButton(state, kPadBtnR1, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1));
            setButton(state, kPadBtnL2, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_2));
            setButton(state, kPadBtnR2, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_2));

            setButton(state, kPadBtnL3, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_THUMB));
            setButton(state, kPadBtnR3, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_THUMB));

            setButton(state, kPadBtnSelect, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT));
            setButton(state, kPadBtnStart, IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT));
        }

        void applyKeyboardState(PadInputState &state, bool allowAnalog)
        {
            if (!IsWindowReady())
            {
                return;
            }

            // Keyboard mapping (PS2 -> keys):
            // D-Pad: arrows, Square/Cross/Circle/Triangle: Z/X/C/V
            // L1/R1: Q/E, L2/R2: 1/3, Start/Select: Enter/RightShift
            // L3/R3: LeftCtrl/RightCtrl, Analog left: WASD
            setButton(state, kPadBtnUp, IsKeyDown(KEY_UP));
            setButton(state, kPadBtnDown, IsKeyDown(KEY_DOWN));
            setButton(state, kPadBtnLeft, IsKeyDown(KEY_LEFT));
            setButton(state, kPadBtnRight, IsKeyDown(KEY_RIGHT));

            setButton(state, kPadBtnSquare, IsKeyDown(KEY_Z));
            setButton(state, kPadBtnCross, IsKeyDown(KEY_X));
            setButton(state, kPadBtnCircle, IsKeyDown(KEY_C));
            setButton(state, kPadBtnTriangle, IsKeyDown(KEY_V));

            setButton(state, kPadBtnL1, IsKeyDown(KEY_Q));
            setButton(state, kPadBtnR1, IsKeyDown(KEY_E));
            setButton(state, kPadBtnL2, IsKeyDown(KEY_ONE));
            setButton(state, kPadBtnR2, IsKeyDown(KEY_THREE));

            setButton(state, kPadBtnStart, IsKeyDown(KEY_ENTER));
            setButton(state, kPadBtnSelect, IsKeyDown(KEY_RIGHT_SHIFT));
            setButton(state, kPadBtnL3, IsKeyDown(KEY_LEFT_CONTROL));
            setButton(state, kPadBtnR3, IsKeyDown(KEY_RIGHT_CONTROL));

            if (!allowAnalog)
            {
                return;
            }

            float ax = 0.0f;
            float ay = 0.0f;
            if (IsKeyDown(KEY_D))
                ax += 1.0f;
            if (IsKeyDown(KEY_A))
                ax -= 1.0f;
            if (IsKeyDown(KEY_S))
                ay += 1.0f;
            if (IsKeyDown(KEY_W))
                ay -= 1.0f;

            if (ax != 0.0f || ay != 0.0f)
            {
                state.lx = axisToByte(ax);
                state.ly = axisToByte(ay);
            }
        }

        void fillPadStatus(uint8_t *data, const PadInputState &state)
        {
            std::memset(data, 0, 32);
            data[1] = kPadModeDualShock;
            data[2] = static_cast<uint8_t>(state.buttons & 0xFFu);
            data[3] = static_cast<uint8_t>((state.buttons >> 8) & 0xFFu);
            data[4] = state.rx;
            data[5] = state.ry;
            data[6] = state.lx;
            data[7] = state.ly;
        }
    }

#include "stubs/helpers/ps2_stubs_helpers.inl"
#include "stubs/ps2_stubs_libc.inl"
#include "stubs/ps2_stubs_ps2.inl"
#include "stubs/ps2_stubs_misc.inl"

#include "stubs/ps2_stubs_gs.inl"
#include "stubs/ps2_stubs_residentEvilCV.inl"

    bool registerCdHostFileAlias(const std::string &syntheticKey,
                                 const std::filesystem::path &hostPath,
                                 uint32_t *baseLbnOut,
                                 uint32_t *sizeBytesOut)
    {
        std::error_code ec;
        const std::filesystem::path normalizedHostPath = std::filesystem::weakly_canonical(hostPath, ec);
        const std::filesystem::path effectiveHostPath =
            (!ec && !normalizedHostPath.empty()) ? normalizedHostPath : hostPath.lexically_normal();

        if (!std::filesystem::exists(effectiveHostPath, ec) || ec ||
            !std::filesystem::is_regular_file(effectiveHostPath, ec))
        {
            g_lastCdError = -1;
            return false;
        }

        for (const auto &[existingKey, existingEntry] : g_cdFilesByKey)
        {
            if (existingEntry.hostPath == effectiveHostPath)
            {
                if (baseLbnOut)
                {
                    *baseLbnOut = existingEntry.baseLbn;
                }
                if (sizeBytesOut)
                {
                    *sizeBytesOut = existingEntry.sizeBytes;
                }
                g_lastCdError = 0;
                return true;
            }
        }

        const std::string key = cdPathKey(syntheticKey);
        if (key.empty())
        {
            g_lastCdError = -1;
            return false;
        }

        const uint64_t sizeBytes64 = std::filesystem::file_size(effectiveHostPath, ec);
        if (ec)
        {
            g_lastCdError = -1;
            return false;
        }

        CdFileEntry entry;
        entry.hostPath = effectiveHostPath;
        entry.sizeBytes = static_cast<uint32_t>(std::min<uint64_t>(sizeBytes64, 0xFFFFFFFFu));
        entry.baseLbn = g_nextPseudoLbn;
        entry.sectors = sectorsForBytes(sizeBytes64);

        g_nextPseudoLbn += entry.sectors + 1;
        g_cdFilesByKey[key] = entry;

        if (baseLbnOut)
        {
            *baseLbnOut = entry.baseLbn;
        }
        if (sizeBytesOut)
        {
            *sizeBytesOut = entry.sizeBytes;
        }
        g_lastCdError = 0;
        return true;
    }

    void TODO(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        TODO_NAMED("unknown", rdram, ctx, runtime);
    }

    void TODO_NAMED(const char *name, uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const std::string stubName = name ? name : "unknown";
        const auto copyGuestCString = [&](uint32_t srcAddr, uint32_t dstAddr, uint32_t maxBytes)
        {
            if (srcAddr == 0u || dstAddr == 0u || maxBytes == 0u)
            {
                return;
            }

            uint32_t i = 0u;
            for (; i + 1u < maxBytes; ++i)
            {
                const uint8_t ch = READ8(srcAddr + i);
                WRITE8(dstAddr + i, ch);
                if (ch == 0u)
                {
                    return;
                }
            }

            WRITE8(dstAddr + i, 0u);
        };

        if (stubName == "__malloc_lock" ||
            stubName == "__malloc_unlock" ||
            stubName == "__malloc_update_mallinfo")
        {
            static std::atomic<uint32_t> s_mallocStubLogCount{0};
            const uint32_t callIndex = ++s_mallocStubLogCount;
            if (callIndex <= 8u)
            {
                std::cerr << "[stub] treating " << stubName << " as a no-op"
                          << " pc=0x" << std::hex << ctx->pc
                          << " ra=0x" << getRegU32(ctx, 31)
                          << std::dec << std::endl;
            }
            else if (callIndex == 9u)
            {
                std::cerr << "[stub] further malloc helper no-op logs suppressed" << std::endl;
            }

            setReturnS32(ctx, 0);
            return;
        }

        if (stubName == "__5bFilePCc9bFileTypeiPFP5bFileP10AsyncEntry_iT4")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            if (uint8_t *fileObj = getMemPtr(rdram, thisAddr))
            {
                std::memset(fileObj, 0, 0x8Cu);

                const uint32_t fileType = getRegU32(ctx, 6);
                const uint32_t fileSize = getRegU32(ctx, 7);
                const uint32_t asyncReadProc = getRegU32(ctx, 8);
                const uint32_t asyncCloseProc = getRegU32(ctx, 9);
                WRITE32(thisAddr + 0x6Cu, fileSize);
                WRITE32(thisAddr + 0x70u, 0u);
                WRITE32(thisAddr + 0x74u, 0u);
                WRITE32(thisAddr + 0x78u, fileType);
                WRITE32(thisAddr + 0x7Cu, asyncReadProc);
                WRITE32(thisAddr + 0x80u, asyncCloseProc);
                WRITE32(thisAddr + 0x84u, 0u);
                WRITE32(thisAddr + 0x88u, 0u);
            }

            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "__10AsyncEntryP5bFilePFP5bFileP10AsyncEntry_i")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            const uint32_t fileAddr = getRegU32(ctx, 5);
            const uint32_t procAddr = getRegU32(ctx, 6);

            if (uint8_t *entryObj = getMemPtr(rdram, thisAddr))
            {
                std::memset(entryObj, 0, 0x38u);

                // AsyncEntry is a bNode-derived queue record consumed by bServiceFileSystem.
                // The list links are overwritten by the queue insertion path, but the
                // function pointer and file ref must be initialized here.
                WRITE32(thisAddr + 0x00u, thisAddr);
                WRITE32(thisAddr + 0x04u, thisAddr);
                WRITE32(thisAddr + 0x08u, fileAddr);
                WRITE32(thisAddr + 0x0Cu, 0u);
                WRITE32(thisAddr + 0x10u, 0u);
                WRITE32(thisAddr + 0x14u, 0u);
                WRITE32(thisAddr + 0x18u, 0u);
                WRITE32(thisAddr + 0x1Cu, 0u);
                WRITE32(thisAddr + 0x20u, 0u);
                WRITE32(thisAddr + 0x24u, 0u);
                WRITE32(thisAddr + 0x28u, 0u);
                WRITE32(thisAddr + 0x2Cu, 0u);
                WRITE32(thisAddr + 0x30u, 0u);
                WRITE32(thisAddr + 0x34u, procAddr);
            }

            if (fileAddr != 0u)
            {
                uint8_t *fileObj = getMemPtr(rdram, fileAddr);
                if (fileObj != nullptr)
                {
                    const uint32_t pendingCount = READ32(fileAddr + 0x74u);
                    WRITE32(fileAddr + 0x74u, pendingCount + 1u);
                    static uint32_t asyncEntryCtorLogCount = 0u;
                    if (asyncEntryCtorLogCount < 32u)
                    {
                        std::cerr << "[AsyncEntry:ctor]"
                                  << " entry=0x" << std::hex << thisAddr
                                  << " file=0x" << fileAddr
                                  << " proc=0x" << procAddr
                                  << " pendingBefore=0x" << pendingCount
                                  << " pendingAfter=0x" << READ32(fileAddr + 0x74u)
                                  << " off70=0x" << READ32(fileAddr + 0x70u)
                                  << " type78=0x" << READ32(fileAddr + 0x78u)
                                  << " read7c=0x" << READ32(fileAddr + 0x7Cu)
                                  << " close80=0x" << READ32(fileAddr + 0x80u)
                                  << " base84=0x" << READ32(fileAddr + 0x84u)
                                  << std::dec << std::endl;
                        ++asyncEntryCtorLogCount;
                    }
                }
            }

            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "__10QueuedFilePvPCciiPFii_vi")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            const uint32_t targetAddr = getRegU32(ctx, 5);
            const uint32_t pathAddr = getRegU32(ctx, 6);
            const uint32_t startOffset = getRegU32(ctx, 7);
            const uint32_t requestSize = getRegU32(ctx, 8);
            const uint32_t callbackAddr = getRegU32(ctx, 9);
            const uint32_t callbackUserData = getRegU32(ctx, 10);

            if (uint8_t *queuedFile = getMemPtr(rdram, thisAddr))
            {
                std::memset(queuedFile, 0, 0x64u);

                // QueuedFile is a list node consumed by ServiceQueuedFiles.
                WRITE32(thisAddr + 0x00u, thisAddr);
                WRITE32(thisAddr + 0x04u, thisAddr);
                WRITE32(thisAddr + 0x08u, targetAddr);
                copyGuestCString(pathAddr, thisAddr + 0x0Cu, 0x30u);
                WRITE32(thisAddr + 0x3Cu, 0u);
                WRITE32(thisAddr + 0x40u, ADD32(startOffset, requestSize));
                WRITE32(thisAddr + 0x44u, startOffset);
                WRITE32(thisAddr + 0x48u, requestSize);
                WRITE32(thisAddr + 0x4Cu, 0u);
                WRITE32(thisAddr + 0x50u, callbackAddr);
                WRITE32(thisAddr + 0x54u, callbackUserData);
                WRITE32(thisAddr + 0x58u, targetAddr);
                WRITE32(thisAddr + 0x5Cu, 0u);
                WRITE32(thisAddr + 0x60u, 0u);
            }

            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "__12ResourceFilePCc16ResourceFileTypeiPFi_vi")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            const uint32_t pathAddr = getRegU32(ctx, 5);
            const uint32_t resourceType = getRegU32(ctx, 6);
            const uint32_t flags = getRegU32(ctx, 7);

            if (uint8_t *resourceFile = getMemPtr(rdram, thisAddr))
            {
                std::memset(resourceFile, 0, 0xD8u);

                WRITE32(thisAddr + 0x00u, thisAddr);
                WRITE32(thisAddr + 0x04u, thisAddr);
                WRITE32(thisAddr + 0x08u, resourceType);
                WRITE32(thisAddr + 0x0Cu, flags);
                copyGuestCString(pathAddr, thisAddr + 0x10u, 0x50u);
                WRITE32(thisAddr + 0xB0u, 0u);
                WRITE32(thisAddr + 0xB4u, 0u);
                WRITE32(thisAddr + 0xC0u, 0u);
                WRITE32(thisAddr + 0xC4u, 0u);
                WRITE32(thisAddr + 0xC8u, 0u);
                WRITE32(thisAddr + 0xCCu, 0u);
                WRITE32(thisAddr + 0xD4u, 0u);
            }

            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "__11TexturePackPCc")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            const uint32_t pathAddr = getRegU32(ctx, 5);

            if (uint8_t *texturePack = getMemPtr(rdram, thisAddr))
            {
                std::memset(texturePack, 0, 0x84u);

                WRITE32(thisAddr + 0x00u, thisAddr);
                WRITE32(thisAddr + 0x04u, thisAddr);
                copyGuestCString(pathAddr, thisAddr + 0x28u, 0x30u);

                // Match the safe default sentinel/state values used by the streaming variant.
                WRITE32(thisAddr + 0x3Cu, 1u);
                WRITE32(thisAddr + 0x40u, 0u);
                WRITE32(thisAddr + 0x44u, 0xFFFFFFFFu);
                WRITE32(thisAddr + 0x48u, 0u);
                WRITE32(thisAddr + 0x4Cu, 0u);
                WRITE32(thisAddr + 0x50u, 0u);
                WRITE32(thisAddr + 0x54u, 0u);
                WRITE32(thisAddr + 0x58u, 0xFFFFFFFFu);
                WRITE32(thisAddr + 0x5Cu, 0u);
                WRITE32(thisAddr + 0x60u, 0xFFFFFFFFu);
                WRITE32(thisAddr + 0x64u, 0u);
                WRITE32(thisAddr + 0x68u, 0u);
                WRITE32(thisAddr + 0x78u, 0u);
                WRITE32(thisAddr + 0x7Cu, 0u);
                WRITE32(thisAddr + 0x80u, 0u);
            }

            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "__dl__10AsyncEntryPv" ||
            stubName == "__dl__5bFilePv" ||
            stubName == "__dl__10QueuedFilePv")
        {
            const uint32_t thisAddr = getRegU32(ctx, 4);
            setReturnU32(ctx, thisAddr);
            return;
        }

        if (stubName == "_str_out_char__FcPi")
        {
            const uint8_t ch = static_cast<uint8_t>(getRegU32(ctx, 4));
            const uint32_t countAddr = getRegU32(ctx, 5);
            const uint32_t outPtrAddr = ADD32(getRegU32(ctx, 28), 4294948868u);
            const uint32_t writePtr = READ32(outPtrAddr);
            if (writePtr != 0u)
            {
                WRITE8(writePtr, ch);
                WRITE32(outPtrAddr, ADD32(writePtr, 1u));
            }

            if (countAddr != 0u)
            {
                WRITE32(countAddr, ADD32(READ32(countAddr), 1u));
            }

            setReturnS32(ctx, 0);
            return;
        }

        if (stubName == "_str_out_str__FPCciPi")
        {
            uint32_t srcAddr = getRegU32(ctx, 4);
            int32_t remaining = static_cast<int32_t>(getRegU32(ctx, 5));
            const uint32_t countAddr = getRegU32(ctx, 6);
            const uint32_t outPtrAddr = ADD32(getRegU32(ctx, 28), 4294948868u);
            uint32_t writePtr = READ32(outPtrAddr);
            uint32_t written = 0u;

            if (srcAddr != 0u && writePtr != 0u)
            {
                if (remaining < 0)
                {
                    remaining = std::numeric_limits<int32_t>::max();
                }

                while (remaining > 0)
                {
                    const uint8_t ch = READ8(srcAddr);
                    if (ch == 0u)
                    {
                        break;
                    }

                    WRITE8(writePtr, ch);
                    srcAddr = ADD32(srcAddr, 1u);
                    writePtr = ADD32(writePtr, 1u);
                    ++written;
                    --remaining;
                }

                WRITE32(outPtrAddr, writePtr);
            }

            if (countAddr != 0u && written != 0u)
            {
                WRITE32(countAddr, ADD32(READ32(countAddr), written));
            }

            setReturnS32(ctx, 0);
            return;
        }

        if (stubName == "sceDevVu0Reset" ||
            stubName == "sceDevVu1Pause" ||
            stubName == "sceDevVu1Reset")
        {
            static std::atomic<uint32_t> s_vuDevStubLogCount{0};
            const uint32_t callIndex = ++s_vuDevStubLogCount;
            if (callIndex <= 8u)
            {
                std::cerr << "[stub] treating " << stubName << " as a no-op"
                          << " pc=0x" << std::hex << ctx->pc
                          << " ra=0x" << getRegU32(ctx, 31)
                          << std::dec << std::endl;
            }
            else if (callIndex == 9u)
            {
                std::cerr << "[stub] further VU dev-helper no-op logs suppressed" << std::endl;
            }

            setReturnS32(ctx, 0);
            return;
        }

        uint32_t callCount = 0;
        {
            std::lock_guard<std::mutex> lock(g_stubWarningMutex);
            callCount = ++g_stubWarningCount[stubName];
        }

        if (callCount > kMaxStubWarningsPerName)
        {
            if (callCount == (kMaxStubWarningsPerName + 1))
            {
                std::cerr << "Warning: Further calls to PS2 stub '" << stubName
                          << "' are suppressed after " << kMaxStubWarningsPerName << " warnings" << std::endl;
            }
            setReturnS32(ctx, -1);
            return;
        }

        uint32_t stub_num = getRegU32(ctx, 2);   // $v0
        uint32_t caller_ra = getRegU32(ctx, 31); // $ra

        std::cerr << "Warning: Unimplemented PS2 stub called. name=" << stubName
                  << " PC=0x" << std::hex << ctx->pc
                  << ", RA=0x" << caller_ra
                  << ", Stub# guess (from $v0)=0x" << stub_num << std::dec << std::endl;

        // More context for debugging
        std::cerr << "  Args: $a0=0x" << std::hex << getRegU32(ctx, 4)
                  << ", $a1=0x" << getRegU32(ctx, 5)
                  << ", $a2=0x" << getRegU32(ctx, 6)
                  << ", $a3=0x" << getRegU32(ctx, 7) << std::dec << std::endl;

        setReturnS32(ctx, -1); // Return error
    }

    void setPadOverrideState(uint16_t buttons, uint8_t lx, uint8_t ly, uint8_t rx, uint8_t ry)
    {
        std::lock_guard<std::mutex> lock(g_padOverrideMutex);
        g_padOverrideEnabled = true;
        g_padOverrideState.buttons = buttons;
        g_padOverrideState.lx = lx;
        g_padOverrideState.ly = ly;
        g_padOverrideState.rx = rx;
        g_padOverrideState.ry = ry;
    }

    void clearPadOverrideState()
    {
        std::lock_guard<std::mutex> lock(g_padOverrideMutex);
        g_padOverrideEnabled = false;
        g_padOverrideState = PadInputState{};
    }

}
