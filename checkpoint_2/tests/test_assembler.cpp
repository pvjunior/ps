// ====================================================================
// Test Suite — Montador SIC/XE (Checkpoint 2)
// Integrante 7 — Testes, Exemplos e Apresentacao
// 
// Utiliza exclusivamente C++17 e assert para manter independencia
// de bibliotecas externas.
// Cobertura completa:
//   1. Parser (comentarios, indentacao, indexado, aspas invalidas)
//   2. Passagem 1 (START, enderecos, BYTE, reservas, rotulo duplicado, opcode desconhecido)
//   3. Passagem 2 (formatos 1/2/3/4, imediato, indireto, indexado, PC-rel, base-rel, deslocamento impossivel)
//   4. Programa Objeto (H/T/M/E, quebra por 30 bytes, quebra por lacuna)
//   5. Integracao (valid.asm monta, gera objeto e carrega na memoria do simulador)
// ====================================================================

#include "AssemblerTypes.h"
#include "OpcodeTable.h"
#include "SourceParser.h"
#include "PassOne.h"
#include "PassTwo.h"
#include "Encoder.h"
#include "ObjectProgram.h"
#include "SimulatorBridge.h"
#include "Machine.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_passedTests = 0;

void recordTest(const std::string& name) {
    std::cout << "  [PASS] " << name << "\n";
    g_passedTests++;
}

// ====================================================================
// 1. TESTES DO PARSER
// ====================================================================
void testParser() {
    std::cout << "\n=== Executando Testes do Parser ===\n";

    // 1.1 Comentarios e linhas em branco
    {
        const std::string text = ". Linha inteira de comentario\n"
                                 "   . Comentario indentado\n"
                                 "   \n"
                                 "FIRST LDA #3 . Comentario ao lado\n";
        ParseResult res = parseText(text, "test.asm");
        assert(res.success());
        assert(res.lines.size() == 4);
        assert(res.lines[0].operation.empty());
        assert(res.lines[0].comment.find("Linha inteira") != std::string::npos);
        assert(res.lines[2].operation.empty());
        assert(res.lines[3].label.has_value() && *res.lines[3].label == "FIRST");
        assert(res.lines[3].operation == "LDA");
        assert(res.lines[3].operand == "#3");
        assert(res.lines[3].comment.find("Comentario ao lado") != std::string::npos);
        recordTest("Parser: comentarios e linhas em branco");
    }

    // 1.2 Indentacao e ausencia de rotulo
    {
        const std::string text = "   LDA #10\n"
                                 "\tSTA BUFFER\n";
        ParseResult res = parseText(text, "indent.asm");
        assert(res.success());
        assert(res.lines.size() == 2);
        assert(!res.lines[0].label.has_value());
        assert(res.lines[0].operation == "LDA");
        assert(res.lines[0].operand == "#10");
        assert(!res.lines[1].label.has_value());
        assert(res.lines[1].operation == "STA");
        assert(res.lines[1].operand == "BUFFER");
        recordTest("Parser: indentacao sem rotulo");
    }

    // 1.3 Operando indexado intacto
    {
        const std::string text = "   STCH BUFFER,X\n"
                                 "   LDA  TABLE,X\n";
        ParseResult res = parseText(text, "indexed.asm");
        assert(res.success());
        assert(res.lines[0].operand == "BUFFER,X");
        assert(res.lines[1].operand == "TABLE,X");
        recordTest("Parser: preservacao de operando indexado");
    }

    // 1.4 Aspas simples nao terminadas
    {
        const std::string text = "   BYTE C'ASPAS_SEM_FIM\n"
                                 "   LDA  #1\n";
        ParseResult res = parseText(text, "quote.asm");
        assert(!res.success());
        assert(!res.diagnostics.empty());
        bool foundQuoteDiag = false;
        for (const auto& d : res.diagnostics) {
            if (d.message.find("quote") != std::string::npos) {
                foundQuoteDiag = true;
                break;
            }
        }
        assert(foundQuoteDiag);
        assert(res.lines.size() == 2); // parse continua analisando
        recordTest("Parser: diagnostico de aspas nao finalizadas");
    }
}

