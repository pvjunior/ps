#include "Memory.h"
#include <stdexcept>

Memory::Memory(int size) : dados(size, 0) {}

int Memory::size() const {
    return dados.size();
}

uint8_t Memory::readByte(int addr) const {
    if (addr < 0 || addr >= size())
        throw std::out_of_range("Invalid address.");
    return dados[addr];
}

void Memory::writeByte(int addr, uint8_t valor) {
    if (addr < 0 || addr >= size())
        throw std::out_of_range("Invalid address.");
    dados[addr] = valor;
}


uint32_t Memory::readWord(int addr) const {
    // big-endian
    return (readByte(addr) << 16) | (readByte(addr + 1) << 8) | readByte(addr + 2);
}

void Memory::writeWord(int addr, uint32_t val) {
    writeByte(addr,     (val >> 16) & 0xFF);
    writeByte(addr + 1, (val >> 8)  & 0xFF);
    writeByte(addr + 2,  val        & 0xFF);
    // Last byte is discarded.
}

uint64_t Memory::readFloat(int addr) const {
    uint64_t val = 0;
    // Read the following 6 bytes (48 bits) and store them to return
    for (int k = 0; k < 6; k++)
        val = (val << 8) | readByte(addr + k);
    return val;
}

void Memory::writeFloat(int addr, uint64_t val) {
    for (int k = 5; k >= 0; k--) {
        writeByte(addr + k, val & 0xFF);
        val >>= 8;
    }
}