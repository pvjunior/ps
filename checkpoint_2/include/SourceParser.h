#pragma once

#include "AssemblerTypes.h"

#include <string>
#include <vector>

struct ParseResult {
    std::vector<ParsedLine> lines;
    std::vector<Diagnostic> diagnostics;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

ParseResult parseFile(const std::string& path);
ParseResult parseText(const std::string& text,
                      const std::string& sourceName = "<memory>");
