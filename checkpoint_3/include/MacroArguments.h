#pragma once

#include "MacroTypes.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// Chaves por nome usam o formato "&PARAM"; chaves por posicao usam "#(d,i)".
// A segunda forma identifica parametros de niveis diferentes sem ambiguidade.
using MacroArgumentMap = std::map<std::string, std::string>;

struct MacroBindingResult {
    MacroArgumentMap arguments;
    std::vector<MacroDiagnostic> diagnostics;

    bool success() const noexcept {
        return diagnostics.empty();
    }
};

MacroBindingResult bindMacroArguments(
    const MacroDefinition& definition,
    const std::vector<std::string>& arguments,
    const MacroSourceLocation& callLocation);

// A variante sem serial preserva .SER. A variante com serial troca apenas
// ocorrencias completas de .SER fora de comentarios e constantes entre aspas.
std::string substituteMacroParameters(
    const std::string& text,
    const MacroArgumentMap& arguments);

std::string substituteMacroParameters(
    const std::string& text,
    const MacroArgumentMap& arguments,
    std::size_t serial);

// Copia a linha e substitui os campos de codigo, mantendo comentario e metadados.
MacroLine substituteMacroParameters(
    const MacroLine& line,
    const MacroArgumentMap& arguments,
    std::size_t serial);

