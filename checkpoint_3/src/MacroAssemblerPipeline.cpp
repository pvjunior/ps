#include "MacroAssemblerPipeline.h"

// [PONTO DE CONEXÃO 1 - Headers do Montador do Checkpoint 2]
#include "ObjectProgram.h"
#include "PassOne.h"
#include "PassTwo.h"
#include "SourceParser.h"

// [PONTO DE CONEXÃO 2 - Headers do Parser e Processador de Macros (Integrantes 2 e 4)]
#if __has_include("MacroParser.h") && __has_include("MacroProcessor.h")
#include "MacroParser.h"
#include "MacroProcessor.h"
#define HAS_MACRO_FRONTEND_HEADERS 1
#else
#define HAS_MACRO_FRONTEND_HEADERS 0
#endif

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>
#include <type_traits>

namespace {

// ============================================================================
// ADAPTADORES DE METADADOS E CAMPOS DE DIAGNÓSTICO
// ============================================================================

template <typename T, typename = void>
struct HasMacrosDefinedField : std::false_type {};
template <typename T>
struct HasMacrosDefinedField<T, std::void_t<decltype(std::declval<const T&>().macrosDefined)>>
    : std::true_type {};

template <typename T, typename = void>
struct HasExpansionsPerformedField : std::false_type {};
template <typename T>
struct HasExpansionsPerformedField<
    T, std::void_t<decltype(std::declval<const T&>().expansionsPerformed)>>
    : std::true_type {};

template <typename ResultT = MacroExpansionResult>
inline std::size_t extractMacrosDefinedCount(const ResultT& result) {
    if constexpr (HasMacrosDefinedField<ResultT>::value) {
        return static_cast<std::size_t>(result.macrosDefined);
    }
    return 0;
}

template <typename ResultT = MacroExpansionResult>
inline std::size_t extractExpansionsPerformedCount(const ResultT& result) {
    if constexpr (HasExpansionsPerformedField<ResultT>::value) {
        return static_cast<std::size_t>(result.expansionsPerformed);
    }
    return 0;
}

inline bool hasFatalMacroErrors(const MacroExpansionResult& result) {
    return !result.diagnostics.empty();
}

std::string ensureStagePrefix(const std::string& stageLabel, const std::string& message) {
    const std::string tag = "[" + stageLabel + "]";
    if (message.find(tag) != std::string::npos) {
        return message;
    }
    return tag + " " + message;
}

PipelineDiagnostic makePipelineDiagnostic(PipelineStage stage,
                                          const std::string& stageName,
                                          const std::string& file,
                                          int line,
                                          const std::string& message) {
    PipelineDiagnostic diag;
    diag.stage = stage;
    diag.stageName = stageName;
    diag.where = SourceLocation{file, line};
    diag.message = ensureStagePrefix(stageName, message);
    return diag;
}

std::string resolveDefaultObjectPath(const std::string& referencePath,
                                     const std::string& expandedPath) {
    std::filesystem::path base =
        !referencePath.empty() ? std::filesystem::path(referencePath)
                               : std::filesystem::path(expandedPath);
    if (base.empty()) {
        return "output.obj";
    }
    std::filesystem::path candidate = base;
    candidate.replace_extension(".obj");
    if (candidate.lexically_normal() == base.lexically_normal() ||
        (!expandedPath.empty() &&
         candidate.lexically_normal() == std::filesystem::path(expandedPath).lexically_normal())) {
        candidate = std::filesystem::path(base.string() + ".obj");
    }
    return candidate.string();
}

bool saveObjectFileAtomically(const std::string& objectOutputPath,
                              const std::string& objectText,
                              std::string& errorMessage) {
    if (objectOutputPath.empty()) {
        errorMessage = "caminho de saida do arquivo objeto vazio";
        return false;
    }

    const std::filesystem::path targetPath(objectOutputPath);
    const std::filesystem::path tempPath(objectOutputPath + ".tmp");

    {
        std::ofstream stream(tempPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open() || !stream.good()) {
            errorMessage =
                "nao foi possivel abrir o arquivo temporario do objeto para escrita";
            return false;
        }
        stream.write(objectText.data(), static_cast<std::streamsize>(objectText.size()));
        stream.flush();
        const bool flushOk = stream.good();
        stream.close();

        if (!flushOk || stream.fail() || stream.bad()) {
            std::error_code removeEc;
            std::filesystem::remove(tempPath, removeEc);
            errorMessage = "falha ao escrever o programa objeto";
            return false;
        }
    }

    std::error_code renameEc;
    std::filesystem::rename(tempPath, targetPath, renameEc);
    if (renameEc) {
        std::error_code existsEc;
        if (std::filesystem::exists(targetPath, existsEc) && !existsEc) {
            std::error_code removeTargetEc;
            std::filesystem::remove(targetPath, removeTargetEc);
            if (!removeTargetEc) {
                renameEc.clear();
                std::filesystem::rename(tempPath, targetPath, renameEc);
            }
        }
    }

    if (renameEc) {
        std::error_code cleanupEc;
        std::filesystem::remove(tempPath, cleanupEc);
        errorMessage =
            "nao foi possivel atualizar o arquivo objeto: " + renameEc.message();
        return false;
    }

    return true;
}

#if !HAS_MACRO_FRONTEND_HEADERS
// Leitor auxiliar utilizado apenas enquanto MacroParser.h e MacroProcessor.h
// (Integrantes 2 e 4) ainda não foram adicionados ao repositório.
MacroExpansionResult fallbackReadSourceLines(const std::string& inputPath) {
    MacroExpansionResult result;
    std::ifstream input(inputPath);
    if (!input.is_open()) {
        MacroDiagnostic diag;
        diag.where.file = inputPath;
        diag.where.line = 0;
        diag.message = "could not open macro source file";
        result.diagnostics.push_back(diag);
        return result;
    }

    std::string rawLine;
    int lineNumber = 1;
    int openDefinitions = 0;
    int lastMcdefnLine = 0;

    while (std::getline(input, rawLine)) {
        if (!rawLine.empty() && rawLine.back() == '\r') {
            rawLine.pop_back();
        }

        MacroLine line;
        line.location.file = inputPath;
        line.location.line = lineNumber;
        line.originalText = rawLine;

        // Extrai operação básica para validação mínima de MCDEFN/MCEND órfãos no modo fallback.
        std::istringstream iss(rawLine);
        std::string firstToken;
        std::string secondToken;
        if (iss >> firstToken) {
            if (firstToken.front() != '.') {
                const bool startsAtCol0 =
                    !rawLine.empty() &&
                    std::isspace(static_cast<unsigned char>(rawLine.front())) == 0;
                if (startsAtCol0) {
                    line.label = normalizeIdentifier(firstToken);
                    if (iss >> secondToken && secondToken.front() != '.') {
                        line.operation = normalizeIdentifier(secondToken);
                    }
                } else {
                    line.operation = normalizeIdentifier(firstToken);
                }
            }
        }

        if (line.operation == "MCDEFN") {
            ++openDefinitions;
            lastMcdefnLine = lineNumber;
            ++result.macrosDefined;
        } else if (line.operation == "MCEND") {
            if (openDefinitions <= 0) {
                MacroDiagnostic diag;
                diag.where.file = inputPath;
                diag.where.line = lineNumber;
                diag.message = "MCEND sem MCDEFN correspondente";
                result.diagnostics.push_back(diag);
            } else {
                --openDefinitions;
            }
        }

        if (openDefinitions == 0 && line.operation != "MCDEFN" && line.operation != "MCEND") {
            result.expandedLines.push_back(line);
        }

        ++lineNumber;
    }

    if (openDefinitions > 0) {
        MacroDiagnostic diag;
        diag.where.file = inputPath;
        diag.where.line = lastMcdefnLine;
        diag.message = "MCDEFN sem MCEND correspondente";
        result.diagnostics.push_back(diag);
    }

    return result;
}
#endif

// ============================================================================
// FUNÇÕES PONTE ISOLADAS (MACRO PROCESSOR STAGE & ASSEMBLER STAGE)
// ============================================================================

// 1. Ponte com o Parser e o Processador de Macros (Integrantes 2 e 4)
MacroExpansionResult invokeMacroProcessorStage(const std::string& inputPath) {
#if HAS_MACRO_FRONTEND_HEADERS
    // [PONTO DE CONEXÃO 3 - Invocação do Parser (Integrante 2) e Expansor (Integrante 4)]
    // parseMacroFile já está definido em MacroParser.h retornando MacroParseResult{lines, diagnostics}.
    const MacroParseResult parsed = parseMacroFile(inputPath);
    if (!parsed.diagnostics.empty()) {
        MacroExpansionResult parseFailedResult;
        parseFailedResult.diagnostics = parsed.diagnostics;
        return parseFailedResult;
    }

    MacroProcessor processor;
    return processor.process(parsed.lines);
#else
    return fallbackReadSourceLines(inputPath);
#endif
}

// 2. Ponte com o Montador de Duas Passagens do Checkpoint 2
bool invokeAssemblerStage(const std::string& expandedPath,
                          const std::string& objectOutputPath,
                          std::vector<PipelineDiagnostic>& diagnostics,
                          std::vector<Diagnostic>* rawAssemblerDiagnostics = nullptr,
                          AssemblyResult* outAssemblyResult = nullptr) {
    auto recordAssemblerDiagnostics = [&](const std::vector<Diagnostic>& stageDiags) {
        for (const Diagnostic& d : stageDiags) {
            diagnostics.push_back(makePipelineDiagnostic(
                PipelineStage::Assembly, "Montagem", d.where.file, d.where.line, d.message));
            if (rawAssemblerDiagnostics != nullptr) {
                Diagnostic tagged = d;
                tagged.message = ensureStagePrefix("Montagem", d.message);
                rawAssemblerDiagnostics->push_back(tagged);
            }
            if (outAssemblyResult != nullptr) {
                outAssemblyResult->diagnostics.push_back(d);
            }
        }
    };

    // [PONTO DE CONEXÃO 4 - Invocação das APIs públicas do Montador (Checkpoint 2)]
    // Etapa 4.1: Análise léxica/sintática do arquivo expandido (MASMAPRG.ASM).
    const ParseResult parsed = parseFile(expandedPath);
    if (!parsed.diagnostics.empty()) {
        recordAssemblerDiagnostics(parsed.diagnostics);
        return false;
    }

    // Etapa 4.2: Passagem 1 (endereços e tabela de símbolos).
    const PassOneResult passOne = runPassOne(parsed.lines);
    if (!passOne.diagnostics.empty()) {
        recordAssemblerDiagnostics(passOne.diagnostics);
        return false;
    }

    // Etapa 4.3: Passagem 2 (codificação de instruções e blocos de código máquina).
    const PassTwoResult passTwo = runPassTwo(passOne);
    if (!passTwo.diagnostics.empty()) {
        recordAssemblerDiagnostics(passTwo.diagnostics);
        return false;
    }

    // Etapa 4.4: Formatação dos registros H, T, M e E do Programa Objeto SIC/XE.
    ObjectProgram objectProgram(passTwo.programName,
                                passTwo.startAddress,
                                passTwo.programLength,
                                passTwo.executionStart);
    for (const ObjectBlock& block : passTwo.blocks) {
        objectProgram.addBlock(block);
    }
    for (const ModificationEntry& mod : passTwo.modifications) {
        if (mod.lengthInHalfBytes > std::numeric_limits<std::uint8_t>::max()) {
            const Diagnostic modDiag{
                SourceLocation{expandedPath, 0},
                "tamanho invalido de registro de modificacao"};
            recordAssemblerDiagnostics({modDiag});
            return false;
        }
        objectProgram.addModification(ModificationRecord{
            mod.address, static_cast<std::uint8_t>(mod.lengthInHalfBytes)});
    }

    const std::string objectText = objectProgram.toText();

    // Etapa 4.5: Gravação atômica do arquivo .obj (preserva objeto anterior se houver erro).
    if (!objectOutputPath.empty()) {
        std::string ioError;
        if (!saveObjectFileAtomically(objectOutputPath, objectText, ioError)) {
            const Diagnostic ioDiag{SourceLocation{objectOutputPath, 0}, ioError};
            recordAssemblerDiagnostics({ioDiag});
            return false;
        }
    }

    if (outAssemblyResult != nullptr) {
        outAssemblyResult->objectProgram = objectText;
        outAssemblyResult->symbols = passOne.symbols;
    }

    return true;
}

} // namespace

