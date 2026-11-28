// ====================================================================
// Suite de Testes — Processador de Macros SIC/XE (Checkpoint 3)
// Integrante 7 — Testes, Exemplos e Validacao
//
// Utiliza exclusivamente C++17 padrao e assert, consumindo apenas
// as APIs publicas dos modulos do Checkpoint 3 sem alterar o codigo
// de producao dos demais integrantes.
// ====================================================================

#include "MacroArguments.h"
#include "MacroAssemblerPipeline.h"
#include "MacroOutput.h"
#include "MacroParser.h"
#include "MacroProcessor.h"
#include "MacroTable.h"
#include "MacroTypes.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace {

int g_passedTests = 0;

void recordTest(const std::string& name) {
    std::cout << "  [PASS] " << name << "\n";
    ++g_passedTests;
}

std::string readFileContents(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

void writeFileContents(const std::filesystem::path& path,
                       const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    assert(output.is_open());
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
    output.flush();
    assert(output.good());
}

bool containsSubstring(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool hasMacroDiag(const std::vector<MacroDiagnostic>& diagnostics,
                  const std::string& substring) {
    for (const MacroDiagnostic& diagnostic : diagnostics) {
        if (containsSubstring(diagnostic.message, substring)) {
            return true;
        }
    }
    return false;
}

bool hasAsmDiag(const std::vector<Diagnostic>& diagnostics,
                const std::string& substring) {
    for (const Diagnostic& diagnostic : diagnostics) {
        if (containsSubstring(diagnostic.message, substring)) {
            return true;
        }
    }
    return false;
}

std::string joinExpandedText(const std::vector<MacroLine>& lines) {
    std::ostringstream out;
    for (const MacroLine& line : lines) {
        out << (!line.originalText.empty() ? line.originalText : line.text) << '\n';
    }
    return out.str();
}

// Altera temporariamente o diretorio de trabalho para validar a geracao
// literal do nome padrao "MASMAPRG.ASM" sem sujar a raiz de checkpoint_3.
class ScopedCurrentDir {
public:
    explicit ScopedCurrentDir(const std::filesystem::path& targetDir)
        : previous_(std::filesystem::current_path()) {
        std::filesystem::create_directories(targetDir);
        std::filesystem::current_path(targetDir);
    }

    ~ScopedCurrentDir() {
        std::error_code ec;
        std::filesystem::current_path(previous_, ec);
    }

    ScopedCurrentDir(const ScopedCurrentDir&) = delete;
    ScopedCurrentDir& operator=(const ScopedCurrentDir&) = delete;

private:
    std::filesystem::path previous_;
};

const std::filesystem::path kTestTempRoot = "build/test_tmp";

// ====================================================================
// 1. TIPOS COMPARTILHADOS E PARSER DE MACROS
// ====================================================================
void testTypesAndParser() {
    std::cout << "\n=== 1. Testes de Tipos e Parser ===\n";

    // T01: Normalizacao e parsing de linhas comuns, comentarios e delimitadores
    {
        assert(normalizeMacroIdentifier("  &soma_um ") == "SOMA_UM");
        assert(normalizeMacroIdentifier("   mcdefn  ") == "MCDEFN");

        const std::string source =
            ". Comentario de linha inteira\r\n"
            "   \r\n"
            "MCDEFN\r\n"
            "COPIA   &ORIG,&DEST=BUFFER\r\n"
            "ST.SER  LDCH    &ORIG,X            . Comentario inline\r\n"
            "        BYTE    C'A.B'\r\n"
            "        MCEND\r\n"
            "FIRST   COPIA   TAB,SAIDA\r\n";

        const MacroParseResult parsed = parseMacroText(source, "parser.asm");
        assert(parsed.success());
        assert(parsed.lines.size() == 8);

        assert(parsed.lines[0].operation.empty());
        assert(containsSubstring(parsed.lines[0].comment, "Comentario de linha inteira"));
        assert(parsed.lines[1].operation.empty());
        assert(parsed.lines[2].label.empty());
        assert(parsed.lines[2].operation == "MCDEFN");
        assert(parsed.lines[3].label == "COPIA");
        assert(parsed.lines[3].operation == "&ORIG,&DEST=BUFFER");
        assert(parsed.lines[4].label == "ST.SER");
        assert(parsed.lines[4].operation == "LDCH");
        assert(parsed.lines[4].operands == "&ORIG,X");
        assert(containsSubstring(parsed.lines[4].comment, "Comentario inline"));
        assert(parsed.lines[5].operation == "BYTE");
        assert(parsed.lines[5].operands == "C'A.B'");
        assert(parsed.lines[5].comment.empty());
        assert(parsed.lines[6].label.empty());
        assert(parsed.lines[6].operation == "MCEND");
        assert(parsed.lines[7].label == "FIRST");
        assert(parsed.lines[7].operation == "COPIA");
        assert(parsed.lines[7].operands == "TAB,SAIDA");
        assert(parsed.lines[7].location.file == "parser.asm");
        assert(parsed.lines[7].location.line == 8);
        recordTest("T01 Parser: linhas comuns, MCDEFN/MCEND, parametros e comentarios");
    }

    // T02: Deteccao de aspas simples nao terminadas e arquivo inexistente
    {
        const std::string badQuotes =
            "PROG    START   1000\n"
            "        BYTE    C'SEM_FECHAMENTO\n"
            "        LDA     #1\n";

        const MacroParseResult parsed = parseMacroText(badQuotes, "aspas.asm");
        assert(!parsed.success());
        assert(parsed.lines.size() == 3);
        assert(hasMacroDiag(parsed.diagnostics, "unterminated single quote"));
        assert(parsed.diagnostics.front().where.file == "aspas.asm");
        assert(parsed.diagnostics.front().where.line == 2);

        const MacroParseResult missing =
            parseMacroFile("examples/arquivo_inexistente.asm");
        assert(!missing.success());
        assert(hasMacroDiag(missing.diagnostics, "could not open macro source file"));
        recordTest("T02 Parser: diagnostico de aspas invalidas e arquivo ausente");
    }
}

// ====================================================================
// 2. TABELA DE MACROS
// ====================================================================
void testMacroTable() {
    std::cout << "\n=== 2. Testes da Tabela de Macros ===\n";

    // T03: Insercao, busca normalizada e rejeicao de nome vazio
    {
        MacroTable table;
        MacroDiagnostic error;

        MacroDefinition emptyName;
        emptyName.location = MacroSourceLocation{"table.asm", 1};
        assert(!table.define(emptyName, &error));
        assert(containsSubstring(error.message, "empty name"));

        MacroDefinition def;
        def.name = "  soma_um  ";
        def.location = MacroSourceLocation{"table.asm", 3};
        MacroLine line;
        line.operation = "ADD";
        line.operands = "#1";
        def.body.push_back(line);

        assert(table.define(def, &error));
        assert(error.message.empty());
        const MacroDefinition* found = table.find("SOMA_UM");
        assert(found != nullptr);
        assert(found->name == "SOMA_UM");
        assert(found->body.size() == 1);
        assert(found->location.line == 3);
        assert(table.find("INEXISTENTE") == nullptr);
        recordTest("T03 Tabela: insercao, busca por nome normalizado e nome vazio");
    }

    // T04: Redefinicoes encadeadas preservando historico sem destruir versao anterior
    {
        MacroTable table;

        MacroDefinition v1;
        v1.name = "CARREGA";
        v1.location = MacroSourceLocation{"chain.asm", 2};
        v1.body.push_back(MacroLine{"", "LDA", "#(1,1)", "", {"chain.asm", 3}, "", ""});
        assert(table.define(v1));
        const MacroDefinition* ptrV1 = table.find("CARREGA");
        assert(ptrV1 != nullptr);

        MacroDefinition v2;
        v2.name = "carrega";
        v2.location = MacroSourceLocation{"chain.asm", 10};
        v2.body.push_back(MacroLine{"", "CLEAR", "A", "", {"chain.asm", 11}, "", ""});
        v2.body.push_back(MacroLine{"", "ADD", "#(1,1)", "", {"chain.asm", 12}, "", ""});
        v2.body.push_back(MacroLine{"", "ADD", "#1", "", {"chain.asm", 13}, "", ""});
        assert(table.define(v2));

        const MacroDefinition* ptrV2 = table.find("CARREGA");
        assert(ptrV2 != nullptr);
        assert(ptrV2 != ptrV1);
        assert(ptrV2->body.size() == 3);
        assert(ptrV2->location.line == 10);
        // Ponteiro anterior permanece integro e com o corpo antigo intacto.
        assert(ptrV1->body.size() == 1);
        assert(ptrV1->location.line == 2);
        recordTest("T04 Tabela: redefinicoes encadeadas mantem versao anterior");
    }
}

// ====================================================================
// 3. ASSOCIACAO DE ARGUMENTOS E SUBSTITUICAO DE PARAMETROS
// ====================================================================
void testMacroArguments() {
    std::cout << "\n=== 3. Testes de Argumentos e Substituicao ===\n";

    // T05: Posicionais, nomeados, valores padrao e delimitadores de identificador
    {
        MacroDefinition def;
        def.name = "OPERA";
        def.parameters = {
            MacroParameter{"&A", std::nullopt, 1, 1},
            MacroParameter{"&B", std::string("#1"), 1, 2},
            MacroParameter{"&C", std::string("BUF"), 1, 3},
        };

        const MacroBindingResult posDefault =
            bindMacroArguments(def, {"X1"}, {"args.asm", 10});
        assert(posDefault.success());
        assert(posDefault.arguments.at("&A") == "X1");
        assert(posDefault.arguments.at("&B") == "#1");
        assert(posDefault.arguments.at("&C") == "BUF");
        assert(posDefault.arguments.at("#(1,1)") == "X1");
        assert(posDefault.arguments.at("#(1,2)") == "#1");
        assert(posDefault.arguments.at("#(1,3)") == "BUF");

        const MacroBindingResult namedMixed =
            bindMacroArguments(def, {"TOTAL,C=SAIDA,&B=#9"}, {"args.asm", 11});
        assert(namedMixed.success());
        assert(namedMixed.arguments.at("&A") == "TOTAL");
        assert(namedMixed.arguments.at("&B") == "#9");
        assert(namedMixed.arguments.at("&C") == "SAIDA");

        const std::string line =
            "L.SER   LDA     &A+&AB+X&A,C'&A'+C'L.SER' . nao muda &A nem .SER";
        const std::string withoutSerial =
            substituteMacroParameters(line, namedMixed.arguments);
        assert(withoutSerial ==
               "L.SER   LDA     TOTAL+&AB+X&A,C'&A'+C'L.SER' . nao muda &A nem .SER");

        const std::string withSerial =
            substituteMacroParameters(line, namedMixed.arguments, 7);
        assert(withSerial ==
               "L7   LDA     TOTAL+&AB+X&A,C'&A'+C'L.SER' . nao muda &A nem .SER");
        recordTest("T05 Argumentos: posicionais, nomeados, padroes e substituicao segura");
    }

    // T06: Diagnosticos de argumentos ausentes, excedentes, desconhecidos e repetidos
    {
        MacroDefinition def;
        def.name = "DUO";
        def.parameters = {
            MacroParameter{"&P1", std::nullopt, 1, 1},
            MacroParameter{"&P2", std::nullopt, 1, 2},
        };

        const MacroBindingResult missing =
            bindMacroArguments(def, {"#1"}, {"diag.asm", 20});
        assert(!missing.success());
        assert(hasMacroDiag(missing.diagnostics, "ausente"));

        const MacroBindingResult excess =
            bindMacroArguments(def, {"#1,#2,#3"}, {"diag.asm", 21});
        assert(!excess.success());
        assert(hasMacroDiag(excess.diagnostics, "excesso"));

        const MacroBindingResult unknown =
            bindMacroArguments(def, {"#1,FOO=#2"}, {"diag.asm", 22});
        assert(!unknown.success());
        assert(hasMacroDiag(unknown.diagnostics, "desconhecido"));

        const MacroBindingResult repeated =
            bindMacroArguments(def, {"#1,P1=#9"}, {"diag.asm", 23});
        assert(!repeated.success());
        assert(hasMacroDiag(repeated.diagnostics, "repetido"));
        recordTest("T06 Argumentos: diagnosticos de falta, excesso, desconhecido e repetido");
    }

    // T07: Parametros coincidentes em diferentes niveis via #(d,i)
    {
        MacroDefinition nestedDef;
        nestedDef.name = "ANINHADA";
        nestedDef.parameters = {
            MacroParameter{"&X", std::nullopt, 1, 1},
            MacroParameter{"&X", std::nullopt, 2, 1},
        };

        const MacroBindingResult ambig =
            bindMacroArguments(nestedDef, {"OUTER,X=INNER"}, {"lvl.asm", 5});
        assert(!ambig.success());
        assert(hasMacroDiag(ambig.diagnostics, "ambiguo"));

        const MacroBindingResult ok =
            bindMacroArguments(nestedDef, {"OUTER,INNER"}, {"lvl.asm", 6});
        assert(ok.success());
        assert(ok.arguments.at("#(1,1)") == "OUTER");
        assert(ok.arguments.at("#(2,1)") == "INNER");
        assert(ok.arguments.find("&X") == ok.arguments.end());

        const std::string body = "        LDA     #(1,1)\n        ADD     #(2,1)\n";
        const std::string expanded = substituteMacroParameters(body, ok.arguments, 1);
        assert(expanded == "        LDA     OUTER\n        ADD     INNER\n");
        recordTest("T07 Argumentos: parametros coincidentes distinguidos por #(d,i)");
    }
}

// ====================================================================
// 4. EXPANSAO SIMPLES E ANINHAMENTO
// ====================================================================
void testSimpleExpansion() {
    std::cout << "\n=== 4. Testes de Expansao Simples ===\n";

    // T08: Expansao simples com uma e varias chamadas (posicionais, nomeados e padrao)
    {
        const std::string source =
            "PROG    START   1000\n"
            "        MCDEFN\n"
            "SOMA    &REG,&VAL=#1\n"
            "        LDA     &REG\n"
            "        ADD     &VAL\n"
            "        MCEND\n"
            "FIRST   SOMA    X1\n"
            "        SOMA    VAL=#5,REG=X2\n"
            "        SOMA    X3,#9\n"
            "X1      WORD    1\n"
            "X2      WORD    2\n"
            "X3      WORD    3\n"
            "        END     FIRST\n";

        const MacroParseResult parsed = parseMacroText(source, "simple.asm");
        assert(parsed.success());

        MacroProcessor processor;
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 1);
        assert(expanded.expansionsPerformed == 3);

        const std::string out = joinExpandedText(expanded.expandedLines);
        assert(!containsSubstring(out, "MCDEFN"));
        assert(!containsSubstring(out, "MCEND"));
        assert(containsSubstring(out, "FIRST   LDA     X1"));
        assert(containsSubstring(out, "        ADD     #1"));
        assert(containsSubstring(out, "        LDA     X2"));
        assert(containsSubstring(out, "        ADD     #5"));
        assert(containsSubstring(out, "        LDA     X3"));
        assert(containsSubstring(out, "        ADD     #9"));
        recordTest("T08 Expansao simples: uma e varias chamadas com rotulo e padrao");
    }
}

void testNestedDefinitionsAndFormalStack() {
    std::cout << "\n=== 5. Testes de Definicao Aninhada e Pilha #(d,i) ===\n";

    // T09: Definicao dentro de definicao, contador de nivel e pilha de parametros formais
    {
        const std::string source =
            "PROG    START   1000\n"
            "        MCDEFN\n"
            "EXTERNA &X,&Y\n"
            "        LDA     &X\n"
            "        MCDEFN\n"
            "INTERNA &X,&Z=#7\n"
            "        ADD     &X\n"
            "        SUB     &Y\n"
            "        MUL     &Z\n"
            "        MCEND\n"
            "        STA     &X\n"
            "        MCEND\n"
            "        INTERNA #99,#88\n"
            "        EXTERNA #10,#20\n"
            "        INTERNA #30\n"
            "        END\n";

        const MacroParseResult parsed = parseMacroText(source, "nest_def.asm");
        assert(parsed.success());

        std::ostringstream traceLog;
        MacroProcessor processor;
        processor.setTraceStream(&traceLog);
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 2);
        assert(expanded.expansionsPerformed == 2);

        const std::string trace = traceLog.str();
        assert(containsSubstring(trace, "nivel de definicao = 1"));
        assert(containsSubstring(trace, "nivel de definicao = 2"));
        assert(containsSubstring(trace, "nivel de definicao = 0"));

        assert(expanded.expandedLines.size() == 8);
        // Antes da expansao de EXTERNA, INTERNA nao esta na tabela e segue intacta.
        assert(expanded.expandedLines[1].operation == "INTERNA");
        // Expansao de EXTERNA: &X=10 antes e depois da definicao interna.
        assert(expanded.expandedLines[2].operation == "LDA");
        assert(expanded.expandedLines[2].operands == "#10");
        assert(expanded.expandedLines[3].operation == "STA");
        assert(expanded.expandedLines[3].operands == "#10");
        // Expansao de INTERNA: &X=30 (nivel 2), &Y=20 (nivel 1), &Z=#7 (padrao).
        assert(expanded.expandedLines[4].operation == "ADD");
        assert(expanded.expandedLines[4].operands == "#30");
        assert(expanded.expandedLines[5].operation == "SUB");
        assert(expanded.expandedLines[5].operands == "#20");
        assert(expanded.expandedLines[6].operation == "MUL");
        assert(expanded.expandedLines[6].operands == "#7");
        assert(expanded.expandedLines[7].operation == "END");
        recordTest("T09 Definicao aninhada: contador de nivel, pilha formal e #(d,i)");
    }
}

