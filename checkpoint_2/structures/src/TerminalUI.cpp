#include "TerminalUI.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>

// Códigos de cores ANSI para estilizar a interface no terminal
namespace Color {
    const char* RESET   = "\033[0m";
    const char* BOLD    = "\033[1m";
    const char* CYAN    = "\033[1;36m";
    const char* GREEN   = "\033[1;32m";
    const char* YELLOW  = "\033[1;33m";
    const char* RED     = "\033[1;31m";
    const char* BLUE    = "\033[1;34m";
    const char* MAGENTA = "\033[1;35m";
    const char* GRAY    = "\033[90m";
    const char* REVERSE = "\033[7m";
}

TerminalUI::TerminalUI()
    : machine(1024), memAddress(0), running(true) {
    message = "Simulador iniciado. Pressione [ENTER] ou [s] para avancar um passo.";
    loadDefaultProgram();
}

// Carrega o programa de teste padrao:
// 1. LDA #5     (01 00 05) -> Carrega valor imediato 5 em A
// 2. ADD #3     (19 00 03) -> Soma valor imediato 3 em A (A vira 8)
// 3. STA 0x100  (0F 01 00) -> Armazena A no endereco 0x100
// 4. J *        (3F 2F FD) -> Desvia para si mesmo (Halt)
void TerminalUI::loadDefaultProgram() {
    machine.reset();
    machine.loader().clear();
    machine.loader().insertHex("01 00 05"); // LDA #5
    machine.loader().insertHex("19 00 03"); // ADD #3
    machine.loader().insertHex("0F 01 00"); // STA 0x100
    machine.loader().insertHex("3F 2F FD"); // J * (Parada/Halt)
    message = "Programa de teste padrao carregado na memoria (0x0000).";
}

// Converte byte para string hexadecimal com 2 digitos
std::string TerminalUI::hexByte(uint8_t b) {
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return ss.str();
}

// Converte palavra de 24 bits para string hexadecimal com 6 digitos
std::string TerminalUI::hexWord(uint32_t w) {
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(6) << std::setfill('0') << (w & 0xFFFFFF);
    return ss.str();
}

// Converte endereco para hexadecimal com 4 digitos
std::string TerminalUI::hexAddr(int addr) {
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << addr;
    return ss.str();
}

// Desenha o cabecalho principal
void TerminalUI::renderHeader() const {
    std::cout << Color::CYAN << "================================================================================" << Color::RESET << "\n";
    std::cout << Color::BOLD << Color::CYAN << "               SIMULADOR DE MAQUINA SIC/XE - INTERFACE TERMINAL                 " << Color::RESET << "\n";
    std::cout << Color::CYAN << "================================================================================" << Color::RESET << "\n";
}

// Exibe o status da maquina (Passos, Estado, Cursor do Loader, Memoria) e mensagens recentes
void TerminalUI::renderStatus() const {
    std::cout << Color::BOLD << " ESTADO: " << Color::RESET;

    if (machine.hasError()) {
        std::cout << Color::RED << "[ERRO: " << machine.getError() << "]" << Color::RESET;
    } else if (machine.isHalted()) {
        std::cout << Color::YELLOW << "[PARADA (HALT / J *)]" << Color::RESET;
    } else {
        std::cout << Color::GREEN << "[EXECUTANDO / PRONTO]" << Color::RESET;
    }

    std::cout << " | " << Color::BOLD << "Passos: " << Color::RESET << machine.getStepCount();
    std::cout << " | " << Color::BOLD << "Cursor Loader: " << Color::RESET << hexAddr(machine.loader().cursor());
    std::cout << " | " << Color::BOLD << "Tam. Memoria: " << Color::RESET << machine.getMemorySize() << " B\n";

    // Mensagem de feedback da ultima acao
    std::cout << Color::BLUE << " >> " << Color::RESET << message << "\n";
    std::cout << Color::GRAY << "--------------------------------------------------------------------------------" << Color::RESET << "\n";
}

