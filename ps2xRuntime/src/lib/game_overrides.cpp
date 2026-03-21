#include "game_overrides.h"
#include "ps2_runtime.h"
#include "ps2_runtime_macros.h"
#include "ps2_runtime_calls.h"
#include "ps2_stubs.h"
#include "ps2_syscalls.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <vector>

namespace
{
    std::mutex &registryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    std::vector<ps2_game_overrides::Descriptor> &descriptorRegistry()
    {
        static std::vector<ps2_game_overrides::Descriptor> registry;
        return registry;
    }

    bool equalsIgnoreCaseAscii(std::string_view lhs, std::string_view rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            const auto l = static_cast<unsigned char>(lhs[i]);
            const auto r = static_cast<unsigned char>(rhs[i]);
            if (std::tolower(l) != std::tolower(r))
            {
                return false;
            }
        }

        return true;
    }

    std::string basenameFromPath(const std::string &path)
    {
        std::error_code ec;
        const std::filesystem::path fsPath(path);
        const std::filesystem::path leaf = fsPath.filename();
        if (leaf.empty())
        {
            return path;
        }
        return leaf.string();
    }

    uint32_t crc32Update(uint32_t crc, const uint8_t *data, size_t size)
    {
        static std::array<uint32_t, 256> table = []()
        {
            std::array<uint32_t, 256> values{};
            for (uint32_t i = 0; i < 256u; ++i)
            {
                uint32_t c = i;
                for (int bit = 0; bit < 8; ++bit)
                {
                    c = (c & 1u) ? (0xEDB88320u ^ (c >> 1u)) : (c >> 1u);
                }
                values[i] = c;
            }
            return values;
        }();

        uint32_t out = crc;
        for (size_t i = 0; i < size; ++i)
        {
            out = table[(out ^ data[i]) & 0xFFu] ^ (out >> 8u);
        }
        return out;
    }

    bool computeFileCrc32(const std::string &path, uint32_t &crcOut)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        std::array<uint8_t, 4096> chunk{};
        uint32_t crc = 0xFFFFFFFFu;

        while (file.good())
        {
            file.read(reinterpret_cast<char *>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
            const std::streamsize got = file.gcount();
            if (got <= 0)
            {
                break;
            }
            crc = crc32Update(crc, chunk.data(), static_cast<size_t>(got));
        }

        crcOut = ~crc;
        return true;
    }

    std::optional<PS2Runtime::RecompiledFunction> resolveHandlerByName(std::string_view handlerName)
    {
        const std::string_view resolvedSyscall = ps2_runtime_calls::resolveSyscallName(handlerName);
        if (!resolvedSyscall.empty())
        {
#define PS2_RESOLVE_SYSCALL(name)                      \
    if (resolvedSyscall == std::string_view{#name})    \
    {                                                  \
        return &ps2_syscalls::name;                    \
    }
            PS2_SYSCALL_LIST(PS2_RESOLVE_SYSCALL)
#undef PS2_RESOLVE_SYSCALL
        }

        const std::string_view resolvedStub = ps2_runtime_calls::resolveStubName(handlerName);
        if (!resolvedStub.empty())
        {
#define PS2_RESOLVE_STUB(name)                       \
    if (resolvedStub == std::string_view{#name})     \
    {                                                \
        return &ps2_stubs::name;                     \
    }
            PS2_STUB_LIST(PS2_RESOLVE_STUB)
#undef PS2_RESOLVE_STUB
        }

        return std::nullopt;
    }

    void nfsHp2AlphaRecurseDirectoryContinuation(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        auto callRegistered = [&](uint32_t address, uint32_t returnPc) -> bool
        {
            if (!runtime->hasFunction(address))
            {
                std::cerr << "[nfs-hp2-alpha] missing continuation target 0x"
                          << std::hex << address << std::dec << std::endl;
                runtime->requestStop();
                return false;
            }

            const uint32_t entryPc = address;
            ctx->pc = address;
            auto fn = runtime->lookupFunction(address);
            fn(rdram, ctx, runtime);
            if (ctx->pc == entryPc)
            {
                ctx->pc = returnPc;
            }
            return ctx->pc == returnPc;
        };

        switch (ctx->pc)
        {
        case 0x1FA6C0u:
            goto label_1fa6c0;
        case 0x1FA7A0u:
            goto label_1fa7a0;
        case 0x1FA7B0u:
            goto label_1fa7b0;
        case 0x1FA804u:
            goto label_1fa804;
        case 0x1FA834u:
            goto label_1fa834;
        default:
            std::cerr << "[nfs-hp2-alpha] unexpected RecurseDirectory continuation pc=0x"
                      << std::hex << ctx->pc << std::dec << std::endl;
            runtime->requestStop();
            return;
        }

label_1fa6c0:
        ctx->pc = 0x1FA6C0u;
        SET_GPR_U64(ctx, 2, GPR_U64(ctx, 2) & (uint64_t)(uint16_t)2);
        ctx->pc = 0x1FA6C4u;
        {
            const bool branch_taken_0x1fa6c4 = (GPR_U64(ctx, 2) == GPR_U64(ctx, 0));
            ctx->pc = 0x1FA6C8u;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA6C4u;
            SET_GPR_S32(ctx, 4, (int32_t)ADD32(GPR_U32(ctx, 19), 2));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa6c4)
            {
                ctx->pc = 0x1FA7B0u;
                goto label_1fa7b0;
            }
        }

        ctx->pc = 0x1FA6CCu;
        {
            const bool branch_taken_0x1fa6cc = (GPR_U64(ctx, 23) == GPR_U64(ctx, 0));
            if (branch_taken_0x1fa6cc)
            {
                ctx->pc = 0x1FA6D0u;
                ctx->in_delay_slot = true;
                ctx->branch_pc = 0x1FA6CCu;
                SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));
                ctx->in_delay_slot = false;
                ctx->pc = 0x1FA804u;
                goto label_1fa804;
            }
        }

        ctx->pc = 0x1FA6D4u;
        SET_GPR_U32(ctx, 31, 0x1FA6DCu);
        ctx->pc = 0x1FA6D8u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA6D4u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 22) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8650u, 0x1FA6DCu))
        {
            return;
        }

        ctx->pc = 0x1FA6DCu;
        SET_GPR_S32(ctx, 4, (int32_t)ADD32(GPR_U32(ctx, 2), 13));
        ctx->pc = 0x1FA6E0u;
        SET_GPR_U32(ctx, 31, 0x1FA6E8u);
        ctx->pc = 0x1FA6E4u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA6E0u;
        SET_GPR_S32(ctx, 5, (int32_t)ADD32(GPR_U32(ctx, 0), 16));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FBFE0u, 0x1FA6E8u))
        {
            return;
        }

        ctx->pc = 0x1FA6E8u;
        SET_GPR_U64(ctx, 17, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 22) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA6F0u;
        SET_GPR_U32(ctx, 31, 0x1FA6F8u);
        ctx->pc = 0x1FA6F4u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA6F0u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8690u, 0x1FA6F8u))
        {
            return;
        }

        ctx->pc = 0x1FA6F8u;
        SET_GPR_U32(ctx, 31, 0x1FA700u);
        ctx->pc = 0x1FA6FCu;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA6F8u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8650u, 0x1FA700u))
        {
            return;
        }

        ctx->pc = 0x1FA700u;
        SET_GPR_U64(ctx, 6, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA704u;
        {
            const bool branch_taken_0x1fa704 = (GPR_U64(ctx, 7) != GPR_U64(ctx, 0));
            ctx->pc = 0x1FA708u;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA704u;
            SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa704)
            {
                ctx->pc = 0x1FA794u;
                SET_GPR_U64(ctx, 7, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
                ctx->pc = 0x1FA798u;
                SET_GPR_U32(ctx, 31, 0x1FA7A0u);
                ctx->pc = 0x1FA79Cu;
                ctx->in_delay_slot = true;
                ctx->branch_pc = 0x1FA798u;
                SET_GPR_U64(ctx, 8, (uint64_t)GPR_U64(ctx, 23) + (uint64_t)GPR_U64(ctx, 0));
                ctx->in_delay_slot = false;
                ctx->pc = 0x1FA648u;
                if (!callRegistered(0x1FA648u, 0x1FA7A0u))
                {
                    return;
                }
                goto label_1fa7a0;
            }
        }

        ctx->pc = 0x1FA70Cu;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA710u;
        SET_GPR_U32(ctx, 31, 0x1FA718u);
        ctx->pc = 0x1FA714u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA710u;
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 6) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8690u, 0x1FA718u))
        {
            return;
        }

        ctx->pc = 0x1FA718u;
        SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));
        ctx->pc = 0x1FA71Cu;
        {
            const bool branch_taken_0x1fa71c = (GPR_U64(ctx, 2) == GPR_U64(ctx, 0));
            ctx->pc = 0x1FA720u;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA71Cu;
            SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa71c)
            {
                ctx->pc = 0x1FA7A0u;
                goto label_1fa7a0;
            }
        }

        ctx->pc = 0x1FA724u;
        SET_GPR_U32(ctx, 31, 0x1FA72Cu);
        ctx->pc = 0x1FA728u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA724u;
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 6) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8650u, 0x1FA72Cu))
        {
            return;
        }

        ctx->pc = 0x1FA72Cu;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA730u;
        SET_GPR_U32(ctx, 31, 0x1FA738u);
        ctx->pc = 0x1FA734u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA730u;
        SET_GPR_S32(ctx, 5, (int32_t)ADD32(GPR_U32(ctx, 0), 16));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FBFE0u, 0x1FA738u))
        {
            return;
        }

        ctx->pc = 0x1FA738u;
        SET_GPR_U64(ctx, 17, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 6) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA740u;
        SET_GPR_U32(ctx, 31, 0x1FA748u);
        ctx->pc = 0x1FA744u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA740u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8690u, 0x1FA748u))
        {
            return;
        }

        ctx->pc = 0x1FA748u;
        SET_GPR_U32(ctx, 31, 0x1FA750u);
        ctx->pc = 0x1FA74Cu;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA748u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8650u, 0x1FA750u))
        {
            return;
        }

        ctx->pc = 0x1FA750u;
        SET_GPR_U64(ctx, 6, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA754u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 21) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA75Cu;
        SET_GPR_U32(ctx, 31, 0x1FA764u);
        ctx->pc = 0x1FA760u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA75Cu;
        SET_GPR_U64(ctx, 7, (uint64_t)GPR_U64(ctx, 22) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA548u, 0x1FA764u))
        {
            return;
        }

        ctx->pc = 0x1FA764u;
        SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));
        ctx->pc = 0x1FA768u;
        {
            const bool branch_taken_0x1fa768 = (GPR_U64(ctx, 2) == GPR_U64(ctx, 0));
            ctx->pc = 0x1FA76Cu;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA768u;
            SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa768)
            {
                ctx->pc = 0x1FA7A0u;
                goto label_1fa7a0;
            }
        }

        ctx->pc = 0x1FA770u;
        SET_GPR_U32(ctx, 31, 0x1FA778u);
        ctx->pc = 0x1FA774u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA770u;
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 6) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1F8650u, 0x1FA778u))
        {
            return;
        }

        ctx->pc = 0x1FA778u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA77Cu;
        SET_GPR_U32(ctx, 31, 0x1FA784u);
        ctx->pc = 0x1FA780u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA77Cu;
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA4E0u, 0x1FA784u))
        {
            return;
        }

        ctx->pc = 0x1FA784u;
        goto label_1fa788;

