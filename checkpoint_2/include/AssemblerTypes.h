#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

struct SourceLocation {
    std::string file;
    int line = 0;
};

struct Diagnostic {
    SourceLocation where;
    std::string message;
};

struct ParsedLine {
    std::optional<std::string> label;
    std::string operation;
    std::string operand;
    std::string comment;
    SourceLocation location;
    std::string originalText;
};

struct IntermediateLine {
    ParsedLine parsed;
    std::uint32_t address = 0;
    std::uint32_t sizeInBytes = 0;
};

using SymbolTable = std::map<std::string, std::uint32_t>;

struct AssemblyResult {
    std::string objectProgram;
    SymbolTable symbols;
    std::vector<Diagnostic> diagnostics;

    bool success() const noexcept;
};

std::string normalizeIdentifier(const std::string& identifier);
