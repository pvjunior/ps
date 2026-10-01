#include <iostream>
#include <iomanip>
#include "Machine.h"

int main() {
    Machine m;

    // --- LOADER ---
    // Inserção sequencial usando o cursor interno (começa em 0x00)
    auto r1 = m.loader().insertHex("01 00 05"); // LDA #5
    auto r2 = m.loader().insertHex("19 00 03"); // ADD #3
    auto r3 = m.loader().insertHex("0F 01 00"); // STA 0x100

    if (!r1.ok || !r2.ok || !r3.ok) {
        std::cerr << "Falha no carregamento dos bytes.\n";
        return 1;
    }

    // Posição atual do cursor após os inserts
    std::cout << "Cursor do loader: " << m.loader().cursor() << "\n";

    // Utilitários do loader (se precisar alterar endereço de escrita)
    // m.loader().setCursor(0x0200);
    // m.loader().cursorToPC();
    // m.loader().write(0x100, {0x01, 0x02});


    // --- INSPEÇÃO DE MEMÓRIA (DUMP) ---
    // Retorna std::vector<uint8_t> de um trecho da memória
    auto memDump = m.dump(0x00, 9);
    std::cout << "Dump 0x00..0x08: ";
    for (uint8_t byte : memDump) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << "\n\n";


    // --- EXECUÇÃO (STEP LOOP) ---
    // step() roda 1 instrução por vez. Retorna false se halt, erro ou fim de memória
    while (m.step()) {
        // Observador da instrução executada no último step
        auto lastInst = m.getLastInstruction();
        
        // Snapshots dos registradores
        RegisterSnapshot regs = m.getRegisters();

        std::cout << "Inst: " << lastInst.name 
                  << " | PC: 0x" << std::hex << regs.PC 
                  << " | A: " << std::dec << regs.A 
                  << " | CC: " << m.getCCSymbol() << "\n";

        // Checa se a instrução escreveu na memória para destacar na GUI
        if (lastInst.memWrite) {
            std::cout << "  -> Escreveu em 0x" << std::hex << lastInst.memAddr 
                      << " (size: " << std::dec << lastInst.memSize << ")\n";
        }
    }


    // --- CHECAGEM DE ESTADO DE ERRO ---
    if (m.hasError()) {
        std::cout << "Erro na execução: " << m.getError() << "\n";
        std::cout << "PC travado em: 0x" << std::hex << m.getRegisters().PC << std::dec << "\n";
    } else if (m.isHalted()) {
        std::cout << "CPU parada (Halt).\n";
    }


    // --- LEITURA DIRETA ---
    // getByte(addr) e getWord(addr) para ler valores diretos
    std::cout << "Memoria em 0x100 (Word): " << m.getWord(0x100) << "\n";


    // --- RESET ---
    // Reseta registradores e flags de erro, mantendo a memória
    m.reset();
    std::cout << "Passos apos reset: " << m.getStepCount() << "\n";

    return 0;
}