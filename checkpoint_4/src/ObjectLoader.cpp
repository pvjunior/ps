#include "ObjectLoader.h"

#include "LoadPlanner.h"
#include "ObjectParser.h"

// [PONTO DE CONEXÃO 1 - Header do Relocador (Integrante 4)]
// Detecta automaticamente Relocator.h assim que for adicionado em checkpoint_4/include/.
#if __has_include("Relocator.h")
#include "Relocator.h"
#define HAS_EXTERNAL_RELOCATOR 1
#else
#define HAS_EXTERNAL_RELOCATOR 0
#endif

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kMaxSicXeAddress = 0xFFFFFFu;

LoaderDiagnostic makeLoaderDiagnostic(const std::string& file,
                                      std::uint32_t line,
                                      const std::string& message,
                                      const std::string& originalText = {}) {
    return LoaderDiagnostic{LoaderSourceLocation{file, line}, message, originalText};
}

#if !HAS_EXTERNAL_RELOCATOR
// Implementação de fallback em memória temporária usada apenas enquanto Relocator.h/.cpp
// (Integrante 4) ainda não está presente no repositório.
LoadWritePlanResult fallbackRelocate(const ValidatedLoadPlan& plan) {
    LoadWritePlanResult result;
    LoadWritePlan writePlan;
    writePlan.executionAddress = plan.finalExecutionAddress;
    writePlan.blocks.reserve(plan.relocatedBlocks.size());

    // Mapeia cada endereço final carregado para (índice do bloco, deslocamento interno),
    // permitindo aplicar registros M inclusive quando cruzam blocos T contíguos e rejeitar lacunas.
    std::map<std::uint32_t, std::pair<std::size_t, std::size_t>> addressMap;

    for (const RelocatedBlock& block : plan.relocatedBlocks) {
        const std::size_t blockIndex = writePlan.blocks.size();
        LoadWriteBlock writeBlock{block.finalAddress, block.bytes};

        for (std::size_t offset = 0; offset < writeBlock.bytes.size(); ++offset) {
            const std::uint32_t byteAddr =
                block.finalAddress + static_cast<std::uint32_t>(offset);
            addressMap[byteAddr] = {blockIndex, offset};
        }

        writePlan.blocks.push_back(std::move(writeBlock));
    }

    for (const RelocatedModification& mod : plan.relocatedModifications) {
        const std::uint8_t halfBytes = mod.fieldLengthHalfBytes;
        if (halfBytes == 0 || halfBytes > 8) {
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<relocator>", 0,
                "invalid M record field length in half-bytes: " +
                    std::to_string(halfBytes)));
            continue;
        }

        const std::size_t byteCount = (static_cast<std::size_t>(halfBytes) + 1U) / 2U;
        std::vector<std::uint8_t*> fieldBytes;
        fieldBytes.reserve(byteCount);

        bool missingByte = false;
        for (std::size_t i = 0; i < byteCount; ++i) {
            const std::uint64_t addr64 =
                static_cast<std::uint64_t>(mod.finalAddress) + i;
            if (addr64 > kMaxSicXeAddress) {
                missingByte = true;
                break;
            }
            const auto it = addressMap.find(static_cast<std::uint32_t>(addr64));
            if (it == addressMap.end()) {
                missingByte = true;
                break;
            }
            fieldBytes.push_back(
                &writePlan.blocks[it->second.first].bytes[it->second.second]);
        }

        if (missingByte) {
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<relocator>", 0,
                "M record references bytes outside loaded T records (gap)"));
            continue;
        }

        // Extrai o valor atual do campo (big-endian), preservando o nibble alto quando ímpar.
        const bool isOddWidth = (halfBytes % 2U) != 0U;
        const std::uint8_t preservedHighNibble =
            isOddWidth ? static_cast<std::uint8_t>((*fieldBytes[0]) & 0xF0U) : 0U;

        std::uint64_t currentValue = 0;
        for (std::size_t i = 0; i < byteCount; ++i) {
            std::uint8_t byteVal = *fieldBytes[i];
            if (i == 0 && isOddWidth) {
                byteVal = static_cast<std::uint8_t>(byteVal & 0x0FU);
            }
            currentValue = (currentValue << 8U) | byteVal;
        }

        const std::int64_t relocatedValue =
            static_cast<std::int64_t>(currentValue) + plan.relocationDelta;
        const std::uint64_t maxFieldValue =
            (halfBytes == 16) ? std::numeric_limits<std::uint64_t>::max()
                              : ((1ULL << (static_cast<unsigned>(halfBytes) * 4U)) - 1ULL);

        if (relocatedValue < 0 ||
            static_cast<std::uint64_t>(relocatedValue) > maxFieldValue) {
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<relocator>", 0,
                "relocated field value overflows M record field width"));
            continue;
        }

        std::uint64_t remaining = static_cast<std::uint64_t>(relocatedValue);
        for (std::size_t idx = byteCount; idx > 0; --idx) {
            const std::size_t i = idx - 1U;
            const std::uint8_t lowByte = static_cast<std::uint8_t>(remaining & 0xFFU);
            remaining >>= 8U;

            if (i == 0 && isOddWidth) {
                *fieldBytes[0] = static_cast<std::uint8_t>(
                    preservedHighNibble | (lowByte & 0x0FU));
            } else {
                *fieldBytes[i] = lowByte;
            }
        }
    }

    if (result.diagnostics.empty()) {
        result.plan = std::move(writePlan);
    }
    return result;
}
#endif // !HAS_EXTERNAL_RELOCATOR

