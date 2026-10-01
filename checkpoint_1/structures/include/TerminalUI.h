#pragma once
#include <string>
#include <vector>
#include "Machine.h"

// Classe responsável pela interface visual no terminal (TUI).
// Utiliza os métodos públicos da classe Machine para exibir o estado completo
// da máquina (registradores, memória, instruções e status) de forma amigável.
class TerminalUI {
private:
    Machine machine;          // Instância do simulador da máquina SIC/XE
    int memAddress;           // Endereço base de exibição da memória
    std::string message;      // Mensagem informativa exibida na barra de status
    bool running;             // Indica se o loop da interface continua ativo

    // Métodos para desenhar as seções da tela
    void renderHeader() const;
    void renderStatus() const;
    void renderLastInstruction() const;
    void renderRegisters() const;
    void renderMemory() const;
    void renderHelp() const;

    // Métodos para processar as ações do usuário
    void doStep();
    void doRun();
    void doReset();
    void doChangeMemAddress(const std::string& arg = "");
    void doInspectWord(const std::string& arg = "");
    void doInsertHex(const std::string& arg = "");
    void doLoadExample(const std::string& arg = "");

    // Funções utilitárias de formatação
    static std::string hexByte(uint8_t b);
    static std::string hexWord(uint32_t w);
    static std::string hexAddr(int addr);

public:
    TerminalUI();

    // Carrega um programa de exemplo inicial na memória
    void loadDefaultProgram();

    // Renderiza a interface completa no terminal
    void render() const;

    // Inicia o loop interativo no terminal
    void run();
};
