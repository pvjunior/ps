// Checkpoint 3 - Modulo principal e CLI (integrante 6).
// A logica de macros e montagem pertence a fachada do integrante 5.
// Contrato esperado: consultar checkpoint_3/README.md (acordo entre integrantes 5 e 6).

#include "MacroAssemblerPipeline.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>

namespace {

struct CliOptions {
    std::filesystem::path input;
    std::filesystem::path object;
    std::filesystem::path expanded = "MASMAPRG.ASM";
};

void printUsage(const char* executable) {
    std::cerr << "Uso: " << executable
              << " <entrada.asm> [-o programa.obj] [--expanded caminho]\n";
}

// Compara os caminhos normalizados, incluindo equivalencias como ./a.asm e a.asm.
bool samePath(const std::filesystem::path& first,
              const std::filesystem::path& second) {
    std::error_code error;
    const auto firstAbsolute = std::filesystem::absolute(first, error);
    if (error) return false;
    const auto secondAbsolute = std::filesystem::absolute(second, error);
    if (error) return false;
    return firstAbsolute.lexically_normal() == secondAbsolute.lexically_normal();
}

bool parseArguments(int argc, char* argv[], CliOptions& options) {
    bool hasInput = false;
    bool hasObject = false;
    bool hasExpanded = false;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "-o" || argument == "--expanded") {
            const bool isObject = argument == "-o";
            if ((isObject && hasObject) || (!isObject && hasExpanded) ||
                i + 1 >= argc) {
                return false;
            }
            const std::string value = argv[++i];
            if (value.empty() || value[0] == '-') return false;

            if (isObject) {
                options.object = value;
                hasObject = true;
            } else {
                options.expanded = value;
                hasExpanded = true;
            }
        } else if (argument.empty() || argument[0] == '-' || hasInput) {
            return false;
        } else {
            options.input = argument;
            hasInput = true;
        }
    }

    if (!hasInput) return false;
    if (!hasObject) {
        options.object = options.input;
        options.object.replace_extension(".obj");
    }

    // Nunca permita que uma saida sobrescreva o fonte ou a outra saida.
    return !samePath(options.input, options.expanded) &&
           !samePath(options.input, options.object) &&
           !samePath(options.expanded, options.object);
}

template <typename Diagnostics>
void printDiagnostics(const Diagnostics& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        const std::string file = diagnostic.where.file.empty()
                                     ? "<desconhecido>" : diagnostic.where.file;
        std::cerr << file << ':' << diagnostic.where.line << ": "
                  << diagnostic.message << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    CliOptions options;
    if (!parseArguments(argc, argv, options)) {
        printUsage(argv[0]);
        return 2;
    }

    // A fachada do integrante 5 implementa esta sequencia, sem abrir TUI:
    // 1. Ler o fonte; 2. processar macros; 3. gravar MASMAPRG.ASM;
    // 4. executar parser, passagem 1, passagem 2 e geracao do objeto SIC/XE.
    try {
        const MacroAssemblerResult result = runMacroAssembler(
            options.input.string(), options.expanded.string(),
            options.object.string());

        printDiagnostics(result.macroDiagnostics);
        printDiagnostics(result.assemblerDiagnostics);
        if (!result.success()) return 1;

        std::cout << "[OK] Macro-montagem concluida.\n"
                  << "--------------------------------\n"
                  << "Macros definidas: " << result.macrosDefined << '\n'
                  << "Expansoes realizadas: " << result.expansionsPerformed << '\n'
                  << "Fonte expandido: " << options.expanded.string() << '\n'
                  << "Arquivo objeto: " << options.object.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        // A fachada deve retornar diagnosticos, mas uma excecao nao pode
        // derrubar a CLI sem explicar a falha ao usuario.
        std::cerr << options.input.string() << ":0: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << options.input.string()
                  << ":0: falha inesperada no macro-montador\n";
        return 1;
    }
}
