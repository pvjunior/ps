#pragma once

#include "MacroTable.h"
#include "MacroTypes.h"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

// Motor de expansao de macros em UMA passagem sobre o fonte principal
// (Parte 4). Consome apenas as APIs publicas de MacroTypes.h, MacroTable.h
// e MacroArguments.h. O parser (Parte 2) ja entrega as linhas separadas em
// rotulo/operacao/operandos; esta classe decide o que e definicao, chamada
// ou linha comum.
//
// Resumo do algoritmo:
//   * MCDEFN incrementa o nivel de definicao; MCEND decrementa. A definicao
//     externa so termina quando o nivel volta a zero.
//   * Uma pilha de quadros de parametros formais (um por nivel) converte
//     "&NOME" para "#(d,i)" ANTES de a definicao ir para a tabela.
//   * Operacao presente na tabela => associa argumentos e expande o corpo no
//     ponto da chamada; as linhas geradas sao processadas recursivamente
//     (chamadas e definicoes dentro de macros).
//   * Nao ha referencia adiante: so e chamada o que ja esta ativo na tabela.
//   * Cada expansao recebe um serial novo, que substitui ".SER".
class MacroProcessor {
public:
    static constexpr std::size_t kDefaultMaxExpansionDepth = 32;

    explicit MacroProcessor(
        std::size_t maxExpansionDepth = kDefaultMaxExpansionDepth);

    // Limite de chamadas de macro aninhadas (pilha de expansao).
    void setMaxExpansionDepth(std::size_t depth);
    std::size_t maxExpansionDepth() const noexcept;

    // Opcional: escreve em `out` um rastro do nivel de definicao e da pilha
    // de expansao (util para demonstracao). nullptr desliga o rastro.
    void setTraceStream(std::ostream* out) noexcept;

    // Cada chamada recomeca com tabela vazia e serial zerado.
    MacroExpansionResult process(const std::vector<MacroLine>& lines);

private:
    void processLines(const std::vector<MacroLine>& lines);
    void handleLine(const MacroLine& line);
    void expandCall(const MacroLine& call, const MacroDefinition& definition);
    void emit(MacroLine line);
    void flushPendingLabel(const MacroSourceLocation& where);
    void addDiagnostic(const MacroSourceLocation& where,
                       const std::string& message);
    void trace(const std::string& message) const;

    MacroTable table_;
    std::size_t maxDepth_;
    std::ostream* trace_ = nullptr;

    // Estado de uma chamada a process().
    MacroExpansionResult* result_ = nullptr;
    std::size_t serial_ = 0;
    std::size_t lineDepth_ = 0; // nivel de definicao corrente (so para rastro)
    std::vector<const MacroDefinition*> callStack_;
    std::string pendingLabel_;
};
