#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct MacroSourceLocation {
    std::string file;
    int line = 0;
};

struct MacroDiagnostic {
    MacroSourceLocation where;
    std::string message;
    std::string originalText;

    MacroDiagnostic() = default;
    MacroDiagnostic(MacroSourceLocation location, std::string diagnosticMessage,
                    std::string sourceText = {})
        : where(std::move(location)),
          message(std::move(diagnosticMessage)),
          originalText(std::move(sourceText)) {}
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
    // Keep MCDEFN/MCEND lines in the body to represent nested definitions.
    std::vector<MacroLine> body;
    MacroSourceLocation location;
};

struct MacroExpansionResult {
    std::vector<MacroLine> expandedLines;
    std::vector<MacroDiagnostic> diagnostics;
    std::size_t macrosDefined = 0;
    // Also supplies a unique serial for each expansion and `.SER` substitution.
    std::size_t expansionsPerformed = 0;

    bool success() const noexcept;
};

// Trims surrounding whitespace, removes an optional leading '&', and
// uppercases ASCII letters for both macro names and parameter names.
std::string normalizeMacroIdentifier(const std::string& identifier);
