#include "game_overrides.h"
#include "ps2_runtime.h"
#include "ps2_runtime_macros.h"
#include "ps2_runtime_calls.h"
#include "ps2_stubs.h"
#include "ps2_syscalls.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

void BeginRead__10QueuedFile_0x181d20(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ServiceQueuedFiles__Fv_0x181e10(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void bServiceFileSystem__Fv_0x1f8ae8(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void eFixUpTables__Fv_0x102f50(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void eGetRenderTargetTextureInfo__Fi_0x104168(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void FixStripSTs__6eSolid_0x108ba0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void afxUpdateTextures__Fv_0x11fc60(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void LoadCommonRegion__6bSound_0x20cf50(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void eFindAnimationSolid__FUi_0x17ca70(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ReconnectSolid__10eAnimation_0x17bfd0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ReconnectAllAnimations__Fv_0x17c520(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ReconnectSolid__6eModel_0x109018(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ApplyReplacementTextureTable__6eModel_0x109248(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ReconnectAllModels__Fv_0x1094d0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ps2___11TexturePackPCc_0x180788(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void AttachTextureTable__11TexturePackP11TextureInfoi_0x1808e0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void AssignTextureData__11TexturePackPcii_0x1809f0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void LoadTextureTableFromFile__11TexturePackPCc_0x180c18(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void GetLoadedTexture__11TexturePackUi_0x180b30(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void GetTextureInfo__FUii_0x180650(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void ps2____10NisManager_0x185b60(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void Start__10NisManageriiP3CarT3_0x185cf8(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void Load__10NisManageri_0x185ee0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void StartAnimating__10NisManager_0x186228(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void Render__10NisManagerP5eView_0x1864a0(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void DrawNisObjects__FP5eView_0x186718(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void bOpen__FPCci_0x1f94d8(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void bOpen__FPCc_0x1f9590(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void OpenPlatformFile__FPCc_0x1fb360(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void InitFile__Q22M211MoviePlayerPCc_0x1eee08(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void AddTail__t6bTList1Z14eActiveTextureP5bList_0x111738(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void Sort__5bListPFP5bNodeP5bNode_i_0x1f5680(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void BeginLoad__6bSoundPcT1_0x20c760(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);
void Open__12cBaseSpeakerPCcT1_0x217820(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime);

namespace ps2_stubs
{
    bool registerCdHostFileAlias(const std::string &syntheticKey,
                                 const std::filesystem::path &hostPath,
                                 uint32_t *baseLbnOut,
                                 uint32_t *sizeBytesOut);
}

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

    constexpr uint32_t kPs2RdramSize = 0x02000000u;
    constexpr uint32_t kTextureInfoStride = 0xA0u;
    constexpr uint32_t kMaxReasonableTextureEntryCount = 0x20000u;

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

    struct TexturePackIndexCache
    {
        uint32_t packAddr = 0u;
        uint32_t tableAddr = 0u;
        uint32_t entryCount = 0u;
        std::unordered_map<uint32_t, std::vector<uint32_t>> entriesByTextureId;
    };

    struct TexturePackPayloadWindow
    {
        uint32_t dataAddr = 0u;
        uint32_t dataSize = 0u;
    };

    std::unordered_map<uint32_t, TexturePackIndexCache> &texturePackIndexCache()
    {
        static std::unordered_map<uint32_t, TexturePackIndexCache> cache;
        return cache;
    }

    std::unordered_map<uint32_t, TexturePackPayloadWindow> &texturePackPayloadWindows()
    {
        static std::unordered_map<uint32_t, TexturePackPayloadWindow> cache;
        return cache;
    }

    void writeGuestU32Raw(uint8_t *rdram, uint32_t addr, uint32_t value);
    uint32_t nfsBinPathHash(std::string path);
    std::filesystem::path detectNfsHp2UnpackedRoot(const PS2Runtime::IoPaths &paths);
    const std::unordered_map<uint32_t, std::filesystem::path> &getNfsHp2UnpackedHashIndex(
        const std::filesystem::path &root);

    uint32_t readGuestU32Raw(uint8_t *rdram, uint32_t addr)
    {
        const uint32_t masked = addr & PS2_RAM_MASK;
        return static_cast<uint32_t>(rdram[masked + 0]) |
               (static_cast<uint32_t>(rdram[masked + 1]) << 8) |
               (static_cast<uint32_t>(rdram[masked + 2]) << 16) |
               (static_cast<uint32_t>(rdram[masked + 3]) << 24);
    }

    uint8_t readGuestU8Raw(uint8_t *rdram, uint32_t addr)
    {
        if (!rdram)
        {
            return 0u;
        }
        return rdram[addr & PS2_RAM_MASK];
    }

    std::string readGuestCStringRaw(uint8_t *rdram, uint32_t addr, uint32_t maxLen = 0x200u)
    {
        if (!rdram || addr == 0u || maxLen == 0u)
        {
            return {};
        }

        std::string result;
        result.reserve(std::min<uint32_t>(maxLen, 64u));
        for (uint32_t i = 0; i < maxLen; ++i)
        {
            const uint8_t *guestPtr = getConstMemPtr(rdram, addr + i);
            if (!guestPtr)
            {
                break;
            }
            const char ch = static_cast<char>(*guestPtr);
            if (ch == '\0')
            {
                break;
            }
            result.push_back(ch);
        }
        return result;
    }

    std::array<uint8_t, 16> readGuestPreviewBytes(uint8_t *rdram, uint32_t addr)
    {
        std::array<uint8_t, 16> preview{};
        if (!rdram || addr == 0u)
        {
            return preview;
        }

        for (size_t i = 0; i < preview.size(); ++i)
        {
            const uint8_t *guestPtr = getConstMemPtr(rdram, addr + static_cast<uint32_t>(i));
            preview[i] = guestPtr ? *guestPtr : 0u;
        }
        return preview;
    }

    void mirrorScratchpadCStringToMaskedAlias(uint8_t *rdram,
                                              uint32_t sourceAddr,
                                              uint32_t maxLen = 0x100u)
    {
        if (!rdram || sourceAddr == 0u || maxLen == 0u || !ps2IsScratchpadAddress(sourceAddr))
        {
            return;
        }

        const uint32_t aliasBase = sourceAddr & PS2_RAM_MASK;
        uint32_t copied = 0u;
        for (; copied + 1u < maxLen; ++copied)
        {
            const uint8_t *guestPtr = getConstMemPtr(rdram, sourceAddr + copied);
            const uint8_t value = guestPtr ? *guestPtr : 0u;
            rdram[(aliasBase + copied) & PS2_RAM_MASK] = value;
            if (value == 0u)
            {
                return;
            }
        }

        rdram[(aliasBase + copied) & PS2_RAM_MASK] = 0u;
    }

    bool resolveNfsHp2UnpackedFile(const PS2Runtime::IoPaths &paths,
                                   const std::string &sourcePath,
                                   std::filesystem::path &hostPathOut)
    {
        if (sourcePath.empty())
        {
            return false;
        }

        const std::filesystem::path unpackedRoot = detectNfsHp2UnpackedRoot(paths);
        if (unpackedRoot.empty())
        {
            return false;
        }

        std::filesystem::path hostPath = (unpackedRoot / std::filesystem::path(sourcePath)).lexically_normal();
        std::error_code ec;
        if (std::filesystem::exists(hostPath, ec) && !ec && std::filesystem::is_regular_file(hostPath, ec))
        {
            hostPathOut = hostPath;
            return true;
        }

        const auto &hashIndex = getNfsHp2UnpackedHashIndex(unpackedRoot);
        const auto found = hashIndex.find(nfsBinPathHash(sourcePath));
        if (found == hashIndex.end())
        {
            return false;
        }

        hostPathOut = found->second;
        return true;
    }

    bool buildTexturePackIndex(uint8_t *rdram,
                               uint32_t packAddr,
                               uint32_t tableAddr,
                               uint32_t entryCount,
                               TexturePackIndexCache &cache)
    {
        if (!rdram || packAddr == 0u || tableAddr == 0u)
        {
            return false;
        }

        if (entryCount == 0u || entryCount > kMaxReasonableTextureEntryCount)
        {
            return false;
        }

        const uint64_t tableEnd = static_cast<uint64_t>(tableAddr) +
                                  (static_cast<uint64_t>(entryCount) * kTextureInfoStride);
        if (tableAddr >= kPs2RdramSize || tableEnd > kPs2RdramSize)
        {
            return false;
        }

        cache.packAddr = packAddr;
        cache.tableAddr = tableAddr;
        cache.entryCount = entryCount;
        cache.entriesByTextureId.clear();
        cache.entriesByTextureId.reserve(entryCount);

        for (uint32_t i = 0; i < entryCount; ++i)
        {
            const uint32_t entryAddr = tableAddr + (i * kTextureInfoStride);
            const uint32_t textureId = readGuestU32Raw(rdram, entryAddr + 0x20u);
            cache.entriesByTextureId[textureId].push_back(entryAddr);
        }

        return true;
    }

    bool tryLookupLoadedTextureFast(uint8_t *rdram,
                                    uint32_t packAddr,
                                    uint32_t textureId,
                                    uint32_t &resultAddr)
    {
        if (!rdram || packAddr == 0u)
        {
            return false;
        }

        const uint32_t tableAddr = readGuestU32Raw(rdram, packAddr + 0x60u);
        const uint32_t entryCount = readGuestU32Raw(rdram, packAddr + 0x64u);
        if (tableAddr == 0u || tableAddr == 0xFFFFFFFFu || entryCount == 0u)
        {
            resultAddr = 0u;
            return true;
        }

        auto &cacheMap = texturePackIndexCache();
        auto cacheIt = cacheMap.find(packAddr);
        if (cacheIt == cacheMap.end() ||
            cacheIt->second.tableAddr != tableAddr ||
            cacheIt->second.entryCount != entryCount)
        {
            TexturePackIndexCache rebuilt;
            if (!buildTexturePackIndex(rdram, packAddr, tableAddr, entryCount, rebuilt))
            {
                return false;
            }

            static uint32_t s_textureIndexLogCount = 0u;
            if (s_textureIndexLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] indexed TexturePack"
                          << " pack=0x" << std::hex << packAddr
                          << " table=0x" << tableAddr
                          << " count=0x" << entryCount
                          << " uniqueIds=0x" << rebuilt.entriesByTextureId.size()
                          << std::dec << std::endl;
                ++s_textureIndexLogCount;
            }

            cacheIt = cacheMap.insert_or_assign(packAddr, std::move(rebuilt)).first;
        }

        const auto found = cacheIt->second.entriesByTextureId.find(textureId);
        if (found == cacheIt->second.entriesByTextureId.end())
        {
            resultAddr = 0u;
            return true;
        }

        for (const uint32_t entryAddr : found->second)
        {
            uint32_t field8c = readGuestU32Raw(rdram, entryAddr + 0x8Cu);
            uint32_t field90 = readGuestU32Raw(rdram, entryAddr + 0x90u);
            if (field8c == 0u)
            {
                const auto payloadIt = texturePackPayloadWindows().find(packAddr);
                if (payloadIt != texturePackPayloadWindows().end())
                {
                    const uint32_t offset30 = readGuestU32Raw(rdram, entryAddr + 0x30u);
                    const uint32_t offset34 = readGuestU32Raw(rdram, entryAddr + 0x34u);
                    const TexturePackPayloadWindow &payload = payloadIt->second;
                    if (offset30 < payload.dataSize)
                    {
                        field8c = payload.dataAddr + offset30;
                        writeGuestU32Raw(rdram, entryAddr + 0x8Cu, field8c);
                    }
                    if (offset34 < payload.dataSize)
                    {
                        field90 = payload.dataAddr + offset34;
                        writeGuestU32Raw(rdram, entryAddr + 0x90u, field90);
                    }
                }
            }

            if (field8c == 0u)
            {
                continue;
            }

            const uint32_t field3c = readGuestU32Raw(rdram, entryAddr + 0x3Cu);
            if (field3c == 0u || field90 != 0u)
            {
                resultAddr = entryAddr;
                return true;
            }
        }

        resultAddr = 0u;
        return true;
    }

    void writeGuestU32Raw(uint8_t *rdram, uint32_t addr, uint32_t value)
    {
        uint8_t *dst = &rdram[addr & PS2_RAM_MASK];
        dst[0] = static_cast<uint8_t>((value >> 0) & 0xFFu);
        dst[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
        dst[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
        dst[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
    }

    void initializeGuestListSentinel(uint8_t *rdram, uint32_t sentinelAddr)
    {
        if (!rdram || sentinelAddr == 0u)
        {
            return;
        }

        writeGuestU32Raw(rdram, sentinelAddr + 0x0u, sentinelAddr);
        writeGuestU32Raw(rdram, sentinelAddr + 0x4u, sentinelAddr);
    }

    void insertGuestNodeAtListHead(uint8_t *rdram, uint32_t sentinelAddr, uint32_t nodeAddr)
    {
        if (!rdram || sentinelAddr == 0u || nodeAddr == 0u || sentinelAddr == nodeAddr)
        {
            return;
        }

        uint32_t currentHead = readGuestU32Raw(rdram, sentinelAddr + 0x0u);
        uint32_t currentTail = readGuestU32Raw(rdram, sentinelAddr + 0x4u);
        if (currentHead == 0u || currentTail == 0u)
        {
            initializeGuestListSentinel(rdram, sentinelAddr);
            currentHead = sentinelAddr;
            currentTail = sentinelAddr;
        }

        if (currentHead == nodeAddr || currentTail == nodeAddr)
        {
            return;
        }

        writeGuestU32Raw(rdram, nodeAddr + 0x0u, currentHead);
        writeGuestU32Raw(rdram, nodeAddr + 0x4u, sentinelAddr);
        writeGuestU32Raw(rdram, currentHead + 0x4u, nodeAddr);
        writeGuestU32Raw(rdram, sentinelAddr + 0x0u, nodeAddr);
        if (currentTail == sentinelAddr)
        {
            writeGuestU32Raw(rdram, sentinelAddr + 0x4u, nodeAddr);
        }
    }

    uint32_t readLeU32(const uint8_t *src)
    {
        return static_cast<uint32_t>(src[0]) |
               (static_cast<uint32_t>(src[1]) << 8) |
               (static_cast<uint32_t>(src[2]) << 16) |
               (static_cast<uint32_t>(src[3]) << 24);
    }

    void writeLeU32(uint8_t *dst, uint32_t value)
    {
        dst[0] = static_cast<uint8_t>((value >> 0) & 0xFFu);
        dst[1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
        dst[2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
        dst[3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
    }

    uint32_t nfsBinPathHash(std::string path)
    {
        std::replace(path.begin(), path.end(), '/', '\\');
        std::transform(path.begin(), path.end(), path.begin(),
                       [](unsigned char c)
                       { return static_cast<char>(std::toupper(c)); });

        uint32_t hash = 0xFFFFFFFFu;
        for (const unsigned char c : path)
        {
            hash = 33u * hash + static_cast<uint32_t>(c);
        }
        return hash;
    }

    std::vector<std::string> parseCsvRow(const std::string &line)
    {
        std::vector<std::string> fields;
        std::string current;
        bool inQuotes = false;
        for (size_t i = 0; i < line.size(); ++i)
        {
            const char ch = line[i];
            if (ch == '"')
            {
                if (inQuotes && (i + 1) < line.size() && line[i + 1] == '"')
                {
                    current.push_back('"');
                    ++i;
                }
                else
                {
                    inQuotes = !inQuotes;
                }
                continue;
            }

            if (ch == ',' && !inQuotes)
            {
                fields.push_back(current);
                current.clear();
                continue;
            }

            current.push_back(ch);
        }
        fields.push_back(current);
        return fields;
    }

    std::filesystem::path detectNfsHp2UnpackedRoot(const PS2Runtime::IoPaths &paths)
    {
        std::vector<std::filesystem::path> probes;

        auto appendProbeBase = [&](const std::filesystem::path &base)
        {
            if (base.empty())
            {
                return;
            }

            probes.push_back(base);
            probes.push_back(base / "ZZDATA");

            const std::filesystem::path parent = base.parent_path();
            if (!parent.empty())
            {
                probes.push_back(parent / (base.filename().string() + "_OUT_Unpacker") / "ZZDATA");
            }
        };

        appendProbeBase(paths.cdRoot);
        appendProbeBase(paths.hostRoot);
        appendProbeBase(paths.elfDirectory);
        if (!paths.cdImage.empty())
        {
            appendProbeBase(paths.cdImage.parent_path());
            appendProbeBase(paths.cdImage.parent_path() / paths.cdImage.stem());
        }

        std::error_code ec;
        for (const std::filesystem::path &probe : probes)
        {
            if (probe.empty())
            {
                continue;
            }

            if (std::filesystem::exists(probe, ec) && !ec &&
                std::filesystem::is_directory(probe, ec) && !ec &&
                probe.filename().string() == "ZZDATA")
            {
                return probe.lexically_normal();
            }
            ec.clear();
        }

        return {};
    }

    const std::unordered_map<uint32_t, std::filesystem::path> &getNfsHp2UnpackedHashIndex(
        const std::filesystem::path &root)
    {
        static std::mutex mutex;
        static std::filesystem::path cachedRoot;
        static std::unordered_map<uint32_t, std::filesystem::path> cachedIndex;

        std::lock_guard<std::mutex> lock(mutex);
        if (root.empty())
        {
            static const std::unordered_map<uint32_t, std::filesystem::path> empty;
            return empty;
        }

        if (cachedRoot == root && !cachedIndex.empty())
        {
            return cachedIndex;
        }

        cachedRoot = root;
        cachedIndex.clear();

        std::error_code ec;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied, ec))
        {
            if (ec)
            {
                break;
            }
            if (!entry.is_regular_file())
            {
                continue;
            }

            const std::filesystem::path relative = std::filesystem::relative(entry.path(), root, ec);
            if (ec || relative.empty())
            {
                ec.clear();
                continue;
            }

            const std::string firstComponent = (*relative.begin()).string();
            if (firstComponent == "__Unknown" || firstComponent == "__Unknown_Resolved")
            {
                continue;
            }

            cachedIndex.emplace(nfsBinPathHash(relative.generic_string()), entry.path().lexically_normal());
        }

        ec.clear();
        const std::filesystem::path inventoryPath = root / "__Unknown_Resolved" / "inventory.csv";
        std::ifstream inventory(inventoryPath);
        if (inventory.is_open())
        {
            std::string line;
            bool isFirstLine = true;
            while (std::getline(inventory, line))
            {
                if (isFirstLine)
                {
                    isFirstLine = false;
                    continue;
                }

                const std::vector<std::string> fields = parseCsvRow(line);
                if (fields.size() < 9 || fields[0].empty() || fields[8].empty())
                {
                    continue;
                }

                char *end = nullptr;
                const unsigned long parsed = std::strtoul(fields[0].c_str(), &end, 16);
                if (!end || *end != '\0')
                {
                    continue;
                }

                const std::filesystem::path hostPath(fields[8]);
                if (std::filesystem::exists(hostPath, ec) && !ec)
                {
                    cachedIndex.emplace(static_cast<uint32_t>(parsed), hostPath.lexically_normal());
                }
                ec.clear();
            }
        }

        return cachedIndex;
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

    void nfsHp2AlphaUpdateCameraCoordinatorsContinuation(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto readGuestU64 = [&](uint32_t addr) -> uint64_t
        {
            const uint32_t lo = readLeU32(rdram + (addr & PS2_RAM_MASK));
            const uint32_t hi = readLeU32(rdram + (ADD32(addr, 4u) & PS2_RAM_MASK));
            return static_cast<uint64_t>(lo) | (static_cast<uint64_t>(hi) << 32);
        };

        switch (ctx->pc)
        {
        case 0x170B50u:
            SET_GPR_S32(ctx, 16, static_cast<int32_t>(readGuestU32(ADD32(GPR_U32(ctx, 16), 0u))));
            [[fallthrough]];

        case 0x170B54u:
            if (GPR_U64(ctx, 16) != GPR_U64(ctx, 17))
            {
                SET_GPR_S32(ctx, 2, static_cast<int32_t>(readGuestU32(ADD32(GPR_U32(ctx, 16), 0xCu))));
                ctx->pc = 0x170B30u;
                return;
            }
            [[fallthrough]];

        case 0x170B5Cu:
        {
            const uint32_t savedSp = GPR_U32(ctx, 29);
            const uint64_t savedRa = readGuestU64(ADD32(savedSp, 0x20u));
            const uint64_t savedS1 = readGuestU64(ADD32(savedSp, 0x10u));
            const uint64_t savedS0 = readGuestU64(ADD32(savedSp, 0x00u));
            const uint32_t savedF20Bits = readGuestU32(ADD32(savedSp, 0x30u));
            float savedF20 = 0.0f;
            std::memcpy(&savedF20, &savedF20Bits, sizeof(savedF20));

            SET_GPR_U64(ctx, 31, savedRa);
            SET_GPR_U64(ctx, 17, savedS1);
            SET_GPR_U64(ctx, 16, savedS0);
            ctx->f[20] = savedF20;
            SET_GPR_S32(ctx, 29, static_cast<int32_t>(ADD32(savedSp, 0x40u)));
            ctx->pc = GPR_U32(ctx, 31);
            return;
        }

        default:
            std::cerr << "[nfs-hp2-alpha] unexpected UpdateCameraCoordinators continuation pc=0x"
                      << std::hex << ctx->pc
                      << " ra=0x" << getRegU32(ctx, 31)
                      << " sp=0x" << getRegU32(ctx, 29)
                      << std::dec << std::endl;
            runtime->requestStop();
            return;
        }
    }

    void nfsHp2AlphaUpdateCameraCoordinators(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const float deltaSeconds = ctx->f[12];
        const uint32_t sentinel = getRegU32(ctx, 28) - 0x5A70u;
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto readGuestS16 = [&](uint32_t addr) -> int32_t
        {
            const uint32_t masked = addr & PS2_RAM_MASK;
            const uint16_t value = static_cast<uint16_t>(rdram[masked + 0]) |
                                   (static_cast<uint16_t>(rdram[masked + 1]) << 8);
            return static_cast<int16_t>(value);
        };

        std::unordered_set<uint32_t> seen;
        seen.reserve(32u);

        uint32_t node = readGuestU32(sentinel);
        uint32_t iterations = 0u;
        while (node != sentinel && node != 0u && iterations < 256u)
        {
            if (!seen.insert(node).second)
            {
                std::cerr << "[nfs-hp2-alpha] camera coordinator cycle detected"
                          << " sentinel=0x" << std::hex << sentinel
                          << " node=0x" << node
                          << " next=0x" << readGuestU32(node + 0u)
                          << std::dec << std::endl;
                break;
            }

            if (readGuestU32(node + 0x0Cu) == 0u)
            {
                const uint32_t descriptor = readGuestU32(node + 0x18u);
                if (descriptor != 0u)
                {
                    const int32_t offset = readGuestS16(descriptor + 0x10u);
                    const uint32_t callback = readGuestU32(descriptor + 0x14u);
                    if (callback >= 0x1000u && runtime->hasFunction(callback))
                    {
                        ctx->f[12] = deltaSeconds;
                        SET_GPR_U64(ctx, 4, ADD32(node, static_cast<uint32_t>(offset)));
                        ctx->pc = callback;
                        auto targetFn = runtime->lookupFunction(callback);
                        targetFn(rdram, ctx, runtime);
                    }
                    else
                    {
                        static uint32_t loggedInvalidCallbackCount = 0u;
                        if (loggedInvalidCallbackCount < 16u)
                        {
                            std::cerr << "[nfs-hp2-alpha] skipped invalid camera callback"
                                      << " node=0x" << std::hex << node
                                      << " descriptor=0x" << descriptor
                                      << " callback=0x" << callback
                                      << " offset=0x" << static_cast<uint32_t>(offset & 0xFFFF)
                                      << std::dec << std::endl;
                            ++loggedInvalidCallbackCount;
                        }
                    }
                }
            }

            node = readGuestU32(node + 0u);
            ++iterations;
        }

        ctx->f[12] = deltaSeconds;
        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaRenderCameraMovers(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t wantedView = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = ADD32(gp, 4294942552u);
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readGuestU32Raw(rdram, addr);
        };
        const auto readGuestS16 = [&](uint32_t addr) -> int32_t
        {
            const uint32_t value = readGuestU32Raw(rdram, addr & ~0x1u);
            return static_cast<int16_t>((addr & 0x2u) ? (value >> 16) : (value & 0xFFFFu));
        };

        std::unordered_set<uint32_t> seen;
        seen.reserve(32u);

        uint32_t node = readGuestU32(sentinel);
        uint32_t iterations = 0u;
        while (node != sentinel && node != 0u && iterations < 256u)
        {
            if (!seen.insert(node).second)
            {
                std::cerr << "[nfs-hp2-alpha] camera mover render cycle detected"
                          << " sentinel=0x" << std::hex << sentinel
                          << " node=0x" << node
                          << " next=0x" << readGuestU32(node + 0u)
                          << std::dec << std::endl;
                break;
            }

            const uint32_t viewIndex = readGuestU32(node + 0x0Cu);
            const uint32_t viewAddr = 0x00260B30u + (viewIndex * 0x60u);
            if (viewAddr == wantedView)
            {
                const uint32_t descriptor = readGuestU32(node + 0x28u);
                if (descriptor != 0u)
                {
                    const int32_t offset = readGuestS16(descriptor + 0x18u);
                    const uint32_t callback = readGuestU32(descriptor + 0x1Cu);
                    if (callback >= 0x1000u && runtime->hasFunction(callback))
                    {
                        SET_GPR_U64(ctx, 4, ADD32(node, static_cast<uint32_t>(offset)));
                        ctx->pc = callback;
                        runtime->lookupFunction(callback)(rdram, ctx, runtime);
                    }
                }
            }

            node = readGuestU32(node + 0u);
            ++iterations;
        }

        SET_GPR_U32(ctx, 4, wantedView);
        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaUpdateCameraMovers(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = ADD32(gp, 4294942552u);
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readGuestU32Raw(rdram, addr);
        };
        const auto readGuestS16 = [&](uint32_t addr) -> int32_t
        {
            const uint32_t value = readGuestU32Raw(rdram, addr & ~0x1u);
            return static_cast<int16_t>((addr & 0x2u) ? (value >> 16) : (value & 0xFFFFu));
        };

        std::unordered_set<uint32_t> seen;
        seen.reserve(32u);

        uint32_t node = readGuestU32(sentinel);
        uint32_t iterations = 0u;
        while (node != sentinel && node != 0u && iterations < 256u)
        {
            if (!seen.insert(node).second)
            {
                std::cerr << "[nfs-hp2-alpha] camera mover update cycle detected"
                          << " sentinel=0x" << std::hex << sentinel
                          << " node=0x" << node
                          << " next=0x" << readGuestU32(node + 0u)
                          << std::dec << std::endl;
                break;
            }

            const uint32_t viewIndex = readGuestU32(node + 0x0Cu);
            const uint32_t viewAddr = 0x00260B30u + (viewIndex * 0x60u);
            if (readGuestU32(viewAddr + 0x38u) == node)
            {
                const uint32_t descriptor = readGuestU32(node + 0x28u);
                if (descriptor != 0u)
                {
                    const int32_t offset = readGuestS16(descriptor + 0x10u);
                    const uint32_t callback = readGuestU32(descriptor + 0x14u);
                    if (callback >= 0x1000u && runtime->hasFunction(callback))
                    {
                        SET_GPR_U64(ctx, 4, ADD32(node, static_cast<uint32_t>(offset)));
                        ctx->pc = callback;
                        runtime->lookupFunction(callback)(rdram, ctx, runtime);
                    }
                }
            }

            node = readGuestU32(node + 0u);
            ++iterations;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaGetLightMaterial(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t wantedId = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = ADD32(gp, 4294940704u);
        constexpr uint32_t kWildcardId = 0x2C420D64u;

        std::unordered_set<uint32_t> seen;
        seen.reserve(64u);

        uint32_t fallback = 0u;
        uint32_t node = readGuestU32Raw(rdram, sentinel);
        uint32_t iterations = 0u;
        while (node != 0u && node != sentinel && iterations < 2048u)
        {
            if (!seen.insert(node).second)
            {
                static uint32_t s_cycleLogCount = 0u;
                if (s_cycleLogCount < 12u)
                {
                    std::cerr << "[nfs-hp2-alpha] light material list cycle detected"
                              << " sentinel=0x" << std::hex << sentinel
                              << " wanted=0x" << wantedId
                              << " node=0x" << node
                              << " next=0x" << readGuestU32Raw(rdram, node + 0x0u)
                              << " id=0x" << readGuestU32Raw(rdram, node + 0x8u)
                              << std::dec << std::endl;
                    ++s_cycleLogCount;
                }
                break;
            }

            const uint32_t materialId = readGuestU32Raw(rdram, node + 0x8u);
            if (materialId == wantedId)
            {
                setReturnU32(ctx, node);
                ctx->pc = getRegU32(ctx, 31);
                return;
            }

            if (materialId == kWildcardId)
            {
                fallback = node;
            }

            node = readGuestU32Raw(rdram, node + 0x0u);
            ++iterations;
        }

        if (iterations >= 2048u)
        {
            static uint32_t s_runawayLogCount = 0u;
            if (s_runawayLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] light material list runaway"
                          << " sentinel=0x" << std::hex << sentinel
                          << " wanted=0x" << wantedId
                          << " lastNode=0x" << node
                          << std::dec << std::endl;
                ++s_runawayLogCount;
            }
        }

        setReturnU32(ctx, fallback);
        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaSortFlailerObjectList(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t sentinel = getRegU32(ctx, 4);
        if (sentinel == 0u)
        {
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        std::vector<uint32_t> nodes;
        nodes.reserve(128u);
        std::unordered_set<uint32_t> seen;
        seen.reserve(128u);

        uint32_t node = readGuestU32Raw(rdram, sentinel + 0x0u);
        bool cycleDetected = false;
        while (node != 0u && node != sentinel && nodes.size() < 4096u)
        {
            if (!seen.insert(node).second)
            {
                cycleDetected = true;
                break;
            }

            nodes.push_back(node);
            node = readGuestU32Raw(rdram, node + 0x0u);
        }

        if (node != 0u && node != sentinel && !cycleDetected && nodes.size() >= 4096u)
        {
            static uint32_t s_runawayLogCount = 0u;
            if (s_runawayLogCount < 8u)
            {
                std::cerr << "[nfs-hp2-alpha] flailer sort runaway"
                          << " sentinel=0x" << std::hex << sentinel
                          << " count=0x" << nodes.size()
                          << " last=0x" << node
                          << std::dec << std::endl;
                ++s_runawayLogCount;
            }
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        if (cycleDetected)
        {
            static uint32_t s_cycleLogCount = 0u;
            if (s_cycleLogCount < 8u)
            {
                std::cerr << "[nfs-hp2-alpha] flailer sort cycle detected"
                          << " sentinel=0x" << std::hex << sentinel
                          << " repeated=0x" << node
                          << " count=0x" << nodes.size()
                          << std::dec << std::endl;
                ++s_cycleLogCount;
            }
        }

        std::stable_sort(nodes.begin(), nodes.end(), [&](uint32_t lhs, uint32_t rhs) {
            const int32_t lhsOrder = static_cast<int32_t>(static_cast<int8_t>(readGuestU8Raw(rdram, lhs + 0x12u)));
            const int32_t rhsOrder = static_cast<int32_t>(static_cast<int8_t>(readGuestU8Raw(rdram, rhs + 0x12u)));
            return lhsOrder <= rhsOrder;
        });

        if (nodes.empty())
        {
            writeGuestU32Raw(rdram, sentinel + 0x0u, sentinel);
            writeGuestU32Raw(rdram, sentinel + 0x4u, sentinel);
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const uint32_t current = nodes[i];
            const uint32_t next = (i + 1u < nodes.size()) ? nodes[i + 1u] : sentinel;
            const uint32_t prev = (i == 0u) ? sentinel : nodes[i - 1u];
            writeGuestU32Raw(rdram, current + 0x0u, next);
            writeGuestU32Raw(rdram, current + 0x4u, prev);
        }

        writeGuestU32Raw(rdram, sentinel + 0x0u, nodes.front());
        writeGuestU32Raw(rdram, sentinel + 0x4u, nodes.back());

        static uint32_t s_logCount = 0u;
        if (s_logCount < 8u)
        {
            const uint32_t first = nodes.front();
            const uint32_t last = nodes.back();
            std::cerr << "[nfs-hp2-alpha] sorted flailer list"
                      << " sentinel=0x" << std::hex << sentinel
                      << " count=0x" << nodes.size()
                      << " first=0x" << first
                      << " firstOrder=0x" << static_cast<uint32_t>(readGuestU8Raw(rdram, first + 0x12u))
                      << " last=0x" << last
                      << " lastOrder=0x" << static_cast<uint32_t>(readGuestU8Raw(rdram, last + 0x12u))
                      << std::dec << std::endl;
            ++s_logCount;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaSortBList(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t compareFn = getRegU32(ctx, 5);
        if (compareFn == 0x134188u)
        {
            static uint32_t s_logCount = 0u;
            if (s_logCount < 8u)
            {
                std::cerr << "[nfs-hp2-alpha] intercepted generic bList sort for flailer objects"
                          << " sentinel=0x" << std::hex << getRegU32(ctx, 4)
                          << " compare=0x" << compareFn
                          << " ra=0x" << getRegU32(ctx, 31)
                          << std::dec << std::endl;
                ++s_logCount;
            }

            nfsHp2AlphaSortFlailerObjectList(rdram, ctx, runtime);
            if (ctx->pc == entryPc)
            {
                ctx->pc = getRegU32(ctx, 31);
            }
            return;
        }

        Sort__5bListPFP5bNodeP5bNode_i_0x1f5680(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = getRegU32(ctx, 31);
        }
    }

    void nfsHp2AlphaAddTexturesSorted(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t downloaderAddr = getRegU32(ctx, 4);
        const uint32_t sourceListAddr = getRegU32(ctx, 5);
        const uint32_t destListAddr = ADD32(downloaderAddr, 0x14u);
        const bool downloaderScratch = ps2IsScratchpadAddress(downloaderAddr);
        const bool destScratch = ps2IsScratchpadAddress(destListAddr);
        const bool sourceScratch = ps2IsScratchpadAddress(sourceListAddr);
        const uint32_t sourceHead = (sourceListAddr < kPs2RdramSize) ? readGuestU32Raw(rdram, sourceListAddr + 0x0u) : 0u;
        const uint32_t sourceTail = (sourceListAddr < kPs2RdramSize) ? readGuestU32Raw(rdram, sourceListAddr + 0x4u) : 0u;
        const uint32_t destHead = (destListAddr < kPs2RdramSize) ? readGuestU32Raw(rdram, destListAddr + 0x0u) : 0u;
        const uint32_t destTail = (destListAddr < kPs2RdramSize) ? readGuestU32Raw(rdram, destListAddr + 0x4u) : 0u;

        static uint32_t s_logCount = 0u;
        if (s_logCount < 8u)
        {
            std::cerr << "[nfs-hp2-alpha] AddTexturesSorted intercept"
                      << " downloader=0x" << std::hex << downloaderAddr
                      << " source=0x" << sourceListAddr
                      << " sourceHead=0x" << sourceHead
                      << " sourceTail=0x" << sourceTail
                      << " dest=0x" << destListAddr
                      << " destHead=0x" << destHead
                      << " destTail=0x" << destTail
                      << " downloaderScratch=" << (downloaderScratch ? 1 : 0)
                      << " sourceScratch=" << (sourceScratch ? 1 : 0)
                      << " destScratch=" << (destScratch ? 1 : 0)
                      << std::dec << std::endl;
            ++s_logCount;
        }

        const bool sourceLooksEmpty = (sourceHead == 0u && sourceTail == 0u);
        const bool invalidListAddr = (sourceListAddr == 0u || sourceListAddr >= kPs2RdramSize ||
                                      destListAddr == 0u || destListAddr >= kPs2RdramSize);
        if (downloaderScratch || sourceScratch || destScratch || sourceLooksEmpty || invalidListAddr)
        {
            static uint32_t s_skipLogCount = 0u;
            if (s_skipLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] AddTexturesSorted skip fallback"
                          << " downloader=0x" << std::hex << downloaderAddr
                          << " source=0x" << sourceListAddr
                          << " dest=0x" << destListAddr
                          << " reason="
                          << (downloaderScratch ? "downloader-scratch " : "")
                          << (sourceScratch ? "source-scratch " : "")
                          << (destScratch ? "dest-scratch " : "")
                          << (sourceLooksEmpty ? "source-empty " : "")
                          << (invalidListAddr ? "invalid-list-addr" : "")
                          << std::dec << std::endl;
                ++s_skipLogCount;
            }

            if (ctx->pc == entryPc)
            {
                ctx->pc = getRegU32(ctx, 31);
            }
            return;
        }

        SET_GPR_U32(ctx, 4, destListAddr);
        SET_GPR_U32(ctx, 5, sourceListAddr);
        AddTail__t6bTList1Z14eActiveTextureP5bList_0x111738(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = getRegU32(ctx, 31);
        }
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

        const std::filesystem::path unpackedRoot = detectNfsHp2UnpackedRoot(paths);
        const auto &unpackedHashIndex = getNfsHp2UnpackedHashIndex(unpackedRoot);
        uint32_t remappedEntries = 0;
        for (size_t offset = 0; offset < bytes.size(); offset += 12u)
        {
            const uint32_t nameHash = readLeU32(bytes.data() + offset + 0u);
            const auto found = unpackedHashIndex.find(nameHash);
            if (found == unpackedHashIndex.end())
            {
                continue;
            }

            char syntheticKey[32];
            std::snprintf(syntheticKey, sizeof(syntheticKey), "__hash\\%08X", nameHash);

            uint32_t pseudoLbn = 0;
            uint32_t sizeBytes = 0;
            if (!ps2_stubs::registerCdHostFileAlias(syntheticKey, found->second, &pseudoLbn, &sizeBytes))
            {
                continue;
            }

            writeLeU32(bytes.data() + offset + 4u, pseudoLbn);
            writeLeU32(bytes.data() + offset + 8u, sizeBytes);
            ++remappedEntries;
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
                      << " remapped=" << remappedEntries
                      << " unpackedRoot=\"" << unpackedRoot.string() << "\""
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

    void nfsHp2AlphaFindEventNode(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx)
        {
            if (ctx)
            {
                setReturnU32(ctx, 0u);
                ctx->pc = getRegU32(ctx, 31);
            }
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t sentinel = getRegU32(ctx, 4);
        const uint32_t wantedEvent = getRegU32(ctx, 5);
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto writeGuestU32 = [&](uint32_t addr, uint32_t value)
        {
            writeLeU32(rdram + (addr & PS2_RAM_MASK), value);
        };

        uint32_t node = readGuestU32(sentinel + 0u);
        
        if (node == sentinel || node == 0u)
        {
            static uint32_t s_syntheticNodeAddr = 0u;
            if (s_syntheticNodeAddr == 0u)
            {
                s_syntheticNodeAddr = 0x2a2000u;
                writeGuestU32(s_syntheticNodeAddr + 0u, s_syntheticNodeAddr);
                writeGuestU32(s_syntheticNodeAddr + 4u, s_syntheticNodeAddr);
                writeGuestU32(s_syntheticNodeAddr + 8u, wantedEvent);
                writeGuestU32(s_syntheticNodeAddr + 0x10u, 0u);
                writeGuestU32(s_syntheticNodeAddr + 0x14u, 0u);
                std::cerr << "[nfs-hp2-alpha] created synthetic keyboard event node addr=0x" << std::hex << s_syntheticNodeAddr << " event=" << wantedEvent << std::dec << std::endl;
            }
            setReturnU32(ctx, s_syntheticNodeAddr);
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        std::unordered_set<uint32_t> seen;
        seen.reserve(64u);
        uint32_t iterations = 0u;
        while (node != sentinel)
        {
            if (!seen.insert(node).second)
            {
                std::cerr << "[nfs-hp2-alpha] joystick event list cycle detected"
                          << " sentinel=0x" << std::hex << sentinel
                          << " node=0x" << node
                          << " event=0x" << wantedEvent
                          << " next=0x" << readGuestU32(node + 0u)
                          << " prev=0x" << readGuestU32(node + 4u)
                          << " nodeEvent=0x" << readGuestU32(node + 8u)
                          << std::dec << std::endl;
                setReturnU32(ctx, 0u);
                ctx->pc = getRegU32(ctx, 31);
                return;
            }

            const uint32_t nodeEvent = readGuestU32(node + 8u);
            if (nodeEvent == wantedEvent)
            {
                setReturnU32(ctx, node);
                ctx->pc = getRegU32(ctx, 31);
                return;
            }

            node = readGuestU32(node + 0u);
            ++iterations;
            if (iterations >= 256u)
            {
                std::cerr << "[nfs-hp2-alpha] joystick event list runaway"
                          << " sentinel=0x" << std::hex << sentinel
                          << " event=0x" << wantedEvent
                          << " lastNode=0x" << node
                          << std::dec
                          << " iterations=" << iterations << std::endl;
                setReturnU32(ctx, 0u);
                ctx->pc = getRegU32(ctx, 31);
                return;
            }
        }

        setReturnU32(ctx, 0u);
        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaFindScannerConfig(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx)
        {
            if (ctx)
            {
                setReturnU32(ctx, 0u);
                ctx->pc = getRegU32(ctx, 31);
            }
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t wantedEvent = getRegU32(ctx, 4);
        const uint32_t wantedPort = getRegU32(ctx, 5);
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto readGuestS8 = [&](uint32_t addr) -> int32_t
        {
            return static_cast<int8_t>(rdram[addr & PS2_RAM_MASK]);
        };

        constexpr uint32_t kScannerTableBase = 0x28F180u;
        constexpr uint32_t kScannerStride = 0x24u;
        constexpr uint32_t kScannerSlotCount = 8u;
        constexpr int32_t kScannerSentinel = -1;
        constexpr uint32_t kExpectedMaxScannerConfigs = 64u;

        uint32_t scannerCount = readGuestU32(getRegU32(ctx, 28) - 0x6660u);
        if (scannerCount > kExpectedMaxScannerConfigs)
        {
            static bool loggedBadCount = false;
            if (!loggedBadCount)
            {
                std::cerr << "[nfs-hp2-alpha] clamped suspicious scanner config count"
                          << " count=0x" << std::hex << scannerCount
                          << " gp=0x" << getRegU32(ctx, 28)
                          << " wantedEvent=0x" << wantedEvent
                          << " wantedPort=0x" << wantedPort
                          << std::dec << std::endl;
                loggedBadCount = true;
            }
            scannerCount = kExpectedMaxScannerConfigs;
        }

        for (uint32_t i = 0; i < scannerCount; ++i)
        {
            const uint32_t row = kScannerTableBase + (i * kScannerStride);
            if (readGuestU32(row + 8u) != wantedEvent)
            {
                continue;
            }

            bool match = false;
            for (uint32_t slot = 0; slot < kScannerSlotCount; ++slot)
            {
                const int32_t value = readGuestS8(row + slot);
                if (wantedPort != 0u)
                {
                    if (value == static_cast<int32_t>(wantedPort) || value == kScannerSentinel)
                    {
                        match = true;
                        break;
                    }
                }
                else
                {
                    if ((wantedEvent == 1u && value == 1) || value == kScannerSentinel || value == 0)
                    {
                        match = true;
                        break;
                    }
                }
            }

            if (match)
            {
                setReturnU32(ctx, row);
                ctx->pc = getRegU32(ctx, 31);
                return;
            }
        }

        static uint32_t missLogCount = 0u;
        if (missLogCount < 8u)
        {
            std::cerr << "[nfs-hp2-alpha] FindScannerConfig miss"
                      << " wantedEvent=0x" << std::hex << wantedEvent
                      << " wantedPort=0x" << wantedPort
                      << " count=0x" << scannerCount
                      << std::dec << std::endl;
            ++missLogCount;
        }

        setReturnU32(ctx, 0u);
        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaGenerateEventsFromScanners(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t joystickAddr = getRegU32(ctx, 4);
        uint32_t requestedConfig = getRegU32(ctx, 5);
        if (requestedConfig == 0xFFFFFFFFu)
        {
            requestedConfig = 0u;
        }
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto writeGuestU32 = [&](uint32_t addr, uint32_t value)
        {
            writeLeU32(rdram + (addr & PS2_RAM_MASK), value);
        };

        if (joystickAddr != 0u)
        {
            const uint32_t currentConfig = readGuestU32(joystickAddr + 0x1Cu);
            const uint32_t pendingConfig = readGuestU32(joystickAddr + 0x20u);
            uint32_t pendingTicks = readGuestU32(joystickAddr + 0x24u);

            if (requestedConfig != currentConfig)
            {
                if (requestedConfig == pendingConfig)
                {
                    ++pendingTicks;
                    if (pendingTicks >= 10u)
                    {
                        writeGuestU32(joystickAddr + 0x1Cu, requestedConfig);
                        writeGuestU32(joystickAddr + 0x24u, 0u);
                    }
                    else
                    {
                        writeGuestU32(joystickAddr + 0x24u, pendingTicks);
                    }
                }
                else
                {
                    writeGuestU32(joystickAddr + 0x20u, requestedConfig);
                    writeGuestU32(joystickAddr + 0x24u, 0u);
                }
            }
        }

        static bool logged = false;
        if (!logged)
        {
            std::cerr << "[nfs-hp2-alpha] bypassed GenerateEventsFromScanners"
                      << " joystick=0x" << std::hex << joystickAddr
                      << " requestedConfig=0x" << requestedConfig
                      << std::dec << std::endl;
            logged = true;
        }

        ctx->pc = getRegU32(ctx, 31);
    }

    void nfsHp2AlphaBeginReadQueuedFile(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const uint32_t queuedFile = getRegU32(ctx, 4);
        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto writeGuestU32 = [&](uint32_t addr, uint32_t value)
        {
            writeLeU32(rdram + (addr & PS2_RAM_MASK), value);
        };
        const auto readGuestCString = [&](uint32_t addr, size_t maxLen) -> std::string
        {
            std::string out;
            out.reserve(maxLen);
            for (size_t i = 0; i < maxLen; ++i)
            {
                const char ch = static_cast<char>(rdram[(addr + static_cast<uint32_t>(i)) & PS2_RAM_MASK]);
                if (ch == '\0')
                {
                    break;
                }
                out.push_back(ch);
            }
            return out;
        };
        const auto normalizeAsyncEntryQueue = [&]()
        {
            const uint32_t sentinel = ADD32(getRegU32(ctx, 28), 4294948200u);
            uint32_t head = readGuestU32(sentinel + 0x0u);
            uint32_t tail = readGuestU32(sentinel + 0x4u);
            bool repaired = false;

            const bool headLooksBroken = (head == 0u);
            const bool tailLooksBroken = (tail == 0u);

            if (headLooksBroken && tailLooksBroken)
            {
                writeGuestU32(sentinel + 0x0u, sentinel);
                writeGuestU32(sentinel + 0x4u, sentinel);
                repaired = true;
            }
            else
            {
                if (tailLooksBroken)
                {
                    tail = sentinel;
                    writeGuestU32(sentinel + 0x4u, tail);
                    repaired = true;
                }

                if (headLooksBroken)
                {
                    const uint32_t recoveredHead = (tail != sentinel) ? tail : sentinel;
                    writeGuestU32(sentinel + 0x0u, recoveredHead);
                    repaired = true;
                }
            }

            if (repaired)
            {
                std::cerr << "[nfs-hp2-alpha] repaired async-entry queue before BeginRead"
                          << " sentinel=0x" << std::hex << sentinel
                          << " head=0x" << readGuestU32(sentinel + 0x0u)
                          << " tail=0x" << readGuestU32(sentinel + 0x4u)
                          << std::dec << std::endl;
            }
        };

        normalizeAsyncEntryQueue();

        const std::string path = readGuestCString(queuedFile + 0x0Cu, 0x30u);
        const bool watchPath = equalsIgnoreCaseAscii(path, "GLOBAL\\DYNTEX.BIN");

        if (watchPath)
        {
            std::cerr << "[nfs-hp2-alpha] BeginRead queuedFile=0x" << std::hex << queuedFile
                      << " path=" << path
                      << " target=0x" << readGuestU32(queuedFile + 0x08u)
                      << " start=0x" << readGuestU32(queuedFile + 0x44u)
                      << " limit=0x" << readGuestU32(queuedFile + 0x40u)
                      << " reqSize=0x" << readGuestU32(queuedFile + 0x48u)
                      << " cb=0x" << readGuestU32(queuedFile + 0x50u)
                      << " cbUser=0x" << readGuestU32(queuedFile + 0x54u)
                      << " state5c=0x" << readGuestU32(queuedFile + 0x5Cu)
                      << " state60=0x" << readGuestU32(queuedFile + 0x60u)
                      << std::dec << std::endl;
        }

        const uint32_t entryPc = ctx->pc;
        BeginRead__10QueuedFile_0x181d20(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = getRegU32(ctx, 31);
        }

        if (watchPath)
        {
            std::cerr << "[nfs-hp2-alpha] BeginRead:return queuedFile=0x" << std::hex << queuedFile
                      << " state5c=0x" << readGuestU32(queuedFile + 0x5Cu)
                      << " state60=0x" << readGuestU32(queuedFile + 0x60u)
                      << " nextOff=0x" << readGuestU32(queuedFile + 0x44u)
                      << " pc=0x" << ctx->pc
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
        }
    }

    void nfsHp2AlphaServiceFileSystem(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto writeGuestU32 = [&](uint32_t addr, uint32_t value)
        {
            writeLeU32(rdram + (addr & PS2_RAM_MASK), value);
        };
        const uint32_t queueSentinelAddr = ADD32(getRegU32(ctx, 28), 4294948200u);
        const uint32_t sentinel = queueSentinelAddr;
        const auto normalizeAsyncEntryQueue = [&]()
        {
            uint32_t head = readGuestU32(queueSentinelAddr + 0x0u);
            uint32_t tail = readGuestU32(queueSentinelAddr + 0x4u);
            bool repaired = false;

            if (head == 0u && tail == 0u)
            {
                writeGuestU32(queueSentinelAddr + 0x0u, sentinel);
                writeGuestU32(queueSentinelAddr + 0x4u, sentinel);
                repaired = true;
            }
            else
            {
                if (tail == 0u)
                {
                    tail = sentinel;
                    writeGuestU32(queueSentinelAddr + 0x4u, tail);
                    repaired = true;
                }

                if (head == 0u)
                {
                    const uint32_t recoveredHead = (tail != sentinel) ? tail : sentinel;
                    writeGuestU32(queueSentinelAddr + 0x0u, recoveredHead);
                    repaired = true;
                }
            }

            if (repaired)
            {
                std::cerr << "[nfs-hp2-alpha] repaired async-entry queue before bServiceFileSystem"
                          << " sentinel=0x" << std::hex << sentinel
                          << " head=0x" << readGuestU32(queueSentinelAddr + 0x0u)
                          << " tail=0x" << readGuestU32(queueSentinelAddr + 0x4u)
                          << std::dec << std::endl;
            }
        };

        normalizeAsyncEntryQueue();

        auto logEntry = [&](const char *tag)
        {
            const uint32_t sentinelNext = readGuestU32(queueSentinelAddr + 0x0u);
            const uint32_t sentinelPrev = readGuestU32(queueSentinelAddr + 0x4u);
            uint32_t head = sentinelNext;

            if ((head == 0u || head == sentinel) && sentinelPrev != 0u && sentinelPrev != sentinel)
            {
                // When the head link is corrupted or still zero, the tail link often still points at
                // the only live entry. Logging that gives us the concrete async record instead of a
                // meaningless dump from address zero.
                head = sentinelPrev;
            }

            const bool queueLooksEmpty = (sentinelNext == sentinel && sentinelPrev == sentinel);
            const bool headLooksValid = (head != 0u && head != sentinel);

            const uint32_t fileAddr = headLooksValid ? readGuestU32(head + 0x08u) : 0u;
            const uint32_t state = headLooksValid ? readGuestU32(head + 0x0Cu) : 0u;
            const uint32_t arg14 = headLooksValid ? readGuestU32(head + 0x14u) : 0u;
            const uint32_t arg18 = headLooksValid ? readGuestU32(head + 0x18u) : 0u;
            const uint32_t arg1c = headLooksValid ? readGuestU32(head + 0x1Cu) : 0u;
            const uint32_t arg20 = headLooksValid ? readGuestU32(head + 0x20u) : 0u;
            const uint32_t proc = headLooksValid ? readGuestU32(head + 0x34u) : 0u;
            const uint32_t pending = (fileAddr != 0u) ? readGuestU32(fileAddr + 0x74u) : 0u;
            const uint32_t readProc = (fileAddr != 0u) ? readGuestU32(fileAddr + 0x7Cu) : 0u;
            const uint32_t closeProc = (fileAddr != 0u) ? readGuestU32(fileAddr + 0x80u) : 0u;
            const uint32_t lbn = (fileAddr != 0u) ? readGuestU32(fileAddr + 0x84u) : 0u;
            const uint32_t off70 = (fileAddr != 0u) ? readGuestU32(fileAddr + 0x70u) : 0u;

            static uint32_t serviceLogCount = 0u;
            if (serviceLogCount < 128u)
            {
                std::cerr << "[nfs-hp2-alpha] bServiceFileSystem:" << tag
                          << " gp=0x" << std::hex << getRegU32(ctx, 28)
                          << " sentinel=0x" << sentinel
                          << " sentinelNext=0x" << sentinelNext
                          << " sentinelPrev=0x" << sentinelPrev
                          << " head=0x" << head
                          << " empty=" << (queueLooksEmpty ? 1 : 0)
                          << " valid=" << (headLooksValid ? 1 : 0)
                          << " file=0x" << fileAddr
                          << " proc=0x" << proc
                          << " state=0x" << state
                          << " arg14=0x" << arg14
                          << " arg18=0x" << arg18
                          << " arg1c=0x" << arg1c
                          << " arg20=0x" << arg20
                          << " pending74=0x" << pending
                          << " off70=0x" << off70
                          << " read7c=0x" << readProc
                          << " close80=0x" << closeProc
                          << " base84=0x" << lbn
                          << " next=0x" << (headLooksValid ? readGuestU32(head + 0x00u) : 0u)
                          << " prev=0x" << (headLooksValid ? readGuestU32(head + 0x04u) : 0u)
                          << std::dec << std::endl;
                ++serviceLogCount;
            }
        };

        logEntry("before");
        const uint32_t entryPc = ctx->pc;
        bServiceFileSystem__Fv_0x1f8ae8(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = getRegU32(ctx, 31);
        }
        logEntry("after");
    }

    void nfsHp2AlphaServiceQueuedFiles(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx || !runtime)
        {
            if (runtime)
            {
                runtime->requestStop();
            }
            return;
        }

        const auto readGuestU32 = [&](uint32_t addr) -> uint32_t
        {
            return readLeU32(rdram + (addr & PS2_RAM_MASK));
        };
        const auto writeGuestU32 = [&](uint32_t addr, uint32_t value)
        {
            writeLeU32(rdram + (addr & PS2_RAM_MASK), value);
        };

        const uint32_t sentinel = ADD32(getRegU32(ctx, 28), 4294944800u);
        uint32_t head = readGuestU32(sentinel + 0x0u);
        uint32_t tail = readGuestU32(sentinel + 0x4u);
        bool repaired = false;

        if (head == 0u && tail == 0u)
        {
            writeGuestU32(sentinel + 0x0u, sentinel);
            writeGuestU32(sentinel + 0x4u, sentinel);
            repaired = true;
        }
        else
        {
            if (tail == 0u)
            {
                tail = sentinel;
                writeGuestU32(sentinel + 0x4u, tail);
                repaired = true;
            }

            if (head == 0u)
            {
                const uint32_t recoveredHead = (tail != sentinel) ? tail : sentinel;
                writeGuestU32(sentinel + 0x0u, recoveredHead);
                repaired = true;
            }
        }

        if (repaired)
        {
            std::cerr << "[nfs-hp2-alpha] repaired queued-file queue before ServiceQueuedFiles"
                      << " sentinel=0x" << std::hex << sentinel
                      << " head=0x" << readGuestU32(sentinel + 0x0u)
                      << " tail=0x" << readGuestU32(sentinel + 0x4u)
                      << std::dec << std::endl;
        }

        const uint32_t currentHead = readGuestU32(sentinel + 0x0u);
        if (currentHead != 0u && currentHead != sentinel)
        {
            std::cerr << "[nfs-hp2-alpha] ServiceQueuedFiles head=0x" << std::hex << currentHead
                      << " next=0x" << readGuestU32(currentHead + 0x0u)
                      << " prev=0x" << readGuestU32(currentHead + 0x4u)
                      << " target=0x" << readGuestU32(currentHead + 0x08u)
                      << " prio=0x" << readGuestU32(currentHead + 0x4Cu)
                      << " cb=0x" << readGuestU32(currentHead + 0x50u)
                      << " state5c=0x" << readGuestU32(currentHead + 0x5Cu)
                      << " state60=0x" << readGuestU32(currentHead + 0x60u)
                      << std::dec << std::endl;
        }

        const uint32_t entryPc = ctx->pc;
        ServiceQueuedFiles__Fv_0x181e10(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = getRegU32(ctx, 31);
        }
    }

    void nfsHp2AlphaGetLoadedTexture(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t packAddr = getRegU32(ctx, 4);
        const uint32_t textureId = getRegU32(ctx, 5);
        static uint32_t s_entryLogCount = 0u;
        if (s_entryLogCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] enter GetLoadedTexture override"
                      << " pack=0x" << std::hex << packAddr
                      << " texture=0x" << textureId
                      << " table=0x" << readGuestU32Raw(rdram, packAddr + 0x60u)
                      << " count=0x" << readGuestU32Raw(rdram, packAddr + 0x64u)
                      << " entryPc=0x" << entryPc
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            ++s_entryLogCount;
        }

        uint32_t resultAddr = 0u;
        if (tryLookupLoadedTextureFast(rdram, packAddr, textureId, resultAddr))
        {
            static uint32_t s_lookupLogCount = 0u;
            if (s_lookupLogCount < 24u)
            {
                std::cerr << "[nfs-hp2-alpha] GetLoadedTexture"
                          << " pack=0x" << std::hex << packAddr
                          << " texture=0x" << textureId
                          << " table=0x" << readGuestU32Raw(rdram, packAddr + 0x60u)
                          << " count=0x" << readGuestU32Raw(rdram, packAddr + 0x64u)
                          << " result=0x" << resultAddr
                          << std::dec << std::endl;
                ++s_lookupLogCount;
            }

            setReturnU32(ctx, resultAddr);
            ctx->pc = entryPc;
            return;
        }

        static uint32_t s_fallbackLogCount = 0u;
        if (s_fallbackLogCount < 8u)
        {
            std::cerr << "[nfs-hp2-alpha] GetLoadedTexture fallback to generated scan"
                      << " pack=0x" << std::hex << packAddr
                      << " texture=0x" << textureId
                      << " table=0x" << readGuestU32Raw(rdram, packAddr + 0x60u)
                      << " count=0x" << readGuestU32Raw(rdram, packAddr + 0x64u)
                      << std::dec << std::endl;
            ++s_fallbackLogCount;
        }

        GetLoadedTexture__11TexturePackUi_0x180b30(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaGetTextureInfo(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t wantedTextureId = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = gp - 0x5808u;
        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t resultAddr = 0u;

        static uint32_t s_walkLogCount = 0u;
        uint32_t walkLimit = 3u;

        for (uint32_t walked = 0u; current != 0u && current != sentinel && walked < 0x200u; ++walked)
        {
            if (current >= kPs2RdramSize)
            {
                if (s_walkLogCount < 8u)
                {
                    std::cerr << "[nfs-hp2-alpha] GetTextureInfo walk invalid addr=0x" << std::hex << current << std::dec << std::endl;
                    ++s_walkLogCount;
                }
                break;
            }

            const uint32_t nextNode = readGuestU32Raw(rdram, current + 0x0u);
            const uint32_t tableAddr = readGuestU32Raw(rdram, current + 0x60u);
            const uint32_t entryCount = readGuestU32Raw(rdram, current + 0x64u);

            if (s_walkLogCount < 8u && walked < walkLimit)
            {
                std::cerr << "[nfs-hp2-alpha] GetTextureInfo walk pack=0x" << std::hex << current
                          << " next=0x" << nextNode << " table=0x" << tableAddr
                          << " count=0x" << entryCount << " wanted=0x" << wantedTextureId << std::dec << std::endl;
                ++s_walkLogCount;
            }

            if (!tryLookupLoadedTextureFast(rdram, current, wantedTextureId, resultAddr))
            {
                break;
            }
            if (resultAddr != 0u)
            {
                setReturnU32(ctx, resultAddr);
                ctx->pc = entryPc;
                return;
            }
            current = nextNode;
        }

        static uint32_t s_fallbackReachCount = 0u;
        if (s_fallbackReachCount < 4u)
        {
            std::cerr << "[nfs-hp2-alpha] GetTextureInfo fallback-reached wanted=0x" << std::hex << wantedTextureId << std::dec << std::endl;
            ++s_fallbackReachCount;
        }

        static const std::array<uint32_t, 3> kKnownTexturePacks = {{
            0x1bbcd00u,  // DYNTEX
            0x1a3d0a0u,  // HUDTEX
            0x18f3560u,   // CARS
        }};

        for (const uint32_t packAddr : kKnownTexturePacks)
        {
            if (packAddr == current)
            {
                continue;
            }
            if (!tryLookupLoadedTextureFast(rdram, packAddr, wantedTextureId, resultAddr))
            {
                static uint32_t s_fallbackSkipCount = 0u;
                if (s_fallbackSkipCount < 8u)
                {
                    std::cerr << "[nfs-hp2-alpha] GetTextureInfo fallback-skip pack=0x" << std::hex << packAddr
                              << " wanted=0x" << wantedTextureId << std::dec << std::endl;
                    ++s_fallbackSkipCount;
                }
                continue;
            }
            if (resultAddr != 0u)
            {
                static uint32_t s_fallbackHitCount = 0u;
                if (s_fallbackHitCount < 8u)
                {
                    std::cerr << "[nfs-hp2-alpha] GetTextureInfo fallback-hit pack=0x" << std::hex << packAddr
                              << " wanted=0x" << wantedTextureId << " found=0x" << resultAddr << std::dec << std::endl;
                    ++s_fallbackHitCount;
                }
                setReturnU32(ctx, resultAddr);
                ctx->pc = entryPc;
                return;
            }
        }

        for (uint32_t i = 0u; i < 13u; ++i)
        {
            const uint32_t renderTargetAddr = 0x0027ED80u + (i * 0x500u);
            const uint32_t renderTargetIndex = readGuestU32Raw(rdram, renderTargetAddr + 0x0u);
            const uint32_t textureInfoAddr = 0x00260270u + (renderTargetIndex * kTextureInfoStride);
            if (readGuestU32Raw(rdram, textureInfoAddr + 0x20u) == wantedTextureId)
            {
                setReturnU32(ctx, textureInfoAddr);
                ctx->pc = entryPc;
                return;
            }
        }

        static uint32_t s_missLogCount = 0u;
        if (s_missLogCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] GetTextureInfo miss"
                      << " wanted=0x" << std::hex << wantedTextureId
                      << " listHead=0x" << readGuestU32Raw(rdram, sentinel + 0x0u)
                      << " sentinel=0x" << sentinel
                      << std::dec << std::endl;
            ++s_missLogCount;
        }

        uint32_t fallbackTexture = 0u;
        if (tryLookupLoadedTextureFast(rdram, 0x1bbcd00u, 0xABCDEF01u, fallbackTexture) && fallbackTexture != 0u)
        {
            setReturnU32(ctx, fallbackTexture);
            ctx->pc = entryPc;
            return;
        }

        setReturnU32(ctx, 0u);
        ctx->pc = entryPc;
    }

    bool tryResolveTextureInfoFast(uint8_t *rdram, uint32_t gp, uint32_t wantedTextureId, uint32_t &resultAddr)
    {
        const uint32_t sentinel = gp - 0x5808u;
        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);
        resultAddr = 0u;

        for (uint32_t walked = 0u; current != 0u && current != sentinel && walked < 0x200u; ++walked)
        {
            if (current >= kPs2RdramSize)
            {
                break;
            }
            if (!tryLookupLoadedTextureFast(rdram, current, wantedTextureId, resultAddr))
            {
                return false;
            }
            if (resultAddr != 0u)
            {
                return true;
            }
            current = readGuestU32Raw(rdram, current + 0x0u);
        }

        for (uint32_t i = 0u; i < 13u; ++i)
        {
            const uint32_t renderTargetAddr = 0x0027ED80u + (i * 0x500u);
            const uint32_t renderTargetIndex = readGuestU32Raw(rdram, renderTargetAddr + 0x0u);
            const uint32_t textureInfoAddr = 0x00260270u + (renderTargetIndex * kTextureInfoStride);
            if (readGuestU32Raw(rdram, textureInfoAddr + 0x20u) == wantedTextureId)
            {
                resultAddr = textureInfoAddr;
                return true;
            }
        }

        return true;
    }

    bool isLikelyPoisonTextureId(uint32_t value)
    {
        if (value == 0u || value == 0xFFFFFFFFu)
        {
            return false;
        }

        const uint8_t b0 = static_cast<uint8_t>(value >> 0);
        const uint8_t b1 = static_cast<uint8_t>(value >> 8);
        const uint8_t b2 = static_cast<uint8_t>(value >> 16);
        const uint8_t b3 = static_cast<uint8_t>(value >> 24);
        const bool allEe = (b0 == 0xEEu && b1 == 0xEEu && b2 == 0xEEu && b3 == 0xEEu);
        const bool mostlyEe = (b1 == 0xEEu && b2 == 0xEEu && b3 == 0xEEu);
        const bool allAa = (b0 == 0xAAu && b1 == 0xAAu && b2 == 0xAAu && b3 == 0xAAu);
        return allEe || mostlyEe || allAa;
    }

    bool looksLikePrintableAsciiWord(uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8)
        {
            const uint8_t ch = static_cast<uint8_t>((value >> shift) & 0xFFu);
            if (ch == 0u)
            {
                return false;
            }
            if (ch < 0x20u || ch > 0x7Eu)
            {
                return false;
            }
        }
        return true;
    }

    bool isLikelyModelReconnectTable(uint32_t entryBase, uint32_t entryCount)
    {
        if (entryBase == 0u && entryCount == 0u)
        {
            return true;
        }

        if (entryBase == 0u || entryBase >= kPs2RdramSize)
        {
            return false;
        }

        if (entryCount > 0x80u)
        {
            return false;
        }

        const uint64_t end = static_cast<uint64_t>(entryBase) + (static_cast<uint64_t>(entryCount) * 0x1Cu);
        if (end > kPs2RdramSize)
        {
            return false;
        }

        if (looksLikePrintableAsciiWord(entryBase))
        {
            return false;
        }

        return true;
    }

    void unlinkGuestListNode(uint8_t *rdram, uint32_t nodeAddr)
    {
        if (!rdram || nodeAddr == 0u || nodeAddr >= kPs2RdramSize)
        {
            return;
        }

        const uint32_t next = readGuestU32Raw(rdram, nodeAddr + 0x0u);
        const uint32_t prev = readGuestU32Raw(rdram, nodeAddr + 0x4u);
        if (next == 0u || prev == 0u || next >= kPs2RdramSize || prev >= kPs2RdramSize)
        {
            return;
        }

        writeGuestU32Raw(rdram, prev + 0x0u, next);
        writeGuestU32Raw(rdram, next + 0x4u, prev);
        writeGuestU32Raw(rdram, nodeAddr + 0x0u, nodeAddr);
        writeGuestU32Raw(rdram, nodeAddr + 0x4u, nodeAddr);
    }

    void nfsHp2AlphaReconnectTextureTable(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t modelAddr = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        if (modelAddr == 0u || modelAddr >= kPs2RdramSize)
        {
            ctx->pc = 0x109248u;
            ApplyReplacementTextureTable__6eModel_0x109248(rdram, ctx, runtime);
            if (ctx->pc == 0x109248u)
            {
                ctx->pc = entryPc;
            }
            return;
        }

        uint32_t entryBase = readGuestU32Raw(rdram, modelAddr + 0x10u);
        uint32_t entryCount = static_cast<uint8_t>(rdram[(modelAddr + 0x14u) & PS2_RAM_MASK]);
        const uint32_t replacementOffset = static_cast<uint8_t>(rdram[(modelAddr + 0x15u) & PS2_RAM_MASK]);
        const uint32_t solidAddr = readGuestU32Raw(rdram, modelAddr + 0x0Cu);

        static uint32_t s_reconnectLogCount = 0u;
        if (s_reconnectLogCount < 12u)
        {
            std::cerr << "[nfs-hp2-alpha] ReconnectTextureTable:enter"
                      << " model=0x" << std::hex << modelAddr
                      << " entryBase=0x" << entryBase
                      << " count=0x" << entryCount
                      << " replOff=0x" << replacementOffset
                      << " solid=0x" << solidAddr
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            ++s_reconnectLogCount;
        }

        if (entryBase >= kPs2RdramSize)
        {
            entryBase = 0u;
            entryCount = 0u;
            writeGuestU32Raw(rdram, modelAddr + 0x10u, 0u);
            rdram[(modelAddr + 0x14u) & PS2_RAM_MASK] = 0u;
        }
        else
        {
            const uint64_t entryEnd = static_cast<uint64_t>(entryBase) + (static_cast<uint64_t>(entryCount) * 0x1Cu);
            if (entryBase == 0u || entryEnd > kPs2RdramSize || entryCount > 0x200u)
            {
                static uint32_t s_badRangeLogCount = 0u;
                if (s_badRangeLogCount < 8u)
                {
                    std::cerr << "[nfs-hp2-alpha] ReconnectTextureTable:clamped"
                              << " model=0x" << std::hex << modelAddr
                              << " entryBase=0x" << entryBase
                              << " count=0x" << entryCount
                              << " end=0x" << static_cast<uint32_t>(entryEnd & 0xFFFFFFFFu)
                              << std::dec << std::endl;
                    ++s_badRangeLogCount;
                }
                entryCount = 0u;
                writeGuestU32Raw(rdram, modelAddr + 0x10u, 0u);
                rdram[(modelAddr + 0x14u) & PS2_RAM_MASK] = 0u;
            }
        }

        for (uint32_t i = 0u; i < entryCount; ++i)
        {
            const uint32_t entryAddr = entryBase + (i * 0x1Cu);
            const uint32_t wantedTextureId = readGuestU32Raw(rdram, entryAddr + 0x4u);
            uint32_t resolvedTextureInfo = 0u;

            if (!isLikelyPoisonTextureId(wantedTextureId))
            {
                if (!tryResolveTextureInfoFast(rdram, gp, wantedTextureId, resolvedTextureInfo))
                {
                    resolvedTextureInfo = 0u;
                }
            }
            else
            {
                static uint32_t s_poisonLogCount = 0u;
                if (s_poisonLogCount < 24u)
                {
                    std::cerr << "[nfs-hp2-alpha] ReconnectTextureTable:poison"
                              << " model=0x" << std::hex << modelAddr
                              << " entry=0x" << entryAddr
                              << " idx=0x" << i
                              << " texture=0x" << wantedTextureId
                              << std::dec << std::endl;
                    ++s_poisonLogCount;
                }
            }

            writeGuestU32Raw(rdram, entryAddr + 0x8u, resolvedTextureInfo);
            const uint32_t replacementFlagAddr = (entryAddr + 0x0Cu + replacementOffset) & PS2_RAM_MASK;
            rdram[replacementFlagAddr] = 0xFFu;

            if (solidAddr != 0u && solidAddr < kPs2RdramSize)
            {
                const int16_t solidCount = static_cast<int16_t>(readGuestU32Raw(rdram, solidAddr + 0xACu) & 0xFFFFu);
                const uint32_t solidTable = readGuestU32Raw(rdram, solidAddr + 0xB8u);
                if (solidCount > 0 && solidTable < kPs2RdramSize)
                {
                    const uint32_t solidId = readGuestU32Raw(rdram, entryAddr + 0x0u);
                    for (int32_t solidIndex = 0; solidIndex < solidCount; ++solidIndex)
                    {
                        const uint32_t solidEntry = solidTable + (static_cast<uint32_t>(solidIndex) * 8u);
                        if (solidEntry + 8u > kPs2RdramSize)
                        {
                            break;
                        }

                        if (readGuestU32Raw(rdram, solidEntry + 0x0u) == solidId)
                        {
                            rdram[replacementFlagAddr] = static_cast<uint8_t>(solidIndex);
                            break;
                        }
                    }
                }
            }
        }

        ctx->pc = 0x109248u;
        ApplyReplacementTextureTable__6eModel_0x109248(rdram, ctx, runtime);
        if (ctx->pc == 0x109248u)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaReconnectAllModels(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = gp - 0x6B00u;
        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);

        static uint32_t s_allModelsLogCount = 0u;
        if (s_allModelsLogCount < 6u)
        {
            std::cerr << "[nfs-hp2-alpha] ReconnectAllModels:enter"
                      << " sentinel=0x" << std::hex << sentinel
                      << " head=0x" << current
                      << " tail=0x" << readGuestU32Raw(rdram, sentinel + 0x4u)
                      << std::dec << std::endl;
            ++s_allModelsLogCount;
        }

        uint32_t walked = 0u;
        while (current != 0u && current != sentinel && walked < 0x400u)
        {
            const uint32_t next = readGuestU32Raw(rdram, current + 0x0u);
            const uint32_t solidAddr = readGuestU32Raw(rdram, current + 0x0Cu);
            const uint32_t entryBase = readGuestU32Raw(rdram, current + 0x10u);
            const uint32_t entryCount = static_cast<uint8_t>(rdram[(current + 0x14u) & PS2_RAM_MASK]);

            if (!isLikelyModelReconnectTable(entryBase, entryCount))
            {
                static uint32_t s_dropLogCount = 0u;
                if (s_dropLogCount < 16u)
                {
                    std::cerr << "[nfs-hp2-alpha] ReconnectAllModels:drop-node"
                              << " node=0x" << std::hex << current
                              << " next=0x" << next
                              << " solid=0x" << solidAddr
                              << " entryBase=0x" << entryBase
                              << " count=0x" << entryCount
                              << std::dec << std::endl;
                    ++s_dropLogCount;
                }
                unlinkGuestListNode(rdram, current);
                current = next;
                ++walked;
                continue;
            }

            SET_GPR_U32(ctx, 4, current);
            const uint32_t solidEntryPc = 0x109018u;
            ctx->pc = solidEntryPc;
            ReconnectSolid__6eModel_0x109018(rdram, ctx, runtime);
            if (ctx->pc != 0x1094F8u && ctx->pc != solidEntryPc)
            {
                return;
            }

            SET_GPR_U32(ctx, 4, current);
            ctx->pc = 0x109050u;
            nfsHp2AlphaReconnectTextureTable(rdram, ctx, runtime);
            if (ctx->pc != 0x109500u && ctx->pc != 0x109050u)
            {
                return;
            }

            const uint32_t refreshedSolidAddr = readGuestU32Raw(rdram, current + 0x0Cu);
            if (refreshedSolidAddr != 0u && refreshedSolidAddr < kPs2RdramSize)
            {
                SET_GPR_U32(ctx, 4, refreshedSolidAddr);
                const uint32_t fixStripEntryPc = 0x108BA0u;
                ctx->pc = fixStripEntryPc;
                FixStripSTs__6eSolid_0x108ba0(rdram, ctx, runtime);
                if (ctx->pc != 0x109514u && ctx->pc != fixStripEntryPc)
                {
                    return;
                }
            }

            current = next;
            ++walked;
        }

        ctx->pc = entryPc;
    }

    void nfsHp2AlphaFindAnimationSolid(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t wantedSolidId = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = gp - 0x58B0u;
        uint32_t head = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t tail = readGuestU32Raw(rdram, sentinel + 0x4u);

        if (head == 0u || tail == 0u)
        {
            initializeGuestListSentinel(rdram, sentinel);
            head = sentinel;
            tail = sentinel;
            static uint32_t s_animSentinelRepairCount = 0u;
            if (s_animSentinelRepairCount < 4u)
            {
                std::cerr << "[nfs-hp2-alpha] repaired animation solid sentinel"
                          << " sentinel=0x" << std::hex << sentinel
                          << " oldHead=0x" << head
                          << " oldTail=0x" << tail
                          << std::dec << std::endl;
                ++s_animSentinelRepairCount;
            }
        }

        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t result = 0u;
        for (uint32_t walked = 0u; current != 0u && current != sentinel && walked < 0x400u; ++walked)
        {
            if (current >= kPs2RdramSize)
            {
                break;
            }

            const uint32_t nodeSolidId = readGuestU32Raw(rdram, current + 0x18u);
            if (nodeSolidId == wantedSolidId)
            {
                result = current;
                break;
            }

            const uint32_t next = readGuestU32Raw(rdram, current + 0x0u);
            if (next == current || next == 0u || next >= kPs2RdramSize)
            {
                static uint32_t s_animBadNodeCount = 0u;
                if (s_animBadNodeCount < 12u)
                {
                    std::cerr << "[nfs-hp2-alpha] animation solid list break"
                              << " wanted=0x" << std::hex << wantedSolidId
                              << " node=0x" << current
                              << " next=0x" << next
                              << " solidId=0x" << nodeSolidId
                              << std::dec << std::endl;
                    ++s_animBadNodeCount;
                }
                break;
            }
            current = next;
        }

        setReturnU32(ctx, result);
        ctx->pc = entryPc;
    }

    void nfsHp2AlphaReconnectAllAnimations(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = gp - 0x58B8u;
        uint32_t head = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t tail = readGuestU32Raw(rdram, sentinel + 0x4u);

        if (head == 0u || tail == 0u)
        {
            const uint32_t oldHead = head;
            const uint32_t oldTail = tail;
            initializeGuestListSentinel(rdram, sentinel);
            static uint32_t s_animListRepairCount = 0u;
            if (s_animListRepairCount < 4u)
            {
                std::cerr << "[nfs-hp2-alpha] repaired animation list sentinel"
                          << " sentinel=0x" << std::hex << sentinel
                          << " oldHead=0x" << oldHead
                          << " oldTail=0x" << oldTail
                          << std::dec << std::endl;
                ++s_animListRepairCount;
            }
        }

        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);
        for (uint32_t walked = 0u; current != 0u && current != sentinel && walked < 0x400u; ++walked)
        {
            if (current >= kPs2RdramSize)
            {
                break;
            }

            const uint32_t next = readGuestU32Raw(rdram, current + 0x0u);
            SET_GPR_U32(ctx, 4, current);
            const uint32_t reconnectEntryPc = 0x17BFD0u;
            ctx->pc = reconnectEntryPc;
            ReconnectSolid__10eAnimation_0x17bfd0(rdram, ctx, runtime);
            if (ctx->pc != 0x17C548u && ctx->pc != reconnectEntryPc)
            {
                return;
            }

            if (next == current || next == 0u || next >= kPs2RdramSize)
            {
                static uint32_t s_animNodeBreakCount = 0u;
                if (s_animNodeBreakCount < 12u)
                {
                    std::cerr << "[nfs-hp2-alpha] animation list break"
                              << " node=0x" << std::hex << current
                              << " next=0x" << next
                              << " solidA=0x" << readGuestU32Raw(rdram, current + 0x20u)
                              << " solidB=0x" << readGuestU32Raw(rdram, current + 0x24u)
                              << std::dec << std::endl;
                    ++s_animNodeBreakCount;
                }
                break;
            }

            current = next;
        }

        ctx->pc = entryPc;
    }

    void nfsHp2AlphaAfxUpdateTextures(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = gp - 0x65A0u;
        uint32_t head = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t tail = readGuestU32Raw(rdram, sentinel + 0x4u);
        if (head == 0u || tail == 0u)
        {
            const uint32_t oldHead = head;
            const uint32_t oldTail = tail;
            initializeGuestListSentinel(rdram, sentinel);
            static uint32_t s_afxListRepairCount = 0u;
            if (s_afxListRepairCount < 4u)
            {
                std::cerr << "[nfs-hp2-alpha] repaired afx texture list sentinel"
                          << " sentinel=0x" << std::hex << sentinel
                          << " oldHead=0x" << oldHead
                          << " oldTail=0x" << oldTail
                          << std::dec << std::endl;
                ++s_afxListRepairCount;
            }
        }

        uint32_t current = readGuestU32Raw(rdram, sentinel + 0x0u);
        for (uint32_t walked = 0u; current != 0u && current != sentinel && walked < 0x400u; ++walked)
        {
            if (current >= kPs2RdramSize)
            {
                break;
            }

            const uint32_t next = readGuestU32Raw(rdram, current + 0x0u);
            const uint32_t wantedTextureId = readGuestU32Raw(rdram, current + 0x0Cu);
            const uint32_t cachedTextureInfo = readGuestU32Raw(rdram, current + 0x14u);
            bool needsRefresh = (cachedTextureInfo == 0u);
            if (!needsRefresh && cachedTextureInfo < kPs2RdramSize)
            {
                needsRefresh = (readGuestU32Raw(rdram, cachedTextureInfo + 0x20u) != wantedTextureId);
            }

            if (needsRefresh)
            {
                uint32_t resolved = 0u;
                if (!isLikelyPoisonTextureId(wantedTextureId))
                {
                    if (!tryResolveTextureInfoFast(rdram, gp, wantedTextureId, resolved))
                    {
                        resolved = 0u;
                    }
                }
                writeGuestU32Raw(rdram, current + 0x14u, resolved);

                static uint32_t s_afxUpdateLogCount = 0u;
                if (s_afxUpdateLogCount < 16u)
                {
                    std::cerr << "[nfs-hp2-alpha] afxUpdateTextures"
                              << " node=0x" << std::hex << current
                              << " wanted=0x" << wantedTextureId
                              << " cached=0x" << cachedTextureInfo
                              << " resolved=0x" << resolved
                              << std::dec << std::endl;
                    ++s_afxUpdateLogCount;
                }
            }

            if (next == current || next == 0u || next >= kPs2RdramSize)
            {
                static uint32_t s_afxBreakLogCount = 0u;
                if (s_afxBreakLogCount < 12u)
                {
                    std::cerr << "[nfs-hp2-alpha] afx texture list break"
                              << " node=0x" << std::hex << current
                              << " next=0x" << next
                              << " wanted=0x" << wantedTextureId
                              << std::dec << std::endl;
                    ++s_afxBreakLogCount;
                }
                break;
            }

            current = next;
        }

        ctx->pc = entryPc;
    }

    void nfsHp2AlphaLoadCommonRegion(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t soundAddr = getRegU32(ctx, 4);
        if (soundAddr == 0u || soundAddr >= kPs2RdramSize)
        {
            ctx->pc = entryPc;
            return;
        }

        const uint32_t commonState = readGuestU32Raw(rdram, soundAddr + 0xA0Cu);
        const uint32_t regionBase = readGuestU32Raw(rdram, soundAddr + 0x200u);
        const uint32_t regionSize = readGuestU32Raw(rdram, soundAddr + 0xA10u);
        const uint32_t regionPtr = readGuestU32Raw(rdram, soundAddr + 0xC28u);
        const uint32_t gp = getRegU32(ctx, 28);

        auto disableCommonRegion = [&](const char *reason)
        {
            writeGuestU32Raw(rdram, soundAddr + 0xA0Cu, 0u);
            writeGuestU32Raw(rdram, soundAddr + 0xC28u, 0u);
            if (gp != 0u && gp < kPs2RdramSize && gp >= 0x57A0u)
            {
                writeGuestU32Raw(rdram, gp - 0x57A0u, 1u);
            }
            static uint32_t s_soundDisableLogCount = 0u;
            if (s_soundDisableLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] disabled sound common region"
                          << " sound=0x" << std::hex << soundAddr
                          << " gp=0x" << gp
                          << " state=0x" << commonState
                          << " base=0x" << regionBase
                          << " size=0x" << regionSize
                          << " region=0x" << regionPtr
                          << " initReady=1"
                          << " reason=" << reason
                          << std::dec << std::endl;
                ++s_soundDisableLogCount;
            }
        };

        if (commonState != 1u)
        {
            ctx->pc = entryPc;
            return;
        }

        const uint64_t regionEnd = static_cast<uint64_t>(regionBase) + static_cast<uint64_t>(regionSize);
        if (regionBase == 0u || regionBase >= kPs2RdramSize || regionSize == 0u || regionEnd > kPs2RdramSize)
        {
            disableCommonRegion("invalid-range");
            ctx->pc = entryPc;
            return;
        }

        uint32_t cursor = regionBase;
        bool foundCommonRegion = false;
        for (uint32_t walked = 0u; cursor < regionEnd && walked < 0x800u; ++walked)
        {
            const uint32_t tag = readGuestU32Raw(rdram, cursor + 0x0u);
            if (tag == 0x80014101u)
            {
                foundCommonRegion = true;
                break;
            }

            const uint32_t stepSize = readGuestU32Raw(rdram, cursor + 0x4u);
            if (stepSize == 0u || stepSize > 0x100000u)
            {
                disableCommonRegion("invalid-step");
                ctx->pc = entryPc;
                return;
            }

            const uint64_t nextCursor = static_cast<uint64_t>(cursor) + static_cast<uint64_t>(stepSize) + 8u;
            if (nextCursor <= cursor || nextCursor > regionEnd)
            {
                disableCommonRegion("step-out-of-range");
                ctx->pc = entryPc;
                return;
            }

            cursor = static_cast<uint32_t>(nextCursor);
        }

        if (!foundCommonRegion)
        {
            disableCommonRegion("scan-exhausted");
            ctx->pc = entryPc;
            return;
        }

        LoadCommonRegion__6bSound_0x20cf50(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaLoadTextureTableFromFile(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t packAddr = getRegU32(ctx, 4);
        const uint32_t sourcePathAddr = getRegU32(ctx, 5);
        const uint32_t preTable = (packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x60u) : 0u;
        const uint32_t preCount = (packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x64u) : 0u;
        const std::string sourcePath = readGuestCStringRaw(rdram, sourcePathAddr, 0x200u);

        static uint32_t s_enterLogCount = 0u;
        if (s_enterLogCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] LoadTextureTableFromFile:enter"
                      << " pack=0x" << std::hex << packAddr
                      << " argPath=0x" << sourcePathAddr
                      << " preTable=0x" << preTable
                      << " preCount=0x" << preCount
                      << " path=\"" << sourcePath << "\""
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            ++s_enterLogCount;
        }

        auto tryHostFastPath = [&]() -> bool
        {
            if (packAddr == 0u || sourcePath.empty())
            {
                return false;
            }

            const auto unpackedRoot = detectNfsHp2UnpackedRoot(runtime->getIoPaths());
            if (unpackedRoot.empty())
            {
                return false;
            }

            std::filesystem::path relativePath(sourcePath);
            std::filesystem::path hostPath = (unpackedRoot / relativePath).lexically_normal();
            std::error_code ec;
            if (!std::filesystem::exists(hostPath, ec) || ec || !std::filesystem::is_regular_file(hostPath, ec))
            {
                return false;
            }

            const uint64_t fileSize = std::filesystem::file_size(hostPath, ec);
            if (ec || fileSize < 0x2Cu)
            {
                return false;
            }

            const size_t prefixSize = static_cast<size_t>(std::min<uint64_t>(fileSize, 0x20000u));
            std::vector<uint8_t> bytes(prefixSize);
            std::ifstream stream(hostPath, std::ios::binary);
            if (!stream)
            {
                return false;
            }

            stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            if (stream.gcount() != static_cast<std::streamsize>(bytes.size()))
            {
                return false;
            }

            auto readHostU32 = [&](size_t offset) -> uint32_t
            {
                if ((offset + 4u) > bytes.size())
                {
                    return 0u;
                }

                return static_cast<uint32_t>(bytes[offset + 0u]) |
                       (static_cast<uint32_t>(bytes[offset + 1u]) << 8) |
                       (static_cast<uint32_t>(bytes[offset + 2u]) << 16) |
                       (static_cast<uint32_t>(bytes[offset + 3u]) << 24);
            };

            const uint32_t firstBlockOffset = readHostU32(0x0Cu);
            const size_t sectionBase = 0x08u + static_cast<size_t>(firstBlockOffset);
            const size_t nestedBase = sectionBase + 0x08u;
            if ((nestedBase + 0x08u) > bytes.size())
            {
                return false;
            }

            const uint32_t textureTableBytes = readHostU32(nestedBase + 0x04u);
            if (textureTableBytes == 0u || (textureTableBytes % kTextureInfoStride) != 0u)
            {
                return false;
            }

            const size_t tableSrcOffset = sectionBase + 0x10u;
            const size_t nestedEnd = nestedBase + static_cast<size_t>(textureTableBytes);
            const size_t nestedEndWithHeader = nestedEnd + 0x08u;
            if (tableSrcOffset >= bytes.size() || nestedEndWithHeader > bytes.size())
            {
                return false;
            }

            const uint32_t textureEntryCount = textureTableBytes / kTextureInfoStride;
            if (textureEntryCount == 0u || textureEntryCount > kMaxReasonableTextureEntryCount)
            {
                return false;
            }

            auto callGuestMalloc = [&](uint32_t sizeBytes) -> uint32_t
            {
                if (!runtime->hasFunction(0x1FBFE0u))
                {
                    return 0u;
                }

                R5900Context mallocCtx = *ctx;
                SET_GPR_U32(&mallocCtx, 4, sizeBytes);
                SET_GPR_U32(&mallocCtx, 5, 0x10u);
                mallocCtx.pc = 0x1FBFE0u;
                auto fn = runtime->lookupFunction(0x1FBFE0u);
                fn(rdram, &mallocCtx, runtime);
                return getRegU32(&mallocCtx, 2);
            };

            const uint32_t guestTableAddr = callGuestMalloc(textureTableBytes);
            if (guestTableAddr == 0u)
            {
                return false;
            }

            std::memcpy(rdram + (guestTableAddr & PS2_RAM_MASK),
                        bytes.data() + tableSrcOffset,
                        textureTableBytes);

            std::memset(rdram + ((packAddr + 0x28u) & PS2_RAM_MASK), 0, 0x34u);
            const size_t boundedPathLen = std::min<size_t>(sourcePath.size(), 0x33u);
            std::memcpy(rdram + ((packAddr + 0x28u) & PS2_RAM_MASK), sourcePath.data(), boundedPathLen);

            const uint32_t dataOffsetAligned =
                static_cast<uint32_t>((nestedEnd + 0x8Fu) & ~static_cast<size_t>(0x7Fu));
            writeGuestU32Raw(rdram, packAddr + 0x6Cu, dataOffsetAligned);

            uint32_t guestDataAddr = 0u;
            size_t payloadBytes = 0u;
            if (static_cast<uint64_t>(dataOffsetAligned) < fileSize)
            {
                payloadBytes = static_cast<size_t>(fileSize - static_cast<uint64_t>(dataOffsetAligned));
                if (payloadBytes > 0u)
                {
                    guestDataAddr = callGuestMalloc(static_cast<uint32_t>(payloadBytes));
                    if (guestDataAddr != 0u)
                    {
                        std::vector<uint8_t> payload(payloadBytes);
                        stream.clear();
                        stream.seekg(static_cast<std::streamoff>(dataOffsetAligned), std::ios::beg);
                        stream.read(reinterpret_cast<char *>(payload.data()), static_cast<std::streamsize>(payload.size()));
                        if (stream.gcount() != static_cast<std::streamsize>(payload.size()))
                        {
                            guestDataAddr = 0u;
                            payloadBytes = 0u;
                        }
                        else
                        {
                            std::memcpy(rdram + (guestDataAddr & PS2_RAM_MASK), payload.data(), payloadBytes);
                            texturePackPayloadWindows()[packAddr] = {
                                guestDataAddr,
                                static_cast<uint32_t>(payloadBytes)};

                            for (uint32_t entryIndex = 0u; entryIndex < textureEntryCount; ++entryIndex)
                            {
                                const uint32_t entryAddr = guestTableAddr + (entryIndex * kTextureInfoStride);
                                const uint32_t offset30 = readGuestU32Raw(rdram, entryAddr + 0x30u);
                                const uint32_t offset34 = readGuestU32Raw(rdram, entryAddr + 0x34u);
                                if (offset30 < payloadBytes)
                                {
                                    writeGuestU32Raw(rdram, entryAddr + 0x8Cu, guestDataAddr + offset30);
                                }
                                if (offset34 < payloadBytes)
                                {
                                    writeGuestU32Raw(rdram, entryAddr + 0x90u, guestDataAddr + offset34);
                                }
                            }

                            R5900Context assignCtx = *ctx;
                            SET_GPR_U32(&assignCtx, 4, packAddr);
                            SET_GPR_U32(&assignCtx, 5, guestDataAddr);
                            SET_GPR_U32(&assignCtx, 6, 0u);
                            SET_GPR_U32(&assignCtx, 7, static_cast<uint32_t>(payloadBytes));
                            assignCtx.pc = 0x1809F0u;
                            AssignTextureData__11TexturePackPcii_0x1809f0(rdram, &assignCtx, runtime);
                        }
                    }
                }
            }

            R5900Context attachCtx = *ctx;
            SET_GPR_U32(&attachCtx, 4, packAddr);
            SET_GPR_U32(&attachCtx, 5, guestTableAddr);
            SET_GPR_U32(&attachCtx, 6, textureEntryCount);
            attachCtx.pc = 0x1808E0u;
            AttachTextureTable__11TexturePackP11TextureInfoi_0x1808e0(rdram, &attachCtx, runtime);

            writeGuestU32Raw(rdram, packAddr + 0x68u, 1u);

            static uint32_t s_fastPathLogCount = 0u;
            if (s_fastPathLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] LoadTextureTableFromFile:host-fast-path"
                          << " pack=0x" << std::hex << packAddr
                          << " host=\"" << hostPath.string() << "\""
                          << " guestTable=0x" << guestTableAddr
                          << " guestData=0x" << guestDataAddr
                          << " count=0x" << textureEntryCount
                          << " dataOffset=0x" << dataOffsetAligned
                          << " payloadBytes=0x" << static_cast<uint32_t>(payloadBytes)
                          << " postTable=0x" << readGuestU32Raw(rdram, packAddr + 0x60u)
                          << " postCount=0x" << readGuestU32Raw(rdram, packAddr + 0x64u)
                          << std::dec << std::endl;
                ++s_fastPathLogCount;
            }

            return readGuestU32Raw(rdram, packAddr + 0x60u) != 0u &&
                   readGuestU32Raw(rdram, packAddr + 0x64u) != 0u;
        };

        if (tryHostFastPath())
        {
            ctx->pc = entryPc;
            return;
        }

        LoadTextureTableFromFile__11TexturePackPCc_0x180c18(rdram, ctx, runtime);

        const uint32_t postTable = (packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x60u) : 0u;
        const uint32_t postCount = (packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x64u) : 0u;
        const uint32_t packPathAddr = (packAddr != 0u) ? (packAddr + 0x28u) : 0u;
        const std::string packPath = readGuestCStringRaw(rdram, packPathAddr, 0x200u);

        static uint32_t s_exitLogCount = 0u;
        if (s_exitLogCount < 20u)
        {
            std::cerr << "[nfs-hp2-alpha] LoadTextureTableFromFile:return"
                      << " pack=0x" << std::hex << packAddr
                      << " postTable=0x" << postTable
                      << " postCount=0x" << postCount
                      << " packPath=\"" << packPath << "\""
                      << " pc=0x" << ctx->pc
                      << std::dec << std::endl;
            ++s_exitLogCount;
        }

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaOpenPlatformFile(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t pathAddr = getRegU32(ctx, 4);
        const uint32_t returnAddr = getRegU32(ctx, 31);
        std::string sourcePath = readGuestCStringRaw(rdram, pathAddr, 0x100u);
        if (ps2IsScratchpadAddress(pathAddr))
        {
            mirrorScratchpadCStringToMaskedAlias(rdram, pathAddr, 0x100u);
            static uint32_t s_scratchpadPathLogCount = 0u;
            if (s_scratchpadPathLogCount < 16u)
            {
                std::cerr << "[nfs-hp2-alpha] OpenPlatformFile:mirrored-scratchpad-path"
                          << " guest=0x" << std::hex << pathAddr
                          << " alias=0x" << (pathAddr & PS2_RAM_MASK)
                          << " ra=0x" << returnAddr
                          << " path=\"" << sourcePath << "\""
                          << std::dec << std::endl;
                ++s_scratchpadPathLogCount;
            }

            std::filesystem::path hostPath;
            if (!sourcePath.empty() &&
                resolveNfsHp2UnpackedFile(runtime->getIoPaths(), sourcePath, hostPath))
            {
                uint32_t pseudoLbn = 0u;
                uint32_t sizeBytes = 0u;
                if (ps2_stubs::registerCdHostFileAlias(sourcePath, hostPath, &pseudoLbn, &sizeBytes))
                {
                    const uint32_t gp = getRegU32(ctx, 28);
                    const uint32_t slotPoolAddr = readGuestU32Raw(rdram, ADD32(gp, 4294948176));
                    const uint32_t savedPc = ctx->pc;
                    const uint32_t savedA0 = getRegU32(ctx, 4);
                    const uint32_t savedA1 = getRegU32(ctx, 5);
                    const uint32_t savedA2 = getRegU32(ctx, 6);
                    const uint32_t savedA3 = getRegU32(ctx, 7);
                    const uint32_t savedT0 = getRegU32(ctx, 8);
                    const uint32_t savedT1 = getRegU32(ctx, 9);

                    uint32_t fileAddr = 0u;
                    if (runtime->hasFunction(0x1FC530u))
                    {
                        SET_GPR_U32(ctx, 4, slotPoolAddr);
                        ctx->pc = 0x1FC530u;
                        const uint32_t allocEntryPc = ctx->pc;
                        runtime->lookupFunction(0x1FC530u)(rdram, ctx, runtime);
                        if (ctx->pc == allocEntryPc)
                        {
                            ctx->pc = savedPc;
                        }
                        fileAddr = getRegU32(ctx, 2);
                    }

                    if (fileAddr != 0u && runtime->hasFunction(0x1F8958u))
                    {
                        SET_GPR_U32(ctx, 4, fileAddr);
                        SET_GPR_U32(ctx, 5, pathAddr);
                        SET_GPR_U32(ctx, 6, 2u);
                        SET_GPR_U32(ctx, 7, sizeBytes);
                        SET_GPR_U32(ctx, 8, 0x1FADC0u);
                        SET_GPR_U32(ctx, 9, 0x1FADA0u);
                        ctx->pc = 0x1F8958u;
                        const uint32_t ctorEntryPc = ctx->pc;
                        runtime->lookupFunction(0x1F8958u)(rdram, ctx, runtime);
                        if (ctx->pc == ctorEntryPc)
                        {
                            ctx->pc = savedPc;
                        }

                        writeGuestU32Raw(rdram, fileAddr + 0x84u, pseudoLbn);
                        writeGuestU32Raw(rdram, fileAddr + 0x88u, 0u);
                        setReturnU32(ctx, fileAddr);
                        SET_GPR_U32(ctx, 4, savedA0);
                        SET_GPR_U32(ctx, 5, savedA1);
                        SET_GPR_U32(ctx, 6, savedA2);
                        SET_GPR_U32(ctx, 7, savedA3);
                        SET_GPR_U32(ctx, 8, savedT0);
                        SET_GPR_U32(ctx, 9, savedT1);
                        ctx->pc = entryPc;

                        std::cerr << "[nfs-hp2-alpha] OpenPlatformFile:host-fast-path"
                                  << " guest=0x" << std::hex << pathAddr
                                  << " host=\"" << hostPath.string() << "\""
                                  << " file=0x" << fileAddr
                                  << " lbn=0x" << pseudoLbn
                                  << " size=0x" << sizeBytes
                                  << std::dec << std::endl;
                        return;
                    }

                    SET_GPR_U32(ctx, 4, savedA0);
                    SET_GPR_U32(ctx, 5, savedA1);
                    SET_GPR_U32(ctx, 6, savedA2);
                    SET_GPR_U32(ctx, 7, savedA3);
                    SET_GPR_U32(ctx, 8, savedT0);
                    SET_GPR_U32(ctx, 9, savedT1);
                    ctx->pc = savedPc;
                }
            }
        }

        bool shouldLog = sourcePath.empty();
        if (!shouldLog)
        {
            std::string lowerPath = sourcePath;
            std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
                           [](unsigned char ch)
                           {
                               return static_cast<char>(std::tolower(ch));
                           });
            shouldLog = (lowerPath.find("sound") != std::string::npos) ||
                        (lowerPath.find("movie") != std::string::npos) ||
                        (lowerPath.find("speech") != std::string::npos);
        }

        if (shouldLog)
        {
            const std::array<uint8_t, 16> previewBytes = readGuestPreviewBytes(rdram, pathAddr);

            std::cerr << "[nfs-hp2-alpha] OpenPlatformFile:enter"
                      << " pathAddr=0x" << std::hex << pathAddr
                      << " ra=0x" << returnAddr
                      << " path=\"" << sourcePath << "\""
                      << " preview=";
            for (size_t i = 0; i < previewBytes.size(); ++i)
            {
                if (i != 0u)
                {
                    std::cerr << ' ';
                }
                std::cerr << static_cast<uint32_t>(previewBytes[i]);
            }
            std::cerr << std::dec << std::endl;
        }

        OpenPlatformFile__FPCc_0x1fb360(rdram, ctx, runtime);

        if (shouldLog)
        {
            std::cerr << "[nfs-hp2-alpha] OpenPlatformFile:return"
                      << " pathAddr=0x" << std::hex << pathAddr
                      << " result=0x" << getRegU32(ctx, 2)
                      << " pc=0x" << ctx->pc
                      << std::dec << std::endl;
        }

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaBOpenSingleArg(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        static uint32_t s_logCount = 0u;
        if (s_logCount < 8u)
        {
            const uint32_t pathAddr = getRegU32(ctx, 4);
            const uint32_t ra = getRegU32(ctx, 31);
            std::cerr << "[nfs-hp2-alpha] bOpen__FPCc entry path=0x" << std::hex << pathAddr << " ra=0x" << ra << std::dec << std::endl;
            ++s_logCount;
        }
        bOpen__FPCc_0x1f9590(rdram, ctx, runtime);
    }

    void nfsHp2AlphaInitMoviePlayerFile(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t thisAddr = getRegU32(ctx, 4);
        const uint32_t pathAddr = getRegU32(ctx, 5);

        static uint32_t s_logCount = 0u;
        if (s_logCount < 8u)
        {
            std::cerr << "[nfs-hp2-alpha] InitMoviePlayerFile this=0x" << std::hex << thisAddr
                      << " path=0x" << pathAddr << " ra=0x" << getRegU32(ctx, 31) << std::dec << std::endl;
            ++s_logCount;
        }

        if (pathAddr == 0u || pathAddr == 0xAAAAAAAAu || pathAddr == 0xEEEEEEEEu || pathAddr == 0x44444444u)
        {
            SET_GPR_U32(ctx, 2, 0u);
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        InitFile__Q22M211MoviePlayerPCc_0x1eee08(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaBOpen(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t pathAddr = getRegU32(ctx, 4);
        const uint32_t openFlags = getRegU32(ctx, 5);
        const uint32_t returnAddr = getRegU32(ctx, 31);
        const uint32_t gp = getRegU32(ctx, 28);

        if (pathAddr == 0u || pathAddr == 0xAAAAAAAAu || pathAddr == 0xEEEEEEEEu || pathAddr == 0x44444444u)
        {
            const uint32_t currentSp = getRegU32(ctx, 29);
            const uint32_t callerRa = readGuestU32Raw(rdram, currentSp + 0x30u);
            const uint32_t callerCallerRa = readGuestU32Raw(rdram, currentSp + 0x50u);
            static uint32_t s_badPathCount = 0u;
            if (s_badPathCount < 8u)
            {
                std::cerr << "[nfs-hp2-alpha] bOpen:rejected-bad-path pathAddr=0x" << std::hex << pathAddr
                          << " ra=0x" << returnAddr << " callerRa=0x" << callerRa << " callerCallerRa=0x" << callerCallerRa
                          << " sp=0x" << currentSp << " gp=0x" << gp << " entryPc=0x" << entryPc << std::dec << std::endl;
                std::cerr << "  stack dump around sp+0x20:";
                for (int i = -4; i <= 4; i++)
                {
                    uint32_t offset = static_cast<uint32_t>(i * 4);
                    uint32_t val = readGuestU32Raw(rdram, currentSp + offset + 0x20u);
                    std::cerr << " " << std::hex << "@" << (currentSp + offset + 0x20u) << "=" << val;
                }
                std::cerr << std::dec << std::endl;
                ++s_badPathCount;
            }
            SET_GPR_U32(ctx, 2, 0u);
            ctx->pc = returnAddr;
            return;
        }

        const uint32_t currentSp = getRegU32(ctx, 29);
        const uint32_t wrapperCallerRa = readGuestU32Raw(rdram, currentSp + 0x30u);
        std::string sourcePath = readGuestCStringRaw(rdram, pathAddr, 0x100u);
        if (ps2IsScratchpadAddress(pathAddr))
        {
            mirrorScratchpadCStringToMaskedAlias(rdram, pathAddr, 0x100u);
            sourcePath = readGuestCStringRaw(rdram, pathAddr & PS2_RAM_MASK, 0x100u);
        }

        static uint32_t s_enterLogCount = 0u;
        if ((sourcePath.empty() || sourcePath == "badptr") && s_enterLogCount < 32u)
        {
            const std::array<uint8_t, 16> previewBytes = readGuestPreviewBytes(rdram, pathAddr);
            std::cerr << "[nfs-hp2-alpha] bOpen:enter"
                      << " pathAddr=0x" << std::hex << pathAddr
                      << " ra=0x" << returnAddr
                      << " callerRa=0x" << wrapperCallerRa
                      << " flags=0x" << openFlags
                      << " path=\"" << sourcePath << "\""
                      << " sp=0x" << currentSp
                      << " preview=";
            for (size_t i = 0; i < previewBytes.size(); ++i)
            {
                if (i != 0u)
                {
                    std::cerr << ' ';
                }
                std::cerr << static_cast<uint32_t>(previewBytes[i]);
            }
            std::cerr << std::dec << std::endl;
            ++s_enterLogCount;
        }

        bOpen__FPCci_0x1f94d8(rdram, ctx, runtime);

        static uint32_t s_failLogCount = 0u;
        if (getRegU32(ctx, 2) == 0u && s_failLogCount < 32u)
        {
            std::cerr << "[nfs-hp2-alpha] bOpen:return-null"
                      << " pathAddr=0x" << std::hex << pathAddr
                      << " ra=0x" << returnAddr
                      << " callerRa=0x" << wrapperCallerRa
                      << " flags=0x" << openFlags
                      << " path=\"" << sourcePath << "\""
                      << " pc=0x" << ctx->pc
                      << std::dec << std::endl;
            ++s_failLogCount;
        }

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void logNisManagerState(uint8_t *rdram,
                            const char *phase,
                            uint32_t managerAddr,
                            uint32_t returnAddr,
                            uint32_t extra0 = 0u,
                            uint32_t extra1 = 0u,
                            uint32_t extra2 = 0u,
                            uint32_t extra3 = 0u)
    {
        std::cerr << "[nfs-hp2-alpha] " << phase
                  << " mgr=0x" << std::hex << managerAddr
                  << " next=0x" << readGuestU32Raw(rdram, managerAddr + 0x00u)
                  << " prev=0x" << readGuestU32Raw(rdram, managerAddr + 0x04u)
                  << " state0c=0x" << readGuestU32Raw(rdram, managerAddr + 0x0Cu)
                  << " state10=0x" << readGuestU32Raw(rdram, managerAddr + 0x10u)
                  << " res14=0x" << readGuestU32Raw(rdram, managerAddr + 0x14u)
                  << " res18=0x" << readGuestU32Raw(rdram, managerAddr + 0x18u)
                  << " field3c=0x" << readGuestU32Raw(rdram, managerAddr + 0x3Cu)
                  << " idx44=0x" << readGuestU32Raw(rdram, managerAddr + 0x44u)
                  << " car48=0x" << readGuestU32Raw(rdram, managerAddr + 0x48u)
                  << " car4c=0x" << readGuestU32Raw(rdram, managerAddr + 0x4Cu)
                  << " field50=0x" << readGuestU32Raw(rdram, managerAddr + 0x50u)
                  << " state54=0x" << readGuestU32Raw(rdram, managerAddr + 0x54u)
                  << " state58=0x" << readGuestU32Raw(rdram, managerAddr + 0x58u)
                  << " coord5c=0x" << readGuestU32Raw(rdram, managerAddr + 0x5Cu)
                  << " anim80=0x" << readGuestU32Raw(rdram, managerAddr + 0x80u)
                  << " matA0=0x" << readGuestU32Raw(rdram, managerAddr + 0xA0u)
                  << " animB0=0x" << readGuestU32Raw(rdram, managerAddr + 0xB0u)
                  << " extra0=0x" << extra0
                  << " extra1=0x" << extra1
                  << " extra2=0x" << extra2
                  << " extra3=0x" << extra3
                  << " ra=0x" << returnAddr
                  << std::dec << std::endl;
    }

    void nfsHp2AlphaNisManagerCtor(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t managerAddr = getRegU32(ctx, 4);
        const uint32_t kind = getRegU32(ctx, 5);
        static uint32_t s_ctorLogCount = 0u;
        if (managerAddr != 0u && s_ctorLogCount < 12u)
        {
            logNisManagerState(rdram, "NisManager::ctor:before", managerAddr, getRegU32(ctx, 31), kind);
        }
        ps2____10NisManager_0x185b60(rdram, ctx, runtime);
        if (managerAddr != 0u && s_ctorLogCount < 12u)
        {
            logNisManagerState(rdram, "NisManager::ctor:after", managerAddr, getRegU32(ctx, 31), kind);
            ++s_ctorLogCount;
        }
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaStartNisManager(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t managerAddr = getRegU32(ctx, 4);
        const uint32_t idx = getRegU32(ctx, 5);
        const uint32_t car0 = getRegU32(ctx, 6);
        const uint32_t car1 = getRegU32(ctx, 7);
        const uint32_t arg8 = getRegU32(ctx, 8);
        static uint32_t s_startLogCount = 0u;
        if (managerAddr != 0u && s_startLogCount < 12u)
        {
            logNisManagerState(rdram, "NisManager::Start:before", managerAddr, getRegU32(ctx, 31), idx, car0, car1, arg8);
        }
        Start__10NisManageriiP3CarT3_0x185cf8(rdram, ctx, runtime);
        if (managerAddr != 0u && s_startLogCount < 12u)
        {
            logNisManagerState(rdram, "NisManager::Start:after", managerAddr, getRegU32(ctx, 31), idx, car0, car1, arg8);
            ++s_startLogCount;
        }
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaLoadNisManager(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t managerAddr = getRegU32(ctx, 4);
        const uint32_t idx = getRegU32(ctx, 5);
        static uint32_t s_loadLogCount = 0u;
        if (managerAddr != 0u && s_loadLogCount < 16u)
        {
            logNisManagerState(rdram, "NisManager::Load:before", managerAddr, getRegU32(ctx, 31), idx);
        }
        Load__10NisManageri_0x185ee0(rdram, ctx, runtime);
        if (managerAddr != 0u && s_loadLogCount < 16u)
        {
            logNisManagerState(rdram, "NisManager::Load:after", managerAddr, getRegU32(ctx, 31), idx);
            ++s_loadLogCount;
        }
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaStartAnimatingNisManager(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t managerAddr = getRegU32(ctx, 4);
        static uint32_t s_startAnimatingLogCount = 0u;
        if (managerAddr != 0u && s_startAnimatingLogCount < 16u)
        {
            logNisManagerState(rdram, "NisManager::StartAnimating:before", managerAddr, getRegU32(ctx, 31));
        }
        StartAnimating__10NisManager_0x186228(rdram, ctx, runtime);
        if (managerAddr != 0u && s_startAnimatingLogCount < 16u)
        {
            logNisManagerState(rdram, "NisManager::StartAnimating:after", managerAddr, getRegU32(ctx, 31));
            ++s_startAnimatingLogCount;
        }
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaRenderNisManager(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t managerAddr = getRegU32(ctx, 4);
        const uint32_t viewAddr = getRegU32(ctx, 5);

        static uint32_t s_renderLogCount = 0u;
        if (managerAddr != 0u && s_renderLogCount < 24u)
        {
            std::cerr << "[nfs-hp2-alpha] NisManager::Render:enter"
                      << " mgr=0x" << std::hex << managerAddr
                      << " view=0x" << viewAddr
                      << " anim80=0x" << readGuestU32Raw(rdram, managerAddr + 0x80u)
                      << " matA0=0x" << readGuestU32Raw(rdram, managerAddr + 0xA0u)
                      << " animB0=0x" << readGuestU32Raw(rdram, managerAddr + 0xB0u)
                      << " car48=0x" << readGuestU32Raw(rdram, managerAddr + 0x48u)
                      << " car4c=0x" << readGuestU32Raw(rdram, managerAddr + 0x4Cu)
                      << " idx44=0x" << readGuestU32Raw(rdram, managerAddr + 0x44u)
                      << " state54=0x" << readGuestU32Raw(rdram, managerAddr + 0x54u)
                      << " state58=0x" << readGuestU32Raw(rdram, managerAddr + 0x58u)
                      << " coordinator5c=0x" << readGuestU32Raw(rdram, managerAddr + 0x5Cu)
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            ++s_renderLogCount;
        }

        Render__10NisManagerP5eView_0x1864a0(rdram, ctx, runtime);

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaDrawNisObjects(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t sentinel = ADD32(gp, 4294945032u); // gp - 0x56F8
        uint32_t head = readGuestU32Raw(rdram, sentinel + 0x0u);
        uint32_t tail = readGuestU32Raw(rdram, sentinel + 0x4u);

        auto resetNisList = [&](const char *reason, uint32_t addr) {
            writeGuestU32Raw(rdram, sentinel + 0x0u, sentinel);
            writeGuestU32Raw(rdram, sentinel + 0x4u, sentinel);

            static uint32_t s_resetLogCount = 0u;
            if (s_resetLogCount < 12u)
            {
                std::cerr << "[nfs-hp2-alpha] reset NisManager list"
                          << " reason=" << reason
                          << " sentinel=0x" << std::hex << sentinel
                          << " bad=0x" << addr
                          << " head=0x" << head
                          << " tail=0x" << tail
                          << std::dec << std::endl;
                ++s_resetLogCount;
            }
        };

        auto isBadNodeAddr = [&](uint32_t addr) {
            if (addr == 0u || addr == sentinel)
            {
                return false;
            }

            if (ps2IsScratchpadAddress(addr))
            {
                return true;
            }

            if (addr >= kPs2RdramSize)
            {
                return true;
            }

            if (addr == 0xEEEEEEEEu || addr == 0x44444444u || addr == 0xAAAAAAAAu || addr == 0x3C800000u)
            {
                return true;
            }

            return false;
        };

        if (head == 0u || tail == 0u || isBadNodeAddr(head) || isBadNodeAddr(tail))
        {
            resetNisList("invalid-head-tail", (isBadNodeAddr(head) || head == 0u) ? head : tail);
            return;
        }

        uint32_t current = head;
        uint32_t walked = 0u;
        while (current != sentinel && walked < 32u)
        {
            if (isBadNodeAddr(current))
            {
                resetNisList("invalid-node", current);
                return;
            }

            const uint32_t next = readGuestU32Raw(rdram, current + 0x0u);
            const uint32_t prev = readGuestU32Raw(rdram, current + 0x4u);
            if (next == 0u || prev == 0u || isBadNodeAddr(next) || isBadNodeAddr(prev))
            {
                resetNisList("invalid-link", current);
                return;
            }

            const uint32_t resourceA = readGuestU32Raw(rdram, current + 0x14u);
            const uint32_t resourceB = readGuestU32Raw(rdram, current + 0x18u);
            const uint32_t carA = readGuestU32Raw(rdram, current + 0x48u);
            const uint32_t carB = readGuestU32Raw(rdram, current + 0x4Cu);
            const uint32_t anim = readGuestU32Raw(rdram, current + 0x80u);
            const uint32_t mat = readGuestU32Raw(rdram, current + 0xA0u);
            const uint32_t animRenderable = readGuestU32Raw(rdram, current + 0xB0u);
            const bool looksUnready = (resourceA == 0u && resourceB == 0u &&
                                       carA == 0u && carB == 0u &&
                                       anim == 0u && mat == 0u && animRenderable == 0u);
            if (looksUnready)
            {
                static uint32_t s_dropLogCount = 0u;
                if (s_dropLogCount < 12u)
                {
                    std::cerr << "[nfs-hp2-alpha] dropped unready NisManager"
                              << " mgr=0x" << std::hex << current
                              << " next=0x" << next
                              << " prev=0x" << prev
                              << std::dec << std::endl;
                    ++s_dropLogCount;
                }
                unlinkGuestListNode(rdram, current);
                current = next;
                ++walked;
                continue;
            }

            current = next;
            ++walked;
        }

        DrawNisObjects__FP5eView_0x186718(rdram, ctx, runtime);

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    bool looksLikeBadGuestPathPointer(uint32_t addr)
    {
        if (addr == 0u || addr == 0xAAAAAAAAu || addr == 0xEEEEEEEEu || addr == 0x44444444u)
        {
            return true;
        }

        if (ps2IsScratchpadAddress(addr))
        {
            return false;
        }

        return addr >= kPs2RdramSize;
    }

    void nfsHp2AlphaBeginLoadSound(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t soundAddr = getRegU32(ctx, 4);
        const uint32_t pathA = getRegU32(ctx, 5);
        const uint32_t pathB = getRegU32(ctx, 6);

        static uint32_t s_logCount = 0u;
        if (s_logCount < 12u)
        {
            std::cerr << "[nfs-hp2-alpha] BeginLoad__6bSound"
                      << " sound=0x" << std::hex << soundAddr
                      << " pathA=0x" << pathA
                      << " pathB=0x" << pathB
                      << " badA=" << (looksLikeBadGuestPathPointer(pathA) ? 1 : 0)
                      << " badB=" << (looksLikeBadGuestPathPointer(pathB) ? 1 : 0)
                      << " pathAText=\"" << readGuestCStringRaw(rdram, pathA, 0x80u) << "\""
                      << " pathBText=\"" << readGuestCStringRaw(rdram, pathB, 0x80u) << "\""
                      << std::dec << std::endl;
            ++s_logCount;
        }

        BeginLoad__6bSoundPcT1_0x20c760(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaOpenBaseSpeaker(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        const uint32_t entryPc = ctx->pc;
        const uint32_t thisAddr = getRegU32(ctx, 4);
        const uint32_t primaryPath = getRegU32(ctx, 5);
        const uint32_t secondaryPath = getRegU32(ctx, 6);

        static uint32_t s_logCount = 0u;
        if (s_logCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] Open__12cBaseSpeaker"
                      << " this=0x" << std::hex << thisAddr
                      << " primary=0x" << primaryPath
                      << " secondary=0x" << secondaryPath
                      << " badPrimary=" << (looksLikeBadGuestPathPointer(primaryPath) ? 1 : 0)
                      << " badSecondary=" << (looksLikeBadGuestPathPointer(secondaryPath) ? 1 : 0)
                      << " primaryText=\"" << readGuestCStringRaw(rdram, primaryPath, 0x80u) << "\""
                      << " secondaryText=\"" << readGuestCStringRaw(rdram, secondaryPath, 0x80u) << "\""
                      << std::dec << std::endl;
            ++s_logCount;
        }

        Open__12cBaseSpeakerPCcT1_0x217820(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaAttachTextureTable(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t packAddr = getRegU32(ctx, 4);
        const uint32_t tableAddr = getRegU32(ctx, 5);
        const int32_t entryCount = static_cast<int32_t>(getRegU32(ctx, 6));

        static uint32_t s_enterLogCount = 0u;
        if (s_enterLogCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] AttachTextureTable:enter"
                      << " pack=0x" << std::hex << packAddr
                      << " table=0x" << tableAddr
                      << " count=0x" << static_cast<uint32_t>(entryCount)
                      << " prePackTable=0x" << ((packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x60u) : 0u)
                      << " prePackCount=0x" << ((packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x64u) : 0u)
                      << " ra=0x" << getRegU32(ctx, 31)
                      << std::dec << std::endl;
            ++s_enterLogCount;
        }

        AttachTextureTable__11TexturePackP11TextureInfoi_0x1808e0(rdram, ctx, runtime);

        static uint32_t s_exitLogCount = 0u;
        if (s_exitLogCount < 20u)
        {
            std::cerr << "[nfs-hp2-alpha] AttachTextureTable:return"
                      << " pack=0x" << std::hex << packAddr
                      << " postPackTable=0x" << ((packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x60u) : 0u)
                      << " postPackCount=0x" << ((packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x64u) : 0u)
                      << " packBase5c=0x" << ((packAddr != 0u) ? readGuestU32Raw(rdram, packAddr + 0x5Cu) : 0u)
                      << " pc=0x" << ctx->pc
                      << std::dec << std::endl;
            ++s_exitLogCount;
        }

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaTexturePackCtor(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t thisAddr = getRegU32(ctx, 4);
        const uint32_t gp = getRegU32(ctx, 28);
        if (thisAddr == 0u || thisAddr >= kPs2RdramSize)
        {
            setReturnU32(ctx, thisAddr);
            ctx->pc = entryPc;
            return;
        }

        std::memset(rdram + (thisAddr & PS2_RAM_MASK), 0, 0x84u);

        const uint32_t texturePackListSentinel = gp - 0x5808u;
        const uint32_t loadedTextureListSentinel = thisAddr + 0x70u;
        const uint32_t listHead = readGuestU32Raw(rdram, texturePackListSentinel + 0x0u);
        const uint32_t listTail = readGuestU32Raw(rdram, texturePackListSentinel + 0x4u);
        if (listHead == 0u || listTail == 0u ||
            listHead >= kPs2RdramSize || listTail >= kPs2RdramSize)
        {
            initializeGuestListSentinel(rdram, texturePackListSentinel);
        }

        initializeGuestListSentinel(rdram, loadedTextureListSentinel);

        writeGuestU32Raw(rdram, thisAddr + 0x60u, 0u);
        writeGuestU32Raw(rdram, thisAddr + 0x64u, 0u);
        writeGuestU32Raw(rdram, thisAddr + 0x68u, 0u);
        writeGuestU32Raw(rdram, thisAddr + 0x78u, 0u);
        writeGuestU32Raw(rdram, thisAddr + 0x7Cu, 0u);
        writeGuestU32Raw(rdram, thisAddr + 0x80u, 0u);

        insertGuestNodeAtListHead(rdram, texturePackListSentinel, thisAddr);

        static uint32_t s_ctorLogCount = 0u;
        if (s_ctorLogCount < 12u)
        {
            std::cerr << "[nfs-hp2-alpha] TexturePack ctor"
                      << " this=0x" << std::hex << thisAddr
                      << " listSentinel=0x" << texturePackListSentinel
                      << " head=0x" << readGuestU32Raw(rdram, texturePackListSentinel + 0x0u)
                      << " tail=0x" << readGuestU32Raw(rdram, texturePackListSentinel + 0x4u)
                      << std::dec << std::endl;
            ++s_ctorLogCount;
        }

        setReturnU32(ctx, thisAddr);
        ctx->pc = entryPc;
    }

    void nfsHp2AlphaEFixUpTables(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t gp = getRegU32(ctx, 28);
        const uint32_t solidListSentinel = gp - 0x6B08u;
        uint32_t solidListHead = readGuestU32Raw(rdram, solidListSentinel + 0x0u);
        const uint32_t solidListTail = readGuestU32Raw(rdram, solidListSentinel + 0x4u);
        const uint32_t oldSolidListHead = solidListHead;

        static uint32_t s_fixupRepairLogCount = 0u;
        if (solidListHead == 0u || solidListTail == 0u)
        {
            initializeGuestListSentinel(rdram, solidListSentinel);
            solidListHead = solidListSentinel;

            if (s_fixupRepairLogCount < 6u)
            {
                std::cerr << "[nfs-hp2-alpha] repaired eSolid list sentinel"
                          << " sentinel=0x" << std::hex << solidListSentinel
                          << " oldHead=0x" << oldSolidListHead
                          << " oldTail=0x" << solidListTail
                          << std::dec << std::endl;
                ++s_fixupRepairLogCount;
            }
        }

        static uint32_t s_fixupDiagCount = 0u;
        if (s_fixupDiagCount < 6u)
        {
            std::unordered_set<uint32_t> visited;
            uint32_t current = solidListHead;
            uint32_t nodeCount = 0u;
            uint32_t repeatedNode = 0u;
            constexpr uint32_t kMaxNodesToWalk = 0x2000u;

            while (current != 0u && current != solidListSentinel && nodeCount < kMaxNodesToWalk)
            {
                if (current >= kPs2RdramSize || !visited.insert(current).second)
                {
                    repeatedNode = current;
                    break;
                }

                if (nodeCount < 6u)
                {
                    std::cerr << "[nfs-hp2-alpha] eFixUpTables solid"
                              << " idx=" << std::dec << nodeCount
                              << " node=0x" << std::hex << current
                              << " next=0x" << readGuestU32Raw(rdram, current + 0x0u)
                              << " stripCount=0x" << static_cast<uint32_t>(static_cast<uint16_t>(READ16(current + 0x34u)))
                              << " texCount=0x" << static_cast<uint32_t>(static_cast<uint16_t>(READ16(current + 0xACu)))
                              << " stripBase=0x" << readGuestU32Raw(rdram, current + 0xB0u)
                              << " texBase=0x" << readGuestU32Raw(rdram, current + 0xB8u)
                              << std::dec << std::endl;
                }

                current = readGuestU32Raw(rdram, current + 0x0u);
                ++nodeCount;
            }

            std::cerr << "[nfs-hp2-alpha] eFixUpTables list"
                      << " sentinel=0x" << std::hex << solidListSentinel
                      << " head=0x" << solidListHead
                      << " walked=0x" << nodeCount
                      << " repeated=0x" << repeatedNode
                      << " endedAt=0x" << current
                      << std::dec << std::endl;
            ++s_fixupDiagCount;
        }

        eFixUpTables__Fv_0x102f50(rdram, ctx, runtime);
        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaGetRenderTargetTextureInfo(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
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
        const uint32_t wantedTextureId = getRegU32(ctx, 4);

        static uint32_t s_logCount = 0u;
        if (s_logCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] eGetRenderTargetTextureInfo:enter"
                      << " wanted=0x" << std::hex << wantedTextureId
                      << " ids=[";
            for (uint32_t i = 0u; i < 13u; ++i)
            {
                const uint32_t renderTargetAddr = 0x0027ED80u + (i * 0x500u);
                const uint32_t renderTargetIndex = readGuestU32Raw(rdram, renderTargetAddr + 0x0u);
                const uint32_t textureInfoAddr = 0x00260270u + (renderTargetIndex * kTextureInfoStride);
                const uint32_t textureId = readGuestU32Raw(rdram, textureInfoAddr + 0x20u);
                if (i != 0u)
                {
                    std::cerr << ",";
                }
                std::cerr << "0x" << textureId;
            }
            std::cerr << "] ra=0x" << getRegU32(ctx, 31) << std::dec << std::endl;
        }

        eGetRenderTargetTextureInfo__Fi_0x104168(rdram, ctx, runtime);

        if (s_logCount < 16u)
        {
            std::cerr << "[nfs-hp2-alpha] eGetRenderTargetTextureInfo:return"
                      << " wanted=0x" << std::hex << wantedTextureId
                      << " result=0x" << getRegU32(ctx, 2)
                      << " pc=0x" << ctx->pc
                      << std::dec << std::endl;
            ++s_logCount;
        }

        if (ctx->pc == entryPc)
        {
            ctx->pc = entryPc;
        }
    }

    void nfsHp2AlphaPadReadOverride(uint8_t *rdram, R5900Context *ctx, PS2Runtime *runtime)
    {
        if (!rdram || !ctx)
        {
            return;
        }

        const int port = static_cast<int>(getRegU32(ctx, 4));
        const int slot = static_cast<int>(getRegU32(ctx, 5));
        const uint32_t dataAddr = getRegU32(ctx, 6);
        uint8_t *data = getMemPtr(rdram, dataAddr);

        static uint32_t s_callCount = 0;
        if (data && port == 0 && slot == 0)
        {
            std::memset(data, 0, 32);
            data[1] = 7;
            data[2] = 0x00;
            data[3] = 0x00;
            data[4] = 0x80;
            data[5] = 0x80;
            data[6] = 0x80;
            data[7] = 0x80;

            ++s_callCount;
            if ((s_callCount % 120) == 0)
            {
                std::cerr << "[nfs-hp2-alpha] PadReadOverride: all buttons pressed count=" << s_callCount << std::endl;
            }

            setReturnS32(ctx, 1);
            ctx->pc = getRegU32(ctx, 31);
            return;
        }

        ps2_stubs::scePadRead(rdram, ctx, runtime);
    }

    void applyNfsHp2AlphaOverrides(PS2Runtime &runtime)
    {
        runtime.registerFunction(0x21B198u, &nfsHp2AlphaPadReadOverride);
        std::cerr << "[nfs-hp2-alpha] pad override registered" << std::endl;
        runtime.registerFunction(0x102F50u, &nfsHp2AlphaEFixUpTables);
        runtime.registerFunction(0x104168u, &nfsHp2AlphaGetRenderTargetTextureInfo);
        runtime.registerFunction(0x106860u, &nfsHp2AlphaAddTexturesSorted);
        runtime.registerFunction(0x1094D0u, &nfsHp2AlphaReconnectAllModels);
        runtime.registerFunction(0x109050u, &nfsHp2AlphaReconnectTextureTable);
        runtime.registerFunction(0x10F1B8u, &nfsHp2AlphaSkipDebugInitHelper);
        runtime.registerFunction(0x11FC60u, &nfsHp2AlphaAfxUpdateTextures);
        runtime.registerFunction(0x20CF50u, &nfsHp2AlphaLoadCommonRegion);
        runtime.registerFunction(0x10F380u, &nfsHp2AlphaSkipDebugInitHelper);
        runtime.registerFunction(0x1027B8u, &nfsHp2AlphaDownloadVuCode);
        runtime.registerFunction(0x113670u, &nfsHp2AlphaGetLightMaterial);
        runtime.registerFunction(0x115618u, &nfsHp2AlphaCreateDepthIntoAlphaBuffer);
        runtime.registerFunction(0x136F10u, &nfsHp2AlphaSortFlailerObjectList);
        runtime.registerFunction(0x1F5680u, &nfsHp2AlphaSortBList);
        runtime.registerFunction(0x140098u, &nfsHp2AlphaGenerateEventsFromScanners);
        runtime.registerFunction(0x140208u, &nfsHp2AlphaFindEventNode);
        runtime.registerFunction(0x141678u, &nfsHp2AlphaFindScannerConfig);
        runtime.registerFunction(0x149408u, &nfsHp2AlphaRenderCameraMovers);
        runtime.registerFunction(0x149478u, &nfsHp2AlphaUpdateCameraMovers);
        runtime.registerFunction(0x170B08u, &nfsHp2AlphaUpdateCameraCoordinators);
        runtime.registerFunction(0x170B50u, &nfsHp2AlphaUpdateCameraCoordinatorsContinuation);
        runtime.registerFunction(0x170B54u, &nfsHp2AlphaUpdateCameraCoordinatorsContinuation);
        runtime.registerFunction(0x170B5Cu, &nfsHp2AlphaUpdateCameraCoordinatorsContinuation);
        runtime.registerFunction(0x17C520u, &nfsHp2AlphaReconnectAllAnimations);
        runtime.registerFunction(0x17CA70u, &nfsHp2AlphaFindAnimationSolid);
        runtime.registerFunction(0x180650u, &nfsHp2AlphaGetTextureInfo);
        runtime.registerFunction(0x180788u, &nfsHp2AlphaTexturePackCtor);
        runtime.registerFunction(0x1808E0u, &nfsHp2AlphaAttachTextureTable);
        runtime.registerFunction(0x180C18u, &nfsHp2AlphaLoadTextureTableFromFile);
        runtime.registerFunction(0x180B30u, &nfsHp2AlphaGetLoadedTexture);
        runtime.registerFunction(0x185B60u, &nfsHp2AlphaNisManagerCtor);
        runtime.registerFunction(0x185CF8u, &nfsHp2AlphaStartNisManager);
        runtime.registerFunction(0x185EE0u, &nfsHp2AlphaLoadNisManager);
        runtime.registerFunction(0x186228u, &nfsHp2AlphaStartAnimatingNisManager);
        runtime.registerFunction(0x1864A0u, &nfsHp2AlphaRenderNisManager);
        runtime.registerFunction(0x186718u, &nfsHp2AlphaDrawNisObjects);
        runtime.registerFunction(0x20C760u, &nfsHp2AlphaBeginLoadSound);
        runtime.registerFunction(0x217820u, &nfsHp2AlphaOpenBaseSpeaker);
        runtime.registerFunction(0x181D20u, &nfsHp2AlphaBeginReadQueuedFile);
        runtime.registerFunction(0x181E10u, &nfsHp2AlphaServiceQueuedFiles);
        runtime.registerFunction(0x1F94D8u, &nfsHp2AlphaBOpen);
        runtime.registerFunction(0x1F9590u, &nfsHp2AlphaBOpenSingleArg);
        runtime.registerFunction(0x1EEE08u, &nfsHp2AlphaInitMoviePlayerFile);
        runtime.registerFunction(0x1FB360u, &nfsHp2AlphaOpenPlatformFile);
        runtime.registerFunction(0x1FA888u, &nfsHp2AlphaLoadDirectory);
        runtime.registerFunction(0x1FA548u, &nfsHp2AlphaTempDirectoryEntryCtor);
        runtime.registerFunction(0x1F8AE8u, &nfsHp2AlphaServiceFileSystem);
        runtime.registerFunction(0x1FA6C0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA7A0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA7B0u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA804u, &nfsHp2AlphaRecurseDirectoryContinuation);
        runtime.registerFunction(0x1FA834u, &nfsHp2AlphaRecurseDirectoryContinuation);
        std::cout << "[game_overrides] alpha registrations ready"
                  << " eFixUpTables=0x102f50"
                  << " eGetRenderTargetTextureInfo=0x104168"
                  << " reconnectAllModels=0x1094d0"
                  << " reconnectTextureTable=0x109050"
                  << " afxUpdateTextures=0x11fc60"
                  << " loadCommonRegion=0x20cf50"
                  << " reconnectAllAnimations=0x17c520"
                  << " findAnimationSolid=0x17ca70"
                  << " getTextureInfo=0x180650"
                  << " texturePackCtor=0x180788"
                  << " attachTextureTable=0x1808e0"
                  << " loadTextureTable=0x180c18"
                  << " getLoadedTexture=0x180b30"
                  << " nisCtor=0x185b60"
                  << " nisStart=0x185cf8"
                  << " nisLoad=0x185ee0"
                  << " nisStartAnimating=0x186228"
                  << " drawNisObjects=0x186718"
                  << " serviceQueuedFiles=0x181e10"
                  << std::endl;
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
