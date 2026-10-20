#include "CPU.h"
#include <stdexcept>

// fixed-width hex text (for error messages)
static std::string hex(int v, int digits) {
    static const char* d = "0123456789ABCDEF";
    std::string s;
    for (int k = digits - 1; k >= 0; k--) s += d[(v >> (4 * k)) & 0xF];
    return s;
}

CPU::CPU(Memory& memory, Registers& registers)
    : mem(memory), regs(registers), halted(false) {}

bool CPU::isHalted() const { return halted; }

bool CPU::hasError() const { return !error.empty(); }

const std::string& CPU::getError() const { return error; }

const Instruction& CPU::getInstruction() const { return inst; }

void CPU::reset() {
    halted = false;
    error.clear();
    inst = Instruction();
}

// ---------- helpers ----------

// 24-bit two's complement -> signed int
int CPU::signed24(uint32_t v) {
    v &= 0xFFFFFF;
    return (v & 0x800000) ? (int)v - 0x1000000 : (int)v;
}

// sets CC: 0 = '<', 1 = '=', 2 = '>'
void CPU::compare(int a, int b) {
    if (a < b)       regs.setCC(0);
    else if (a == b) regs.setCC(1);
    else             regs.setCC(2);
}

// a jump to its own address ends the program (classic halt idiom)
void CPU::jump(int addr) {
    if (addr == inst.address) halted = true;
    regs.set(8, addr);
}

// stops the CPU with an error message
void CPU::fail(const std::string& msg) {
    error = msg;
    halted = true;
}

// records the memory operand touched by the current instruction (the GUI reads it)
void CPU::touch(int addr, int size, bool write) {
    inst.memAddr = addr;
    inst.memSize = size;
    inst.memWrite = write;
}

// ---------- decoding ----------

// format 2: [opcode 8] [r1 4 | r2 4]
void CPU::decode2() {
    int pc = inst.address;
    int b2 = mem.readByte(pc + 1);

    inst.format = 2;
    inst.size = 2;
    inst.r1 = b2 >> 4;
    inst.r2 = b2 & 0x0F;

    regs.set(8, pc + 2);
}

// format 3/4: [opcode 6 | n i] [x b p e | disp 4] [disp 8] ([disp 8] if e = 1)
void CPU::decode34() {
    int pc = inst.address;
    int b2 = mem.readByte(pc + 1);

    inst.n = (inst.b1 >> 1) & 1;
    inst.i = inst.b1 & 1;
    inst.x = (b2 >> 7) & 1;

    // standard SIC (n = 0, i = 0): x + 15-bit address (b, p, e are address bits)
    if (inst.n == 0 && inst.i == 0) {
        inst.format = 3;
        inst.size = 3;
        inst.disp = ((b2 & 0x7F) << 8) | mem.readByte(pc + 2);
        regs.set(8, pc + 3);

        int ta = inst.disp;
        if (inst.x) ta += regs.get(1);
        inst.ta = ta & 0xFFFFF;
        return;
    }

    inst.b = (b2 >> 6) & 1;
    inst.p = (b2 >> 5) & 1;
    inst.e = (b2 >> 4) & 1;

    if (inst.e) {   // format 4: 20-bit address
        inst.format = 4;
        inst.size = 4;
        inst.disp = ((b2 & 0x0F) << 16) | (mem.readByte(pc + 2) << 8) | mem.readByte(pc + 3);
    } else {        // format 3: 12-bit displacement
        inst.format = 3;
        inst.size = 3;
        inst.disp = ((b2 & 0x0F) << 8) | mem.readByte(pc + 2);
        if (inst.p && inst.disp >= 0x800) inst.disp -= 0x1000;   // two's complement
    }

    regs.set(8, pc + inst.size);   // PC now points to the next instruction

    int ta = inst.disp;
    if (inst.b) ta += regs.get(3);   // B + disp
    if (inst.p) ta += regs.get(8);   // PC + disp
    if (inst.x) ta += regs.get(1);   // + X
    inst.ta = ta & 0xFFFFF;
}

