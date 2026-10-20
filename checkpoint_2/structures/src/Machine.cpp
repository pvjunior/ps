#include "Machine.h"
#include <algorithm>

// register numbers that differ between two snapshots (CC lives in SW, so it shows up as 9)
static std::vector<int> diff(const RegisterSnapshot& a, const RegisterSnapshot& b) {
    std::vector<int> d;
    if (a.A  != b.A)  d.push_back(0);
    if (a.X  != b.X)  d.push_back(1);
    if (a.L  != b.L)  d.push_back(2);
    if (a.B  != b.B)  d.push_back(3);
    if (a.S  != b.S)  d.push_back(4);
    if (a.T  != b.T)  d.push_back(5);
    if (a.F  != b.F)  d.push_back(6);
    if (a.PC != b.PC) d.push_back(8);
    if (a.SW != b.SW) d.push_back(9);
    return d;
}

Machine::Machine(int memorySize)
    : mem(memorySize), regs(), cpu(mem, regs), ldr(mem, regs), steps(0) {}

// ---------- control ----------

bool Machine::step() {
    if (cpu.isHalted()) return false;   // keeps the info of the last step intact

    RegisterSnapshot before = getRegisters();
    bool ok = cpu.step();
    changed = diff(before, getRegisters());
    if (ok) steps++;
    return ok;
}

void Machine::reset() {
    regs = Registers();
    cpu.reset();
    ldr.cursorToPC();       // PC is 0 again
    changed.clear();
    steps = 0;
}

Loader& Machine::loader() { return ldr; }

// ---------- observers: registers ----------

RegisterSnapshot Machine::getRegisters() const {
    RegisterSnapshot s;
    s.A  = regs.get(0);
    s.X  = regs.get(1);
    s.L  = regs.get(2);
    s.B  = regs.get(3);
    s.S  = regs.get(4);
    s.T  = regs.get(5);
    s.PC = regs.get(8);
    s.SW = regs.get(9);
    s.F  = regs.getF();
    s.cc = regs.getCC();
    return s;
}

char Machine::getCCSymbol() const {
    switch (regs.getCC()) {
        case 0:  return '<';
        case 1:  return '=';
        default: return '>';
    }
}

const std::vector<int>& Machine::getChangedRegisters() const { return changed; }

std::string Machine::registerName(int n) {
    switch (n) {
        case 0: return "A";
        case 1: return "X";
        case 2: return "L";
        case 3: return "B";
        case 4: return "S";
        case 5: return "T";
        case 6: return "F";
        case 8: return "PC";
        case 9: return "SW";
        default: return "?";
    }
}

// ---------- observers: memory ----------

int Machine::getMemorySize() const { return mem.size(); }

uint8_t Machine::getByte(int addr) const {
    if (addr < 0 || addr >= mem.size()) return 0;
    return mem.readByte(addr);
}

uint32_t Machine::getWord(int addr) const {
    if (addr < 0 || addr + 2 >= mem.size()) return 0;
    return mem.readWord(addr);
}

std::vector<uint8_t> Machine::dump(int start, int length) const {
    std::vector<uint8_t> out;
    int from = std::max(start, 0);
    int to = std::min(start + length, mem.size());   // exclusive
    for (int a = from; a < to; a++) out.push_back(mem.readByte(a));
    return out;
}

// ---------- observers: execution ----------

const Instruction& Machine::getLastInstruction() const { return cpu.getInstruction(); }

int Machine::getStepCount() const { return steps; }

bool Machine::isHalted() const { return cpu.isHalted(); }

bool Machine::hasError() const { return cpu.hasError(); }

const std::string& Machine::getError() const { return cpu.getError(); }

const Loader& Machine::loader() const { return ldr; }
