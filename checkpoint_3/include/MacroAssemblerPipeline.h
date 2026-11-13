#pragma once

#include "MacroOutput.h"
#include "AssemblerTypes.h"

#include <cstddef>
#include <string>
#include <vector>

// Etapa do pipeline na qual um diagnóstico foi originado.
enum class PipelineStage {
    None,
    MacroExpansion,
    ExpandedOutput,
    Assembly
};

// Diagnóstico unificado do pipeline identificando explicitamente a etapa de origem.
struct PipelineDiagnostic {
    PipelineStage stage = PipelineStage::None;
    std::string stageName;
    SourceLocation where;
    std::string message;
};

// Resultado completo da execução do Macro-Montador (Expansão + Gravação MASMAPRG.ASM + Montagem).
// Compatível tanto com o contrato da CLI (checkpoint_3/main.cpp) quanto com testes de integração.
struct MacroAssemblerResult {
    // Campos consumidos diretamente pela CLI (Integrante 6 - checkpoint_3/main.cpp):
    std::vector<MacroDiagnostic> macroDiagnostics;
    std::vector<Diagnostic> assemblerDiagnostics;
    std::size_t macrosDefined = 0;
    std::size_t expansionsPerformed = 0;

    // Diagnósticos detalhados por etapa e artefatos intermediários/finais para inspeção em testes:
    std::vector<PipelineDiagnostic> diagnostics;
    MacroExpansionResult expansionResult;
    MacroWriteResult writeResult;
    AssemblyResult assemblyResult;
    std::string expandedPath;
    std::string objectPath;
    std::string objectProgram;
    PipelineStage failedStage = PipelineStage::None;
    bool macroStageSucceeded = false;
    bool outputStageSucceeded = false;
    bool assemblyStageSucceeded = false;
    bool assemblerInvoked = false;

    bool success() const noexcept {
        return macroStageSucceeded &&
               outputStageSucceeded &&
               assemblyStageSucceeded &&
               macroDiagnostics.empty() &&
               assemblerDiagnostics.empty() &&
               diagnostics.empty();
    }

    explicit operator bool() const noexcept {
        return success();
    }
};

// Executa o pipeline a partir de um `MacroExpansionResult` já construído em memória:
// 1. Propaga diagnósticos da expansão e aborta antes da escrita/montagem se houver erros.
// 2. Grava `expandedPath` (padrão: "MASMAPRG.ASM") de forma atômica via `writeExpandedSource`.
// 3. Invoca o montador do Checkpoint 2 sobre `expandedPath` e grava o programa objeto.
// Permite validar a saída e a ponte com o montador independentemente dos módulos 2, 3 e 4.
MacroAssemblerResult runMacroAssemblerFromExpandedResult(
    const MacroExpansionResult& expansionResult,
    const std::string& expandedPath = "MASMAPRG.ASM",
    const std::string& objectPath = "");

// Fachada pública obrigatória do Macro-Montador:
// 1. Executa o parser e o processador de macros sobre `inputPath`.
// 2. Propaga diagnósticos da etapa de Expansão de Macros; se houver erros, interrompe o fluxo
//    sem sobrescrever `MASMAPRG.ASM` e sem acionar o montador.
// 3. Grava o código expandido em `expandedPath` (padrão: "MASMAPRG.ASM").
// 4. Somente após `MASMAPRG.ASM` estar gravado em disco, invoca o montador do Checkpoint 2
//    (parseFile -> runPassOne -> runPassTwo -> ObjectProgram) e gera o arquivo objeto.
MacroAssemblerResult runMacroAssembler(
    const std::string& inputPath,
    const std::string& expandedPath = "MASMAPRG.ASM",
    const std::string& objectPath = "");