// Mostra detalhes da ultima instrucao decodificada pela CPU
void TerminalUI::renderLastInstruction() const {
    std::cout << Color::BOLD << Color::MAGENTA << " [ ULTIMA INSTRUCAO EXECUTADA ]" << Color::RESET << "\n";

    if (machine.getStepCount() == 0) {
        std::cout << Color::GRAY << "  Nenhuma instrucao executada ainda. Pressione [ENTER] para dar o 1o passo." << Color::RESET << "\n";
    } else {
        const Instruction& inst = machine.getLastInstruction();

        std::cout << "  Instrucao: " << Color::BOLD << Color::GREEN << inst.name << Color::RESET
                  << " (Opcode: 0x" << std::uppercase << std::hex << inst.op << std::dec << ")"
                  << " | Formato: " << inst.format
                  << " (" << inst.size << " bytes)"
                  << " | Endereco: " << hexAddr(inst.address) << "\n";

        if (inst.format == 3 || inst.format == 4) {
            std::cout << "  Flags: n=" << inst.n << " i=" << inst.i
                      << " x=" << inst.x << " b=" << inst.b
                      << " p=" << inst.p << " e=" << inst.e
                      << " | Deslocamento: " << hexAddr(inst.disp)
                      << " | End. Alvo (TA): " << hexAddr(inst.ta) << "\n";
        } else if (inst.format == 2) {
            std::cout << "  Registradores operandos: r1=" << Machine::registerName(inst.r1)
                      << " (" << inst.r1 << "), r2=" << Machine::registerName(inst.r2)
                      << " (" << inst.r2 << ")\n";
        }

        // Mostra se a instrucao leu ou escreveu na memoria
        if (inst.memSize > 0) {
            if (inst.memWrite) {
                std::cout << Color::YELLOW << "  Memoria afetada: ESCRITA em " << hexAddr(inst.memAddr)
                          << " (Tamanho: " << inst.memSize << " bytes)" << Color::RESET << "\n";
            } else {
                std::cout << Color::BLUE << "  Memoria afetada: LEITURA em " << hexAddr(inst.memAddr)
                          << " (Tamanho: " << inst.memSize << " bytes)" << Color::RESET << "\n";
            }
        }
    }
    std::cout << Color::GRAY << "--------------------------------------------------------------------------------" << Color::RESET << "\n";
}

