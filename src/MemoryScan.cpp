#include "MemoryScan.h"
#include <sstream>

namespace MemoryScan {

    std::vector<PatternByte> ParsePattern(const std::string& pattern) {
        std::vector<PatternByte> result;
        std::istringstream iss(pattern);
        std::string token;
        while (iss >> token) {
            if (token == "?" || token == "??") {
                result.push_back({ 0, true });
            } else {
                uint8_t value = static_cast<uint8_t>(std::stoul(token, nullptr, 16));
                result.push_back({ value, false });
            }
        }
        return result;
    }

    uint8_t* FindPattern(uint8_t* start, size_t size, const std::string& pattern) {
        std::vector<PatternByte> bytes = ParsePattern(pattern);
        if (bytes.empty() || size < bytes.size()) return nullptr;

        const size_t last = size - bytes.size();
        for (size_t i = 0; i <= last; i++) {
            bool found = true;
            for (size_t j = 0; j < bytes.size(); j++) {
                if (!bytes[j].wildcard && start[i + j] != bytes[j].value) {
                    found = false;
                    break;
                }
            }
            if (found) return start + i;
        }
        return nullptr;
    }

    uint8_t* FindPatternInModule(HMODULE module, const std::string& pattern) {
        auto base = reinterpret_cast<uint8_t*>(module);
        auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
        auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dos->e_lfanew);

        auto section = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
            // Only scan executable sections (.text and similar).
            if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                uint8_t* sectionStart = base + section->VirtualAddress;
                size_t sectionSize = section->Misc.VirtualSize;
                uint8_t* found = FindPattern(sectionStart, sectionSize, pattern);
                if (found) return found;
            }
        }
        return nullptr;
    }

    uint8_t* ResolveRip(uint8_t* instructionAddress, int operandOffset, int instructionLength) {
        int32_t rel = *reinterpret_cast<int32_t*>(instructionAddress + operandOffset);
        return instructionAddress + instructionLength + rel;
    }
}
