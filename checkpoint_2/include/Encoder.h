#pragma once

#include "AssemblerTypes.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Entrada de modificação (registro M): endereço do primeiro meio-byte a ser
// relocado e quantidade de meios-bytes. Formato 4 gera sempre {end + 1, 5}.
struct ModificationEntry {
    std::uint32_t address = 0;
    std::uint32_t lengthInHalfBytes = 0;
};

// Bytes de uma única linha de código-fonte (instrução ou BYTE/WORD).
struct EncodedStatement {
    std::vector<std::uint8_t> bytes;
    std::optional<ModificationEntry> modification;
};

// Contexto necessário para resolver operandos.
struct EncodeContext {
    const SymbolTable* symbols = nullptr;  // tabela da primeira passagem
    std::optional<std::uint32_t> base;     // valor atual do registrador B (BASE)
};

// Em caso de erro, `ok == false`, `error` descreve o problema e `statement`
// fica vazio: nunca existem bytes parciais de uma linha com erro.
struct EncodeOutcome {
    bool ok = false;
    std::string error;
    EncodedStatement statement;
};

// Avalia um operando que seja número (decimal ou 0x...) ou símbolo.
// `isSymbol` (opcional) indica se o valor veio da tabela de símbolos.
bool evaluateValue(const std::string& text, const SymbolTable& symbols,
                   std::int64_t& value, bool* isSymbol, std::string& error);

// Codifica uma instrução dos formatos 1, 2, 3 ou 4 (`+MNEMONICO` = formato 4).
// `address` é o endereço da instrução (usado no cálculo PC-relative).
EncodeOutcome encodeInstruction(const std::string& operation,
                                const std::string& operand,
                                std::uint32_t address,
                                const EncodeContext& context);

// BYTE: C'...' ou X'...'.
EncodeOutcome encodeByteDirective(const std::string& operand);

// WORD: inteiro decimal ou 0x..., reduzido a 24 bits.
EncodeOutcome encodeWordDirective(const std::string& operand);