// Exibe os registradores da maquina, destacando quais foram alterados
void TerminalUI::renderRegisters() const {
    RegisterSnapshot r = machine.getRegisters();
    const auto& changed = machine.getChangedRegisters();

    // Funcao auxiliar para verificar se um registrador mudou no ultimo passo
    auto isChanged = [&](int regNum) {
        return std::find(changed.begin(), changed.end(), regNum) != changed.end();
    };

    std::cout << Color::BOLD << Color::MAGENTA << " [ REGISTRADORES ]                                [ CONDICAO & EXECUCAO ]" << Color::RESET << "\n";

    // Formata cada linha comparando com a coluna da direita
    // Registrador A (0)
    std::cout << "  A : " << (isChanged(0) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.A) << " (" << std::setw(8) << std::dec << r.A << ")"
              << (isChanged(0) ? " [*]" : "    ") << Color::RESET
              << "    |  CC : " << Color::BOLD << Color::CYAN << machine.getCCSymbol() << " " << Color::RESET;

    if (r.cc == 0) std::cout << "(MENOR <)";
    else if (r.cc == 1) std::cout << "(IGUAL =)";
    else std::cout << "(MAIOR >)";
    std::cout << "\n";

    // Registrador X (1)
    std::cout << "  X : " << (isChanged(1) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.X) << " (" << std::setw(8) << std::dec << r.X << ")"
              << (isChanged(1) ? " [*]" : "    ") << Color::RESET
              << "    |  PC : " << Color::BOLD << Color::YELLOW << hexAddr(r.PC)
              << " (" << std::dec << r.PC << ")" << Color::RESET << "\n";

    // Registrador L (2)
    std::cout << "  L : " << (isChanged(2) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.L) << " (" << std::setw(8) << std::dec << r.L << ")"
              << (isChanged(2) ? " [*]" : "    ") << Color::RESET
              << "    |  SW : 0x" << hexWord(r.SW) << "\n";

    // Registrador B (3)
    std::cout << "  B : " << (isChanged(3) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.B) << " (" << std::setw(8) << std::dec << r.B << ")"
              << (isChanged(3) ? " [*]" : "    ") << Color::RESET
              << "    |\n";

    // Registrador S (4)
    std::cout << "  S : " << (isChanged(4) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.S) << " (" << std::setw(8) << std::dec << r.S << ")"
              << (isChanged(4) ? " [*]" : "    ") << Color::RESET
              << "    |  " << Color::YELLOW << "[*]" << Color::RESET << " = Registrador alterado\n";

    // Registrador T (5)
    std::cout << "  T : " << (isChanged(5) ? Color::YELLOW : Color::GREEN)
              << "0x" << hexWord(r.T) << " (" << std::setw(8) << std::dec << r.T << ")"
              << (isChanged(5) ? " [*]" : "    ") << Color::RESET
              << "    |      no ultimo passo executado\n";

    // Registrador F (6 - 48 bits)
    std::stringstream fHex;
    fHex << std::uppercase << std::hex << std::setw(12) << std::setfill('0') << (r.F & 0xFFFFFFFFFFFFULL);
    std::cout << "  F : 0x" << fHex.str() << " (48 bits)"
              << (isChanged(6) ? " [*]" : "") << "\n";

    std::cout << Color::GRAY << "--------------------------------------------------------------------------------" << Color::RESET << "\n";
}

// Exibe uma janela de 64 bytes da memoria usando o metodo machine.dump()
void TerminalUI::renderMemory() const {
    int maxMem = machine.getMemorySize();
    int start = std::max(0, std::min(memAddress, maxMem - 64));
    int length = std::min(64, maxMem - start);

    // Obtem a fatia de memoria atraves do metodo dump() da Machine
    std::vector<uint8_t> bytes = machine.dump(start, length);
    RegisterSnapshot r = machine.getRegisters();
    const Instruction& lastInst = machine.getLastInstruction();

    std::cout << Color::BOLD << Color::MAGENTA << " [ MEMORIA ]" << Color::RESET
              << " (Exibindo " << hexAddr(start) << " ate " << hexAddr(start + length - 1)
              << " de " << maxMem << " bytes)  "
              << Color::GRAY << "[m] ir p/ end.  [+] prox  [-] ant" << Color::RESET << "\n";

    std::cout << Color::GRAY << " Endereco   00 01 02 03  04 05 06 07  08 09 0A 0B  0C 0D 0E 0F   Texto ASCII" << Color::RESET << "\n";

    for (int row = 0; row < (int)bytes.size(); row += 16) {
        int currentAddr = start + row;
        std::cout << " " << Color::CYAN << hexAddr(currentAddr) << Color::RESET << "   ";

        // Exibe os 16 bytes em hexadecimal
        for (int col = 0; col < 16; col++) {
            int idx = row + col;
            int byteAddr = currentAddr + col;

            if (idx < (int)bytes.size()) {
                uint8_t val = bytes[idx];

                // Destaques visuais:
                // 1. Se coincide com o PC -> invertido / amarelo
                // 2. Se foi escrito na ultima instrucao -> verde
                bool isPC = (byteAddr == (int)r.PC);
                bool isWritten = (lastInst.memWrite && byteAddr >= lastInst.memAddr && byteAddr < lastInst.memAddr + lastInst.memSize);

                if (isPC) {
                    std::cout << Color::YELLOW << Color::REVERSE << hexByte(val) << Color::RESET;
                } else if (isWritten) {
                    std::cout << Color::GREEN << Color::BOLD << hexByte(val) << Color::RESET;
                } else if (val == 0) {
                    std::cout << Color::GRAY << hexByte(val) << Color::RESET;
                } else {
                    std::cout << hexByte(val);
                }
            } else {
                std::cout << "  ";
            }

            // Espacamento agradavel a cada 4 bytes
            if (col % 4 == 3 && col != 15) std::cout << "  ";
            else std::cout << " ";
        }

        std::cout << "  " << Color::GRAY << "|" << Color::RESET;

        // Exibe representacao ASCII
        for (int col = 0; col < 16; col++) {
            int idx = row + col;
            if (idx < (int)bytes.size()) {
                uint8_t val = bytes[idx];
                if (std::isprint(val)) {
                    std::cout << (char)val;
                } else {
                    std::cout << Color::GRAY << "." << Color::RESET;
                }
            } else {
                std::cout << " ";
            }
        }
        std::cout << Color::GRAY << "|" << Color::RESET << "\n";
    }
    std::cout << Color::GRAY << "--------------------------------------------------------------------------------" << Color::RESET << "\n";
}

