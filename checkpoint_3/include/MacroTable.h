#pragma once

#include "MacroTypes.h"

#include <deque>
#include <string>
#include <unordered_map>

// Armazena definicoes por nome normalizado. Cada redefinicao e mantida no
// historico da tabela; find() sempre retorna a versao ativa.
class MacroTable {
public:
    bool define(const MacroDefinition& definition, MacroDiagnostic* error = nullptr);
    const MacroDefinition* find(const std::string& name) const;

private:
    std::unordered_map<std::string, std::deque<MacroDefinition>> definitions_;
};

