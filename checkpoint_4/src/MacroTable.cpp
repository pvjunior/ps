#include "MacroTable.h"

#include <cctype>
#include <utility>

namespace {

std::string normalizeMacroName(const std::string& name) {
    std::size_t begin = 0;
    std::size_t end = name.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(name[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(name[end - 1]))) {
        --end;
    }

    std::string normalized = name.substr(begin, end - begin);
    for (char& character : normalized) {
        const unsigned char value = static_cast<unsigned char>(character);
        if (value >= 'a' && value <= 'z') {
            character = static_cast<char>(value - 'a' + 'A');
        }
    }
    return normalized;
}

} // namespace

bool MacroTable::define(const MacroDefinition& definition, MacroDiagnostic* error) {
    const std::string normalizedName = normalizeMacroName(definition.name);
    if (normalizedName.empty()) {
        if (error != nullptr) {
            error->where = definition.location;
            error->message = "macro definition has an empty name";
        }
        return false;
    }

    MacroDefinition stored = definition;
    stored.name = normalizedName;
    definitions_[normalizedName].push_back(std::move(stored));
    if (error != nullptr) {
        *error = MacroDiagnostic{};
    }
    return true;
}

const MacroDefinition* MacroTable::find(const std::string& name) const {
    const auto found = definitions_.find(normalizeMacroName(name));
    if (found == definitions_.end() || found->second.empty()) {
        return nullptr;
    }
    return &found->second.back();
}