label_1fa788:
        ctx->pc = 0x1FA788u;
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 16) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 6, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 21) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 7, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA798u;
        SET_GPR_U32(ctx, 31, 0x1FA7A0u);
        ctx->pc = 0x1FA79Cu;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA798u;
        SET_GPR_U64(ctx, 8, (uint64_t)GPR_U64(ctx, 23) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA648u, 0x1FA7A0u))
        {
            return;
        }

label_1fa7a0:
        ctx->pc = 0x1FA7A0u;
        SET_GPR_U32(ctx, 31, 0x1FA7A8u);
        ctx->pc = 0x1FA7A4u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA7A0u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FC190u, 0x1FA7A8u))
        {
            return;
        }

        ctx->pc = 0x1FA7A8u;
        {
            const bool branch_taken_0x1fa7a8 = (GPR_U64(ctx, 0) == GPR_U64(ctx, 0));
            ctx->pc = 0x1FA7ACu;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA7A8u;
            SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa7a8)
            {
                ctx->pc = 0x1FA804u;
                goto label_1fa804;
            }
        }

label_1fa7b0:
        ctx->pc = 0x1FA7B0u;
        SET_GPR_U32(ctx, 31, 0x1FA7B8u);
        ctx->pc = 0x1FA7B4u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA7B0u;
        SET_GPR_S32(ctx, 18, (int32_t)ADD32(GPR_U32(ctx, 19), 32));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA4E0u, 0x1FA7B8u))
        {
            return;
        }

        ctx->pc = 0x1FA7B8u;
        SET_GPR_S32(ctx, 17, (int32_t)ADD32(GPR_U32(ctx, 2), 150));
        ctx->pc = 0x1FA7BCu;
        SET_GPR_U32(ctx, 31, 0x1FA7C4u);
        ctx->pc = 0x1FA7C0u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA7BCu;
        SET_GPR_S32(ctx, 4, (int32_t)ADD32(GPR_U32(ctx, 19), 10));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA4E0u, 0x1FA7C4u))
        {
            return;
        }

        ctx->pc = 0x1FA7C4u;
        SET_GPR_U64(ctx, 16, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_S32(ctx, 4, (int32_t)ADD32(GPR_U32(ctx, 0), 20));
        ctx->pc = 0x1FA7CCu;
        SET_GPR_U32(ctx, 31, 0x1FA7D4u);
        ctx->pc = 0x1FA7D0u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA7CCu;
        SET_GPR_S32(ctx, 5, (int32_t)ADD32(GPR_U32(ctx, 0), 16));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FBFE0u, 0x1FA7D4u))
        {
            return;
        }

        ctx->pc = 0x1FA7D4u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 2) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 5, (uint64_t)GPR_U64(ctx, 17) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 6, (uint64_t)GPR_U64(ctx, 16) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 8, (uint64_t)GPR_U64(ctx, 18) + (uint64_t)GPR_U64(ctx, 0));
        ctx->pc = 0x1FA7E4u;
        SET_GPR_U32(ctx, 31, 0x1FA7ECu);
        ctx->pc = 0x1FA7E8u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA7E4u;
        SET_GPR_U64(ctx, 7, (uint64_t)GPR_U64(ctx, 22) + (uint64_t)GPR_U64(ctx, 0));
        ctx->in_delay_slot = false;
        if (!callRegistered(0x1FA548u, 0x1FA7ECu))
        {
            return;
        }

        ctx->pc = 0x1FA7ECu;
        SET_GPR_S32(ctx, 3, (int32_t)READ32(ADD32(GPR_U32(ctx, 21), 4)));
        WRITE32(ADD32(GPR_U32(ctx, 3), 0), GPR_U32(ctx, 2));
        WRITE32(ADD32(GPR_U32(ctx, 21), 4), GPR_U32(ctx, 2));
        WRITE32(ADD32(GPR_U32(ctx, 2), 4), GPR_U32(ctx, 3));
        WRITE32(ADD32(GPR_U32(ctx, 2), 0), GPR_U32(ctx, 21));
        SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));