// ====================================================================
// 2. TESTES DA PASSAGEM 1
// ====================================================================
void testPassOne() {
    std::cout << "\n=== Executando Testes da Passagem 1 ===\n";

    // 2.1 START e enderecos
    {
        const std::string text = "COPY  START 1000\n"
                                 "FIRST LDA   #3\n"
                                 "      STA   BUFFER\n"
                                 "BUFFER RESW 1\n"
                                 "      END   FIRST\n";
        ParseResult pRes = parseText(text);
        assert(pRes.success());
        PassOneResult p1 = runPassOne(pRes.lines);
        assert(p1.success());
        assert(p1.startAddress == 0x1000);
        assert(p1.symbols.at("FIRST") == 0x1000);
        assert(p1.symbols.at("BUFFER") == 0x1006);
        assert(p1.programLength == 9);
        recordTest("Passagem 1: START 1000 e calculo de LC");
    }

    // 2.2 Ausencia de START (inicia em 0)
    {
        const std::string text = "FIRST LDA #0\n"
                                 "      END FIRST\n";
        ParseResult pRes = parseText(text);
        PassOneResult p1 = runPassOne(pRes.lines);
        assert(p1.success());
        assert(p1.startAddress == 0x0000);
        assert(p1.symbols.at("FIRST") == 0x0000);
        recordTest("Passagem 1: inicio padrao em 0x0000");
    }

    // 2.3 BYTE (C e X) e reservas (RESB e RESW)
    {
        const std::string text = "PROG  START 0\n"
                                 "S1    BYTE  C'EOF'\n"    // 3 bytes
                                 "S2    BYTE  X'F1'\n"     // 1 byte
                                 "S3    BYTE  X'05A0'\n"   // 2 bytes
                                 "S4    RESW  2\n"         // 6 bytes
                                 "S5    RESB  5\n"         // 5 bytes
                                 "      END   S1\n";
        ParseResult pRes = parseText(text);
        PassOneResult p1 = runPassOne(pRes.lines);
        assert(p1.success());
        assert(p1.symbols.at("S1") == 0x00);
        assert(p1.symbols.at("S2") == 0x03);
        assert(p1.symbols.at("S3") == 0x04);
        assert(p1.symbols.at("S4") == 0x06);
        assert(p1.symbols.at("S5") == 0x0C);
        assert(p1.programLength == 0x11); // 17 bytes
        recordTest("Passagem 1: BYTE C/X e RESB/RESW");
    }

    // 2.4 Rotulo duplicado
    {
        const std::string text = "PROG START 0\n"
                                 "DUP  LDA   #1\n"
                                 "DUP  STA   #2\n"
                                 "     END   DUP\n";
        ParseResult pRes = parseText(text);
        PassOneResult p1 = runPassOne(pRes.lines);
        assert(!p1.success());
        bool foundDupDiag = false;
        for (const auto& d : p1.diagnostics) {
            if (d.message.find("duplicate") != std::string::npos) {
                foundDupDiag = true;
                break;
            }
        }
        assert(foundDupDiag);
        recordTest("Passagem 1: deteccao de rotulo duplicado");
    }

    // 2.5 Opcode desconhecido
    {
        const std::string text = "PROG START 0\n"
                                 "     INVALID_OP #1\n"
                                 "     END\n";
        ParseResult pRes = parseText(text);
        PassOneResult p1 = runPassOne(pRes.lines);
        assert(!p1.success());
        bool foundUnknown = false;
        for (const auto& d : p1.diagnostics) {
            if (d.message.find("unknown") != std::string::npos) {
                foundUnknown = true;
                break;
            }
        }
        assert(foundUnknown);
        recordTest("Passagem 1: deteccao de opcode desconhecido");
    }
}