// Menu de comandos disponiveis
void TerminalUI::renderHelp() const {
    std::cout << Color::BOLD << " COMANDOS DISPONIVEIS:" << Color::RESET << "\n";
    std::cout << "  " << Color::YELLOW << "[ENTER/s]" << Color::RESET << " Passo (Step)        "
              << Color::YELLOW << "[r]" << Color::RESET << " Rodar tudo (Run)    "
              << Color::YELLOW << "[p]" << Color::RESET << " Resetar maquina\n";
    std::cout << "  " << Color::YELLOW << "[m]" << Color::RESET << " Ir para endereco mem   "
              << Color::YELLOW << "[w]" << Color::RESET << " Inspecionar Word    "
              << Color::YELLOW << "[c]" << Color::RESET << " Inserir Hex (Loader)\n";
    std::cout << "  " << Color::YELLOW << "[+]" << Color::RESET << " Proxima pag. memoria  "
              << Color::YELLOW << "[-]" << Color::RESET << " Pagina anterior     "
              << Color::YELLOW << "[e]" << Color::RESET << " Carregar Exemplo\n";
    std::cout << "  " << Color::YELLOW << "[q]" << Color::RESET << " Sair\n";
    std::cout << Color::CYAN << "================================================================================" << Color::RESET << "\n";
}

// Limpa a tela e renderiza todas as secoes
void TerminalUI::render() const {
    // Sequencia ANSI para limpar a tela e posicionar o cursor no inicio (0,0)
    std::cout << "\033[2J\033[H";

    renderHeader();
    renderStatus();
    renderLastInstruction();
    renderRegisters();
    renderMemory();
    renderHelp();
}

// Executa um unico passo da maquina (Machine::step)
void TerminalUI::doStep() {
    if (machine.isHalted()) {
        message = "CPU esta parada (Halt). Use [p] para resetar a maquina.";
        return;
    }

    bool ok = machine.step();

    if (ok) {
        message = "Instrucao '" + machine.getLastInstruction().name + "' executada com sucesso.";
    } else if (machine.hasError()) {
        message = "Erro durante execucao: " + machine.getError();
    } else if (machine.isHalted()) {
        message = "CPU parou normalmente (Halt detectado).";
    }
}

// Executa instrucoes continuamente ate atingir Halt ou erro
void TerminalUI::doRun() {
    if (machine.isHalted()) {
        message = "CPU ja esta parada. Use [p] para resetar antes de rodar.";
        return;
    }

    int count = 0;
    const int maxSteps = 100000; // Limite de seguranca para evitar loop infinito

    while (machine.step()) {
        count++;
        if (count >= maxSteps) {
            message = "Execucao interrompida: atingido limite de 100000 passos.";
            return;
        }
    }

    if (machine.hasError()) {
        message = "Execucao interrompida com erro: " + machine.getError() + " (" + std::to_string(count) + " passos adicionais)";
    } else {
        message = "Execucao finalizada (Halt). Total de " + std::to_string(count) + " passos executados.";
    }
}

