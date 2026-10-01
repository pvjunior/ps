#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Memory.h"
#include "Registers.h"

// Result of an insertion. ok == false means nothing was written.
struct LoadResult {
    bool ok = false;
    int address = 0;        // where the first byte went
    int count = 0;          // bytes written
    std::string error;
};

// Puts bytes into memory (there is no assembler yet).
// It keeps an insertion cursor: every insert writes at the cursor and moves it forward,
// so instructions can be typed one after another.
class Loader {
private:
    Memory& mem;
    const Registers& regs;   // read-only: only used to find the PC
    int cur;                 // where the next insertion goes

public:
    Loader(Memory& memory, const Registers& registers);

    int cursor() const;
    bool setCursor(int addr);      // false if outside memory (cursor unchanged)
    bool cursorToPC();             // cursor = PC (false if the PC is outside memory)

    LoadResult write(int addr, const std::vector<uint8_t>& bytes);   // at addr; cursor unchanged
    LoadResult insert(const std::vector<uint8_t>& bytes);            // at cursor; cursor moves forward
    LoadResult insertHex(const std::string& text);                   // e.g. "01 00 05"

    void clear();                  // zero all memory, cursor = 0

    // accepts "01 00 05", "010005" and "0x01, 0x00, 0x05"
    static bool parseHex(const std::string& text, std::vector<uint8_t>& out, std::string& error);
};