// ====================================================================
// 3. TESTES DA PASSAGEM 2 E CODIFICADOR
// ====================================================================
void testPassTwoAndEncoder() {
    std::cout << "\n=== Executando Testes da Passagem 2 e Codificador ===\n";

    // 3.1 Formato 1 via Encoder direto
    {
        EncodeContext ctx;
        EncodeOutcome out = encodeInstruction("NORM", "", 0x1000, ctx);
        assert(out.ok);
        assert(out.statement.bytes.size() == 1);
        assert(out.statement.bytes[0] == 0xC8);
        recordTest("Passagem 2: Formato 1 (NORM -> 0xC8)");
    }

    // 3.2 Formato 2
    {
        EncodeContext ctx;
        EncodeOutcome outClear = encodeInstruction("CLEAR", "X", 0x1000, ctx);
        assert(outClear.ok);
        assert(outClear.statement.bytes.size() == 2);
        assert(outClear.statement.bytes[0] == 0xB4);
        assert(outClear.statement.bytes[1] == 0x10); // X = 1, r2 = 0

        EncodeOutcome outCompr = encodeInstruction("COMPR", "A, S", 0x1002, ctx);
        assert(outCompr.ok);
        assert(outCompr.statement.bytes.size() == 2);
        assert(outCompr.statement.bytes[0] == 0xA0);
        assert(outCompr.statement.bytes[1] == 0x04); // A = 0, S = 4
        recordTest("Passagem 2: Formato 2 (CLEAR X, COMPR A,S)");
    }

    // 3.3 Formato 3 imediato (#), indireto (@) e indexado (,X)
    {
        SymbolTable symbols = {{"PTR", 0x1020}, {"BUFFER", 0x1030}};
        EncodeContext ctx;
        ctx.symbols = &symbols;

        // Imediato: LDA #5 -> opcode 0x00 | 0x01 = 0x01, xbpe = 0, disp = 5
        EncodeOutcome outImm = encodeInstruction("LDA", "#5", 0x1000, ctx);
        assert(outImm.ok);
        assert(outImm.statement.bytes.size() == 3);
        assert(outImm.statement.bytes[0] == 0x01);
        assert(outImm.statement.bytes[1] == 0x00);
        assert(outImm.statement.bytes[2] == 0x05);

        // Indireto: LDA @PTR em 0x1000 -> PC = 0x1003, disp = 0x1020 - 0x1003 = 0x01D
        // opcode 0x00 | 0x02 = 0x02, xbpe = p (0x20)
        EncodeOutcome outInd = encodeInstruction("LDA", "@PTR", 0x1000, ctx);
        assert(outInd.ok);
        assert(outInd.statement.bytes[0] == 0x02);
        assert(outInd.statement.bytes[1] == 0x20);
        assert(outInd.statement.bytes[2] == 0x1D);

        // Indexado: LDA BUFFER,X em 0x1000 -> disp = 0x1030 - 0x1003 = 0x02D
        // opcode 0x00 | 0x03 = 0x03, xbpe = x | p = 0x8 | 0x2 = 0xA -> 0xA0
        EncodeOutcome outIdx = encodeInstruction("LDA", "BUFFER,X", 0x1000, ctx);
        assert(outIdx.ok);
        assert(outIdx.statement.bytes[0] == 0x03);
        assert(outIdx.statement.bytes[1] == 0xA0);
        assert(outIdx.statement.bytes[2] == 0x2D);
        recordTest("Passagem 2: Formato 3 imediato (#), indireto (@) e indexado (,X)");
    }

    // 3.4 PC-relative negativo (deslocamento para tras com complemento de dois)
    {
        SymbolTable symbols = {{"LOOP", 0x1000}};
        EncodeContext ctx;
        ctx.symbols = &symbols;

        // J LOOP em 0x1006 -> PC = 0x1009, disp = 0x1000 - 0x1009 = -9 (0xFF7 em 12 bits)
        // opcode J = 0x3C | 0x03 = 0x3F, xbpe = p (0x2) -> 0x2F
        EncodeOutcome outJump = encodeInstruction("J", "LOOP", 0x1006, ctx);
        assert(outJump.ok);
        assert(outJump.statement.bytes[0] == 0x3F);
        assert(outJump.statement.bytes[1] == 0x2F);
        assert(outJump.statement.bytes[2] == 0xF7);
        recordTest("Passagem 2: PC-relative negativo (complemento de dois)");
    }

    // 3.5 Base-relative e deslocamento impossivel
    {
        SymbolTable symbols = {{"FAR", 0x2500}};
        EncodeContext ctx;
        ctx.symbols = &symbols;

        // Sem BASE definida: deslocamento excede PC-relative [-2048, 2047] -> erro
        EncodeOutcome outFail = encodeInstruction("LDA", "FAR", 0x1000, ctx);
        assert(!outFail.ok);
        assert(outFail.error.find("out of range") != std::string::npos);
        assert(outFail.error.find("format 4") != std::string::npos);

        // Com BASE definida em 0x2000: disp = 0x2500 - 0x2000 = 0x500 (em [0, 4095])
        // opcode 0x00 | 0x03 = 0x03, xbpe = b (0x4) -> 0x45, 0x00
        ctx.base = 0x2000;
        EncodeOutcome outBase = encodeInstruction("LDA", "FAR", 0x1000, ctx);
        assert(outBase.ok);
        assert(outBase.statement.bytes[0] == 0x03);
        assert(outBase.statement.bytes[1] == 0x45);
        assert(outBase.statement.bytes[2] == 0x00);
        recordTest("Passagem 2: Base-relative com BASE e rejeicao sem BASE");
    }

    // 3.6 Formato 4 estendido (+) com registro de modificacao
    {
        SymbolTable symbols = {{"SUBR", 0x1050}};
        EncodeContext ctx;
        ctx.symbols = &symbols;

        EncodeOutcome outF4 = encodeInstruction("+JSUB", "SUBR", 0x1000, ctx);
        assert(outF4.ok);
        assert(outF4.statement.bytes.size() == 4);
        assert(outF4.statement.modification.has_value());
        assert(outF4.statement.modification->address == 0x1001);
        assert(outF4.statement.modification->lengthInHalfBytes == 5);
        // JSUB opcode 0x48 | 0x03 = 0x4B, e=1 -> xbpe = 0x1
        assert(outF4.statement.bytes[0] == 0x4B);
        assert(outF4.statement.bytes[1] == 0x10);
        assert(outF4.statement.bytes[2] == 0x10);
        assert(outF4.statement.bytes[3] == 0x50);
        recordTest("Passagem 2: Formato 4 estendido (+JSUB) e registro M");
    }
}

