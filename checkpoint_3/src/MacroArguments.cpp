#include "MacroArguments.h"

#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace {

bool isSpace(char character) {
    return std::isspace(static_cast<unsigned char>(character)) != 0;
}

bool isIdentifierCharacter(char character) {
    const unsigned char value = static_cast<unsigned char>(character);
    return std::isalnum(value) != 0 || character == '_' || character == '$';
}

char upperAscii(char character) {
    if (character >= 'a' && character <= 'z') {
        return static_cast<char>(character - 'a' + 'A');
    }
    return character;
}

std::string upperAscii(std::string text) {
    for (char& character : text) {
        character = upperAscii(character);
    }
    return text;
}

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && isSpace(text[begin])) {
        ++begin;
    }
    while (end > begin && isSpace(text[end - 1])) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string normalizeParameterName(const std::string& name) {
    std::string normalized = upperAscii(trim(name));
    while (!normalized.empty() && normalized.front() == '&') {
        normalized.erase(normalized.begin());
    }
    return normalized;
}

std::string positionKey(int level, int index) {
    return "#(" + std::to_string(level) + "," + std::to_string(index) + ")";
}

std::vector<std::string> splitArguments(const std::vector<std::string>& input) {
    std::vector<std::string> output;
    for (const std::string& item : input) {
        std::size_t start = 0;
        int parenthesisDepth = 0;
        bool inQuote = false;
        for (std::size_t i = 0; i < item.size(); ++i) {
            const char character = item[i];
            if (character == '\'') {
                if (inQuote && i + 1 < item.size() && item[i + 1] == '\'') {
                    ++i;
                    continue;
                }
                inQuote = !inQuote;
            } else if (!inQuote && character == '(') {
                ++parenthesisDepth;
            } else if (!inQuote && character == ')' && parenthesisDepth > 0) {
                --parenthesisDepth;
            } else if (!inQuote && parenthesisDepth == 0 && character == ',') {
                output.push_back(trim(item.substr(start, i - start)));
                start = i + 1;
            }
        }
        output.push_back(trim(item.substr(start)));
    }
    return output;
}

std::size_t findUnquotedEquals(const std::string& text) {
    bool inQuote = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\'') {
            if (inQuote && i + 1 < text.size() && text[i + 1] == '\'') {
                ++i;
                continue;
            }
            inQuote = !inQuote;
        } else if (!inQuote && text[i] == '=') {
            return i;
        }
    }
    return std::string::npos;
}

std::string normalizeArgumentName(const std::string& name) {
    return normalizeParameterName(name);
}

std::vector<int> effectiveParameterIndices(const MacroDefinition& definition) {
    std::map<int, std::set<int>> indicesByLevel;
    std::set<int> levelsWithDuplicates;
    for (const MacroParameter& parameter : definition.parameters) {
        if (!indicesByLevel[parameter.definitionLevel].insert(parameter.index).second) {
            levelsWithDuplicates.insert(parameter.definitionLevel);
        }
    }

    std::map<int, int> nextFallbackIndex;
    std::vector<int> result;
    result.reserve(definition.parameters.size());
    for (const MacroParameter& parameter : definition.parameters) {
        if (levelsWithDuplicates.find(parameter.definitionLevel) == levelsWithDuplicates.end()) {
            result.push_back(parameter.index);
        } else {
            result.push_back(nextFallbackIndex[parameter.definitionLevel]++);
        }
    }
    return result;
}

void addDiagnostic(MacroBindingResult& result,
                   const MacroSourceLocation& location,
                   const std::string& message) {
    MacroDiagnostic diagnostic;
    diagnostic.where = location;
    diagnostic.message = message;
    result.diagnostics.push_back(std::move(diagnostic));
}

std::string canonicalMapKey(const std::string& key) {
    const std::string trimmed = trim(key);
    if (!trimmed.empty() && trimmed.front() == '#') {
        return trimmed;
    }
    return "&" + normalizeParameterName(trimmed);
}

std::string substituteOutsideCommentsAndConstants(
    const std::string& text,
    const MacroArgumentMap& arguments,
    const std::optional<std::size_t>& serial) {
    std::map<std::string, std::string> normalizedArguments;
    for (const auto& entry : arguments) {
        normalizedArguments[canonicalMapKey(entry.first)] = entry.second;
    }

    std::string output;
    output.reserve(text.size());
    bool inQuote = false;
    std::size_t firstNonSpace = 0;
    while (firstNonSpace < text.size() && isSpace(text[firstNonSpace])) {
        ++firstNonSpace;
    }

    for (std::size_t i = 0; i < text.size();) {
        const char character = text[i];
        if (character == '\'') {
            output.push_back(character);
            if (inQuote && i + 1 < text.size() && text[i + 1] == '\'') {
                output.push_back(text[i + 1]);
                i += 2;
                continue;
            }
            inQuote = !inQuote;
            ++i;
            continue;
        }

        // A dot begins a comment at the first field or after whitespace. Keep
        // the rest of the line byte-for-byte, including any parameter names.
        if (!inQuote && character == '.' &&
            (i == firstNonSpace || (i > 0 && isSpace(text[i - 1])))) {
            output.append(text, i, std::string::npos);
            break;
        }

        if (!inQuote && serial.has_value() && i + 4 <= text.size() &&
            text.compare(i, 4, ".SER") == 0 &&
            (i + 4 == text.size() || !isIdentifierCharacter(text[i + 4]))) {
            output += std::to_string(*serial);
            i += 4;
            continue;
        }

        if (!inQuote && character == '#' && i + 2 < text.size() && text[i + 1] == '(') {
            const std::size_t close = text.find(')', i + 2);
            if (close != std::string::npos &&
                (i == 0 || !isIdentifierCharacter(text[i - 1])) &&
                (close + 1 == text.size() || !isIdentifierCharacter(text[close + 1]))) {
                const std::string key = text.substr(i, close - i + 1);
                const auto found = normalizedArguments.find(key);
                if (found != normalizedArguments.end()) {
                    output += found->second;
                    i = close + 1;
                    continue;
                }
            }
        }

        if (!inQuote && character == '&' && i + 1 < text.size() &&
            (i == 0 || !isIdentifierCharacter(text[i - 1])) &&
            isIdentifierCharacter(text[i + 1])) {
            std::size_t end = i + 2;
            while (end < text.size() && isIdentifierCharacter(text[end])) {
                ++end;
            }
            const std::string key = upperAscii(text.substr(i, end - i));
            const auto found = normalizedArguments.find(key);
            if (found != normalizedArguments.end()) {
                output += found->second;
            } else {
                output.append(text, i, end - i);
            }
            i = end;
            continue;
        }

        output.push_back(character);
        ++i;
    }
    return output;
}

} // namespace

