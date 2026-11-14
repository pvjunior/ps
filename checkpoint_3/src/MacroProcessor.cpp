#include "MacroProcessor.h"

#include "MacroArguments.h"

#include <cctype>
#include <optional>
#include <ostream>
#include <utility>

namespace {

bool isSpace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string upper(std::string text) {
    for (char& c : text) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return text;
}

std::string trim(const std::string& text) {
    std::size_t b = 0;
    std::size_t e = text.size();
    while (b < e && isSpace(text[b])) ++b;
    while (e > b && isSpace(text[e - 1])) --e;
    return text.substr(b, e - b);
}

bool isOperation(const MacroLine& line, const char* name) {
    return upper(trim(line.operation)) == name;
}

bool isBlankOrComment(const MacroLine& line) {
    return trim(line.label).empty() && trim(line.operation).empty();
}

// Divide por virgulas fora de aspas e fora de parenteses ("#(2,1)" tem uma
// virgula interna que nao separa parametros).
std::vector<std::string> splitTopLevel(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    int parens = 0;
    bool quote = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '\'') {
            quote = !quote;
        } else if (!quote && c == '(') {
            ++parens;
        } else if (!quote && c == ')' && parens > 0) {
            --parens;
        } else if (!quote && parens == 0 && c == ',') {
            parts.push_back(trim(text.substr(start, i - start)));
            start = i + 1;
        }
    }
    parts.push_back(trim(text.substr(start)));
    return parts;
}

std::size_t findUnquotedEquals(const std::string& text) {
    bool quote = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\'') {
            quote = !quote;
        } else if (!quote && text[i] == '=') {
            return i;
        }
    }
    return std::string::npos;
}

// Um quadro da pilha de parametros formais: um por nivel de definicao.
struct Frame {
    std::vector<MacroParameter> parameters;
};

// "#(d,i)" -> (d,i). Aceita lixo apos ')' (ex.: "=padrao") e o ignora.
bool parsePositionName(const std::string& text, int& level, int& index) {
    if (text.size() < 6 || text[0] != '#' || text[1] != '(') return false;
    const std::size_t comma = text.find(',');
    const std::size_t close = text.find(')');
    if (comma == std::string::npos || close == std::string::npos ||
        comma > close) {
        return false;
    }
    try {
        level = std::stoi(text.substr(2, comma - 2));
        index = std::stoi(text.substr(comma + 1, close - comma - 1));
    } catch (...) {
        return false;
    }
    return level > 0 && index > 0;
}

struct Prototype {
    std::string name;
    std::vector<MacroParameter> parameters;
    std::vector<std::string> errors;
};

// Protótipo: "NOME &A,&B=5" (rotulo + operacao) ou "  NOME &A" (so operacao).
// Parametros ja convertidos ("#(d,i)", vindos de uma definicao aninhada que
// esta sendo registrada na expansao) mantem nivel e posicao originais.
Prototype parsePrototype(const MacroLine& line, int level) {
    Prototype proto;
    std::string paramText;
    if (!trim(line.label).empty()) {
        proto.name = trim(line.label);
        paramText = trim(line.operation);
        if (!trim(line.operands).empty()) {
            paramText += " " + trim(line.operands);
        }
    } else {
        proto.name = trim(line.operation);
        paramText = trim(line.operands);
    }

    if (paramText.empty()) {
        return proto;
    }

    int nextIndex = 1;
    for (const std::string& piece : splitTopLevel(paramText)) {
        if (piece.empty()) {
            proto.errors.push_back("parametro formal vazio no prototipo");
            continue;
        }
        const std::size_t eq = findUnquotedEquals(piece);
        const std::string namePart =
            trim(eq == std::string::npos ? piece : piece.substr(0, eq));

        MacroParameter parameter;
        int fixedLevel = 0;
        int fixedIndex = 0;
        if (parsePositionName(namePart, fixedLevel, fixedIndex)) {
            parameter.name = namePart;
            parameter.definitionLevel = fixedLevel;
            parameter.index = fixedIndex;
        } else if (namePart.size() > 1 && namePart[0] == '&') {
            parameter.name = upper(namePart);
            parameter.definitionLevel = level;
            parameter.index = nextIndex;
        } else {
            proto.errors.push_back("parametro formal invalido: " + piece);
            continue;
        }
        ++nextIndex;
        if (eq != std::string::npos) {
            parameter.defaultValue = trim(piece.substr(eq + 1));
        }
        proto.parameters.push_back(std::move(parameter));
    }
    return proto;
}