// Reseta a maquina (Machine::reset)
void TerminalUI::doReset() {
    machine.reset();
    message = "Maquina resetada com sucesso. Registradores e PC voltaram ao estado inicial.";
}

// Altera o endereco de memoria exibido na tela
void TerminalUI::doChangeMemAddress(const std::string& arg) {
    std::string input = arg;
    if (input.empty()) {
        std::cout << Color::YELLOW << " Digite o endereco (ex: 0x100 ou 256): " << Color::RESET;
        if (!std::getline(std::cin, input) || input.empty()) return;
    }
    try {
        int addr = std::stoi(input, nullptr, 0);
        int maxMem = machine.getMemorySize();
        memAddress = std::max(0, std::min(addr, maxMem - 64));
        message = "Exibindo memoria a partir de " + hexAddr(memAddress);
    } catch (...) {
        message = "Endereco invalido digitado.";
    }
}

// Inspeciona um valor de Word (3 bytes) e Byte diretamente (Machine::getWord e Machine::getByte)
void TerminalUI::doInspectWord(const std::string& arg) {
    std::string input = arg;
    if (input.empty()) {
        std::cout << Color::YELLOW << " Digite o endereco para inspecionar (ex: 0x100): " << Color::RESET;
        if (!std::getline(std::cin, input) || input.empty()) return;
    }
    try {
        int addr = std::stoi(input, nullptr, 0);
        uint32_t word = machine.getWord(addr);
        uint8_t byte = machine.getByte(addr);

        std::stringstream ss;
        ss << "Inspecao em " << hexAddr(addr)
           << " -> Word (3B): 0x" << hexWord(word) << " (" << word << ")"
           << " | Byte: 0x" << hexByte(byte);
        message = ss.str();
    } catch (...) {
        message = "Endereco invalido.";
    }
}

// Insere bytes hexadecimais na memoria usando o Loader (Machine::loader().insertHex)
void TerminalUI::doInsertHex(const std::string& arg) {
    std::string input = arg;
    if (input.empty()) {
        std::cout << Color::YELLOW << " Digite os bytes em hexadecimal (ex: 01 00 05): " << Color::RESET;
        if (!std::getline(std::cin, input) || input.empty()) return;
    }
    auto res = machine.loader().insertHex(input);
    if (res.ok) {
        message = "Loader inseriu " + std::to_string(res.count) + " bytes no endereco " + hexAddr(res.address);
    } else {
        message = "Erro no loader: " + res.error;
    }
}

