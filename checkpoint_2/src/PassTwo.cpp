#include "PassTwo.h"

#include <cctype>
#include <optional>

namespace {

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

void addDiagnostic(PassTwoResult& result, const SourceLocation& location,
                   const std::string& message) {
    result.diagnostics.push_back(Diagnostic{location, message});
}

// Anexa bytes ao bloco atual se forem contíguos; senão abre um novo bloco.
void appendBytes(PassTwoResult& result, std::uint32_t address,
                 const std::vector<std::uint8_t>& bytes) {
    if (bytes.empty()) {
        return;
    }
    if (!result.blocks.empty()) {
        ObjectBlock& last = result.blocks.back();
        if (last.address + last.bytes.size() == address) {
            last.bytes.insert(last.bytes.end(), bytes.begin(), bytes.end());
            return;
        }
    }
    result.blocks.push_back(ObjectBlock{address, bytes});
}

} // namespace

PassTwoResult runPassTwo(const PassOneResult& passOne) {
    PassTwoResult result;
    result.startAddress = passOne.startAddress;
    result.programLength = passOne.programLength;
    result.executionStart = passOne.startAddress;

    if (!passOne.success()) {
        result.diagnostics = passOne.diagnostics;
        return result;
    }

    EncodeContext context;
    context.symbols = &passOne.symbols;

    for (const IntermediateLine& line : passOne.intermediateLines) {
        const std::string operation = normalizeIdentifier(trim(line.parsed.operation));
        const std::string operand = trim(line.parsed.operand);
        const SourceLocation& where = line.parsed.location;

        if (operation.empty()) {
            continue;  // linha em branco ou comentário
        }

        if (operation == "START") {
            if (line.parsed.label.has_value()) {
                result.programName = *line.parsed.label;
            }
            continue;
        }

        if (operation == "RESB" || operation == "RESW") {
            continue;  // reservam espaço, não emitem bytes
        }

        if (operation == "BASE") {
            std::int64_t value = 0;
            std::string error;
            if (!evaluateValue(operand, passOne.symbols, value, nullptr, error)) {
                addDiagnostic(result, where, "BASE: " + error);
            } else if (value < 0 || value > 0xFFFFFF) {
                addDiagnostic(result, where, "BASE value out of range");
            } else {
                context.base = static_cast<std::uint32_t>(value);
            }
            continue;
        }

        if (operation == "END") {
            if (!operand.empty()) {
                std::int64_t value = 0;
                std::string error;
                if (!evaluateValue(operand, passOne.symbols, value, nullptr, error)) {
                    addDiagnostic(result, where, "END: " + error);
                } else if (value < 0 || value > 0xFFFFFF) {
                    addDiagnostic(result, where, "END address out of range");
                } else {
                    result.executionStart = static_cast<std::uint32_t>(value);
                }
            }
            continue;
        }

        EncodeOutcome outcome;
        if (operation == "BYTE") {
            outcome = encodeByteDirective(operand);
        } else if (operation == "WORD") {
            outcome = encodeWordDirective(operand);
        } else {
            outcome = encodeInstruction(operation, operand, line.address, context);
        }

        if (!outcome.ok) {
            addDiagnostic(result, where, outcome.error);
            continue;  // linha com erro: nenhum byte é emitido
        }
        if (outcome.statement.bytes.size() != line.sizeInBytes) {
            addDiagnostic(result, where,
                          "internal error: encoded size differs from first-pass size");
            continue;
        }

        appendBytes(result, line.address, outcome.statement.bytes);
        if (outcome.statement.modification.has_value()) {
            result.modifications.push_back(*outcome.statement.modification);
        }
    }

    return result;
}