// "&NOME" -> "#(d,i)", do nivel mais externo para o mais interno (o mais
// interno vence em nomes coincidentes). Parametros ja posicionais nao entram.
MacroArgumentMap buildFormalMap(const std::vector<Frame>& frames) {
    MacroArgumentMap map;
    for (const Frame& frame : frames) {
        for (const MacroParameter& p : frame.parameters) {
            if (p.name.empty() || p.name[0] != '&') continue;
            map[p.name] = "#(" + std::to_string(p.definitionLevel) + "," +
                          std::to_string(p.index) + ")";
        }
    }
    return map;
}

// Aplica o mapa nos campos de codigo, preservando ".SER" e o comentario.
MacroLine applyMapKeepSerial(const MacroLine& line,
                             const MacroArgumentMap& map) {
    if (map.empty()) return line;
    MacroLine out = line;
    out.label = substituteMacroParameters(line.label, map);
    out.operation = substituteMacroParameters(line.operation, map);
    out.operands = substituteMacroParameters(line.operands, map);
    out.originalText = substituteMacroParameters(line.originalText, map);
    out.text = substituteMacroParameters(line.text, map);
    return out;
}

// Estado de captura de UMA definicao externa (com aninhamentos).
struct Capture {
    int depth = 0;                // contador de MCDEFN/MCEND
    bool awaitingPrototype = false;
    bool invalid = false;         // prototipo invalido: nao registrar
    MacroSourceLocation location; // MCDEFN externo
    std::string name;
    std::vector<MacroParameter> outerParameters;
    std::vector<Frame> frames;    // pilha de parametros formais
    std::vector<MacroLine> body;

    void reset() { *this = Capture{}; }
};

std::string labelPadded(const std::string& label) {
    std::string padded = label;
    do {
        padded.push_back(' ');
    } while (padded.size() < 8);
    return padded;
}

} // namespace

MacroProcessor::MacroProcessor(std::size_t maxExpansionDepth)
    : maxDepth_(maxExpansionDepth == 0 ? 1 : maxExpansionDepth) {}

void MacroProcessor::setMaxExpansionDepth(std::size_t depth) {
    maxDepth_ = depth == 0 ? 1 : depth;
}

std::size_t MacroProcessor::maxExpansionDepth() const noexcept {
    return maxDepth_;
}

void MacroProcessor::setTraceStream(std::ostream* out) noexcept {
    trace_ = out;
}

void MacroProcessor::trace(const std::string& message) const {
    if (trace_ != nullptr) {
        *trace_ << message << '\n';
    }
}

void MacroProcessor::addDiagnostic(const MacroSourceLocation& where,
                                   const std::string& message) {
    result_->diagnostics.push_back(MacroDiagnostic{where, message});
}

MacroExpansionResult MacroProcessor::process(
    const std::vector<MacroLine>& lines) {
    MacroExpansionResult result;
    table_ = MacroTable{};
    serial_ = 0;
    callStack_.clear();
    pendingLabel_.clear();
    result_ = &result;

    processLines(lines);

    result_ = nullptr;
    return result;
}

