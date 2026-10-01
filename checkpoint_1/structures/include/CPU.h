#pragma once
#include <cstdint>
#include <string>
#include "Memory.h"
#include "Registers.h"

// Filled by the CPU at each step; the GUI only reads it
struct Instruction {
    int address = 0;      // where it was fetched
    int b1 = 0;           // first byte
    int op = 0;           // opcode
    std::string name;
    int format = 0;       // 2, 3 or 4
    int size = 0;         // in bytes
    int n = 0, i = 0, x = 0, b = 0, p = 0, e = 0;   // formats 3/4
    int r1 = 0, r2 = 0;   // format 2
    int disp = 0;         // displacement / address field
    int ta = 0;           // target address (before resolving indirect)

    // memory operand touched by the instruction (memSize == 0 -> none)
    int memAddr = 0;
    int memSize = 0;          // 1 (byte) or 3 (word)
    bool memWrite = false;    // true = store, false = load
};

class CPU {
private:
    Memory& mem;
    Registers& regs;
    Instruction inst;
    bool halted;
    std::string error;

    void decode2();                  // reads r1, r2
    void decode34();                 // reads flags, disp and computes ta
    int targetAddr() const;          // address of the operand (resolves indirect)
    uint32_t wordOperand();          // value of the operand (immediate / direct / indirect)
    void storeWord(uint32_t v);      // writes a word at the operand address
    void storeByte(uint8_t v);       // writes a byte at the operand address
    void touch(int addr, int size, bool write);   // records the memory access in inst
    void compare(int a, int b);      // sets CC
    void jump(int addr);
    void fail(const std::string& msg);            // stops the CPU with an error
    bool execute();                  // one instruction; may throw

    static int signed24(uint32_t v);

public:
    CPU(Memory& memory, Registers& registers);

    bool step();                     // never throws; false = halted or error
    bool isHalted() const;           // true after "J *" or after an error
    bool hasError() const;
    const std::string& getError() const;
    void reset();
    const Instruction& getInstruction() const;
};
