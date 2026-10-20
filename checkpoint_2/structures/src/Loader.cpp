#include "Loader.h"

// value of one hex digit, or -1 if it is not one
static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

Loader::Loader(Memory& memory, const Registers& registers)
    : mem(memory), regs(registers), cur(0) {}

int Loader::cursor() const { return cur; }

bool Loader::setCursor(int addr) {
    if (addr < 0 || addr > mem.size()) return false;   // mem.size() itself = "end of memory"
    cur = addr;
    return true;
}

bool Loader::cursorToPC() {
    return setCursor((int)regs.get(8));
}

LoadResult Loader::write(int addr, const std::vector<uint8_t>& bytes) {
    LoadResult r;
    r.address = addr;
    if (bytes.empty()) {
        r.error = "Nothing to insert";
        return r;
    }
    if (addr < 0 || addr + (int)bytes.size() > mem.size()) {   // checked first: all or nothing
        r.error = "Does not fit in memory";
        return r;
    }
    for (size_t k = 0; k < bytes.size(); k++)
        mem.writeByte(addr + (int)k, bytes[k]);
    r.ok = true;
    r.count = (int)bytes.size();
    return r;
}

LoadResult Loader::insert(const std::vector<uint8_t>& bytes) {
    LoadResult r = write(cur, bytes);
    if (r.ok) cur += r.count;
    return r;
}

LoadResult Loader::insertHex(const std::string& text) {
    std::vector<uint8_t> bytes;
    std::string err;
    if (!parseHex(text, bytes, err)) {
        LoadResult r;
        r.address = cur;
        r.error = err;
        return r;
    }
    return insert(bytes);
}

void Loader::clear() {
    for (int a = 0; a < mem.size(); a++) mem.writeByte(a, 0);
    cur = 0;
}

bool Loader::parseHex(const std::string& text, std::vector<uint8_t>& out, std::string& error) {
    out.clear();
    std::string digits;
    for (size_t k = 0; k < text.size(); k++) {
        char c = text[k];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',') continue;   // separators
        if (c == '0' && k + 1 < text.size() && (text[k + 1] == 'x' || text[k + 1] == 'X')) {
            k++;                                                                     // skip "0x"
            continue;
        }
        if (hexValue(c) < 0) {
            error = std::string("Invalid character '") + c + "'";
            return false;
        }
        digits += c;
    }
    if (digits.empty()) {
        error = "Nothing to insert";
        return false;
    }
    if (digits.size() % 2 != 0) {
        error = "Odd number of hex digits";
        return false;
    }
    for (size_t k = 0; k < digits.size(); k += 2)
        out.push_back((uint8_t)(hexValue(digits[k]) * 16 + hexValue(digits[k + 1])));
    return true;
}