// ---------------------------------------------------------------------------
// Varredura de uma lista de linhas (o fonte principal ou o corpo de uma
// expansao). A captura de definicao e local: um corpo de macro e sempre
// balanceado, entao toda definicao termina dentro da mesma lista.
// ---------------------------------------------------------------------------
void MacroProcessor::processLines(const std::vector<MacroLine>& lines) {
    Capture cap;

    for (const MacroLine& line : lines) {
        if (cap.depth == 0) {
            if (isOperation(line, "MCDEFN")) {
                cap.reset();
                cap.depth = 1;
                cap.awaitingPrototype = true;
                cap.location = line.location;
                trace("  [def] MCDEFN (" + std::to_string(line.location.line) +
                      ") nivel de definicao = 1");
                continue;
            }
            if (isOperation(line, "MCEND")) {
                addDiagnostic(line.location,
                              "MCEND sem MCDEFN correspondente");
                continue;
            }
            handleLine(line);
            continue;
        }

        // ---- capturando uma definicao (depth >= 1) ----
        if (cap.awaitingPrototype) {
            if (isBlankOrComment(line)) {
                if (cap.depth > 1) cap.body.push_back(line);
                continue;
            }
            cap.awaitingPrototype = false;

            if (isOperation(line, "MCDEFN") || isOperation(line, "MCEND")) {
                // MCDEFN logo seguido de MCDEFN/MCEND: falta o prototipo.
                addDiagnostic(line.location, "MCDEFN sem prototipo");
                cap.frames.push_back(Frame{});
                if (cap.depth == 1) cap.invalid = true;
                // cai no tratamento normal abaixo
            } else {
                const Prototype proto = parsePrototype(line, cap.depth);
                for (const std::string& error : proto.errors) {
                    addDiagnostic(line.location, error);
                }
                Frame frame;
                frame.parameters = proto.parameters;
                cap.frames.push_back(std::move(frame));

                if (cap.depth == 1) {
                    cap.name = proto.name;
                    cap.outerParameters = proto.parameters;
                    if (proto.name.empty()) {
                        addDiagnostic(line.location,
                                      "prototipo de macro sem nome");
                        cap.invalid = true;
                    }
                } else {
                    // Prototipo aninhado: vai para o corpo com os formais do
                    // proprio nivel ja trocados por #(d,i).
                    cap.body.push_back(
                        applyMapKeepSerial(line, buildFormalMap(cap.frames)));
                }
                continue;
            }
        }

        if (isOperation(line, "MCDEFN")) {
            ++cap.depth;
            cap.awaitingPrototype = true;
            trace("  [def] MCDEFN aninhado -> nivel de definicao = " +
                  std::to_string(cap.depth));
            cap.body.push_back(line);
            continue;
        }

        if (isOperation(line, "MCEND")) {
            --cap.depth;
            trace("  [def] MCEND -> nivel de definicao = " +
                  std::to_string(cap.depth));
            if (cap.depth > 0) {
                cap.frames.pop_back();
                cap.body.push_back(line);
                continue;
            }

            // Definicao externa fechada (contador voltou a zero).
            if (!cap.invalid) {
                MacroDefinition definition;
                definition.name = cap.name;
                definition.parameters = cap.outerParameters;
                definition.body = std::move(cap.body);
                definition.location = cap.location;
                MacroDiagnostic error;
                if (table_.define(definition, &error)) {
                    ++result_->macrosDefined;
                    trace("  [def] macro " + cap.name + " registrada (" +
                          std::to_string(definition.body.size()) +
                          " linhas de corpo)");
                } else {
                    result_->diagnostics.push_back(error);
                }
            }
            cap.reset();
            continue;
        }

        // Linha comum dentro do corpo: formais -> #(d,i).
        cap.body.push_back(applyMapKeepSerial(line, buildFormalMap(cap.frames)));
    }

    if (cap.depth > 0) {
        addDiagnostic(cap.location,
                      "definicao de macro sem MCEND correspondente");
    }
}

void MacroProcessor::handleLine(const MacroLine& line) {
    const std::string operation = trim(line.operation);
    if (!operation.empty()) {
        // So e chamada o que ja esta ativo na tabela (sem referencia adiante).
        if (const MacroDefinition* definition = table_.find(operation)) {
            expandCall(line, *definition);
            return;
        }
    }
    emit(line);
}

