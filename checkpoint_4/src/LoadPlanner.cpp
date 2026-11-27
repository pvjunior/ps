#include "LoadPlanner.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint64_t kAddressSpaceSize = 0x1000000ULL;
constexpr std::uint32_t kMaximumAddress = 0xFFFFFF;

void addDiagnostic(LoadPlanResult& result, const LoaderSourceLocation& where,
                   const std::string& message,
                   const std::string& originalText = {}) {
    result.diagnostics.push_back(LoaderDiagnostic{where, message, originalText});
}

bool isWithinRange(std::uint64_t start, std::uint64_t length,
                   std::uint64_t rangeStart, std::uint64_t rangeEnd) {
    return start >= rangeStart && start <= rangeEnd &&
           length <= rangeEnd - start;
}

bool translateRange(std::uint32_t originalAddress, std::uint64_t length,
                    std::int64_t delta, std::uint32_t memorySize,
                    std::uint32_t& finalAddress,
                    std::string& error) {
    const std::int64_t translated =
        static_cast<std::int64_t>(originalAddress) + delta;
    if (translated < 0 || translated > kMaximumAddress) {
        error = "relocated address is outside the SIC/XE 24-bit address space";
        return false;
    }

    const std::uint64_t start = static_cast<std::uint64_t>(translated);
    const std::uint64_t end = start + length;
    if (end > kAddressSpaceSize) {
        error = "relocated range exceeds the SIC/XE 24-bit address space";
        return false;
    }
    if (end > memorySize) {
        error = "relocated range exceeds available memory";
        return false;
    }

    finalAddress = static_cast<std::uint32_t>(start);
    return true;
}

struct AddressRange {
    std::uint64_t start;
    std::uint64_t end;
};

} // namespace

LoadPlanResult planLoad(const ParsedObject& object, std::uint32_t base,
                        std::uint32_t memorySize) {
    LoadPlanResult result;

    if (!object.header.has_value()) {
        addDiagnostic(result, object.location, "missing H record",
                      object.originalText);
    }
    if (!object.end.has_value()) {
        addDiagnostic(result, object.location, "missing E record",
                      object.originalText);
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    const HeaderRecord& header = *object.header;
    const EndRecord& endRecord = *object.end;
    const std::uint64_t originalStart = header.originalStartAddress;
    const std::uint64_t programLength = header.programLength;
    const std::uint64_t originalEnd = originalStart + programLength;

    if (originalStart > kMaximumAddress || originalEnd > kAddressSpaceSize) {
        addDiagnostic(result, header.location,
                      "H record declares a range outside the SIC/XE 24-bit address space",
                      header.originalText);
        return result;
    }

    if (base > kMaximumAddress) {
        addDiagnostic(result, header.location,
                      "load base is outside the SIC/XE 24-bit address space",
                      header.originalText);
        return result;
    }

    const std::uint64_t finalEnd = static_cast<std::uint64_t>(base) + programLength;
    if (finalEnd > kAddressSpaceSize) {
        addDiagnostic(result, header.location,
                      "load range exceeds the SIC/XE 24-bit address space",
                      header.originalText);
    }
    if (finalEnd > memorySize) {
        addDiagnostic(result, header.location,
                      "load range exceeds available memory",
                      header.originalText);
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    const std::int64_t delta = static_cast<std::int64_t>(base) -
                               static_cast<std::int64_t>(header.originalStartAddress);
    ValidatedLoadPlan plan;
    plan.relocationDelta = delta;
    plan.originalStartAddress = header.originalStartAddress;
    plan.finalStartAddress = base;

    std::vector<AddressRange> textRanges;
    textRanges.reserve(object.textRecords.size());
    plan.relocatedBlocks.reserve(object.textRecords.size());

    for (const TextRecord& text : object.textRecords) {
        const std::uint64_t textStart = text.originalAddress;
        const std::uint64_t textLength = text.bytes.size();
        if (textLength == 0) {
            addDiagnostic(result, text.location, "T record contains no bytes",
                          text.originalText);
            continue;
        }
        if (!isWithinRange(textStart, textLength, originalStart, originalEnd)) {
            addDiagnostic(result, text.location,
                          "T record lies outside the range declared by H",
                          text.originalText);
            continue;
        }

        const std::uint64_t textEnd = textStart + textLength;
        for (const AddressRange& previous : textRanges) {
            if (textStart < previous.end && previous.start < textEnd) {
                addDiagnostic(result, text.location,
                              "T record overlaps another T record",
                              text.originalText);
                break;
            }
        }
        textRanges.push_back(AddressRange{textStart, textEnd});

        std::uint32_t finalAddress = 0;
        std::string error;
        if (!translateRange(text.originalAddress, textLength, delta, memorySize,
                            finalAddress, error)) {
            addDiagnostic(result, text.location, error, text.originalText);
            continue;
        }

        plan.relocatedBlocks.push_back(
            RelocatedBlock{text.originalAddress, finalAddress, text.bytes});
    }

    plan.relocatedModifications.reserve(object.modificationRecords.size());
    for (const ModificationRecord& modification : object.modificationRecords) {
        if (modification.fieldLengthHalfBytes == 0) {
            addDiagnostic(result, modification.location,
                          "M record field length must be greater than zero",
                          modification.originalText);
            continue;
        }

        const std::uint64_t fieldLengthBytes =
            (static_cast<std::uint64_t>(modification.fieldLengthHalfBytes) + 1) / 2;
        const std::uint64_t modificationStart = modification.originalAddress;
        if (!isWithinRange(modificationStart, fieldLengthBytes, originalStart,
                           originalEnd)) {
            addDiagnostic(result, modification.location,
                          "M record field lies outside the range declared by H",
                          modification.originalText);
            continue;
        }

        std::uint32_t finalAddress = 0;
        std::string error;
        if (!translateRange(modification.originalAddress, fieldLengthBytes,
                            delta, memorySize, finalAddress, error)) {
            addDiagnostic(result, modification.location, error,
                          modification.originalText);
            continue;
        }

        plan.relocatedModifications.push_back(RelocatedModification{
            modification.originalAddress, finalAddress,
            modification.fieldLengthHalfBytes});
    }

    if (endRecord.originalExecutionAddress.has_value()) {
        const std::uint32_t originalEntry = *endRecord.originalExecutionAddress;
        if (originalEntry < originalStart || originalEntry >= originalEnd) {
            addDiagnostic(result, endRecord.location,
                          "E record entry address lies outside the range declared by H",
                          endRecord.originalText);
        } else {
            std::uint32_t finalEntry = 0;
            std::string error;
            if (!translateRange(originalEntry, 1, delta, memorySize,
                                finalEntry, error)) {
                addDiagnostic(result, endRecord.location, error,
                              endRecord.originalText);
            } else {
                plan.originalExecutionAddress = originalEntry;
                plan.finalExecutionAddress = finalEntry;
            }
        }
    }

    if (result.diagnostics.empty()) {
        result.plan = std::move(plan);
    }
    return result;
}
