#include "AssemblerTypes.h"

bool AssemblyResult::success() const noexcept {
    return diagnostics.empty();
}

std::string normalizeIdentifier(const std::string& identifier) {
    std::string normalized = identifier;
    for (char& character : normalized) {
        if (character >= 'a' && character <= 'z') {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }
    return normalized;
}
