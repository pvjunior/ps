#include <iostream>
#include "Machine.h"

int main() {
    Machine m;

    // no assembler yet: type the bytes. Each insert goes right after the previous one.
    m.loader().insertHex("01 00 05");   // LDA #5
    m.loader().insertHex("19 00 03");   // ADD #3
    m.loader().insertHex("0F 01 00");   // STA 0x100
    m.loader().insertHex("3F 2F FD");   // J *

    while (m.step()) {
        RegisterSnapshot r = m.getRegisters();
        std::cout << m.getLastInstruction().name << "  A=" << std::hex << r.A
                  << "  PC=" << r.PC << std::dec << "\n";
    }

    std::cout << "mem[0x100] = " << m.getWord(0x100) << "\n";
    std::cout << (m.hasError() ? m.getError() : std::string("program ended")) << "\n";
}