void testNestedCallsAndMultiLevel() {
    std::cout << "\n=== 6. Testes de Chamadas Aninhadas e Multiplos Niveis ===\n";

    // T10: Chamada de macro dentro de outra macro
    {
        const std::string source =
            "PROG    START   1000\n"
            "        MCDEFN\n"
            "NIVEL3  &V\n"
            "        LDA     &V\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "NIVEL2  &A,&B\n"
            "        NIVEL3  &A\n"
            "        ADD     &B\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "NIVEL1  &P,&Q,&DEST\n"
            "        NIVEL2  &P,&Q\n"
            "        STA     &DEST\n"
            "        MCEND\n"
            "INICIO  NIVEL1  #3,#4,SAIDA\n"
            "SAIDA   RESW    1\n"
            "        END     INICIO\n";

        const MacroParseResult parsed = parseMacroText(source, "nest_call.asm");
        assert(parsed.success());

        MacroProcessor processor;
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 3);
        assert(expanded.expansionsPerformed == 3);

        const std::string out = joinExpandedText(expanded.expandedLines);
        assert(containsSubstring(out, "INICIO  LDA     #3"));
        assert(containsSubstring(out, "        ADD     #4"));
        assert(containsSubstring(out, "        STA     SAIDA"));
        recordTest("T10 Chamada aninhada: macro chama outra em 3 niveis");
    }

    // T11: Combinacao de definicao e chamada aninhadas em 3 niveis com nome &X coincidente
    {
        const std::string source =
            "PROG    START   1000\n"
            "        MCDEFN\n"
            "M1      &X,&EXT\n"
            "        LDA     &X\n"
            "        MCDEFN\n"
            "M2      &X,&MID\n"
            "        ADD     &X\n"
            "        ADD     &MID\n"
            "        ADD     &EXT\n"
            "        MCDEFN\n"
            "M3      &X,&INT\n"
            "        SUB     &X\n"
            "        SUB     &MID\n"
            "        SUB     &EXT\n"
            "        SUB     &INT\n"
            "        MCEND\n"
            "        M3      &X,#9\n"
            "        MCEND\n"
            "        M2      #20,#30\n"
            "        MCEND\n"
            "        M1      #10,#40\n"
            "        END\n";

        const MacroParseResult parsed = parseMacroText(source, "multi_combo.asm");
        assert(parsed.success());

        MacroProcessor processor;
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 3);
        assert(expanded.expansionsPerformed == 3);
        assert(expanded.expandedLines.size() == 10);

        assert(expanded.expandedLines[1].operation == "LDA");
        assert(expanded.expandedLines[1].operands == "#10");
        assert(expanded.expandedLines[2].operation == "ADD");
        assert(expanded.expandedLines[2].operands == "#20");
        assert(expanded.expandedLines[3].operation == "ADD");
        assert(expanded.expandedLines[3].operands == "#30");
        assert(expanded.expandedLines[4].operation == "ADD");
        assert(expanded.expandedLines[4].operands == "#40");
        assert(expanded.expandedLines[5].operation == "SUB");
        assert(expanded.expandedLines[5].operands == "#20");
        assert(expanded.expandedLines[6].operation == "SUB");
        assert(expanded.expandedLines[6].operands == "#30");
        assert(expanded.expandedLines[7].operation == "SUB");
        assert(expanded.expandedLines[7].operands == "#40");
        assert(expanded.expandedLines[8].operation == "SUB");
        assert(expanded.expandedLines[8].operands == "#9");
        recordTest("T11 Aninhamento combinado: definicao + chamada em 3 niveis");
    }
}