// Permite ao usuario escolher entre programas de teste pre-definidos
void TerminalUI::doLoadExample(const std::string& arg) {
    std::string opt = arg;
    if (opt.empty()) {
        std::cout << Color::BOLD << "\n Escolha um programa de exemplo:\n" << Color::RESET;
        std::cout << "  1. Basico: LDA #5, ADD #3, STA 0x100, J *\n";
        std::cout << "  2. Formato 2: CLEAR A, CLEAR X, LDA #10, RMO A, X, ADDR X, A, J *\n";
        std::cout << "  3. Comparacao: LDA #20, LDB #20, COMPR A, B, J *\n";
        std::cout << "  4. Subtracao: LDA #50, SUB #20, STA 0x150, J *\n";
        std::cout << Color::YELLOW << " Opcao (1-4): " << Color::RESET;
        if (!std::getline(std::cin, opt) || opt.empty()) return;
    }

    machine.reset();
    machine.loader().clear();

    if (opt == "1") {
        machine.loader().insertHex("01 00 05"); // LDA #5
        machine.loader().insertHex("19 00 03"); // ADD #3
        machine.loader().insertHex("0F 01 00"); // STA 0x100
        machine.loader().insertHex("3F 2F FD"); // J *
        message = "Exemplo 1 carregado: Soma (A=8) e escrita em 0x100.";
    } else if (opt == "2") {
        machine.loader().insertHex("B4 00");    // CLEAR A
        machine.loader().insertHex("B4 10");    // CLEAR X
        machine.loader().insertHex("01 00 0A"); // LDA #10
        machine.loader().insertHex("AC 01");    // RMO A, X
        machine.loader().insertHex("90 10");    // ADDR X, A
        machine.loader().insertHex("3F 2F FD"); // J *
        message = "Exemplo 2 carregado: Formato 2 (RMO e ADDR, A=20, X=10).";
    } else if (opt == "3") {
        machine.loader().insertHex("01 00 14"); // LDA #20
        machine.loader().insertHex("69 00 14"); // LDB #20
        machine.loader().insertHex("A0 03");    // COMPR A, B
        machine.loader().insertHex("3F 2F FD"); // J *
        message = "Exemplo 3 carregado: Comparacao COMPR (A == B, CC='=').";
    } else if (opt == "4") {
        machine.loader().insertHex("01 00 32"); // LDA #50
        machine.loader().insertHex("1D 00 14"); // SUB #20
        machine.loader().insertHex("0F 01 50"); // STA 0x150
        machine.loader().insertHex("3F 2F FD"); // J *
        message = "Exemplo 4 carregado: Subtracao (A=30) e escrita em 0x150.";
    } else {
        message = "Opcao invalida de exemplo.";
    }
}

// Loop principal de interacao do usuario com a interface
void TerminalUI::run() {
    while (running) {
        render();

        std::cout << Color::BOLD << Color::GREEN << " Digite um comando [ENTER = Passo]: " << Color::RESET;

        std::string line;
        if (!std::getline(std::cin, line)) {
            // Fim do arquivo (EOF / redirecionamento)
            break;
        }

        // Remocao de espacos iniciais e finais
        line.erase(0, line.find_first_not_of(" \t\r\n"));

        // Pressionar ENTER com linha vazia equivale ao passo (Step)
        if (line.empty()) {
            doStep();
            continue;
        }

        // Separa comando e argumento se houver (ex: "m 0x100" ou "w 0x100")
        std::stringstream ss(line);
        std::string cmd, arg;
        ss >> cmd;
        std::getline(ss, arg);
        if (!arg.empty()) {
            arg.erase(0, arg.find_first_not_of(" \t\r\n"));
        }

        // Transforma o comando em minusculo
        std::string cmdLower = cmd;
        for (char& c : cmdLower) c = (char)std::tolower(c);

        if (cmdLower == "s") {
            doStep();
        } else if (cmdLower == "r") {
            doRun();
        } else if (cmdLower == "p") {
            doReset();
        } else if (cmdLower == "m") {
            doChangeMemAddress(arg);
        } else if (cmdLower == "w") {
            doInspectWord(arg);
        } else if (cmdLower == "c") {
            doInsertHex(arg);
        } else if (cmdLower == "e") {
            doLoadExample(arg);
        } else if (cmdLower == "+" || cmdLower == "n") {
            memAddress = std::min(memAddress + 64, machine.getMemorySize() - 64);
            message = "Exibindo memoria a partir de " + hexAddr(memAddress);
        } else if (cmdLower == "-" || cmdLower == "b") {
            memAddress = std::max(0, memAddress - 64);
            message = "Exibindo memoria a partir de " + hexAddr(memAddress);
        } else if (cmdLower == "q") {
            running = false;
            std::cout << "\nEncerrando o simulador. Ate logo!\n";
        } else {
            message = "Comando desconhecido: '" + line + "'. Digite [ENTER/s] para passo ou [q] para sair.";
        }
    }
}