MacroBindingResult bindMacroArguments(
    const MacroDefinition& definition,
    const std::vector<std::string>& arguments,
    const MacroSourceLocation& callLocation) {
    MacroBindingResult result;
    const std::vector<std::string> actuals = splitArguments(arguments);
    const std::vector<int> indices = effectiveParameterIndices(definition);
    std::vector<std::optional<std::string>> bound(definition.parameters.size());

    std::map<std::string, std::vector<std::size_t>> parametersByName;
    for (std::size_t i = 0; i < definition.parameters.size(); ++i) {
        const std::string name = normalizeParameterName(definition.parameters[i].name);
        if (!name.empty()) {
            parametersByName[name].push_back(i);
        }
    }

    std::size_t nextPositional = 0;
    for (const std::string& rawArgument : actuals) {
        const std::size_t equals = findUnquotedEquals(rawArgument);
        if (equals != std::string::npos) {
            const std::string name = normalizeArgumentName(rawArgument.substr(0, equals));
            const std::string value = trim(rawArgument.substr(equals + 1));
            const auto named = parametersByName.find(name);
            if (named == parametersByName.end()) {
                addDiagnostic(result, callLocation,
                              "argument nomeado desconhecido: " + trim(rawArgument.substr(0, equals)));
                continue;
            }
            if (named->second.size() != 1) {
                addDiagnostic(result, callLocation,
                              "argumento nomeado ambiguo: " + trim(rawArgument.substr(0, equals)));
                continue;
            }
            const std::size_t slot = named->second.front();
            if (bound[slot].has_value()) {
                addDiagnostic(result, callLocation,
                              "argumento repetido para o parametro " +
                                  definition.parameters[slot].name);
                continue;
            }
            bound[slot] = value;
            continue;
        }

        while (nextPositional < bound.size() && bound[nextPositional].has_value()) {
            ++nextPositional;
        }
        if (nextPositional >= bound.size()) {
            addDiagnostic(result, callLocation, "excesso de argumentos posicionais");
            continue;
        }
        bound[nextPositional] = trim(rawArgument);
        ++nextPositional;
    }

    for (std::size_t i = 0; i < definition.parameters.size(); ++i) {
        if (!bound[i].has_value()) {
            const MacroParameter& parameter = definition.parameters[i];
            if (parameter.defaultValue.has_value()) {
                bound[i] = *parameter.defaultValue;
            } else {
                addDiagnostic(result, callLocation,
                              "argument ausente para o parametro " + parameter.name);
            }
        }
    }

    std::set<std::string> ambiguousNames;
    for (const auto& entry : parametersByName) {
        if (entry.second.size() > 1) {
            ambiguousNames.insert(entry.first);
        }
    }

    for (std::size_t i = 0; i < definition.parameters.size(); ++i) {
        if (!bound[i].has_value()) {
            continue;
        }
        const MacroParameter& parameter = definition.parameters[i];
        result.arguments[positionKey(parameter.definitionLevel, indices[i])] = *bound[i];
        const std::string name = normalizeParameterName(parameter.name);
        if (!name.empty() && ambiguousNames.find(name) == ambiguousNames.end()) {
            result.arguments["&" + name] = *bound[i];
        }
    }

    return result;
}

std::string substituteMacroParameters(
    const std::string& text,
    const MacroArgumentMap& arguments) {
    return substituteOutsideCommentsAndConstants(text, arguments, std::nullopt);
}

std::string substituteMacroParameters(
    const std::string& text,
    const MacroArgumentMap& arguments,
    std::size_t serial) {
    return substituteOutsideCommentsAndConstants(text, arguments, serial);
}

MacroLine substituteMacroParameters(
    const MacroLine& line,
    const MacroArgumentMap& arguments,
    std::size_t serial) {
    MacroLine substituted = line;
    substituted.label = substituteMacroParameters(line.label, arguments, serial);
    substituted.operation = substituteMacroParameters(line.operation, arguments, serial);
    substituted.operands = substituteMacroParameters(line.operands, arguments, serial);
    substituted.originalText = substituteMacroParameters(line.originalText, arguments, serial);
    substituted.text = substituteMacroParameters(line.text, arguments, serial);
    return substituted;
}

