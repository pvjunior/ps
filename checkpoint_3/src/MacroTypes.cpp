#include "MacroTypes.h"

#include <cctype>

bool MacroExpansionResult::success() const noexcept {
    return diagnostics.empty();
}

std::string normalizeMacroIdentifier(const std::string& identifier) {
    std::size_t begin = 0;
    std::size_t end = identifier.size();
    while (begin < end &&
           std::isspace(static_cast<unsigned char>(identifier[begin])) != 0) {
        ++begin;
    }
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(identifier[end - 1])) != 0) {
        --end;
    }

    if (begin < end && identifier[begin] == '&') {
        ++begin;
        while (begin < end &&
               std::isspace(static_cast<unsigned char>(identifier[begin])) != 0) {
            ++begin;
        }
    }

    std::string normalized = identifier.substr(begin, end - begin);
    for (char& character : normalized) {
        if (character >= 'a' && character <= 'z') {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }
    return normalized;
}
