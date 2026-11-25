#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct LoaderSourceLocation {
    std::string file;
    std::uint32_t line = 0;
};

struct LoaderDiagnostic {
    LoaderSourceLocation where;
    std::string message;
    std::string originalText;
};

struct HeaderRecord {
    std::string programName;
    std::uint32_t originalStartAddress = 0;
    std::uint32_t programLength = 0;
    LoaderSourceLocation location;
    std::string originalText;
};

struct TextRecord {
    std::uint32_t originalAddress = 0;
    std::vector<std::uint8_t> bytes;
    LoaderSourceLocation location;
    std::string originalText;
};

struct ModificationRecord {
    std::uint32_t originalAddress = 0;
    std::uint8_t fieldLengthHalfBytes = 0;
    LoaderSourceLocation location;
    std::string originalText;
};

struct EndRecord {
    std::optional<std::uint32_t> originalExecutionAddress;
    LoaderSourceLocation location;
    std::string originalText;
};

// Parsed file records retain their file addresses; parsing does not relocate
// or validate a load image.
struct ParsedObject {
    std::optional<HeaderRecord> header;
    std::vector<TextRecord> textRecords;
    std::vector<ModificationRecord> modificationRecords;
    std::optional<EndRecord> end;
    LoaderSourceLocation location;
    std::string originalText;
};

struct ObjectParseResult {
    std::optional<ParsedObject> object;
    std::vector<LoaderDiagnostic> diagnostics;

    bool success() const noexcept {
        return object.has_value() && diagnostics.empty();
    }
};

struct RelocatedBlock {
    std::uint32_t originalAddress = 0;
    std::uint32_t finalAddress = 0;
    // Object bytes before applying modification records.
    std::vector<std::uint8_t> bytes;
};

struct RelocatedModification {
    std::uint32_t originalAddress = 0;
    std::uint32_t finalAddress = 0;
    std::uint8_t fieldLengthHalfBytes = 0;
};

// A validated plan uses final addresses computed as original address plus the
// signed delta. Callers must reject results outside the SIC/XE 24-bit range.
struct ValidatedLoadPlan {
    std::int64_t relocationDelta = 0;
    std::uint32_t originalStartAddress = 0;
    std::uint32_t finalStartAddress = 0;
    std::optional<std::uint32_t> originalExecutionAddress;
    std::optional<std::uint32_t> finalExecutionAddress;
    std::vector<RelocatedBlock> relocatedBlocks;
    std::vector<RelocatedModification> relocatedModifications;
};

struct LoadPlanResult {
    std::optional<ValidatedLoadPlan> plan;
    std::vector<LoaderDiagnostic> diagnostics;

    bool success() const noexcept {
        return plan.has_value() && diagnostics.empty();
    }
};

struct LoadWriteBlock {
    std::uint32_t address = 0;
    std::vector<std::uint8_t> bytes;
};

// This stage contains only the final memory writes and entry point, after the
// object records have been parsed and the load plan has been validated.
struct LoadWritePlan {
    std::vector<LoadWriteBlock> blocks;
    std::optional<std::uint32_t> executionAddress;
};

struct LoadWritePlanResult {
    std::optional<LoadWritePlan> plan;
    std::vector<LoaderDiagnostic> diagnostics;

    bool success() const noexcept {
        return plan.has_value() && diagnostics.empty();
    }
};
