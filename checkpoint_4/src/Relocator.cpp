#include "Relocator.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint8_t kMaxFieldHalfBytes = 8;  // 32 bits cabem em uint64_t.
constexpr std::uint64_t kAddressSpaceSize = 0x1000000ULL;  // 24 bits.

std::string hex6(std::uint64_t value) {
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%06llX",
                  static_cast<unsigned long long>(value));
    return buffer;
}

std::string hexValue(std::uint64_t value, unsigned halfBytes) {
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%0*llX", static_cast<int>(halfBytes),
                  static_cast<unsigned long long>(value));
    return buffer;
}

void addDiagnostic(LoadWritePlanResult& result, const std::string& message) {
    // O plano validado não guarda a localização do registro M no arquivo
    // objeto; por isso o endereço entra na própria mensagem.
    result.diagnostics.push_back(
        LoaderDiagnostic{LoaderSourceLocation{"<relocator>", 0}, message, {}});
}

// Faixa de endereços finais ocupada por um bloco T.
struct Span {
    std::uint64_t start;
    std::uint64_t end;  // exclusivo
    std::size_t block;
};

// Índice por endereço final: encontra o bloco que contém um byte sem montar um
// mapa byte a byte (um programa pode ter até 16 MiB de espaço de endereços).
class ByteIndex {
public:
    explicit ByteIndex(const std::vector<LoadWriteBlock>& blocks) {
        spans_.reserve(blocks.size());
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].bytes.empty()) continue;
            spans_.push_back(Span{blocks[i].address,
                                  static_cast<std::uint64_t>(blocks[i].address) +
                                      blocks[i].bytes.size(),
                                  i});
        }
        std::sort(spans_.begin(), spans_.end(),
                  [](const Span& a, const Span& b) { return a.start < b.start; });
    }

    // Retorna o primeiro par de blocos que se sobrepõem, se houver.
    bool findOverlap(Span& first, Span& second) const {
        for (std::size_t i = 1; i < spans_.size(); ++i) {
            if (spans_[i].start < spans_[i - 1].end) {
                first = spans_[i - 1];
                second = spans_[i];
                return true;
            }
        }
        return false;
    }

    bool locate(std::uint64_t address, std::size_t& block,
                std::size_t& offset) const {
        auto it = std::upper_bound(
            spans_.begin(), spans_.end(), address,
            [](std::uint64_t a, const Span& s) { return a < s.start; });
        if (it == spans_.begin()) return false;
        --it;
        if (address >= it->end) return false;
        block = it->block;
        offset = static_cast<std::size_t>(address - it->start);
        return true;
    }

private:
    std::vector<Span> spans_;
};

// Aplica o delta ao valor guardado em um campo. Retorna false e preenche
// `error` se o campo for inválido, estiver incompleto ou o valor estourar.
bool relocateField(std::vector<LoadWriteBlock>& blocks, const ByteIndex& index,
                   const RelocatedModification& mod, std::int64_t delta,
                   std::string& error) {
    const std::string where = "M record at " + hex6(mod.originalAddress) +
                              " (loaded at " + hex6(mod.finalAddress) + "): ";

    const unsigned halfBytes = mod.fieldLengthHalfBytes;
    if (halfBytes == 0 || halfBytes > kMaxFieldHalfBytes) {
        error = where + "invalid field length of " + std::to_string(halfBytes) +
                " half-bytes (must be 1 to " +
                std::to_string(kMaxFieldHalfBytes) + ")";
        return false;
    }

    const std::size_t byteCount = (halfBytes + 1U) / 2U;
    const bool oddWidth = (halfBytes % 2U) != 0U;

    // 1. Localiza todos os bytes do campo antes de alterar qualquer um.
    std::vector<std::uint8_t*> field;
    field.reserve(byteCount);
    for (std::size_t i = 0; i < byteCount; ++i) {
        const std::uint64_t address =
            static_cast<std::uint64_t>(mod.finalAddress) + i;
        std::size_t block = 0;
        std::size_t offset = 0;
        if (address >= kAddressSpaceSize || !index.locate(address, block, offset)) {
            error = where + "byte " + std::to_string(i + 1) + " of " +
                    std::to_string(byteCount) + " of the field (address " +
                    hex6(address) + ") is not covered by any T record (gap)";
            return false;
        }
        field.push_back(&blocks[block].bytes[offset]);
    }

    // 2. Lê o valor (big-endian). Com largura ímpar o nibble alto do primeiro
    //    byte não pertence ao campo.
    const std::uint8_t preservedHighNibble =
        oddWidth ? static_cast<std::uint8_t>(*field[0] & 0xF0U) : std::uint8_t{0};
    std::uint64_t current = 0;
    for (std::size_t i = 0; i < byteCount; ++i) {
        std::uint8_t byte = *field[i];
        if (i == 0 && oddWidth) byte &= 0x0FU;
        current = (current << 8U) | byte;
    }

    // 3. Soma o delta; sem wrap-around.
    const std::uint64_t maxValue = (1ULL << (4U * halfBytes)) - 1ULL;
    const std::int64_t relocated = static_cast<std::int64_t>(current) + delta;
    if (relocated < 0 || static_cast<std::uint64_t>(relocated) > maxValue) {
        error = where + "field value " + hexValue(current, halfBytes) +
                (delta < 0 ? " - " : " + ") +
                hexValue(static_cast<std::uint64_t>(delta < 0 ? -delta : delta),
                         1) +
                " does not fit in " + std::to_string(halfBytes) +
                " half-bytes (overflow)";
        return false;
    }

    // 4. Regrava o valor, devolvendo o nibble alto preservado.
    std::uint64_t remaining = static_cast<std::uint64_t>(relocated);
    for (std::size_t i = byteCount; i > 0; --i) {
        std::uint8_t byte = static_cast<std::uint8_t>(remaining & 0xFFU);
        remaining >>= 8U;
        if (i == 1 && oddWidth) {
            byte = static_cast<std::uint8_t>(preservedHighNibble | (byte & 0x0FU));
        }
        *field[i - 1] = byte;
    }
    return true;
}

}  // namespace

LoadWritePlanResult relocate(const ValidatedLoadPlan& plan) {
    LoadWritePlanResult result;

    // Memória temporária: cópia dos bytes do objeto. O plano de entrada fica
    // intacto.
    LoadWritePlan writePlan;
    writePlan.executionAddress = plan.finalExecutionAddress;
    writePlan.blocks.reserve(plan.relocatedBlocks.size());
    for (const RelocatedBlock& block : plan.relocatedBlocks) {
        writePlan.blocks.push_back(LoadWriteBlock{block.finalAddress, block.bytes});
    }

    const ByteIndex index(writePlan.blocks);

    Span a{}, b{};
    if (index.findOverlap(a, b)) {
        addDiagnostic(result, "load plan has overlapping blocks at " +
                                  hex6(b.start) + " (previous block ends at " +
                                  hex6(a.end) + ")");
        return result;
    }

    for (const RelocatedModification& mod : plan.relocatedModifications) {
        std::string error;
        if (!relocateField(writePlan.blocks, index, mod, plan.relocationDelta,
                           error)) {
            addDiagnostic(result, error);
        }
    }

    if (result.diagnostics.empty()) {
        result.plan = std::move(writePlan);
    }
    return result;
}
