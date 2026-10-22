#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class InstructionFormat : std::uint8_t {
    format1 = 1,
    format2 = 2,
    format3 = 3,
    format4 = 4
};

struct OpcodeInfo {
    std::string mnemonic;
    std::uint8_t opcode = 0;
    std::vector<InstructionFormat> formats;
    std::uint8_t registerOperandCount = 0;
};

class OpcodeTable {
public:
    static const OpcodeInfo* find(const std::string& mnemonic);
    static const std::vector<OpcodeInfo>& entries();
};
