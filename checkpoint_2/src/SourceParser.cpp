#include "SourceParser.h"

#include <cctype>
#include <fstream>
#include <sstream>

namespace {

bool isSpace(char character) {
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

std::size_t firstNonSpace(const std::string& text) {
    std::size_t position = 0;
    while (position < text.size() && isSpace(text[position])) {
        ++position;
    }
    return position;
}

std::size_t findComment(const std::string& text, bool& quoteOpen) {
    bool inQuote = false;
    for (std::size_t position = 0; position < text.size(); ++position) {
        if (text[position] == '\'') {
            inQuote = !inQuote;
        } else if (text[position] == '.' && !inQuote) {
            quoteOpen = false;
            return position;
        }
    }
    quoteOpen = inQuote;
    return std::string::npos;
}

std::vector<std::string> splitFields(const std::string& text) {
    std::vector<std::string> fields;
    std::size_t position = 0;

    while (position < text.size()) {
        while (position < text.size() && isSpace(text[position])) {
            ++position;
        }
        if (position == text.size()) {
            break;
        }

        const std::size_t start = position;
        bool inQuote = false;
        while (position < text.size()) {
            if (text[position] == '\'') {
                inQuote = !inQuote;
            } else if (isSpace(text[position]) && !inQuote) {
                break;
            }
            ++position;
        }
        fields.push_back(text.substr(start, position - start));
    }

    return fields;
}

void addDiagnostic(ParseResult& result, const SourceLocation& location,
                   const std::string& message) {
    result.diagnostics.push_back(Diagnostic{location, message});
}

ParsedLine parseLine(const std::string& original, const SourceLocation& location,
                     ParseResult& result) {
    ParsedLine parsed;
    parsed.location = location;
    parsed.originalText = original;

    const std::size_t contentStart = firstNonSpace(original);
    if (contentStart == original.size()) {
        parsed.comment = original;
        return parsed;
    }
    if (original[contentStart] == '.') {
        parsed.comment = original;
        return parsed;
    }

    bool quoteOpen = false;
    const std::size_t commentStart = findComment(original, quoteOpen);
    const std::string source = original.substr(0, commentStart);
    if (commentStart != std::string::npos) {
        parsed.comment = original.substr(commentStart);
    }
    if (quoteOpen) {
        addDiagnostic(result, location, "unterminated single quote");
    }

    const std::vector<std::string> fields = splitFields(source);
    const bool hasLabel = !original.empty() && !isSpace(original.front());
    const std::size_t maximumFields = hasLabel ? 3U : 2U;
    if (fields.size() > maximumFields) {
        addDiagnostic(result, location, "too many fields in source line");
    }

    if (fields.empty()) {
        return parsed;
    }

    std::size_t operationIndex = 0;
    if (hasLabel) {
        parsed.label = normalizeIdentifier(fields[0]);
        operationIndex = 1;
    }
    if (operationIndex < fields.size()) {
        parsed.operation = normalizeIdentifier(fields[operationIndex]);
    }
    if (operationIndex + 1 < fields.size()) {
        parsed.operand = fields[operationIndex + 1];
    }

    return parsed;
}

} // namespace

ParseResult parseText(const std::string& text, const std::string& sourceName) {
    ParseResult result;
    std::istringstream input(text);
    std::string line;
    int lineNumber = 1;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const SourceLocation location{sourceName, lineNumber};
        result.lines.push_back(parseLine(line, location, result));
        ++lineNumber;
    }

    return result;
}

ParseResult parseFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        ParseResult result;
        addDiagnostic(result, SourceLocation{path, 0},
                      "could not open source file");
        return result;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return parseText(contents.str(), path);
}