// ====================================================================
// 5. .SER, REDEFINICAO, REFERENCIA ADIANTE E SEGURANCA
// ====================================================================
void testSerRedefinitionForwardAndErrors() {
    std::cout << "\n=== 7. Testes de .SER, Redefinicao, Forward Ref e Erros ===\n";

    // T12: Geracao de rotulos unicos por .SER em chamadas repetidas e aninhadas
    {
        const std::string source =
            "PROG    START   1000\n"
            "        MCDEFN\n"
            "FABRICA &NOME\n"
            "        MCDEFN\n"
            "&NOME   &VAL\n"
            "        LDA     &VAL\n"
            "        COMP    #0\n"
            "        JEQ     FIM.SER\n"
            "        ADD     #1\n"
            "FIM.SER STA     OUT         . comentario com .SER preservado\n"
            "        BYTE    C'X.SER'\n"
            "        MCEND\n"
            "        MCEND\n"
            "        FABRICA PASSO\n"
            "        PASSO   #5\n"
            "        PASSO   #6\n"
            "OUT     RESW    1\n"
            "        END\n";

        const MacroParseResult parsed = parseMacroText(source, "ser.asm");
        assert(parsed.success());

        MacroProcessor processor;
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 2);
        assert(expanded.expansionsPerformed == 3);

        const std::string out = joinExpandedText(expanded.expandedLines);
        assert(containsSubstring(out, "JEQ     FIM2"));
        assert(containsSubstring(out, "FIM2 STA     OUT"));
        assert(containsSubstring(out, "JEQ     FIM3"));
        assert(containsSubstring(out, "FIM3 STA     OUT"));
        assert(containsSubstring(out, ". comentario com .SER preservado"));
        assert(containsSubstring(out, "BYTE    C'X.SER'"));
        recordTest("T12 .SER: rotulos unicos em chamadas repetidas e definicao aninhada");
    }

    // T13: Redefinicao de macro e rejeicao de referencia adiante
    {
        const std::string source =
            "PROG    START   1000\n"
            "        DEPOIS  #9\n"
            "        MCDEFN\n"
            "DEPOIS  &V\n"
            "        LDA     &V\n"
            "        MCEND\n"
            "        DEPOIS  #1\n"
            "        MCDEFN\n"
            "DEPOIS  &V\n"
            "        CLEAR   A\n"
            "        ADD     &V\n"
            "        MCEND\n"
            "        DEPOIS  #2\n"
            "        END\n";

        const MacroParseResult parsed = parseMacroText(source, "redef_fwd.asm");
        assert(parsed.success());

        MacroProcessor processor;
        const MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 2);
        assert(expanded.expansionsPerformed == 2);

        // Linha antes da definicao permanece nao expandida (sem referencia adiante).
        assert(expanded.expandedLines[1].operation == "DEPOIS");
        assert(expanded.expandedLines[1].operands == "#9");
        // Primeira chamada usa versao 1; segunda chamada usa versao 2 redefinida.
        assert(expanded.expandedLines[2].operation == "LDA");
        assert(expanded.expandedLines[2].operands == "#1");
        assert(expanded.expandedLines[3].operation == "CLEAR");
        assert(expanded.expandedLines[4].operation == "ADD");
        assert(expanded.expandedLines[4].operands == "#2");
        recordTest("T13 Redefinicao e rejeicao de referencia adiante");
    }

    // T14: Deteccao de MCEND incorreto, definicao sem MCEND, ciclo e limite de profundidade
    {
        const std::string orphanSource =
            "        MCEND\n"
            "        MCDEFN\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "ABERTA  &X\n"
            "        LDA     &X\n";

        const MacroParseResult parsedOrphan = parseMacroText(orphanSource, "struct_err.asm");
        assert(parsedOrphan.success());
        MacroProcessor procOrphan;
        const MacroExpansionResult resOrphan = procOrphan.process(parsedOrphan.lines);
        assert(!resOrphan.success());
        assert(hasMacroDiag(resOrphan.diagnostics, "MCEND sem MCDEFN"));
        assert(hasMacroDiag(resOrphan.diagnostics, "MCDEFN sem prototipo"));
        assert(hasMacroDiag(resOrphan.diagnostics, "sem MCEND"));

        const std::string cycleSource =
            "        MCDEFN\n"
            "CA\n"
            "        CB\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "CB\n"
            "        CA\n"
            "        MCEND\n"
            "        CA\n";
        const MacroParseResult parsedCycle = parseMacroText(cycleSource, "cycle.asm");
        MacroProcessor procCycle;
        const MacroExpansionResult resCycle = procCycle.process(parsedCycle.lines);
        assert(!resCycle.success());
        assert(hasMacroDiag(resCycle.diagnostics, "chamada ciclica de macro"));

        const std::string deepSource =
            "        MCDEFN\n"
            "M3\n"
            "        NOP\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "M2\n"
            "        M3\n"
            "        MCEND\n"
            "        MCDEFN\n"
            "M1\n"
            "        M2\n"
            "        MCEND\n"
            "        M1\n";
        const MacroParseResult parsedDeep = parseMacroText(deepSource, "depth.asm");
        MacroProcessor procDeep(2);
        assert(procDeep.maxExpansionDepth() == 2);
        const MacroExpansionResult resDeep = procDeep.process(parsedDeep.lines);
        assert(!resDeep.success());
        assert(hasMacroDiag(resDeep.diagnostics,
                            "limite de profundidade de expansao excedido"));
        recordTest("T14 Erros estruturais: MCEND orfao, sem MCEND, ciclo e profundidade");
    }
}

