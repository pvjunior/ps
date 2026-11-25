#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

// [PONTO DE CONEXÃO - Contratos do Integrante 1]
// Detecta automaticamente se MacroTypes.h já foi adicionado ao repositório.
// Caso ainda não exista, define estruturas compatíveis com a especificação do Checkpoint 3
// para permitir compilação e testes isolados da Parte 5 sem gerar conflitos no Git.
#if __has_include("MacroTypes.h")
#include "MacroTypes.h"
#else

struct MacroSourceLocation {
    std::string file;
    int line = 0;
};

struct MacroDiagnostic {
    MacroSourceLocation where;
    std::string message;
};

struct MacroLine {
    std::string label;
    std::string operation;
    std::string operands;
    std::string comment;
    MacroSourceLocation location;
    std::string originalText;
    std::string text;
};

struct MacroParameter {
    std::string name;
    std::optional<std::string> defaultValue;
    int definitionLevel = 1;
    int index = 0;
};

struct MacroDefinition {
    std::string name;
    std::vector<MacroParameter> parameters;
    std::vector<MacroLine> body;
    MacroSourceLocation location;
};

struct MacroExpansionResult {
    std::vector<MacroLine> expandedLines;
    std::vector<MacroDiagnostic> diagnostics;
    std::size_t macrosDefined = 0;
    std::size_t expansionsPerformed = 0;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

#endif // __has_include("MacroTypes.h")

// Nome padrão obrigatório do arquivo de saída expandido.
inline constexpr const char* kDefaultExpandedOutputPath = "MASMAPRG.ASM";

// Resultado da operação de gravação atômica do código-fonte expandido.
struct MacroWriteResult {
    bool ok = false;
    bool written = false;
    std::string outputPath;
    std::size_t linesWritten = 0;
    std::string errorMessage;
    std::vector<MacroDiagnostic> diagnostics;

    bool success() const noexcept {
        return ok && written && diagnostics.empty() && errorMessage.empty();
    }

    explicit operator bool() const noexcept {
        return success();
    }
};

// Grava o código-fonte expandido em `path` (padrão: "MASMAPRG.ASM") de forma atômica e segura.
// - Se `result` contiver diagnósticos fatais de expansão, aborta imediatamente sem gravar
//   saída parcial e preserva qualquer arquivo `MASMAPRG.ASM` anterior intacto.
// - Filtra diretivas `MCDEFN`, `MCEND` e eventuais corpos de definições não expandidas.
// - Preserva a ordem exata das linhas expandidas e garante quebra de linha (`\n`) ao final.
MacroWriteResult writeExpandedSource(
    const MacroExpansionResult& result,
    const std::string& path = "MASMAPRG.ASM");
