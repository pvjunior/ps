// Checkpoint 4 - Interface de terminal do carregador SIC/XE.
// A leitura, validacao, relocacao e escrita pertencem ao ObjectLoader.
#include "ObjectLoader.h"
#include "LoaderTypes.h"
#include "Machine.h"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

constexpr int MEMORY_SIZE = 1 << 20; // 1 MiB: espaco de enderecamento SIC/XE.
constexpr std::uint32_t MAX_OBJECT_ADDRESS = 0xFFFFFF;

struct CliOptions {
    std::string path;
    std::optional<std::uint32_t> base;
};

void printUsage(std::ostream& out, const char* executable) {
    out << "Uso: " << executable << " programa.obj [--load-address HEX]\n"
        << "Sem --load-address, utiliza o endereco original do registro H.\n";
}

// Aceita hexadecimal puro ou com prefixo 0x; rejeita sinais, lixo e estouro.
bool parseHexAddress(std::string_view text, std::uint32_t& value) {
    if (text.size() >= 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X')) {
        text.remove_prefix(2);
    }
    if (text.empty()) return false;
    const auto end = text.data() + text.size();
    const auto result = std::from_chars(text.data(), end, value, 16);
    return result.ec == std::errc{} && result.ptr == end &&
           value <= MAX_OBJECT_ADDRESS;
}

bool parseArguments(int argc, char* argv[], CliOptions& options) {
    if (argc != 2 && argc != 4) return false;
    options.path = argv[1];
    if (options.path.empty() || options.path.front() == '-') return false;
    if (argc == 4) {
        std::uint32_t address = 0;
        if (std::string_view(argv[2]) != "--load-address" ||
            !parseHexAddress(argv[3], address)) return false;
        // A faixa real da memoria e validada pela fachada, nao pela CLI.
        options.base = address;
    }
    return true;
}

std::string hexAddress(std::uint32_t address) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(6)
        << std::setfill('0') << address;
    return out.str();
}

void printDiagnostics(const std::vector<LoaderDiagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        std::cerr << (diagnostic.where.file.empty() ? "<desconhecido>"
                                                    : diagnostic.where.file);
        if (diagnostic.where.line != 0) std::cerr << ':' << diagnostic.where.line;
        std::cerr << ": " << diagnostic.message << '\n';
        if (!diagnostic.originalText.empty()) {
            std::cerr << "  " << diagnostic.originalText << '\n';
        }
    }
}

// Ponto de adaptacao ao contrato da Parte 5. Os nomes dos campos abaixo
// sao propostos para o teste com a fachada simulada.
int runLoader(const CliOptions& options) {
    Machine machine(MEMORY_SIZE);
    const auto result = loadObject(options.path, machine, options.base);
    printDiagnostics(result.diagnostics);
    if (!result.success()) {
        std::cerr << "ERRO: carregamento nao concluido.\n";
        return 1;
    }

    std::cout << "Programa carregado com sucesso.\n"
              << "Base de carga: " << hexAddress(result.baseAddress) << '\n'
              << "Bytes carregados: " << result.bytesLoaded << '\n'
              << "Endereco de entrada: ";
    if (result.executionAddress) {
        std::cout << hexAddress(*result.executionAddress);
    } else {
        std::cout << "nao informado";
    }
    // Consulta o PC real da maquina; nao calcula nem altera o registrador.
    std::cout << "\nPC final: " << hexAddress(machine.getRegisters().PC) << '\n';
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string_view(argv[1]) == "--help" ||
                      std::string_view(argv[1]) == "-h")) {
        printUsage(std::cout, argv[0]);
        return 0;
    }
    CliOptions options;
    if (!parseArguments(argc, argv, options)) {
        std::cerr << "ERRO: argumentos invalidos.\n";
        printUsage(std::cerr, argv[0]);
        return 2;
    }
    try {
        return runLoader(options);
    } catch (const std::exception& error) {
        std::cerr << "ERRO: " << error.what() << '\n';
        return 1;
    }
}
