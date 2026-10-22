#include "Encoder.h"

#include "OpcodeTable.h"

#include <cctype>
#include <cstdint>
#include <string>
#include <utility>

namespace {

// Flags do nibble "xbpe" (segundo byte de instruções 3/4).
constexpr std::uint8_t kFlagX = 0x8;
constexpr std::uint8_t kFlagB = 0x4;
constexpr std::uint8_t kFlagP = 0x2;
constexpr std::uint8_t kFlagE = 0x1;

// Bits "ni" (dois bits baixos do primeiro byte de instruções 3/4).
constexpr std::uint8_t kNiSimple = 0x3;     // n=1 i=1
constexpr std::uint8_t kNiImmediate = 0x1;  // n=0 i=1
constexpr std::uint8_t kNiIndirect = 0x2;   // n=1 i=0

constexpr std::int64_t kMaxMagnitude = (static_cast<std::int64_t>(1) << 40);

struct RegisterEntry {
    const char* name;
    std::uint8_t number;
};

const RegisterEntry kRegisters[] = {
    {"A", 0}, {"X", 1}, {"L", 2}, {"B", 3}, {"S", 4},
    {"T", 5}, {"F", 6}, {"PC", 8}, {"SW", 9},
};

std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() &&
           std::isspace(static_cast<unsigned char>(text[first])) != 0) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) {
        --last;
    }
    return text.substr(first, last - first);
}

EncodeOutcome fail(const std::string& message) {
    EncodeOutcome outcome;
    outcome.ok = false;
    outcome.error = message;
    return outcome;
}

EncodeOutcome success(EncodedStatement statement) {
    EncodeOutcome outcome;
    outcome.ok = true;
    outcome.statement = std::move(statement);
    return outcome;
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Um token é número se começa com dígito, ou com sinal seguido de dígito.
bool looksNumeric(const std::string& text) {
    if (text.empty()) return false;
    if (isDigit(text[0])) return true;
    return (text[0] == '+' || text[0] == '-') && text.size() > 1 &&
           isDigit(text[1]);
}

// Inteiro decimal ou hexadecimal (prefixo 0x), com sinal opcional.
bool parseInteger(const std::string& text, std::int64_t& value) {
    std::size_t pos = 0;
    bool negative = false;
    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) {
        negative = text[pos] == '-';
        ++pos;
    }
    if (pos >= text.size()) return false;

    std::int64_t magnitude = 0;
    if (text.size() >= pos + 2 && text[pos] == '0' &&
        (text[pos + 1] == 'x' || text[pos + 1] == 'X')) {
        pos += 2;
        if (pos >= text.size()) return false;
        for (; pos < text.size(); ++pos) {
            const int digit = hexValue(text[pos]);
            if (digit < 0) return false;
            magnitude = magnitude * 16 + digit;
            if (magnitude > kMaxMagnitude) return false;
        }
    } else {
        for (; pos < text.size(); ++pos) {
            if (!isDigit(text[pos])) return false;
            magnitude = magnitude * 10 + (text[pos] - '0');
            if (magnitude > kMaxMagnitude) return false;
        }
    }
    value = negative ? -magnitude : magnitude;
    return true;
}

std::vector<std::string> splitOperands(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = text.find(',', start);
        if (comma == std::string::npos) {
            parts.push_back(trim(text.substr(start)));
            break;
        }
        parts.push_back(trim(text.substr(start, comma - start)));
        start = comma + 1;
    }
    return parts;
}

