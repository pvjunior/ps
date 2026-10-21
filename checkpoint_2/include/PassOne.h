#pragma once

#include "AssemblerTypes.h"

#include <vector>

// Result produced by the assembler's first pass. The intermediate lines retain
// the parsed source and the location/size needed by the object-code pass.
struct PassOneResult {
    std::uint32_t startAddress = 0;
    std::uint32_t programLength = 0;
    std::vector<IntermediateLine> intermediateLines;
    SymbolTable symbols;
    std::vector<Diagnostic> diagnostics;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

PassOneResult runPassOne(const std::vector<ParsedLine>& lines);
