#pragma once
#include <cstdint>

class Registers {
private:
    uint32_t A, X, L, B, S, T, PC, SW;   // 24 bits each
    uint64_t F;                          // 48 bits

public:
    Registers();

    uint32_t get(int n) const;
    void set(int n, uint32_t val);

    uint64_t getF() const;
    void setF(uint64_t val);

    int getCC() const;       // 0 = '<', 1 = '=', 2 = '>'
    void setCC(int cc);
};