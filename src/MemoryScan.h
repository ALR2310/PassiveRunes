#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

// Generic IDA-style AOB (array-of-bytes) pattern scanner.
// Pattern example: "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 07"
// '??' or '?' marks a wildcard byte.
namespace MemoryScan {

    struct PatternByte {
        uint8_t value;
        bool wildcard;
    };

    std::vector<PatternByte> ParsePattern(const std::string& pattern);

    // Scans [start, start+size) for the given pattern. Returns absolute address of the
    // first match, or nullptr if not found.
    uint8_t* FindPattern(uint8_t* start, size_t size, const std::string& pattern);

    // Scans the .text section of the given module for the pattern.
    uint8_t* FindPatternInModule(HMODULE module, const std::string& pattern);

    // Resolves a RIP-relative instruction operand at `instructionAddress` into an
    // absolute address. `instructionLength` is the total length of the instruction,
    // `operandOffset` is where the 4-byte relative displacement starts.
    uint8_t* ResolveRip(uint8_t* instructionAddress, int operandOffset, int instructionLength);
}