// ============================================================================
// FUNÇÃO PONTE ISOLADA PARA O RELOCADOR (INTEGRANTE 4)
// ============================================================================

LoadWritePlanResult invokeRelocatorStage(const ValidatedLoadPlan& plan) {
#if HAS_EXTERNAL_RELOCATOR
    // [PONTO DE CONEXÃO 2 - Chamada do Relocador (Integrante 4)]
    // Ajuste apenas o nome da função aqui caso Relocator.h utilize outra assinatura.
    return relocate(plan);
#else
    return fallbackRelocate(plan);
#endif
}

struct WrittenBackup {
    int address = 0;
    std::vector<std::uint8_t> previousBytes;
};

void rollbackMemoryWrites(Machine& machine,
                          const std::vector<WrittenBackup>& backups) {
    for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
        if (!it->previousBytes.empty()) {
            machine.loader().write(it->address, it->previousBytes);
        }
    }
}

} // namespace

// ============================================================================
// IMPLEMENTAÇÃO DA FACHADA PÚBLICA DO CARREGADOR (INTEGRANTE 5)
// ============================================================================

ObjectLoadResult applyLoadWritePlan(
    const LoadWritePlan& writePlan,
    std::uint32_t baseAddress,
    Machine& machine) {
    ObjectLoadResult result;

    const int rawMemorySize = machine.getMemorySize();
    if (rawMemorySize <= 0) {
        result.diagnostics.push_back(makeLoaderDiagnostic(
            "<machine>", 0, "target machine has no addressable memory"));
        return result;
    }
    const std::uint64_t memorySize = static_cast<std::uint64_t>(rawMemorySize);

    // 1. Pré-validação completa de todos os blocos e do ponto de entrada antes de
    //    gravar qualquer byte na Machine (garante preservação total em caso de erro).
    for (const LoadWriteBlock& block : writePlan.blocks) {
        if (block.bytes.empty()) {
            continue;
        }
        const std::uint64_t start = block.address;
        const std::uint64_t size = block.bytes.size();
        if (start > kMaxSicXeAddress ||
            start > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
            size > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) ||
            start + size > memorySize) {
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<machine>", 0,
                "write block exceeds target machine memory bounds"));
            return result;
        }
    }

    if (writePlan.executionAddress.has_value()) {
        const std::uint32_t entry = *writePlan.executionAddress;
        if (entry > kMaxSicXeAddress || static_cast<std::uint64_t>(entry) >= memorySize) {
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<machine>", 0,
                "execution entry address is outside target machine memory"));
            return result;
        }
    }

    // 2. Escrita dos blocos na memória da Machine via API pública machine.loader().write(),
    //    mantendo backup defensivo para rollback em caso de falha inesperada.
    std::vector<WrittenBackup> backups;
    backups.reserve(writePlan.blocks.size());
    std::size_t totalBytesWritten = 0;

    for (const LoadWriteBlock& block : writePlan.blocks) {
        if (block.bytes.empty()) {
            continue;
        }

        const int targetAddr = static_cast<int>(block.address);
        WrittenBackup backup;
        backup.address = targetAddr;
        backup.previousBytes =
            machine.dump(targetAddr, static_cast<int>(block.bytes.size()));

        const LoadResult writeOutcome =
            machine.loader().write(targetAddr, block.bytes);
        if (!writeOutcome.ok) {
            rollbackMemoryWrites(machine, backups);
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<machine>", 0,
                "failed to write block into machine memory: " + writeOutcome.error));
            return result;
        }

        backups.push_back(std::move(backup));
        totalBytesWritten += block.bytes.size();
    }

    // 3. Ajusta o PC da Machine para o endereço E relocado somente após todas as escritas.
    if (writePlan.executionAddress.has_value()) {
        if (!machine.setPC(*writePlan.executionAddress)) {
            rollbackMemoryWrites(machine, backups);
            result.diagnostics.push_back(makeLoaderDiagnostic(
                "<machine>", 0,
                "failed to set machine PC to relocated execution address"));
            return result;
        }
    }

    result.baseAddress = baseAddress;
    result.bytesLoaded = totalBytesWritten;
    result.executionAddress = writePlan.executionAddress;
    result.loaded = true;
    return result;
}

