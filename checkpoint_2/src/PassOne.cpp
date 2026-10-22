#include "PassOne.h"

#include "OpcodeTable.h"

#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

namespace {

constexpr std::uint32_t kMaximumAddress = 0xFFFFFF;

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

void addDiagnostic(PassOneResult& result, const SourceLocation& location,
                   const std::string& message) {
    result.diagnostics.push_back(Diagnostic{location, message});
}

bool parseUnsignedDecimal(const std::string& text, std::uint64_t& value) {
    if (text.empty()) {
        return false;
    }

    value = 0;
    for (const char character : text) {
        if (character < '0' || character > '9') {
            return false;
        }
        const std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }
    return true;
}

bool parseHexAddress(const std::string& text, std::uint32_t& value) {
    if (text.empty() || text.size() > 6) {
        return false;
    }

    std::uint32_t parsed = 0;
    for (const char character : text) {
        unsigned digit = 0;
        if (character >= '0' && character <= '9') {
            digit = static_cast<unsigned>(character - '0');
        } else if (character >= 'A' && character <= 'F') {
            digit = static_cast<unsigned>(character - 'A' + 10);
        } else if (character >= 'a' && character <= 'f') {
            digit = static_cast<unsigned>(character - 'a' + 10);
        } else {
            return false;
        }
        parsed = static_cast<std::uint32_t>((parsed << 4) | digit);
    }

    value = parsed;
    return true;
}

bool isHexDigit(char character) {
    return (character >= '0' && character <= '9') ||
           (character >= 'A' && character <= 'F') ||
           (character >= 'a' && character <= 'f');
}

bool parseByteOperand(const std::string& operand, std::uint32_t& size) {
    const std::string value = trim(operand);
    if (value.size() < 3 || value[1] != '\'' || value.back() != '\'') {
        return false;
    }

    const char kind = static_cast<char>(
        std::toupper(static_cast<unsigned char>(value[0])));
    const std::string contents = value.substr(2, value.size() - 3);
    if (kind == 'C') {
        if (contents.size() > kMaximumAddress) {
            return false;
        }
        size = static_cast<std::uint32_t>(contents.size());
        return true;
    }
    if (kind == 'X') {
        if (contents.empty() || contents.size() % 2 != 0 ||
            contents.size() / 2 > kMaximumAddress) {
            return false;
        }
        for (const char character : contents) {
            if (!isHexDigit(character)) {
                return false;
            }
        }
        size = static_cast<std::uint32_t>(contents.size() / 2);
        return true;
    }
    return false;
}

bool isValidWordOperand(const std::string& operand) {
    const std::string value = trim(operand);
    if (value.empty()) {
        return false;
    }

    // WORD constants may be decimal or hexadecimal (0x-prefixed) literals.
    std::size_t firstDigit = 0;
    if (value[0] == '+' || value[0] == '-') {
        firstDigit = 1;
    }
    if (firstDigit == value.size()) {
        return false;
    }

    if (value.size() >= firstDigit + 2 && value[firstDigit] == '0' &&
        (value[firstDigit + 1] == 'x' || value[firstDigit + 1] == 'X')) {
        if (firstDigit + 2 == value.size()) {
            return false;
        }
        for (std::size_t i = firstDigit + 2; i < value.size(); ++i) {
            if (!isHexDigit(value[i])) {
                return false;
            }
        }
        return true;
    }

    for (std::size_t i = firstDigit; i < value.size(); ++i) {
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }
    return true;
}

bool isDirective(const std::string& operation) {
    return operation == "START" || operation == "END" ||
           operation == "BYTE" || operation == "WORD" ||
           operation == "RESB" || operation == "RESW" ||
           operation == "BASE";
}

bool addLabel(PassOneResult& result, const ParsedLine& line,
              std::uint32_t location) {
    if (!line.label.has_value() || line.label->empty()) {
        return true;
    }

    const std::string symbol = normalizeIdentifier(*line.label);
    if (!result.symbols.emplace(symbol, location).second) {
        addDiagnostic(result, line.location, "duplicate symbol '" + symbol + "'");
        return false;
    }
    return true;
}

bool getInstructionSize(const std::string& operation, const std::string& operand,
                        std::uint32_t& size, std::string& error) {
    const bool extended = !operation.empty() && operation.front() == '+';
    const std::string mnemonic = extended ? operation.substr(1) : operation;
    const OpcodeInfo* info = OpcodeTable::find(mnemonic);
    if (info == nullptr) {
        error = "unknown opcode or directive '" + operation + "'";
        return false;
    }

    const InstructionFormat requested = extended ? InstructionFormat::format4
                                                 : info->formats.front();
    bool formatSupported = false;
    for (const InstructionFormat format : info->formats) {
        if (format == requested) {
            formatSupported = true;
            break;
        }
    }
    if (!formatSupported) {
        error = "instruction '" + operation + "' does not support this format";
        return false;
    }

    size = static_cast<std::uint32_t>(requested);
    const std::string value = trim(operand);
    if (requested == InstructionFormat::format1 || mnemonic == "RSUB") {
        if (!value.empty()) {
            error = mnemonic + " does not accept an operand";
            return false;
        }
    } else if (value.empty()) {
        error = "instruction '" + operation + "' requires an operand";
        return false;
    }
    return true;
}

} // namespace

