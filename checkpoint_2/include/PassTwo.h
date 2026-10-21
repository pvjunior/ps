#pragma once

#include "AssemblerTypes.h"
#include "Encoder.h"
#include "PassOne.h"

#include <cstdint>
#include <string>
#include <vector>

// Bloco contíguo de bytes de máquina. RESB/RESW (e erros) quebram blocos.
struct ObjectBlock {
    std::uint32_t address = 0;
    std::vector<std::uint8_t> bytes;
};

// Saída da segunda passagem: tudo o que a geração de H/T/M/E precisa.
struct PassTwoResult {
    std::string programName;             // rótulo do START (registro H)
    std::uint32_t startAddress = 0;      // endereço inicial (H)
    std::uint32_t programLength = 0;     // tamanho total em bytes (H)
    std::uint32_t executionStart = 0;    // primeira instrução executável (E)
    std::vector<ObjectBlock> blocks;     // em ordem de endereço (T)
    std::vector<ModificationEntry> modifications;  // formato 4 relocável (M)
    std::vector<Diagnostic> diagnostics;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

// Se a primeira passagem falhou, os diagnósticos dela são repassados e nenhum
// byte é gerado (tamanhos/endereços não são confiáveis).
PassTwoResult runPassTwo(const PassOneResult& passOne);