ObjectLoadResult loadParsedObject(
    const ParsedObject& object,
    Machine& machine,
    std::optional<std::uint32_t> base) {
    ObjectLoadResult result;

    const int rawMemorySize = machine.getMemorySize();
    if (rawMemorySize <= 0) {
        result.diagnostics.push_back(makeLoaderDiagnostic(
            object.location.file, 0, "target machine has no addressable memory"));
        return result;
    }

    // Etapa 2: Determinar base efetiva (explícita ou origem do registro H) e planejar/validar.
    std::uint32_t effectiveBase = 0;
    if (base.has_value()) {
        effectiveBase = *base;
    } else if (object.header.has_value()) {
        effectiveBase = object.header->originalStartAddress;
    }

    const LoadPlanResult planResult =
        planLoad(object, effectiveBase, static_cast<std::uint32_t>(rawMemorySize));
    if (!planResult.success()) {
        result.diagnostics = planResult.diagnostics;
        return result;
    }

    // Etapa 3: Relocação dos campos apontados pelos registros M em memória temporária.
    const LoadWritePlanResult writePlanResult =
        invokeRelocatorStage(*planResult.plan);
    if (!writePlanResult.success()) {
        result.diagnostics = writePlanResult.diagnostics;
        return result;
    }

    // Etapa 4: Escrita atômica na memória da Machine e ajuste do PC.
    return applyLoadWritePlan(
        *writePlanResult.plan, planResult.plan->finalStartAddress, machine);
}

ObjectLoadResult loadObjectText(
    const std::string& text,
    Machine& machine,
    std::optional<std::uint32_t> base,
    const std::string& sourceName) {
    // Etapa 1: Leitura e análise sintática dos registros H/T/M/E.
    const ObjectParseResult parseResult = parseObjectText(text, sourceName);
    if (!parseResult.success()) {
        ObjectLoadResult result;
        result.diagnostics = parseResult.diagnostics;
        return result;
    }

    return loadParsedObject(*parseResult.object, machine, base);
}

ObjectLoadResult loadObject(
    const std::string& path,
    Machine& machine,
    std::optional<std::uint32_t> base) {
    // Etapa 1: Leitura do arquivo objeto em disco via ObjectParser.
    const ObjectParseResult parseResult = parseObjectFile(path);
    if (!parseResult.success()) {
        ObjectLoadResult result;
        result.diagnostics = parseResult.diagnostics;
        return result;
    }

    return loadParsedObject(*parseResult.object, machine, base);
}