void MacroProcessor::expandCall(const MacroLine& call,
                                const MacroDefinition& definition) {
    // Ciclo: a mesma definicao ja esta na pilha de expansao.
    for (const MacroDefinition* active : callStack_) {
        if (active == &definition) {
            std::string chain;
            for (const MacroDefinition* d : callStack_) {
                chain += d->name + " -> ";
            }
            chain += definition.name;
            addDiagnostic(call.location, "chamada ciclica de macro: " + chain);
            return;
        }
    }
    if (callStack_.size() >= maxDepth_) {
        addDiagnostic(call.location,
                      "limite de profundidade de expansao excedido (" +
                          std::to_string(maxDepth_) + ") ao chamar " +
                          definition.name);
        return;
    }

    std::vector<std::string> actuals;
    const std::string operands = trim(call.operands);
    if (!operands.empty()) {
        actuals.push_back(operands);
    }
    const MacroBindingResult binding =
        bindMacroArguments(definition, actuals, call.location);
    if (!binding.success()) {
        for (const MacroDiagnostic& d : binding.diagnostics) {
            result_->diagnostics.push_back(d);
        }
        return;
    }

    const std::size_t serial = ++serial_;
    ++result_->expansionsPerformed;

    std::string stack;
    for (const MacroDefinition* d : callStack_) {
        stack += d->name + " > ";
    }
    stack += definition.name;
    trace("[exp #" + std::to_string(serial) + "] " + definition.name +
          "  profundidade=" + std::to_string(callStack_.size() + 1) +
          "  pilha: " + stack);

    // Rotulo da chamada: vai para a primeira linha emitida da expansao.
    const bool ownLabel = !trim(call.label).empty();
    if (ownLabel) {
        flushPendingLabel(call.location);
        pendingLabel_ = trim(call.label);
    }

    // Monta as linhas expandidas. Dentro de definicoes aninhadas o ".SER" e
    // preservado: ele pertence a futura expansao da macro interna.
    std::vector<MacroLine> expanded;
    expanded.reserve(definition.body.size());
    int nested = 0;
    for (const MacroLine& bodyLine : definition.body) {
        bool keepSerial = false;
        if (isOperation(bodyLine, "MCDEFN")) {
            keepSerial = true;
            ++nested;
        } else if (isOperation(bodyLine, "MCEND")) {
            keepSerial = true;
            if (nested > 0) --nested;
        } else {
            keepSerial = nested > 0;
        }

        if (keepSerial) {
            expanded.push_back(applyMapKeepSerial(bodyLine, binding.arguments));
        } else {
            expanded.push_back(
                substituteMacroParameters(bodyLine, binding.arguments, serial));
        }
    }

    callStack_.push_back(&definition);
    processLines(expanded);
    callStack_.pop_back();

    if (ownLabel && !pendingLabel_.empty()) {
        flushPendingLabel(call.location);
    }
}

void MacroProcessor::flushPendingLabel(const MacroSourceLocation& where) {
    if (pendingLabel_.empty()) return;
    MacroLine labelOnly;
    labelOnly.label = pendingLabel_;
    labelOnly.location = where;
    labelOnly.originalText = pendingLabel_;
    labelOnly.text = pendingLabel_;
    pendingLabel_.clear();
    result_->expandedLines.push_back(std::move(labelOnly));
}

void MacroProcessor::emit(MacroLine line) {
    if (!pendingLabel_.empty() && !trim(line.operation).empty()) {
        if (trim(line.label).empty()) {
            const std::string label = pendingLabel_;
            pendingLabel_.clear();
            line.label = label;
            std::size_t start = 0;
            while (start < line.originalText.size() &&
                   isSpace(line.originalText[start])) {
                ++start;
            }
            line.originalText =
                labelPadded(label) + line.originalText.substr(start);
            if (!line.text.empty()) {
                start = 0;
                while (start < line.text.size() && isSpace(line.text[start])) {
                    ++start;
                }
                line.text = labelPadded(label) + line.text.substr(start);
            }
        } else {
            flushPendingLabel(line.location);
        }
    }
    result_->expandedLines.push_back(std::move(line));
}