PassOneResult runPassOne(const std::vector<ParsedLine>& lines) {
    PassOneResult result;
    std::uint32_t locationCounter = 0;
    bool sawStart = false;
    bool sawStatement = false;
    bool ended = false;

    for (const ParsedLine& line : lines) {
        const std::string operation = normalizeIdentifier(trim(line.operation));
        if (ended) {
            addDiagnostic(result, line.location,
                          "source line appears after END directive");
            continue;
        }

        // Keep blank and comment lines in the intermediate stream for source
        // correspondence; they do not affect the location counter.
        if (operation.empty()) {
            result.intermediateLines.push_back(
                IntermediateLine{line, locationCounter, 0});
            continue;
        }

        if (operation == "START") {
            if (sawStart || sawStatement) {
                addDiagnostic(result, line.location,
                              "START must appear once before other statements");
            }
            std::uint32_t startAddress = 0;
            const std::string operand = trim(line.operand);
            if (!parseHexAddress(operand, startAddress)) {
                addDiagnostic(result, line.location,
                              "START requires a hexadecimal address from 0 to FFFFFF");
            } else {
                locationCounter = startAddress;
                result.startAddress = startAddress;
            }
            sawStart = true;
            addLabel(result, line, locationCounter);
            result.intermediateLines.push_back(
                IntermediateLine{line, locationCounter, 0});
            continue;
        }

        sawStatement = true;
        const std::uint32_t lineAddress = locationCounter;
        addLabel(result, line, lineAddress);

        std::uint32_t size = 0;
        bool valid = true;
        if (operation == "END") {
            const std::string operand = trim(line.operand);
            if (!operand.empty() && operand.find_first_of(" \t\r\n") != std::string::npos) {
                addDiagnostic(result, line.location, "invalid END operand");
            }
            ended = true;
        } else if (operation == "BASE") {
            if (trim(line.operand).empty()) {
                addDiagnostic(result, line.location, "BASE requires an operand");
            }
        } else if (operation == "BYTE") {
            if (!parseByteOperand(line.operand, size)) {
                valid = false;
                addDiagnostic(result, line.location,
                              "invalid BYTE operand; expected C'...' or an even number of hex digits in X'...'");
            }
        } else if (operation == "WORD") {
            if (!isValidWordOperand(line.operand)) {
                valid = false;
                addDiagnostic(result, line.location, "invalid WORD operand");
            } else {
                size = 3;
            }
        } else if (operation == "RESB" || operation == "RESW") {
            std::uint64_t count = 0;
            if (!parseUnsignedDecimal(trim(line.operand), count)) {
                valid = false;
                addDiagnostic(result, line.location,
                              "invalid " + operation + " operand; expected a non-negative decimal integer");
            } else {
                const std::uint64_t multiplier = operation == "RESW" ? 3 : 1;
                if (count > kMaximumAddress / multiplier) {
                    valid = false;
                    addDiagnostic(result, line.location,
                                  operation + " size exceeds the address space");
                } else {
                    size = static_cast<std::uint32_t>(count * multiplier);
                }
            }
        } else if (isDirective(operation)) {
            // START was handled above; this keeps future directive additions
            // from accidentally being treated as instructions.
        } else {
            std::string error;
            if (!getInstructionSize(operation, line.operand, size, error)) {
                valid = false;
                addDiagnostic(result, line.location, error);
            }
        }

        result.intermediateLines.push_back(
            IntermediateLine{line, lineAddress, size});
        if (valid && size != 0) {
            const std::uint64_t next =
                static_cast<std::uint64_t>(locationCounter) + size;
            if (next > kMaximumAddress) {
                addDiagnostic(result, line.location,
                              "location counter exceeds address 0xFFFFFF");
            } else {
                locationCounter = static_cast<std::uint32_t>(next);
            }
        }
    }

    if (locationCounter >= result.startAddress) {
        result.programLength = locationCounter - result.startAddress;
    }
    return result;
}