// ====================================================================
// 6. PRESERVACAO ASSEMBLY E GERACAO DE MASMAPRG.ASM
// ====================================================================
void testPreservationAndMasmaprgOutput() {
    std::cout << "\n=== 8. Testes de Preservacao Assembly e MASMAPRG.ASM ===\n";

    // T15: Preservacao exata de linhas assembly que nao sao macros e geracao de MASMAPRG.ASM
    {
        const std::string asmOnly =
            "COPY    START   1000\n"
            ". Comentario preservado\n"
            "FIRST   CLEAR   X\n"
            "        NORM\n"
            "        +JSUB   SUBR\n"
            "SUBR    RSUB\n"
            "BUF     BYTE    C'EOF'\n"
            "HEX     BYTE    X'F1'\n"
            "        END     FIRST\n";

        const MacroParseResult parsed = parseMacroText(asmOnly, "plain.asm");
        assert(parsed.success());

        MacroProcessor processor;
        MacroExpansionResult expanded = processor.process(parsed.lines);
        assert(expanded.success());
        assert(expanded.macrosDefined == 0);
        assert(expanded.expansionsPerformed == 0);

        const std::filesystem::path dir = kTestTempRoot / "default_masmaprg";
        std::filesystem::remove_all(dir);

        {
            ScopedCurrentDir cwd(dir);
            const MacroWriteResult written = writeExpandedSource(expanded);
            assert(written.success());
            assert(written.outputPath == "MASMAPRG.ASM");
            assert(std::filesystem::exists("MASMAPRG.ASM"));

            const std::string content = readFileContents("MASMAPRG.ASM");
            assert(content == asmOnly);
            assert(!content.empty() && content.back() == '\n');

            // Testa preservacao do MASMAPRG.ASM anterior quando ha erro de expansao.
            MacroExpansionResult failed = expanded;
            failed.diagnostics.push_back(
                MacroDiagnostic{{"plain.asm", 4}, "erro simulado de expansao"});
            const MacroWriteResult aborted = writeExpandedSource(failed);
            assert(!aborted.success());
            assert(!aborted.written);
            assert(readFileContents("MASMAPRG.ASM") == asmOnly);
        }

        // Filtro defensivo para MCDEFN/MCEND em MacroExpansionResult montado em memoria.
        MacroExpansionResult manual;
        manual.expandedLines = {
            MacroLine{"", "START", "1000", "", {"m.asm", 1}, "PROG    START   1000", ""},
            MacroLine{"", "MCDEFN", "", "", {"m.asm", 2}, "        MCDEFN", ""},
            MacroLine{"M", "&A", "", "", {"m.asm", 3}, "M       &A", ""},
            MacroLine{"", "LDA", "&A", "", {"m.asm", 4}, "        LDA     &A", ""},
            MacroLine{"", "MCEND", "", "", {"m.asm", 5}, "        MCEND", ""},
            MacroLine{"", "END", "", "", {"m.asm", 6}, "        END", ""},
        };
        const std::filesystem::path filteredPath = kTestTempRoot / "filtered_masmaprg.asm";
        const MacroWriteResult filteredWrite =
            writeExpandedSource(manual, filteredPath.string());
        assert(filteredWrite.success());
        const std::string filteredContent = readFileContents(filteredPath);
        assert(!containsSubstring(filteredContent, "MCDEFN"));
        assert(!containsSubstring(filteredContent, "MCEND"));
        assert(filteredContent == "PROG    START   1000\n        END\n");
        recordTest("T15 Saida MASMAPRG.ASM: preservacao assembly, nome padrao e filtro");
    }
}

