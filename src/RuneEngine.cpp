#include "RuneEngine.h"
#include "MemoryScan.h"
#include "Logger.h"
#include "Config.h"
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <sstream>

namespace {

    // ---------------------------------------------------------------------
    // Reconstructed byte-for-byte from RunesClock25.dll (FUN_180002770,
    // see ../reverse_engineering/RunesClock25_decompiled.txt around the
    // "uStack_178 = 0x8b0048..." constants). Those constants are a hand
    // built 18-byte AOB pattern that Ghidra folded into a handful of
    // integer stack stores; decoding them byte-by-byte (little endian,
    // 0xFFFF-per-slot = wildcard) gives the disassembly below:
    //
    //   48 8B 05 ?? ?? ?? ??   mov  rax, [rip+disp32]   ; static instance ptr
    //   48 85 C0                test rax, rax
    //   74 05                    jz   +5
    //   48 8B 40 58              mov  rax, [rax+0x58]    ; sub-object accessor
    //   C3                       ret
    //   C3                       ret                     ; (padding/next stub)
    //
    // This is a small "getter" function's machine code, not data - scanning
    // for it and resolving its RIP-relative operand gives the address of a
    // module-scope pointer slot. FromSoftware's Souls-engine games shift
    // internal struct offsets between patches, so OFFSET_1/OFFSET_2 below
    // should be re-verified against the game's current version (e.g. via
    // Cheat Engine "Find out what writes to this address" on the live rune
    // counter) before trusting this in a released build. If the pattern or
    // offsets are stale, ResolveRunePointer() just logs an error each tick
    // and retries - it will not crash the game.
    // ---------------------------------------------------------------------
    const char* GAMEDATAMAN_PATTERN =
        "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 05 48 8B 40 58 C3 C3";
    constexpr int GAMEDATAMAN_RIP_OPERAND_OFFSET = 3;   // offset of the rel32 inside "48 8B 05 xx xx xx xx"
    constexpr int GAMEDATAMAN_INSTRUCTION_LENGTH = 7;   // length of that mov instruction

    // Pointer chain observed after the pattern match in FUN_180002770:
    //   P        = ResolveRip(match)      // address of the static ptr slot (NOT dereferenced yet)
    //   step1    = P + OFFSET_1           // pure pointer arithmetic
    //   val      = *(int64*)step1         // one dereference
    //   runeAddr = val + OFFSET_2         // final address, used directly as int*
    constexpr int OFFSET_1 = 0x8;
    constexpr int OFFSET_2 = 0x6C;

    int* ResolveRunePointer() {
        HMODULE gameModule = GetModuleHandleA(nullptr);
        uint8_t* insn = MemoryScan::FindPatternInModule(gameModule, GAMEDATAMAN_PATTERN);
        if (!insn) {
            Logger::Instance().Log("ERROR: GameDataMan pattern not found. Update GAMEDATAMAN_PATTERN in RuneEngine.cpp.");
            return nullptr;
        }

        uint8_t* ripSlot = MemoryScan::ResolveRip(insn, GAMEDATAMAN_RIP_OPERAND_OFFSET, GAMEDATAMAN_INSTRUCTION_LENGTH);

        // "mov rax, [rip+disp32]" dereferences the resolved address itself.
        auto p = *reinterpret_cast<uint8_t**>(ripSlot);
        if (!p) {
            Logger::Instance().Log("ERROR: GameDataMan pointer is null (not in-game yet?).");
            return nullptr;
        }

        uint8_t* step1 = p + OFFSET_1;
        auto val = *reinterpret_cast<uint8_t**>(step1);
        if (!val) {
            Logger::Instance().Log("ERROR: intermediate pointer is null (not in-game yet?).");
            return nullptr;
        }

        int* runeAddress = reinterpret_cast<int*>(val + OFFSET_2);

        Logger::Instance().Log("GameDataMan address: 0x" + std::to_string(reinterpret_cast<uintptr_t>(p)));
        Logger::Instance().Log("Runes address: 0x" + std::to_string(reinterpret_cast<uintptr_t>(runeAddress)));
        return runeAddress;
    }

    // Bonus granted the first time the mod has been active for this many
    // seconds. Defaults match the milestones observed in the original binary;
    // overridable via the "Milestones" key in PassiveRunes.ini as a
    // comma-separated list of "seconds:bonus" pairs.
    struct Milestone { int atSeconds; int bonus; };

    const char* DEFAULT_MILESTONES =
        "1800:5000,3600:10000,7200:25000,10800:40000,21600:100000,43200:500000,86400:1000000";

    std::vector<Milestone> ParseMilestones(const std::string& spec) {
        std::vector<Milestone> result;
        std::istringstream entries(spec);
        std::string entry;
        while (std::getline(entries, entry, ',')) {
            size_t colon = entry.find(':');
            if (colon == std::string::npos) continue;
            try {
                int atSeconds = std::stoi(entry.substr(0, colon));
                int bonus = std::stoi(entry.substr(colon + 1));
                result.push_back({ atSeconds, bonus });
            } catch (...) {
            }
        }
        return result;
    }
}

namespace RuneEngine {

    DWORD WINAPI Run(LPVOID hModuleRaw) {
        HMODULE hModule = static_cast<HMODULE>(hModuleRaw);

        char moduleDir[MAX_PATH];
        GetModuleFileNameA(hModule, moduleDir, MAX_PATH);
        std::string path(moduleDir);
        std::string dir = path.substr(0, path.find_last_of('\\'));

        Config::Instance().Load(dir + "\\PassiveRunes.ini");
        Logger::Instance().Init(dir, Config::Instance().GetBool("EnableLog", false));
        Logger::Instance().Log("Activating PassiveRunes...");

        const int intervalSeconds = Config::Instance().GetInt("IntervalSeconds", 5);
        const int runesPerInterval = Config::Instance().GetInt("RunesPerInterval", 25);
        const bool enableMilestones = Config::Instance().GetBool("EnableMilestones", true);
        const std::vector<Milestone> milestones =
            ParseMilestones(Config::Instance().GetString("Milestones", DEFAULT_MILESTONES));

        int* runeAddress = nullptr;
        long long elapsed = 0;

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));

            if (!runeAddress) {
                runeAddress = ResolveRunePointer();
                if (!runeAddress) continue;
            }

            *runeAddress += runesPerInterval;
            elapsed += intervalSeconds;

            if (enableMilestones) {
                for (const auto& m : milestones) {
                    if (elapsed == m.atSeconds) {
                        *runeAddress += m.bonus;
                        Logger::Instance().Log("Milestone bonus +" + std::to_string(m.bonus) +
                            " at " + std::to_string(m.atSeconds) + "s");
                    }
                }
            }

            Logger::Instance().Log("Current runes: " + std::to_string(*runeAddress) +
                " | elapsed: " + std::to_string(elapsed) + "s");
        }
    }
}