// address of the operand (resolves indirect)
int CPU::targetAddr() const {
    if (inst.n == 1 && inst.i == 0)   // indirect: memory holds the real address
        return mem.readWord(inst.ta);
    return inst.ta;
}

// value of the operand (resolves immediate / direct / indirect)
uint32_t CPU::wordOperand() {
    if (inst.n == 0 && inst.i == 1)   // immediate: ta itself is the value
        return inst.ta;
    int addr = targetAddr();
    touch(addr, 3, false);
    return mem.readWord(addr);
}

// stores at the operand address
void CPU::storeWord(uint32_t v) {
    int addr = targetAddr();
    touch(addr, 3, true);
    mem.writeWord(addr, v);
}

void CPU::storeByte(uint8_t v) {
    int addr = targetAddr();
    touch(addr, 1, true);
    mem.writeByte(addr, v);
}

// ---------- fetch / decode / execute ----------

// Public entry point: never throws. Any error stops the CPU and is kept in getError().
bool CPU::step() {
    if (halted) return false;
    try {
        return execute();
    } catch (const std::exception& ex) {
        regs.set(8, inst.address);   // PC points at the instruction that failed
        fail(ex.what());
        return false;
    }
}

// One cycle:
//   1. read byte1 at PC
//   2. opcode = byte1 & 0xFC (drops n and i; format 2 opcodes are unchanged)
//   3. the case decodes only the bytes it needs (decode2 / decode34), which also advances PC
//   4. the case executes
bool CPU::execute() {
    int pc = regs.get(8);
    inst = Instruction();
    inst.address = pc;
    inst.b1 = mem.readByte(pc);
    inst.op = inst.b1 & 0xFC;

    switch (inst.op) {

    // ===== format 2 =====
    case 0x90: // ADDR
        inst.name = "ADDR"; decode2();
        regs.set(inst.r2, regs.get(inst.r2) + regs.get(inst.r1));
        break;
    case 0x94: // SUBR
        inst.name = "SUBR"; decode2();
        regs.set(inst.r2, regs.get(inst.r2) - regs.get(inst.r1));
        break;
    case 0x98: { // MULR (long long: 24 x 24 bits does not fit in int)
        inst.name = "MULR"; decode2();
        long long prod = (long long)signed24(regs.get(inst.r2)) * signed24(regs.get(inst.r1));
        regs.set(inst.r2, (uint32_t)prod);
        break;
    }
    case 0x9C: // DIVR
        inst.name = "DIVR"; decode2();
        if (regs.get(inst.r1) == 0) throw std::runtime_error("Division by zero");
        regs.set(inst.r2, signed24(regs.get(inst.r2)) / signed24(regs.get(inst.r1)));
        break;
    case 0xA0: // COMPR
        inst.name = "COMPR"; decode2();
        compare(signed24(regs.get(inst.r1)), signed24(regs.get(inst.r2)));
        break;
    case 0xA4: { // SHIFTL: circular left by (r2 + 1) bits
        inst.name = "SHIFTL"; decode2();
        uint32_t v = regs.get(inst.r1);
        int k = inst.r2 + 1;
        regs.set(inst.r1, (v << k) | (v >> (24 - k)));
        break;
    }
    case 0xA8: // SHIFTR: arithmetic right by (r2 + 1) bits
        inst.name = "SHIFTR"; decode2();
        regs.set(inst.r1, signed24(regs.get(inst.r1)) >> (inst.r2 + 1));
        break;
    case 0xAC: // RMO
        inst.name = "RMO"; decode2();
        regs.set(inst.r2, regs.get(inst.r1));
        break;
    case 0xB4: // CLEAR
        inst.name = "CLEAR"; decode2();
        regs.set(inst.r1, 0);
        break;
    case 0xB8: // TIXR
        inst.name = "TIXR"; decode2();
        regs.set(1, regs.get(1) + 1);
        compare(signed24(regs.get(1)), signed24(regs.get(inst.r1)));
        break;

    // ===== format 3/4: loads =====
    case 0x00: inst.name = "LDA"; decode34(); regs.set(0, wordOperand()); break;
    case 0x68: inst.name = "LDB"; decode34(); regs.set(3, wordOperand()); break;
    case 0x08: inst.name = "LDL"; decode34(); regs.set(2, wordOperand()); break;
    case 0x6C: inst.name = "LDS"; decode34(); regs.set(4, wordOperand()); break;
    case 0x74: inst.name = "LDT"; decode34(); regs.set(5, wordOperand()); break;
    case 0x04: inst.name = "LDX"; decode34(); regs.set(1, wordOperand()); break;
    case 0x50: { // LDCH: rightmost byte of A <- 1 byte
        inst.name = "LDCH"; decode34();
        uint32_t ch;
        if (inst.n == 0 && inst.i == 1) {
            ch = inst.ta & 0xFF;                 // immediate
        } else {
            int addr = targetAddr();
            touch(addr, 1, false);
            ch = mem.readByte(addr);
        }
        regs.set(0, (regs.get(0) & 0xFFFF00) | ch);
        break;
    }

    // ===== format 3/4: stores =====
    case 0x0C: inst.name = "STA";  decode34(); storeWord(regs.get(0)); break;
    case 0x78: inst.name = "STB";  decode34(); storeWord(regs.get(3)); break;
    case 0x14: inst.name = "STL";  decode34(); storeWord(regs.get(2)); break;
    case 0x7C: inst.name = "STS";  decode34(); storeWord(regs.get(4)); break;
    case 0x84: inst.name = "STT";  decode34(); storeWord(regs.get(5)); break;
    case 0x10: inst.name = "STX";  decode34(); storeWord(regs.get(1)); break;
    case 0x54: inst.name = "STCH"; decode34(); storeByte(regs.get(0) & 0xFF); break;

    // ===== format 3/4: arithmetic / logic (result in A) =====
    case 0x18: inst.name = "ADD"; decode34(); regs.set(0, regs.get(0) + wordOperand()); break;
    case 0x1C: inst.name = "SUB"; decode34(); regs.set(0, regs.get(0) - wordOperand()); break;
    case 0x20: { // MUL (long long: 24 x 24 bits does not fit in int)
        inst.name = "MUL"; decode34();
        long long prod = (long long)signed24(regs.get(0)) * signed24(wordOperand());
        regs.set(0, (uint32_t)prod);
        break;
    }
    case 0x24: { // DIV
        inst.name = "DIV"; decode34();
        int d = signed24(wordOperand());
        if (d == 0) throw std::runtime_error("Division by zero");
        regs.set(0, signed24(regs.get(0)) / d);
        break;
    }
    case 0x40: inst.name = "AND"; decode34(); regs.set(0, regs.get(0) & wordOperand()); break;
    case 0x44: inst.name = "OR";  decode34(); regs.set(0, regs.get(0) | wordOperand()); break;

    // ===== format 3/4: compare / index =====
    case 0x28: // COMP
        inst.name = "COMP"; decode34();
        compare(signed24(regs.get(0)), signed24(wordOperand()));
        break;
    case 0x2C: // TIX
        inst.name = "TIX"; decode34();
        regs.set(1, regs.get(1) + 1);
        compare(signed24(regs.get(1)), signed24(wordOperand()));
        break;

    // ===== format 3/4: jumps =====
    case 0x3C: inst.name = "J";   decode34(); jump(targetAddr()); break;
    case 0x30: inst.name = "JEQ"; decode34(); if (regs.getCC() == 1) jump(targetAddr()); break;
    case 0x34: inst.name = "JGT"; decode34(); if (regs.getCC() == 2) jump(targetAddr()); break;
    case 0x38: inst.name = "JLT"; decode34(); if (regs.getCC() == 0) jump(targetAddr()); break;
    case 0x48: // JSUB
        inst.name = "JSUB"; decode34();
        regs.set(2, regs.get(8));   // L <- return address
        jump(targetAddr());
        break;
    case 0x4C: // RSUB (operand ignored)
        inst.name = "RSUB"; decode34();
        jump(regs.get(2));
        break;

    default:   // unknown or not implemented
        inst.name = "???";
        fail("Unknown opcode 0x" + hex(inst.b1, 2) + " at 0x" + hex(pc, 6));
        return false;
    }

    return true;
}
