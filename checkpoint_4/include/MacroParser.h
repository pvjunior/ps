#pragma once

#include "MacroTypes.h"

#include <string>
#include <vector>

struct MacroParseResult {
    std::vector<MacroLine> lines;
    std::vector<MacroDiagnostic> diagnostics;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

MacroParseResult parseMacroFile(const std::string& path);

MacroParseResult parseMacroText(
    const std::string& text,
    const std::string& sourceName = "<memory>");
