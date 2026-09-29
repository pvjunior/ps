#include "Registers.h"
#include <stdexcept>

uint32_t Registers::get(int n) const {
    switch (n) {
        case 0: return A;
        case 1: return X;
        case 2: return L;
        case 3: return B;
        case 4: return S;
        case 5: return T;
        case 8: return PC;
        case 9: return SW;
        default: throw std::invalid_argument("Invalid register");
    }
}

void Registers::set(int n, uint32_t val) {
    val &= 0xFFFFFF;   // keep only 24 bits
    switch (n) {
        case 0: A = val; break;
        case 1: X = val; break;
        case 2: L = val; break;
        case 3: B = val; break;
        case 4: S = val; break;
        case 5: T = val; break;
        case 8: PC = val; break;
        case 9: SW = val; break;
        default: throw std::invalid_argument("Invalid register");
    }
}

uint64_t Registers::getF() const {
    return F;
}

void Registers::setF(uint64_t val) {
    F = val & 0xFFFFFFFFFFFFULL;   // keep only 48 bits
}

int Registers::getCC() const {
    //????
    return -1;
}

void Registers::setCC(int cc) {
    //????
    return;
}