#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "Memory.h"
#include "Registers.h"
#include "CPU.h"
#include "Loader.h"

// Copy of all registers at one moment (safe for the GUI to keep)
struct RegisterSnapshot {
    uint32_t A = 0, X = 0, L = 0, B = 0, S = 0, T = 0, PC = 0, SW = 0;   // 24 bits
    uint64_t F = 0;                                                       // 48 bits
    int cc = 0;                                                           // 0 '<', 1 '=', 2 '>'
};

// The only class the GUI needs to know.
//   step(), reset(), loader()  -> change the machine
//   every const method         -> only observes it (never throws)
class Machine {
private:
    Memory mem;
    Registers regs;
    CPU cpu;
    Loader ldr;
    std::vector<int> changed;   // registers changed by the last step
    int steps;                  // instructions executed since reset

public:
    explicit Machine(int memorySize = 1024);
    Machine(const Machine&) = delete;              // the parts hold references to each other
    Machine& operator=(const Machine&) = delete;

    // ----- control -----
    bool step();                // never throws; false = halted or error
    void reset();               // registers + CPU back to the start (memory is kept)
    Loader& loader();           // insert code into memory

    // ----- observers: registers -----
    RegisterSnapshot getRegisters() const;
    char getCCSymbol() const;                              // '<', '=' or '>'
    const std::vector<int>& getChangedRegisters() const;   // changed by the last step (0-5, 6 = F, 8 = PC, 9 = SW)
    static std::string registerName(int n);                // 0 -> "A", 8 -> "PC", ...

    // ----- observers: memory -----
    int getMemorySize() const;
    uint8_t getByte(int addr) const;                       // 0 if out of range
    uint32_t getWord(int addr) const;                      // 3 bytes; 0 if out of range
    std::vector<uint8_t> dump(int start, int length) const;   // clamped to the memory limits

    // ----- observers: execution -----
    const Instruction& getLastInstruction() const;         // decode info of the last executed instruction
    int getStepCount() const;
    bool isHalted() const;                                 // true after "J *" or after an error
    bool hasError() const;
    const std::string& getError() const;
    const Loader& loader() const;                          // read-only: cursor()
};
