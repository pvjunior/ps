// Checkpoint 2 - interface de linha de comando do montador SIC/XE.
// Integrante 6: orquestracao, diagnosticos, arquivo de saida e carregamento.
//
// APIs REAIS verificadas no commit e264d9a (main, 21/10/2026):
//   parseFile(caminho)          -> ParseResult
//   runPassOne(parsed.lines)    -> PassOneResult
//   runPassTwo(passOneResult)   -> PassTwoResult
//
// APIs previstas no README_05, ainda pendentes no bundle analisado:
//   buildObjectProgram(first, second) -> ObjectProgram (toText())
//   SimulatorBridge(object).loadInto(machine) -> LoadResult (ok, error)
// Ao receber a parte 5, conferir somente essas duas chamadas com seus headers.

#include "SourceParser.h"
#include "PassOne.h"
#include "PassTwo.h"
#include "ObjectProgram.h"
#include "SimulatorBridge.h"
#include "Machine.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct CliOptions {
    std::filesystem::path input;
    std::filesystem::path output;
    bool load = false;
};

void printUsage(const char* program) {
    std::cerr << "Uso: " << program
              << " <entrada.asm> [-o saida.obj] [--load]\n";
}

bool parseArguments(int argc, char* argv[], CliOptions& options) {
    bool hasInput = false;
    bool hasOutput = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--load") {
            if (options.load) return false;
            options.load = true;
        } else if (argument == "-o") {
            if (hasOutput || i + 1 >= argc) return false;
            const std::string next = argv[++i];
            if (next.empty() || next[0] == '-') return false;
            options.output = next;
            hasOutput = true;
        } else if (argument.empty() || argument[0] == '-' || hasInput) {
            return false;
        } else {
            options.input = argument;
            hasInput = true;
        }
    }
    if (!hasInput) return false;
    if (!hasOutput) {
        options.output = options.input;
        options.output.replace_extension(".obj");
    }
    // Evita sobrescrever o proprio fonte se -o apontar para ele.
    return options.input.lexically_normal() != options.output.lexically_normal();
}

void printDiagnostics(const std::vector<Diagnostic>& diagnostics) {
    for (const Diagnostic& error : diagnostics) {
        const std::string file = error.where.file.empty()
                                     ? "<desconhecido>" : error.where.file;
        std::cerr << file << ':' << error.where.line << ": "
                  << error.message << '\n';
    }
}

// Escreve primeiro em arquivo temporario. Caso a montagem falhe, o
// .obj anterior permanece intacto; problemas de escrita sao reportados.
bool saveObject(const std::filesystem::path& output,
                const std::string& text, std::string& error) {
    std::filesystem::path temporary = output;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "nao foi possivel abrir o arquivo temporario para escrita";
            return false;
        }
        stream << text;
        stream.close();
        if (!stream) {
            error = "falha ao escrever o programa objeto";
            std::error_code ec;
            std::filesystem::remove(temporary, ec);
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(temporary, output, ec);
    if (ec) {
        error = "nao foi possivel atualizar o arquivo objeto: " + ec.message();
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return true;
}

void printSummary(const PassOneResult& first, const CliOptions& options) {
    std::cout << "Montagem concluida.\n"
              << "Arquivo objeto: " << options.output.string() << '\n'
              << "Endereco inicial: 0x" << std::hex << std::uppercase
              << first.startAddress << std::dec << '\n'
              << "Tamanho: " << first.programLength << " byte(s)\n"
              << "Simbolos: " << first.symbols.size() << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    CliOptions options;
    if (!parseArguments(argc, argv, options)) {
        printUsage(argv[0]);
        return 2;
    }

    // 1) Fonte -> linhas estruturadas e diagnosticos.
    const ParseResult parsed = parseFile(options.input.string());
    if (!parsed.diagnostics.empty()) {
        printDiagnostics(parsed.diagnostics);
        return 1;
    }

    // 2) Passagem 1 -> contador de localizacao e tabela de simbolos.
    const PassOneResult first = runPassOne(parsed.lines);
    if (!first.diagnostics.empty()) {
        printDiagnostics(first.diagnostics);
        return 1;
    }

    // 3) Passagem 2 -> bytes e informacoes de relocacao.
    const PassTwoResult second = runPassTwo(first);
    if (!second.diagnostics.empty()) {
        printDiagnostics(second.diagnostics);
        return 1;
    }

    // 4) Registros objeto SIC/XE, sem sobrescrever arquivos em erro.
    const ObjectProgram object = buildObjectProgram(first, second);
    std::string ioError;
    if (!saveObject(options.output, object.toText(), ioError)) {
        std::cerr << options.output.string() << ":0: " << ioError << '\n';
        return 1;
    }
    printSummary(first, options);

    // 5) Opcional: carrega na memoria sem iniciar a interface TUI.
    if (options.load) {
        const std::uint64_t last = static_cast<std::uint64_t>(first.startAddress)
                                 + first.programLength;
        if (last > 0x1000000ULL || last > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            std::cerr << "Erro: programa excede o espaco de memoria SIC/XE.\n";
            return 1;
        }
        Machine machine(static_cast<int>(std::max<std::uint64_t>(1024ULL, last)));
        SimulatorBridge bridge(object);
        const auto result = bridge.loadInto(machine);
        if (!result.ok) {
            std::cerr << "Erro ao carregar no simulador: " << result.error << '\n';
            return 1;
        }
        std::cout << "Programa carregado na memoria do simulador (sem iniciar TUI).\n";
    }
    return 0;
}