bool lookupRegister(const std::string& name, std::uint8_t& number) {
    const std::string normalized = normalizeIdentifier(name);
    for (const RegisterEntry& entry : kRegisters) {
        if (normalized == entry.name) {
            number = entry.number;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------- formato 2
// byte0 = opcode ; byte1 = (r1 << 4) | r2
EncodeOutcome encodeFormat2(const OpcodeInfo& info, const std::string& operand) {
    const std::vector<std::string> parts = splitOperands(trim(operand));
    const bool isShift = info.mnemonic == "SHIFTL" || info.mnemonic == "SHIFTR";

    if (trim(operand).empty()) {
        return fail("instruction '" + info.mnemonic + "' requires a register operand");
    }
    for (const std::string& part : parts) {
        if (part.empty()) {
            return fail("empty operand in '" + info.mnemonic + "'");
        }
    }

    std::uint8_t r1 = 0;
    std::uint8_t r2 = 0;

    if (isShift) {
        // SHIFTL/SHIFTR r1,n : n em 1..16 é codificado como n-1 (sem n => 1).
        if (parts.size() > 2) {
            return fail("'" + info.mnemonic + "' accepts a register and a shift count");
        }
        if (!lookupRegister(parts[0], r1)) {
            return fail("invalid register '" + parts[0] + "'");
        }
        if (parts.size() == 2) {
            std::int64_t count = 0;
            if (!parseInteger(parts[1], count) || count < 1 || count > 16) {
                return fail("shift count must be an integer from 1 to 16");
            }
            r2 = static_cast<std::uint8_t>(count - 1);
        }
    } else {
        if (parts.size() != info.registerOperandCount) {
            return fail("instruction '" + info.mnemonic + "' expects " +
                        std::to_string(info.registerOperandCount) +
                        " register operand(s), got " +
                        std::to_string(parts.size()));
        }
        if (!lookupRegister(parts[0], r1)) {
            return fail("invalid register '" + parts[0] + "'");
        }
        if (parts.size() == 2 && !lookupRegister(parts[1], r2)) {
            return fail("invalid register '" + parts[1] + "'");
        }
        // Com um único registrador, r2 permanece 0.
    }

    EncodedStatement statement;
    statement.bytes = {info.opcode,
                       static_cast<std::uint8_t>(((r1 & 0xF) << 4) | (r2 & 0xF))};
    return success(std::move(statement));
}

// ------------------------------------------------------------- formatos 3/4
EncodeOutcome encodeFormat34(const OpcodeInfo& info, bool extended,
                             const std::string& operand, std::uint32_t address,
                             const EncodeContext& context) {
    std::string text = trim(operand);
    std::uint8_t ni = kNiSimple;
    bool indexed = false;
    std::string target;

    if (text.empty()) {
        if (info.mnemonic != "RSUB") {
            return fail("instruction '" + info.mnemonic + "' requires an operand");
        }
        target = "0";  // RSUB: endereço/deslocamento zero
    } else {
        if (text.front() == '#') {
            ni = kNiImmediate;
            text = trim(text.substr(1));
        } else if (text.front() == '@') {
            ni = kNiIndirect;
            text = trim(text.substr(1));
        }

        const std::size_t comma = text.find(',');
        if (comma != std::string::npos) {
            const std::string suffix = normalizeIdentifier(trim(text.substr(comma + 1)));
            if (suffix != "X") {
                return fail("invalid index operand; only ',X' is allowed");
            }
            if (ni != kNiSimple) {
                return fail("indexed addressing cannot be combined with '#' or '@'");
            }
            indexed = true;
            text = trim(text.substr(0, comma));
        }
        if (text.empty()) {
            return fail("missing target in operand '" + trim(operand) + "'");
        }
        target = text;
    }

    static const SymbolTable kEmptySymbols;
    const SymbolTable& symbols =
        context.symbols != nullptr ? *context.symbols : kEmptySymbols;

    std::int64_t value = 0;
    bool isSymbol = false;
    std::string error;
    if (!evaluateValue(target, symbols, value, &isSymbol, error)) {
        return fail(error);
    }

    // nixbpe: x=1 se indexado; e=1 só no formato 4; b/p decididos abaixo.
    std::uint8_t xbpe = 0;
    if (indexed) xbpe |= kFlagX;
    if (extended) xbpe |= kFlagE;

    EncodedStatement statement;
    // byte0 = (opcode & 0xFC) | n<<1 | i  -> ni já vem pronto em `ni`.
    const std::uint8_t byte0 = static_cast<std::uint8_t>((info.opcode & 0xFC) | ni);

    if (extended) {
        // Formato 4: b=0, p=0, endereço de 20 bits.
        std::int64_t field = 0;
        if (isSymbol) {
            if (value < 0 || value > 0xFFFFF) {
                return fail("address of '" + target + "' does not fit in 20 bits");
            }
            field = value;
            statement.modification = ModificationEntry{address + 1, 5};
        } else {
            const std::int64_t minimum = ni == kNiImmediate ? -524288 : 0;
            if (value < minimum || value > 0xFFFFF) {
                return fail("constant " + std::to_string(value) +
                            " does not fit in the 20-bit address field");
            }
            field = value;
        }
        const std::uint32_t address20 = static_cast<std::uint32_t>(field) & 0xFFFFF;
        statement.bytes = {
            byte0,
            static_cast<std::uint8_t>((xbpe << 4) | ((address20 >> 16) & 0xF)),
            static_cast<std::uint8_t>((address20 >> 8) & 0xFF),
            static_cast<std::uint8_t>(address20 & 0xFF)};
        return success(std::move(statement));
    }

    // Formato 3: deslocamento de 12 bits.
    std::uint32_t disp12 = 0;
    if (!isSymbol) {
        // Constante: vai direto no campo (b=0, p=0).
        const std::int64_t minimum = ni == kNiImmediate ? -2048 : 0;
        if (value < minimum || value > 4095) {
            return fail("constant " + std::to_string(value) +
                        " does not fit in 12 bits; use format 4 (+" +
                        info.mnemonic + ")");
        }
        disp12 = static_cast<std::uint32_t>(value) & 0xFFF;
    } else {
        // 1) PC-relative: disp = alvo - (end + 3), em [-2048, 2047], p=1.
        const std::int64_t pcDisp = value - (static_cast<std::int64_t>(address) + 3);
        if (pcDisp >= -2048 && pcDisp <= 2047) {
            xbpe |= kFlagP;
            disp12 = static_cast<std::uint32_t>(pcDisp) & 0xFFF;  // compl. de 2
        } else if (context.base.has_value() &&
                   value - static_cast<std::int64_t>(*context.base) >= 0 &&
                   value - static_cast<std::int64_t>(*context.base) <= 4095) {
            // 2) Base-relative: disp = alvo - B, em [0, 4095], b=1.
            xbpe |= kFlagB;
            disp12 = static_cast<std::uint32_t>(value - *context.base) & 0xFFF;
        } else {
            std::string message = "target '" + target +
                                  "' is out of range for PC-relative";
            message += context.base.has_value()
                           ? " and base-relative addressing"
                           : " addressing (no BASE defined)";
            message += "; use format 4 (+" + info.mnemonic + ")";
            return fail(message);
        }
    }

    statement.bytes = {
        byte0,
        static_cast<std::uint8_t>((xbpe << 4) | ((disp12 >> 8) & 0xF)),
        static_cast<std::uint8_t>(disp12 & 0xFF)};
    return success(std::move(statement));
}

bool formatListContains(const OpcodeInfo& info, InstructionFormat format) {
    for (const InstructionFormat candidate : info.formats) {
        if (candidate == format) return true;
    }
    return false;
}

} // namespace

bool evaluateValue(const std::string& text, const SymbolTable& symbols,
                   std::int64_t& value, bool* isSymbol, std::string& error) {
    const std::string token = trim(text);
    if (token.empty()) {
        error = "missing operand value";
        return false;
    }
    if (looksNumeric(token)) {
        if (!parseInteger(token, value)) {
            error = "invalid numeric operand '" + token + "'";
            return false;
        }
        if (isSymbol != nullptr) *isSymbol = false;
        return true;
    }
    const std::string name = normalizeIdentifier(token);
    const auto it = symbols.find(name);
    if (it == symbols.end()) {
        error = "undefined symbol '" + name + "'";
        return false;
    }
    value = static_cast<std::int64_t>(it->second);
    if (isSymbol != nullptr) *isSymbol = true;
    return true;
}

EncodeOutcome encodeInstruction(const std::string& operation,
                                const std::string& operand,
                                std::uint32_t address,
                                const EncodeContext& context) {
    const bool extended = !operation.empty() && operation.front() == '+';
    const std::string mnemonic = normalizeIdentifier(
        extended ? operation.substr(1) : operation);

    const OpcodeInfo* info = OpcodeTable::find(mnemonic);
    if (info == nullptr) {
        return fail("unknown opcode '" + operation + "'");
    }

    if (formatListContains(*info, InstructionFormat::format1)) {
        if (extended) {
            return fail("format 1 instruction '" + mnemonic + "' cannot use '+'");
        }
        if (!trim(operand).empty()) {
            return fail("'" + mnemonic + "' does not accept an operand");
        }
        EncodedStatement statement;
        statement.bytes = {info->opcode};
        return success(std::move(statement));
    }

    if (formatListContains(*info, InstructionFormat::format2)) {
        if (extended) {
            return fail("format 2 instruction '" + mnemonic + "' cannot use '+'");
        }
        return encodeFormat2(*info, operand);
    }

    const InstructionFormat requested =
        extended ? InstructionFormat::format4 : InstructionFormat::format3;
    if (!formatListContains(*info, requested)) {
        return fail("instruction '" + operation + "' does not support this format");
    }
    return encodeFormat34(*info, extended, operand, address, context);
}

EncodeOutcome encodeByteDirective(const std::string& operand) {
    const std::string value = trim(operand);
    if (value.size() < 3 || value[1] != '\'' || value.back() != '\'') {
        return fail("invalid BYTE operand; expected C'...' or X'...'");
    }
    const char kind = static_cast<char>(
        std::toupper(static_cast<unsigned char>(value[0])));
    const std::string contents = value.substr(2, value.size() - 3);

    EncodedStatement statement;
    if (kind == 'C') {
        for (const char c : contents) {
            statement.bytes.push_back(static_cast<std::uint8_t>(c));
        }
    } else if (kind == 'X') {
        if (contents.empty() || contents.size() % 2 != 0) {
            return fail("X'...' requires an even number of hex digits");
        }
        for (std::size_t i = 0; i < contents.size(); i += 2) {
            const int high = hexValue(contents[i]);
            const int low = hexValue(contents[i + 1]);
            if (high < 0 || low < 0) {
                return fail("invalid hexadecimal digit in BYTE operand");
            }
            statement.bytes.push_back(static_cast<std::uint8_t>((high << 4) | low));
        }
    } else {
        return fail("invalid BYTE operand; expected C'...' or X'...'");
    }
    return success(std::move(statement));
}

EncodeOutcome encodeWordDirective(const std::string& operand) {
    std::int64_t value = 0;
    if (!parseInteger(trim(operand), value)) {
        return fail("invalid WORD operand");
    }
    // Aceita faixa com sinal (-2^23) ou sem sinal (2^24 - 1) de 24 bits.
    if (value < -8388608 || value > 16777215) {
        return fail("WORD value " + std::to_string(value) +
                    " does not fit in 24 bits");
    }
    const std::uint32_t word = static_cast<std::uint32_t>(value) & 0xFFFFFF;
    EncodedStatement statement;
    statement.bytes = {static_cast<std::uint8_t>((word >> 16) & 0xFF),
                       static_cast<std::uint8_t>((word >> 8) & 0xFF),
                       static_cast<std::uint8_t>(word & 0xFF)};
    return success(std::move(statement));
}
