#include "MacroParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

namespace {

bool isSpace(char character) {
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

char toUpperAscii(char character) {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - 'a' + 'A');
    }
    return character;
}

std::string upperAscii(std::string text) {
    for (char& character : text) {
        character = toUpperAscii(character);
    }
    return text;
}

std::size_t skipSpaces(const std::string& text, std::size_t position) {
    while (position < text.size() && isSpace(text[position])) {
        ++position;
    }
    return position;
}

std::size_t trimEnd(const std::string& text, std::size_t end) {
    while (end > 0 && isSpace(text[end - 1])) {
        --end;
    }
    return end;
}

std::string trimmedRange(const std::string& text, std::size_t begin,
                         std::size_t end) {
    begin = skipSpaces(text, begin);
    end = trimEnd(text, end);
    if (begin >= end) {
        return {};
    }
    return text.substr(begin, end - begin);
}

struct Token {
    std::string text;
    std::size_t end = 0;
};

Token readToken(const std::string& text, std::size_t begin,
                std::size_t end) {
    begin = skipSpaces(text, begin);
    std::size_t position = begin;
    bool inQuote = false;

    while (position < end) {
        if (text[position] == '\'') {
            inQuote = !inQuote;
        } else if (isSpace(text[position]) && !inQuote) {
            break;
        }
        ++position;
    }

    return Token{text.substr(begin, position - begin), position};
}

bool isDelimiter(const std::string& token) {
    const std::string normalized = upperAscii(token);
    return normalized == "MCDEFN" || normalized == "MCEND";
}

// A dot begins a comment only at a field boundary. This preserves macro
// constructs such as ST.SER and @.B while still accepting ". comment".
bool startsComment(const std::string& text, std::size_t position,
                   std::size_t firstNonSpace) {
    return text[position] == '.' &&
           (position == firstNonSpace ||
            (position > 0 && isSpace(text[position - 1])));
}

std::size_t findComment(const std::string& text, std::size_t firstNonSpace,
                        bool& unterminatedQuote) {
    bool inQuote = false;

    for (std::size_t position = firstNonSpace; position < text.size();
         ++position) {
        if (text[position] == '\'') {
            inQuote = !inQuote;
        } else if (!inQuote && startsComment(text, position, firstNonSpace)) {
            unterminatedQuote = false;
            return position;
        }
    }

    unterminatedQuote = inQuote;
    return std::string::npos;
}

void addDiagnostic(MacroParseResult& result,
                   const MacroSourceLocation& location,
                   const std::string& message) {
    result.diagnostics.push_back(MacroDiagnostic{location, message});
}

MacroLine parseLine(const std::string& original,
                    const MacroSourceLocation& location,
                    MacroParseResult& result) {
    MacroLine parsed;
    parsed.location = location;
    parsed.originalText = original;

    const std::size_t contentStart = skipSpaces(original, 0);
    if (contentStart == original.size()) {
        return parsed;
    }

    if (original[contentStart] == '.') {
        parsed.comment = original.substr(contentStart);
        return parsed;
    }

    bool unterminatedQuote = false;
    const std::size_t commentStart =
        findComment(original, contentStart, unterminatedQuote);
    const std::size_t sourceEnd =
        commentStart == std::string::npos ? original.size() : commentStart;

    if (commentStart != std::string::npos) {
        parsed.comment = original.substr(commentStart);
    }
    if (unterminatedQuote) {
        addDiagnostic(result, location, "unterminated single quote");
    }

    const Token first = readToken(original, contentStart, sourceEnd);
    if (first.text.empty()) {
        return parsed;
    }

    const bool indented = contentStart > 0;
    const std::size_t afterFirst = skipSpaces(original, first.end);

    // MCDEFN and MCEND are operations even if written in the first column.
    if (indented || isDelimiter(first.text) || afterFirst >= sourceEnd) {
        parsed.operation = first.text;
        parsed.operands = trimmedRange(original, first.end, sourceEnd);
        return parsed;
    }

    const Token second = readToken(original, afterFirst, sourceEnd);
    parsed.label = first.text;
    parsed.operation = second.text;
    parsed.operands = trimmedRange(original, second.end, sourceEnd);
    return parsed;
}

} // namespace

MacroParseResult parseMacroText(const std::string& text,
                                const std::string& sourceName) {
    MacroParseResult result;
    std::istringstream input(text);
    std::string line;
    int lineNumber = 1;

    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const MacroSourceLocation location{sourceName, lineNumber};
        result.lines.push_back(parseLine(line, location, result));
        ++lineNumber;
    }

    return result;
}

MacroParseResult parseMacroFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        MacroParseResult result;
        addDiagnostic(result, MacroSourceLocation{path, 0},
                      "could not open macro source file");
        return result;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    return parseMacroText(contents.str(), path);
}