// ====================================================================
// 7. ORDEM DO PIPELINE E INTEGRACAO COMPLETA
// ====================================================================
void testPipelineOrderAndIntegration() {
    std::cout << "\n=== 9. Testes de Ordem do Pipeline e Integracao Completa ===\n";

    // T16: Processador de macros executa ANTES do montador SIC/XE
    {
        const std::filesystem::path dir = kTestTempRoot / "order";
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        // Caso A: erro de macro + opcode invalido juntos -> falha na expansao e NAO chama montador
        const std::filesystem::path badBothSrc = dir / "bad_both.asm";
        const std::filesystem::path expPath = dir / "MASMAPRG.ASM";
        const std::filesystem::path objPath = dir / "out.obj";
        writeFileContents(expPath, "SENTINEL_EXPANDED\n");
        writeFileContents(objPath, "SENTINEL_OBJ\n");
        writeFileContents(badBothSrc,
                          "BAD     START   1000\n"
                          "        INVALID_SICXE_OPCODE #1\n"
                          "        MCEND\n"
                          "        END\n");

        const MacroAssemblerResult resMacroFirst = runMacroAssembler(
            badBothSrc.string(), expPath.string(), objPath.string());
        assert(!resMacroFirst.success());
        assert(resMacroFirst.failedStage == PipelineStage::MacroExpansion);
        assert(!resMacroFirst.macroStageSucceeded);
        assert(!resMacroFirst.outputStageSucceeded);
        assert(!resMacroFirst.assemblerInvoked);
        assert(resMacroFirst.assemblerDiagnostics.empty());
        assert(readFileContents(expPath) == "SENTINEL_EXPANDED\n");
        assert(readFileContents(objPath) == "SENTINEL_OBJ\n");

        // Caso B: erro de aspas no parser de macros tambem bloqueia o montador
        const std::filesystem::path badQuoteSrc = dir / "bad_quote.asm";
        writeFileContents(badQuoteSrc,
                          "BADQ    START   1000\n"
                          "        BYTE    C'NAO_FECHA\n"
                          "        END\n");
        const MacroAssemblerResult resQuote = runMacroAssembler(
            badQuoteSrc.string(), expPath.string(), objPath.string());
        assert(!resQuote.success());
        assert(resQuote.failedStage == PipelineStage::MacroExpansion);
        assert(!resQuote.assemblerInvoked);
        assert(hasMacroDiag(resQuote.macroDiagnostics, "unterminated single quote"));

        // Caso C: gravacao em caminho invalido bloqueia invocacao do montador
        const std::filesystem::path okMacroSrc = dir / "ok_macro.asm";
        writeFileContents(okMacroSrc,
                          "OKM     START   1000\n"
                          "        MCDEFN\n"
                          "NOPM\n"
                          "        CLEAR   A\n"
                          "        MCEND\n"
                          "FIRST   NOPM\n"
                          "        END     FIRST\n");
        const std::filesystem::path unreachableExp =
            dir / "diretorio_inexistente" / "MASMAPRG.ASM";
        const MacroAssemblerResult resOutputFail = runMacroAssembler(
            okMacroSrc.string(), unreachableExp.string(), objPath.string());
        assert(!resOutputFail.success());
        assert(resOutputFail.macroStageSucceeded);
        assert(!resOutputFail.outputStageSucceeded);
        assert(resOutputFail.failedStage == PipelineStage::ExpandedOutput);
        assert(!resOutputFail.assemblerInvoked);

        // Caso D: referencia adiante nao expande na etapa de macros e falha no montador
        const std::filesystem::path fwdSrc = dir / "forward_ref.asm";
        writeFileContents(fwdSrc,
                          "FWD     START   1000\n"
                          "FIRST   DEPOIS\n"
                          "        MCDEFN\n"
                          "DEPOIS\n"
                          "        CLEAR   A\n"
                          "        MCEND\n"
                          "        END     FIRST\n");
        const MacroAssemblerResult resFwd = runMacroAssembler(
            fwdSrc.string(), expPath.string(), objPath.string());
        assert(!resFwd.success());
        assert(resFwd.macroStageSucceeded);
        assert(resFwd.outputStageSucceeded);
        assert(resFwd.assemblerInvoked);
        assert(!resFwd.assemblyStageSucceeded);
        assert(resFwd.failedStage == PipelineStage::Assembly);
        assert(hasAsmDiag(resFwd.assemblerDiagnostics, "unknown opcode or directive"));
        assert(readFileContents(objPath) == "SENTINEL_OBJ\n");
        recordTest("T16 Ordem do pipeline: macro -> MASMAPRG.ASM -> montador");
    }

    // T17: Integracao completa dos exemplos validos + determinismo + preservacao em errors.asm
    {
        const std::filesystem::path dir = kTestTempRoot / "examples_e2e";
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);

        // T17.1: examples/simple_macro.asm
        const std::filesystem::path simpleExp = dir / "simple_MASMAPRG.ASM";
        const std::filesystem::path simpleObj = dir / "simple.obj";
        const MacroAssemblerResult simpleRes = runMacroAssembler(
            "examples/simple_macro.asm", simpleExp.string(), simpleObj.string());
        assert(simpleRes.success());
        assert(simpleRes.macrosDefined == 2);
        assert(simpleRes.expansionsPerformed == 4);
        assert(simpleRes.assemblerInvoked);
        assert(simpleRes.assemblyStageSucceeded);

        const std::string simpleExpanded = readFileContents(simpleExp);
        assert(!containsSubstring(simpleExpanded, "MCDEFN"));
        assert(!containsSubstring(simpleExpanded, "MCEND"));
        assert(!containsSubstring(simpleExpanded, "SOMAR"));
        assert(!containsSubstring(simpleExpanded, "PULAR"));
        assert(containsSubstring(simpleExpanded, "FIRST   CLEAR   A"));
        assert(containsSubstring(simpleExpanded, "JEQ     FIM3"));
        assert(containsSubstring(simpleExpanded, "FIM3 STA     RESULT"));
        assert(containsSubstring(simpleExpanded, "JEQ     FIM4"));
        assert(containsSubstring(simpleExpanded, "FIM4 STA     RESULT"));

        const std::string simpleObjText = readFileContents(simpleObj);
        assert(simpleObjText == simpleRes.objectProgram);
        assert(simpleObjText.front() == 'H');
        assert(containsSubstring(simpleObjText, "\nT"));
        assert(containsSubstring(simpleObjText, "\nM"));
        assert(containsSubstring(simpleObjText, "\nE"));
        assert(simpleRes.assemblyResult.symbols.count("FIM3") == 1);
        assert(simpleRes.assemblyResult.symbols.count("FIM4") == 1);
        recordTest("T17 Integracao: examples/simple_macro.asm expande e monta objeto SIC/XE");

        // T17.2: examples/nested_definitions.asm (com checagem de determinismo)
        const std::filesystem::path nestDefExp1 = dir / "nestdef1_MASMAPRG.ASM";
        const std::filesystem::path nestDefObj1 = dir / "nestdef1.obj";
        const std::filesystem::path nestDefExp2 = dir / "nestdef2_MASMAPRG.ASM";
        const std::filesystem::path nestDefObj2 = dir / "nestdef2.obj";

        const MacroAssemblerResult nestDefRes1 = runMacroAssembler(
            "examples/nested_definitions.asm", nestDefExp1.string(), nestDefObj1.string());
        const MacroAssemblerResult nestDefRes2 = runMacroAssembler(
            "examples/nested_definitions.asm", nestDefExp2.string(), nestDefObj2.string());
        assert(nestDefRes1.success());
        assert(nestDefRes2.success());
        assert(nestDefRes1.macrosDefined == 3);
        assert(nestDefRes1.expansionsPerformed == 5);

        const std::string nestDefExpanded = readFileContents(nestDefExp1);
        assert(nestDefExpanded == readFileContents(nestDefExp2));
        assert(readFileContents(nestDefObj1) == readFileContents(nestDefObj2));
        assert(!containsSubstring(nestDefExpanded, "MCDEFN"));
        assert(!containsSubstring(nestDefExpanded, "MCEND"));
        assert(!containsSubstring(nestDefExpanded, "FABRICA"));
        assert(!containsSubstring(nestDefExpanded, "SOMA_M"));
        assert(!containsSubstring(nestDefExpanded, "SUB_M"));
        assert(containsSubstring(nestDefExpanded, "ADD     #10"));
        assert(containsSubstring(nestDefExpanded, "ADD     #20"));
        assert(containsSubstring(nestDefExpanded, "SUB     #5"));
        assert(containsSubstring(nestDefExpanded, "IGN3 STA     ULTIMO"));
        assert(containsSubstring(nestDefExpanded, "IGN4 STA     ULTIMO"));
        assert(containsSubstring(nestDefExpanded, "IGN5 STA     ULTIMO"));
        assert(readFileContents(nestDefObj1).front() == 'H');
        assert(containsSubstring(readFileContents(nestDefObj1), "\nE"));
        recordTest("T18 Integracao: examples/nested_definitions.asm deterministico e sem MCDEFN/MCEND");

        // T17.3: examples/nested_calls.asm (com checagem de determinismo e redefinicao)
        const std::filesystem::path nestCallExp1 = dir / "nestcall1_MASMAPRG.ASM";
        const std::filesystem::path nestCallObj1 = dir / "nestcall1.obj";
        const std::filesystem::path nestCallExp2 = dir / "nestcall2_MASMAPRG.ASM";
        const std::filesystem::path nestCallObj2 = dir / "nestcall2.obj";

        const MacroAssemblerResult nestCallRes1 = runMacroAssembler(
            "examples/nested_calls.asm", nestCallExp1.string(), nestCallObj1.string());
        const MacroAssemblerResult nestCallRes2 = runMacroAssembler(
            "examples/nested_calls.asm", nestCallExp2.string(), nestCallObj2.string());
        assert(nestCallRes1.success());
        assert(nestCallRes2.success());
        assert(nestCallRes1.macrosDefined == 4);
        assert(nestCallRes1.expansionsPerformed == 10);

        const std::string nestCallExpanded = readFileContents(nestCallExp1);
        assert(nestCallExpanded == readFileContents(nestCallExp2));
        assert(readFileContents(nestCallObj1) == readFileContents(nestCallObj2));
        assert(!containsSubstring(nestCallExpanded, "MCDEFN"));
        assert(!containsSubstring(nestCallExpanded, "MCEND"));
        assert(!containsSubstring(nestCallExpanded, "CARREGA"));
        assert(!containsSubstring(nestCallExpanded, "ACUMULA"));
        assert(!containsSubstring(nestCallExpanded, "CALCULA"));
        assert(containsSubstring(nestCallExpanded, "FIRST   LDA     NUM1"));
        assert(containsSubstring(nestCallExpanded, "ZERO2 STA    TEMP"));
        assert(containsSubstring(nestCallExpanded, "ZERO4 STA    TEMP"));
        assert(containsSubstring(nestCallExpanded, "CLEAR   A"));
        assert(containsSubstring(nestCallExpanded, "ZERO7 STA    TEMP"));
        assert(containsSubstring(nestCallExpanded, "ZERO9 STA    TEMP"));
        assert(readFileContents(nestCallObj1).front() == 'H');
        assert(containsSubstring(readFileContents(nestCallObj1), "\nE"));
        recordTest("T19 Integracao: examples/nested_calls.asm deterministico e redefinido");

        // T17.4: examples/errors.asm preserva MASMAPRG.ASM e .obj validos anteriores
        const MacroAssemblerResult errRes = runMacroAssembler(
            "examples/errors.asm", simpleExp.string(), simpleObj.string());
        assert(!errRes.success());
        assert(errRes.failedStage == PipelineStage::MacroExpansion);
        assert(!errRes.assemblerInvoked);
        assert(hasMacroDiag(errRes.macroDiagnostics, "MCEND sem MCDEFN"));
        assert(hasMacroDiag(errRes.macroDiagnostics, "ausente"));
        assert(hasMacroDiag(errRes.macroDiagnostics, "excesso"));
        assert(hasMacroDiag(errRes.macroDiagnostics, "desconhecido"));
        assert(hasMacroDiag(errRes.macroDiagnostics, "chamada ciclica de macro"));
        assert(hasMacroDiag(errRes.macroDiagnostics, "sem MCEND"));
        assert(readFileContents(simpleExp) == simpleExpanded);
        assert(readFileContents(simpleObj) == simpleObjText);
        recordTest("T20 Integracao: examples/errors.asm preserva saidas validas anteriores");

        // T17.5: Fluxo padrao gerando literalmente MASMAPRG.ASM no diretorio corrente
        const std::filesystem::path defaultDir = dir / "default_pipeline";
        const std::filesystem::path copiedSrc = defaultDir / "simple_macro.asm";
        writeFileContents(copiedSrc, readFileContents("examples/simple_macro.asm"));
        {
            ScopedCurrentDir cwd(defaultDir);
            const MacroAssemblerResult defaultRes = runMacroAssembler("simple_macro.asm");
            assert(defaultRes.success());
            assert(std::filesystem::exists("MASMAPRG.ASM"));
            assert(std::filesystem::exists("simple_macro.obj"));
            assert(readFileContents("MASMAPRG.ASM") == simpleExpanded);
            assert(readFileContents("simple_macro.obj") == simpleObjText);
        }
        recordTest("T21 Integracao: pipeline padrao gera MASMAPRG.ASM e objeto SIC/XE");
    }
}

} // namespace

int main() {
    std::error_code ec;
    std::filesystem::remove_all(kTestTempRoot, ec);

    std::cout << "====================================================\n";
    std::cout << " SUITE DE TESTES DO PROCESSADOR DE MACROS (CKPT 3)  \n";
    std::cout << "====================================================\n";

    testTypesAndParser();
    testMacroTable();
    testMacroArguments();
    testSimpleExpansion();
    testNestedDefinitionsAndFormalStack();
    testNestedCallsAndMultiLevel();
    testSerRedefinitionForwardAndErrors();
    testPreservationAndMasmaprgOutput();
    testPipelineOrderAndIntegration();

    std::filesystem::remove_all(kTestTempRoot, ec);

    std::cout << "\n====================================================\n";
    std::cout << " SUCESSO: Todos os " << g_passedTests << " testes foram aprovados!\n";
    std::cout << "====================================================\n";
    return 0;
}