// ============================================================================
// ORQUESTRAÇÃO PÚBLICA DO PIPELINE
// ============================================================================

MacroAssemblerResult runMacroAssemblerFromExpandedResult(
    const MacroExpansionResult& expansionResult,
    const std::string& expandedPath,
    const std::string& objectPath) {
    MacroAssemblerResult pipelineResult;
    pipelineResult.expansionResult = expansionResult;
    pipelineResult.expandedPath = expandedPath;
    pipelineResult.objectPath =
        !objectPath.empty() ? objectPath : resolveDefaultObjectPath("", expandedPath);
    pipelineResult.macrosDefined = extractMacrosDefinedCount(expansionResult);
    pipelineResult.expansionsPerformed = extractExpansionsPerformedCount(expansionResult);

    // 1. Propagar todos os diagnósticos da expansão identificando a etapa "Expansao de Macros".
    for (const MacroDiagnostic& diag : expansionResult.diagnostics) {
        MacroDiagnostic taggedMacroDiag = diag;
        taggedMacroDiag.message = ensureStagePrefix("Expansao de Macros", diag.message);
        pipelineResult.macroDiagnostics.push_back(taggedMacroDiag);

        pipelineResult.diagnostics.push_back(makePipelineDiagnostic(
            PipelineStage::MacroExpansion,
            "Expansao de Macros",
            diag.where.file,
            diag.where.line,
            diag.message));
    }

    // 2. Se a expansão falhar, interromper imediatamente:
    //    - MASMAPRG.ASM anterior não é sobrescrito;
    //    - O montador do Checkpoint 2 NÃO é executado.
    if (hasFatalMacroErrors(expansionResult)) {
        pipelineResult.failedStage = PipelineStage::MacroExpansion;
        return pipelineResult;
    }
    pipelineResult.macroStageSucceeded = true;

    // 3. Gravar o código-fonte expandido em MASMAPRG.ASM (ou caminho customizado).
    pipelineResult.writeResult = writeExpandedSource(expansionResult, expandedPath);
    if (!pipelineResult.writeResult.success()) {
        pipelineResult.failedStage = PipelineStage::ExpandedOutput;
        for (const MacroDiagnostic& writeDiag : pipelineResult.writeResult.diagnostics) {
            MacroDiagnostic taggedWriteDiag = writeDiag;
            taggedWriteDiag.message =
                ensureStagePrefix("Expansao de Macros", writeDiag.message);
            pipelineResult.macroDiagnostics.push_back(taggedWriteDiag);

            pipelineResult.diagnostics.push_back(makePipelineDiagnostic(
                PipelineStage::ExpandedOutput,
                "Expansao de Macros",
                writeDiag.where.file,
                writeDiag.where.line,
                writeDiag.message));
        }
        return pipelineResult;
    }
    pipelineResult.outputStageSucceeded = true;

    // 4. Somente após MASMAPRG.ASM estar gravado com sucesso no disco, acionar o montador.
    pipelineResult.assemblerInvoked = true;
    const bool assembledOk = invokeAssemblerStage(
        expandedPath,
        pipelineResult.objectPath,
        pipelineResult.diagnostics,
        &pipelineResult.assemblerDiagnostics,
        &pipelineResult.assemblyResult);

    if (!assembledOk) {
        pipelineResult.failedStage = PipelineStage::Assembly;
        return pipelineResult;
    }

    pipelineResult.assemblyStageSucceeded = true;
    pipelineResult.objectProgram = pipelineResult.assemblyResult.objectProgram;
    return pipelineResult;
}

MacroAssemblerResult runMacroAssembler(
    const std::string& inputPath,
    const std::string& expandedPath,
    const std::string& objectPath) {
    const std::string resolvedObjectPath =
        !objectPath.empty() ? objectPath : resolveDefaultObjectPath(inputPath, expandedPath);

    // 1. Executa a etapa de expansão de macros sobre `inputPath`.
    const MacroExpansionResult expansionResult = invokeMacroProcessorStage(inputPath);

    // 2. Continua o pipeline (validação de erros -> escrita de MASMAPRG.ASM -> montagem).
    return runMacroAssemblerFromExpandedResult(
        expansionResult, expandedPath, resolvedObjectPath);
}
