#pragma once
#include <vector>
#include <cstdint>

class Memory {

private:
    std::vector<uint8_t> dados;


public:
    Memory(int size = 1024);

    int size() const;
    uint8_t readByte(int addr) const;
    void writeByte(int addr, uint8_t valor);

    uint32_t readWord(int addr) const;
    void writeWord(int addr, uint32_t val); // 3 Bytes

    uint64_t readFloat(int addr) const; // 6 Bytes
    void writeFloat(int addr, uint64_t val); 
};