label_1fa804:
        ctx->pc = 0x1FA804u;
        SET_GPR_S32(ctx, 19, (int32_t)ADD32(GPR_U32(ctx, 19), GPR_U32(ctx, 2)));
        SET_GPR_U32(ctx, 3, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 0)));
        ctx->pc = 0x1FA80Cu;
        {
            const bool branch_taken_0x1fa80c = (GPR_U64(ctx, 3) != GPR_U64(ctx, 0));
            ctx->pc = 0x1FA810u;
            ctx->in_delay_slot = true;
            ctx->branch_pc = 0x1FA80Cu;
            SET_GPR_S32(ctx, 4, (int32_t)SUB32(GPR_U32(ctx, 19), GPR_U32(ctx, 20)));
            ctx->in_delay_slot = false;
            if (branch_taken_0x1fa80c)
            {
                ctx->pc = 0x1FA834u;
                goto label_1fa834;
            }
        }

        ctx->pc = 0x1FA814u;
        SET_GPR_U64(ctx, 2, GPR_U64(ctx, 19) - GPR_U64(ctx, 20));
        SET_GPR_S32(ctx, 3, (int32_t)ADD32(GPR_U32(ctx, 0), 4294965248));
        SET_GPR_S64(ctx, 2, (int64_t)GPR_S64(ctx, 2) + (int64_t)(int32_t)2047);
        SET_GPR_U64(ctx, 2, GPR_U64(ctx, 2) & GPR_U64(ctx, 3));
        SET_GPR_U64(ctx, 2, GPR_U64(ctx, 2) << (32 + 0));
        SET_GPR_S64(ctx, 2, GPR_S64(ctx, 2) >> (32 + 0));
        SET_GPR_S32(ctx, 19, (int32_t)ADD32(GPR_U32(ctx, 20), GPR_U32(ctx, 2)));
        SET_GPR_S32(ctx, 4, (int32_t)SUB32(GPR_U32(ctx, 19), GPR_U32(ctx, 20)));