// ====================================================================
// 4. TESTES DO PROGRAMA OBJETO (H, T, M, E)
// ====================================================================
void testObjectProgram() {
    std::cout << "\n=== Executando Testes do Programa Objeto ===\n";

    // 4.1 Registros H, M, E basicos
    {
        ObjectProgram prog("PROG", 0x1000, 0x50, 0x1000);
        prog.addModification(ModificationRecord{0x1017, 5});
        std::string txt = prog.toText();

        assert(txt.find("HPROG  001000000050\n") != std::string::npos);
        assert(txt.find("M00101705\n") != std::string::npos);
        assert(txt.find("E001000\n") != std::string::npos);
        recordTest("Objeto: formatacao de H, M e E");
    }

    // 4.2 Quebra de registro T ao exceder 30 bytes (0x1E)
    {
        ObjectProgram prog("SPLIT", 0x0000, 35, 0x0000);
        std::vector<uint8_t> thirtyFiveBytes(35, 0xAA);
        prog.addBlock(ObjectBlock{0x0000, thirtyFiveBytes});
        std::string txt = prog.toText();

        // Primeiro T deve conter 30 bytes (0x1E)
        std::string expectedT1 = "T0000001E";
        // Segundo T deve conter os 5 bytes restantes no endereco 0x001E (30)
        std::string expectedT2 = "T00001E05";
        assert(txt.find(expectedT1) != std::string::npos);
        assert(txt.find(expectedT2) != std::string::npos);
        recordTest("Objeto: quebra de registro T aos 30 bytes");
    }

    // 4.3 Quebra de registro T por lacuna de memoria (RESW / RESB)
    {
        ObjectProgram prog("GAP", 0x0000, 0x100, 0x0000);
        prog.addBlock(ObjectBlock{0x0000, {0x01, 0x02, 0x03}});
        prog.addBlock(ObjectBlock{0x0020, {0x04, 0x05}});
        std::string txt = prog.toText();

        assert(txt.find("T00000003010203\n") != std::string::npos);
        assert(txt.find("T000020020405\n") != std::string::npos);
        recordTest("Objeto: quebra de registro T por lacuna de memoria");
    }
}

