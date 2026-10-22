#include "OpcodeTable.h"

#include "AssemblerTypes.h"

const std::vector<OpcodeInfo>& OpcodeTable::entries() {
    static const std::vector<OpcodeInfo> opcodeEntries = {
        {"ADD",    0x18, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"ADDR",   0x90, {InstructionFormat::format2}, 2},
        {"AND",    0x40, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"CLEAR",  0xB4, {InstructionFormat::format2}, 1},
        {"COMP",   0x28, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"COMPR",  0xA0, {InstructionFormat::format2}, 2},
        {"DIV",    0x24, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"DIVR",   0x9C, {InstructionFormat::format2}, 2},
        {"FIX",    0xC4, {InstructionFormat::format1}, 0},
        {"FLOAT",  0xC0, {InstructionFormat::format1}, 0},
        {"HIO",    0xF4, {InstructionFormat::format1}, 0},
        {"J",      0x3C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"JEQ",    0x30, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"JGT",    0x34, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"JLT",    0x38, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"JSUB",   0x48, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDA",    0x00, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDB",    0x68, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDCH",   0x50, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDL",    0x08, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDS",    0x6C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDT",    0x74, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"LDX",    0x04, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"MUL",    0x20, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"MULR",   0x98, {InstructionFormat::format2}, 2},
        {"NORM",   0xC8, {InstructionFormat::format1}, 0},
        {"OR",     0x44, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"RMO",    0xAC, {InstructionFormat::format2}, 2},
        {"RSUB",   0x4C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"SHIFTL", 0xA4, {InstructionFormat::format2}, 1},
        {"SHIFTR", 0xA8, {InstructionFormat::format2}, 1},
        {"SIO",    0xF0, {InstructionFormat::format1}, 0},
        {"STA",    0x0C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STB",    0x78, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STCH",   0x54, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STL",    0x14, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STS",    0x7C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STT",    0x84, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"STX",    0x10, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"SUB",    0x1C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"SUBR",   0x94, {InstructionFormat::format2}, 2},
        {"TIX",    0x2C, {InstructionFormat::format3, InstructionFormat::format4}, 0},
        {"TIO",    0xF8, {InstructionFormat::format1}, 0},
        {"TIXR",   0xB8, {InstructionFormat::format2}, 1},
    };
    return opcodeEntries;
}

const OpcodeInfo* OpcodeTable::find(const std::string& mnemonic) {
    const std::string normalized = normalizeIdentifier(mnemonic);
    for (const OpcodeInfo& opcode : entries()) {
        if (opcode.mnemonic == normalized) {
            return &opcode;
        }
    }
    return nullptr;
}