label_1fa834:
        ctx->pc = 0x1FA834u;
        SET_GPR_S32(ctx, 3, (int32_t)ADD32(GPR_U32(ctx, 0), 4294967295));
        SET_GPR_U64(ctx, 3, ((int64_t)GPR_S64(ctx, 3) < (int64_t)GPR_S64(ctx, 4)) ? 1 : 0);
        SET_GPR_S32(ctx, 2, (int32_t)ADD32(GPR_U32(ctx, 4), 2047));
        if (GPR_U64(ctx, 3) != 0)
        {
            SET_GPR_VEC(ctx, 2, GPR_VEC(ctx, 4));
        }
        SET_GPR_S32(ctx, 2, SRA32(GPR_S32(ctx, 2), 11));
        SET_GPR_U64(ctx, 2, ((int64_t)GPR_S64(ctx, 2) < (int64_t)GPR_S64(ctx, 30)) ? 1 : 0);
        ctx->pc = 0x1FA84Cu;
        {
            const bool branch_taken_0x1fa84c = (GPR_U64(ctx, 2) != GPR_U64(ctx, 0));
            if (branch_taken_0x1fa84c)
            {
                ctx->pc = 0x1FA850u;
                ctx->in_delay_slot = true;
                ctx->branch_pc = 0x1FA84Cu;
                SET_GPR_U32(ctx, 2, (uint8_t)READ8(ADD32(GPR_U32(ctx, 19), 25)));
                ctx->in_delay_slot = false;
                ctx->pc = 0x1FA6C0u;
                goto label_1fa6c0;
            }
        }

        ctx->pc = 0x1FA854u;
        SET_GPR_U64(ctx, 4, (uint64_t)GPR_U64(ctx, 20) + (uint64_t)GPR_U64(ctx, 0));
        SET_GPR_U64(ctx, 31, READ64(ADD32(GPR_U32(ctx, 29), 144)));
        SET_GPR_U64(ctx, 30, READ64(ADD32(GPR_U32(ctx, 29), 128)));
        SET_GPR_U64(ctx, 23, READ64(ADD32(GPR_U32(ctx, 29), 112)));
        SET_GPR_U64(ctx, 22, READ64(ADD32(GPR_U32(ctx, 29), 96)));
        SET_GPR_U64(ctx, 21, READ64(ADD32(GPR_U32(ctx, 29), 80)));
        SET_GPR_U64(ctx, 20, READ64(ADD32(GPR_U32(ctx, 29), 64)));
        SET_GPR_U64(ctx, 19, READ64(ADD32(GPR_U32(ctx, 29), 48)));
        SET_GPR_U64(ctx, 18, READ64(ADD32(GPR_U32(ctx, 29), 32)));
        SET_GPR_U64(ctx, 17, READ64(ADD32(GPR_U32(ctx, 29), 16)));
        SET_GPR_U64(ctx, 16, READ64(ADD32(GPR_U32(ctx, 29), 0)));
        ctx->pc = 0x1FA880u;
        ctx->pc = 0x1FA884u;
        ctx->in_delay_slot = true;
        ctx->branch_pc = 0x1FA880u;
        SET_GPR_S32(ctx, 29, (int32_t)ADD32(GPR_U32(ctx, 29), 160));
        ctx->in_delay_slot = false;
        ctx->pc = 0x1FC190u;
        if (runtime->hasFunction(0x1FC190u))
        {
            auto targetFn = runtime->lookupFunction(0x1FC190u);
            targetFn(rdram, ctx, runtime);
            return;
        }

        runtime->requestStop();
    }

    void nfsHp2AlphaTempDirectoryEntryCtor(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t entryPc = ctx->pc;
        const uint32_t thisPtr = getRegU32(ctx, 4);
        const uint32_t lbn = getRegU32(ctx, 5);
        const uint32_t size = getRegU32(ctx, 6);
        const uint32_t srcName = getRegU32(ctx, 7);

        auto readGuestStringLength = [&](uint32_t addr) -> uint32_t
        {
            uint32_t len = 0;
            constexpr uint32_t kMaxLen = 0x1000u;
            while (len < kMaxLen)
            {
                if (READ8(ADD32(addr, len)) == 0)
                {
                    break;
                }
                ++len;
            }
            return len;
        };

        auto callMalloc = [&](uint32_t bytes) -> uint32_t
        {
            if (!runtime->hasFunction(0x1FBFE0u))
            {
                runtime->requestStop();
                return 0u;
            }

            const uint32_t savedPc = ctx->pc;
            const uint32_t savedRa = getRegU32(ctx, 31);
            SET_GPR_U32(ctx, 4, bytes);
            SET_GPR_U32(ctx, 5, 0x10u);
            ctx->pc = 0x1FBFE0u;
            auto fn = runtime->lookupFunction(0x1FBFE0u);
            fn(rdram, ctx, runtime);
            if (ctx->pc == 0x1FBFE0u)
            {
                ctx->pc = savedPc;
            }
            SET_GPR_U32(ctx, 31, savedRa);
            return getRegU32(ctx, 2);
        };

        WRITE32(ADD32(thisPtr, 0), thisPtr);
        WRITE32(ADD32(thisPtr, 4), thisPtr);
        WRITE32(ADD32(thisPtr, 8), 0u);
        WRITE32(ADD32(thisPtr, 12), lbn);
        WRITE32(ADD32(thisPtr, 16), size);

        uint32_t guestNameCopy = 0u;
        if (srcName != 0u)
        {
            const uint32_t nameLen = readGuestStringLength(srcName);
            guestNameCopy = callMalloc(nameLen + 1u);
            if (guestNameCopy == 0u)
            {
                std::cerr << "[nfs-hp2-alpha] TempDirectoryEntry ctor failed to allocate "
                          << (nameLen + 1u) << " bytes" << std::endl;
                runtime->requestStop();
                return;
            }

            for (uint32_t i = 0; i < nameLen; ++i)
            {
                WRITE8(ADD32(guestNameCopy, i), READ8(ADD32(srcName, i)));
            }
            WRITE8(ADD32(guestNameCopy, nameLen), 0);
        }

        WRITE32(ADD32(thisPtr, 8), guestNameCopy);
        SET_GPR_U32(ctx, 2, thisPtr);
        ctx->pc = entryPc;
    }

    void nfsHp2AlphaLoadDirectory(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const auto &paths = PS2Runtime::getIoPaths();
        std::error_code ec;
        std::filesystem::path dirBinPath = paths.cdRoot / "DIR.BIN";
        if (!std::filesystem::exists(dirBinPath, ec) || ec)
        {
            const std::filesystem::path cdImageParent = paths.cdImage.parent_path();
            if (!cdImageParent.empty())
            {
                ec.clear();
                const std::filesystem::path cdImageStemDir = cdImageParent / paths.cdImage.stem();
                if (std::filesystem::exists(cdImageStemDir / "DIR.BIN", ec) && !ec)
                {
                    dirBinPath = cdImageStemDir / "DIR.BIN";
                }
                else
                {
                    ec.clear();
                    const std::string stem = paths.cdImage.stem().string();
                    const size_t paren = stem.find(" (");
                    if (paren != std::string::npos)
                    {
                        const std::filesystem::path trimmedStemDir = cdImageParent / stem.substr(0, paren);
                        if (std::filesystem::exists(trimmedStemDir / "DIR.BIN", ec) && !ec)
                        {
                            dirBinPath = trimmedStemDir / "DIR.BIN";
                        }
                    }
                }

                if (!std::filesystem::exists(dirBinPath, ec) || ec)
                {
                    ec.clear();
                    for (const auto &entry : std::filesystem::directory_iterator(cdImageParent, ec))
                    {
                        if (ec)
                        {
                            break;
                        }

                        if (!entry.is_directory())
                        {
                            continue;
                        }

                        const std::filesystem::path candidate = entry.path() / "DIR.BIN";
                        if (std::filesystem::exists(candidate, ec) && !ec)
                        {
                            dirBinPath = candidate;
                            break;
                        }
                    }
                }
            }
        }
        if (!std::filesystem::exists(dirBinPath, ec) || ec)
        {
            std::cerr << "[nfs-hp2-alpha] DIR.BIN missing at " << dirBinPath.string()
                      << " cdRoot=\"" << paths.cdRoot.string() << "\""
                      << " cdImage=\"" << paths.cdImage.string() << "\""
                      << std::endl;
            runtime->requestStop();
            return;
        }

        std::ifstream file(dirBinPath, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "[nfs-hp2-alpha] failed to open " << dirBinPath.string() << std::endl;
            runtime->requestStop();
            return;
        }

        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
        if (bytes.empty() || (bytes.size() % 12u) != 0u)
        {
            std::cerr << "[nfs-hp2-alpha] invalid DIR.BIN size " << bytes.size()
                      << " at " << dirBinPath.string() << std::endl;
            runtime->requestStop();
            return;
        }

        auto callMalloc = [&](uint32_t allocBytes) -> uint32_t
        {
            if (!runtime->hasFunction(0x1FBFE0u))
            {
                runtime->requestStop();
                return 0u;
            }

            const uint32_t savedPc = ctx->pc;
            const uint32_t savedRa = getRegU32(ctx, 31);
            SET_GPR_U32(ctx, 4, allocBytes);
            SET_GPR_U32(ctx, 5, 0x10u);
            ctx->pc = 0x1FBFE0u;
            auto fn = runtime->lookupFunction(0x1FBFE0u);
            fn(rdram, ctx, runtime);
            if (ctx->pc == 0x1FBFE0u)
            {
                ctx->pc = savedPc;
            }
            SET_GPR_U32(ctx, 31, savedRa);
            return getRegU32(ctx, 2);
        };

        const uint32_t tableBytes = static_cast<uint32_t>(bytes.size());
        const uint32_t tableAddr = callMalloc(tableBytes);
        if (tableAddr == 0u)
        {
            std::cerr << "[nfs-hp2-alpha] failed to allocate guest directory table of "
                      << tableBytes << " bytes" << std::endl;
            runtime->requestStop();
            return;
        }

        for (uint32_t i = 0; i < tableBytes; ++i)
        {
            WRITE8(ADD32(tableAddr, i), bytes[i]);
        }

        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t entryCount = tableBytes / 12u;
        WRITE32(ADD32(gp, 4294948304), tableAddr); // gp - 0x4A30
        WRITE32(ADD32(gp, 4294948308), entryCount); // gp - 0x4A2C
        WRITE32(ADD32(gp, 4294948312), getRegU32(ctx, 4)); // gp - 0x4A28

        static bool logged = false;
        if (!logged)
        {
            std::cerr << "[nfs-hp2-alpha] loaded DIR.BIN override entries=" << entryCount
                      << " table=0x" << std::hex << tableAddr
                      << " firstHash=0x" << READ32(tableAddr + 0)
                      << " firstLbn=0x" << READ32(tableAddr + 4)
                      << " firstSize=0x" << READ32(tableAddr + 8)
                      << std::dec << std::endl;
            logged = true;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaCreateDepthIntoAlphaBuffer(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        constexpr uint32_t kAlphaDepthBufferSize = 0x1401F0u;
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t bufferBase = READ32(ADD32(gp, 4294940744)); // gp - 0x67B8

        if (bufferBase != 0u)
        {
            if (uint8_t *buffer = getMemPtr(rdram, bufferBase))
            {
                std::memset(buffer, 0, kAlphaDepthBufferSize);
            }
        }

        static bool logged = false;
        if (!logged)
        {
            std::cerr << "[nfs-hp2-alpha] bypassed CreateDepthIntoAlphaBuffer"
                      << " base=0x" << std::hex << bufferBase
                      << " size=0x" << kAlphaDepthBufferSize
                      << " mode=0x" << getRegU32(ctx, 4)
                      << std::dec << std::endl;
            logged = true;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaDownloadVuCode(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!ctx)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        static bool logged = false;
        if (!logged)
        {
            std::cerr << "[nfs-hp2-alpha] bypassed DownloadVUCode"
                      << " ra=0x" << std::hex << getRegU32(ctx, 31)
                      << " gp=0x" << getRegU32(ctx, 28)
                      << std::dec << std::endl;
            logged = true;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaSkipDebugInitHelper(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!ctx)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        static bool logged = false;
        const uint32_t pc = ctx->pc;
        if (!logged)
        {
            std::cerr << "[nfs-hp2-alpha] bypassed debug init helper"
                      << " pc=0x" << std::hex << pc
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            logged = true;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void applyNfsHp2AlphaOverrides(PS2Runtime &runtime)
    {
        runtime.registerFunction(0x10F1B8u, &nfsHp2AlphaSkipDebugInitHelper);
        runtime.registerFunction(0x10F380u, &nfsHp2AlphaSkipDebugInitHelper);
        runtime.registerFunction(0x1027B8u, &nfsHp2AlphaDownloadVuCode);
        runtime.registerFunction(0x115618u, &nfsHp2AlphaCreateDepthIntoAlphaBuffer);
        runtime.registerFunction(0x1FA888u, &nfsHp2AlphaLoadDirectory);
        runtime.registerFunction(0x1FA548u, &nfsHp2AlphaTempDirectoryEntryCtor);
        runtime.registerFunction(0x1FA6C0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA7A0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA7B0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA804u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA834u, &nfsHp2AlphaRecurseDirectoryContinuation);
    }
}

PS2_REGISTER_GAME_OVERRIDE("NFS HP2 Alpha RecurseDirectory continuation",
                           "SLUS_203.62",
                           0x125AE0u,
                           0u,
                           &applyNfsHp2AlphaOverrides);

namespace ps2_game_overrides
{
    AutoRegister::AutoRegister(const Descriptor &descriptor)
    {
        registerDescriptor(descriptor);
    }

    void registerDescriptor(const Descriptor &descriptor)
    {
        if (!descriptor.apply)
        {
            std::cerr << "[game_overrides] ignoring descriptor with null apply callback." << std::endl;
            return;
        }

        std::lock_guard<std::mutex> lock(registryMutex());
        descriptorRegistry().push_back(descriptor);
    }

    bool bindAddressHandler(PS2Runtime &runtime, uint32_t address, std::string_view handlerName)
    {
        const auto resolved = resolveHandlerByName(handlerName);
        if (!resolved.has_value())
        {
            std::cerr << "[game_overrides] unresolved handler '" << handlerName
                      << "' for address 0x" << std::hex << address << std::dec << std::endl;
            return false;
        }

        runtime.registerFunction(address, resolved.value());
        return true;
    }

    void applyMatching(PS2Runtime &runtime, const std::string &elfPath, uint32_t entry)
    {
        std::vector<Descriptor> descriptors;
        {
            std::lock_guard<std::mutex> lock(registryMutex());
            descriptors = descriptorRegistry();
        }

        if (descriptors.empty())
        {
            std::cout << "[game_overrides] no registered descriptors" << std::endl;
            return;
        }

        const std::string elfName = basenameFromPath(elfPath);
        uint32_t fileCrc32 = 0u;
        bool fileCrcComputed = false;
        bool fileCrcValid = false;

        std::cout << "[game_overrides] probe elf='" << elfName
                  << "' entry=0x" << std::hex << entry << std::dec
                  << " descriptors=" << descriptors.size() << std::endl;

        size_t appliedCount = 0;
        for (const Descriptor &descriptor : descriptors)
        {
            if (!descriptor.apply)
            {
                continue;
            }

            if (descriptor.elfName && descriptor.elfName[0] != '\0')
            {
                if (!equalsIgnoreCaseAscii(descriptor.elfName, elfName))
                {
                    continue;
                }
            }

            if (descriptor.entry != 0u && descriptor.entry != entry)
            {
                continue;
            }

            if (descriptor.crc32 != 0u)
            {
                if (!fileCrcComputed)
                {
                    fileCrcComputed = true;
                    fileCrcValid = computeFileCrc32(elfPath, fileCrc32);
                    if (!fileCrcValid)
                    {
                        std::cerr << "[game_overrides] failed to compute CRC32 for '" << elfPath << "'" << std::endl;
                    }
                }

                if (!fileCrcValid || fileCrc32 != descriptor.crc32)
                {
                    continue;
                }
            }

            const char *name = (descriptor.name && descriptor.name[0] != '\0')
                                   ? descriptor.name
                                   : "unnamed";
            std::cout << "[game_overrides] applying '" << name << "'" << std::endl;
            descriptor.apply(runtime);
            ++appliedCount;
        }

        if (appliedCount > 0)
        {
            std::cout << "[game_overrides] applied " << appliedCount << " matching override(s)." << std::endl;
        }
        else
        {
            std::cout << "[game_overrides] no matching overrides for elf='" << elfName
                      << "' entry=0x" << std::hex << entry << std::dec << std::endl;
        }
    }
}