// ====================================================================
// 5. TESTES DE INTEGRACAO E CARGA NO SIMULADOR
// ====================================================================
void testIntegrationAndSimulator() {
    std::cout << "\n=== Executando Testes de Integracao e Simulador ===\n";

    // 5.1 Montagem ponta a ponta de valid.asm
    ParseResult parsed = parseFile("examples/valid.asm");
    assert(parsed.success());

    PassOneResult p1 = runPassOne(parsed.lines);
    assert(p1.success());
    assert(p1.startAddress == 0x1000);
    assert(p1.programLength > 0);

    PassTwoResult p2 = runPassTwo(p1);
    assert(p2.success());
    assert(!p2.blocks.empty());
    assert(!p2.modifications.empty());

    ObjectProgram prog(p2.programName, p2.startAddress, p2.programLength, p2.executionStart);
    for (const auto& b : p2.blocks) {
        prog.addBlock(b);
    }
    for (const auto& m : p2.modifications) {
        prog.addModification(ModificationRecord{m.address, static_cast<uint8_t>(m.lengthInHalfBytes)});
    }
    std::string objText = prog.toText();
    assert(objText.front() == 'H');
    assert(objText.find("\nM") != std::string::npos);
    assert(objText.find("\nE") != std::string::npos);
    recordTest("Integracao: montagem ponta a ponta de examples/valid.asm");

    // 5.2 Carregamento na Machine via SimulatorBridge sem tocar no checkpoint 1
    {
        const uint32_t neededSize = p1.startAddress + p1.programLength + 64;
        Machine machine(neededSize);
        SimulatorBridge bridge(prog);

        std::vector<Diagnostic> diags = bridge.loadInto(machine);
        assert(diags.empty()); // nenhuma falha de memoria

        // Verifica se os primeiros bytes da primeira instrucao foram gravados
        // FIRST CLEAR X -> B4 10 em 0x1000
        assert(machine.getByte(0x1000) == 0xB4);
        assert(machine.getByte(0x1001) == 0x10);

        // Verifica constante C'EOF' em BUFFER
        uint32_t bufAddr = p1.symbols.at("BUFFER");
        assert(machine.getByte(bufAddr) == 'E');
        assert(machine.getByte(bufAddr + 1) == 'O');
        assert(machine.getByte(bufAddr + 2) == 'F');

        // Verifica constante X'F1' em HEXVAL
        uint32_t hexAddr = p1.symbols.at("HEXVAL");
        assert(machine.getByte(hexAddr) == 0xF1);

        recordTest("Integracao: carga correta na Machine via SimulatorBridge");
    }

    // 5.3 Validacao de examples/errors.asm (deve gerar erro e nao prosseguir)
    {
        ParseResult errParsed = parseFile("examples/errors.asm");
        assert(!errParsed.success());
        recordTest("Integracao: rejeicao com diagnostico para examples/errors.asm");
    }
}

} // namespace

int main() {
    std::cout << "====================================================\n";
    std::cout << " INICIANDO SUITE DE TESTES DO MONTADOR SIC/XE       \n";
    std::cout << "====================================================\n";

    testParser();
    testPassOne();
    testPassTwoAndEncoder();
    testObjectProgram();
    testIntegrationAndSimulator();

    std::cout << "\n====================================================\n";
    std::cout << " SUCESSO: Todos os " << g_passedTests << " testes foram aprovados!\n";
    std::cout << "====================================================\n";

    return 0;
}
