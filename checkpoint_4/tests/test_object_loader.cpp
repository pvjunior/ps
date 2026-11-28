// ====================================================================
// Suite de Testes — Carregador SIC/XE (Checkpoint 4)
// Integrante 7 — Testes, Exemplos e Validacao
//
// Utiliza exclusivamente C++17 padrao e assert, consumindo apenas
// as APIs publicas do carregador e do simulador sem alterar os
// modulos dos demais integrantes.
//
// Cobertura:
//   1. ObjectParser (arquivo real, texto CRLF, E sem endereco, erros)
//   2. LoadPlanner (carga na origem, delta +/-, sobreposicao, limites)
//   3. Carga absoluta vs relocadora (blocos, campo M e bytes intactos)
//   4. Relocacao de formato 4 (flags, campo par, M cruzando blocos T)
//   5. Preservacao de memoria e PC em caso de falha em todas as etapas
// ====================================================================

#include "LoadPlanner.h"
#include "LoaderTypes.h"
#include "Machine.h"
#include "ObjectLoader.h"
#include "ObjectParser.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr int kDefaultMemorySize = 1 << 20; // 1 MiB (espaco SIC/XE)
constexpr std::uint8_t kSentinelByte = 0xA5;

int g_passedTests = 0;

void recordTest(const std::string& name) {
    std::cout << "  [PASS] " << name << "\n";
    ++g_passedTests;
}

bool containsSubstring(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

bool hasLoaderDiag(const std::vector<LoaderDiagnostic>& diagnostics,
                   const std::string& substring) {
    for (const LoaderDiagnostic& diagnostic : diagnostics) {
        if (containsSubstring(diagnostic.message, substring)) {
            return true;
        }
    }
    return false;
}

void fillRegion(Machine& machine, int start, int length, std::uint8_t value) {
    std::vector<std::uint8_t> pattern(static_cast<std::size_t>(length), value);
    const LoadResult writeRes = machine.loader().write(start, pattern);
    assert(writeRes.ok);
}

void assertRegionFilled(const Machine& machine, int start, int length,
                        std::uint8_t expected) {
    const std::vector<std::uint8_t> actual = machine.dump(start, length);
    assert(actual.size() == static_cast<std::size_t>(length));
    for (std::uint8_t byte : actual) {
        assert(byte == expected);
    }
}

void assertFailurePreservesState(const ObjectLoadResult& result,
                                 const Machine& machine,
                                 std::uint32_t expectedPc,
                                 int sentinelStart,
                                 int sentinelLength,
                                 const std::vector<std::uint8_t>& expectedMem) {
    assert(!result.success());
    assert(!result.loaded);
    assert(!result.diagnostics.empty());
    assert(machine.getRegisters().PC == expectedPc);
    assert(machine.getStepCount() == 0);
    assert(machine.dump(sentinelStart, sentinelLength) == expectedMem);
}

// ====================================================================
// 1. TESTES DO OBJECT PARSER
// ====================================================================
void testObjectParser() {
    std::cout << "\n=== 1. Testes do ObjectParser ===\n";

    // T01: Parsing de examples/valid.obj (objeto real do montador CP2)
    {
        const ObjectParseResult res = parseObjectFile("examples/valid.obj");
        assert(res.success());
        assert(res.object.has_value());
        const ParsedObject& obj = *res.object;

        assert(obj.header.has_value());
        assert(obj.header->programName == "PROG");
        assert(obj.header->originalStartAddress == 0x001000U);
        assert(obj.header->programLength == 0x000035U);
        assert(obj.header->location.file == "examples/valid.obj");
        assert(obj.header->location.line == 1);

        assert(obj.textRecords.size() == 3);
        assert(obj.textRecords[0].originalAddress == 0x001000U);
        assert(obj.textRecords[0].bytes.size() == 30);
        assert(obj.textRecords[0].bytes[0] == 0xB4U);
        assert(obj.textRecords[0].bytes[1] == 0x10U);
        assert(obj.textRecords[1].originalAddress == 0x00101EU);
        assert(obj.textRecords[1].bytes.size() == 9);
        assert(obj.textRecords[2].originalAddress == 0x00102EU);
        assert(obj.textRecords[2].bytes.size() == 7);

        assert(obj.modificationRecords.size() == 1);
        assert(obj.modificationRecords[0].originalAddress == 0x001017U);
        assert(obj.modificationRecords[0].fieldLengthHalfBytes == 5);

        assert(obj.end.has_value());
        assert(obj.end->originalExecutionAddress.has_value());
        assert(*obj.end->originalExecutionAddress == 0x001000U);
        recordTest("T01 ObjectParser: parsing completo de examples/valid.obj");
    }

    // T02: Parsing em memoria com CRLF, linhas vazias e E sem endereco
    {
        const std::string text =
            "HDEMO  000200000006\r\n"
            "\r\n"
            "T00020003010005\r\n"
            "E\r\n";
        const ObjectParseResult res = parseObjectText(text, "crlf.obj");
        assert(res.success());
        assert(res.object.has_value());
        assert(res.object->header->programName == "DEMO");
        assert(res.object->header->originalStartAddress == 0x000200U);
        assert(res.object->textRecords.size() == 1);
        assert(res.object->end.has_value());
        assert(!res.object->end->originalExecutionAddress.has_value());
        recordTest("T02 ObjectParser: CRLF, linhas vazias e registro E sem entrada");
    }

    // T03: Erros sintaticos, ordem de registros e arquivo inexistente
    {
        const ObjectParseResult fileErr = parseObjectFile("examples/error_syntax.obj");
        assert(!fileErr.success());
        assert(!fileErr.object.has_value());
        assert(hasLoaderDiag(fileErr.diagnostics, "invalid T record"));
        assert(hasLoaderDiag(fileErr.diagnostics, "T record after M"));
        assert(hasLoaderDiag(fileErr.diagnostics, "unknown object record"));
        assert(hasLoaderDiag(fileErr.diagnostics, "record after E"));

        const ObjectParseResult missingFile = parseObjectFile("examples/nao_existe.obj");
        assert(!missingFile.success());
        assert(hasLoaderDiag(missingFile.diagnostics, "could not open object file"));

        const ObjectParseResult noHeader = parseObjectText("T00100001AA\nE001000\n", "no_h.obj");
        assert(!noHeader.success());
        assert(hasLoaderDiag(noHeader.diagnostics, "first record must be H"));
        assert(hasLoaderDiag(noHeader.diagnostics, "missing H record"));

        const ObjectParseResult dupHeaderAndBad = parseObjectText(
            "HONE   001000000010\n"
            "HTWO   001000000010\n"
            "T00100000\n"
            "M00100000\n",
            "dup_h.obj");
        assert(!dupHeaderAndBad.success());
        assert(hasLoaderDiag(dupHeaderAndBad.diagnostics, "duplicate H record"));
        assert(hasLoaderDiag(dupHeaderAndBad.diagnostics, "invalid T record"));
        assert(hasLoaderDiag(dupHeaderAndBad.diagnostics, "invalid M record"));
        assert(hasLoaderDiag(dupHeaderAndBad.diagnostics, "missing E record"));
        recordTest("T03 ObjectParser: diagnosticos de sintaxe, ordem e arquivo ausente");
    }
}

// ====================================================================
// 2. TESTES DO LOAD PLANNER
// ====================================================================
void testLoadPlanner() {
    std::cout << "\n=== 2. Testes do LoadPlanner ===\n";

    const ObjectParseResult validParse = parseObjectFile("examples/valid.obj");
    assert(validParse.success());
    const ParsedObject& validObj = *validParse.object;

    // T04: Planejamento de carga absoluta (base == origem H, delta == 0)
    {
        const LoadPlanResult planRes = planLoad(validObj, 0x001000U, kDefaultMemorySize);
        assert(planRes.success());
        const ValidatedLoadPlan& plan = *planRes.plan;
        assert(plan.relocationDelta == 0);
        assert(plan.originalStartAddress == 0x001000U);
        assert(plan.finalStartAddress == 0x001000U);
        assert(plan.finalExecutionAddress == std::optional<std::uint32_t>(0x001000U));
        assert(plan.relocatedBlocks.size() == 3);
        assert(plan.relocatedBlocks[0].finalAddress == 0x001000U);
        assert(plan.relocatedBlocks[1].finalAddress == 0x00101EU);
        assert(plan.relocatedBlocks[2].finalAddress == 0x00102EU);
        assert(plan.relocatedModifications.size() == 1);
        assert(plan.relocatedModifications[0].finalAddress == 0x001017U);
        recordTest("T04 LoadPlanner: carga absoluta na origem (delta == 0)");
    }

    // T05: Planejamento com delta positivo e delta negativo
    {
        const LoadPlanResult posRes = planLoad(validObj, 0x002000U, kDefaultMemorySize);
        assert(posRes.success());
        assert(posRes.plan->relocationDelta == 0x1000);
        assert(posRes.plan->finalStartAddress == 0x002000U);
        assert(posRes.plan->finalExecutionAddress == std::optional<std::uint32_t>(0x002000U));
        assert(posRes.plan->relocatedBlocks[0].finalAddress == 0x002000U);
        assert(posRes.plan->relocatedBlocks[1].finalAddress == 0x00201EU);
        assert(posRes.plan->relocatedBlocks[2].finalAddress == 0x00202EU);
        assert(posRes.plan->relocatedModifications[0].finalAddress == 0x002017U);

        const LoadPlanResult negRes = planLoad(validObj, 0x000500U, kDefaultMemorySize);
        assert(negRes.success());
        assert(negRes.plan->relocationDelta == -0x0B00);
        assert(negRes.plan->finalStartAddress == 0x000500U);
        assert(negRes.plan->finalExecutionAddress == std::optional<std::uint32_t>(0x000500U));
        assert(negRes.plan->relocatedBlocks[0].finalAddress == 0x000500U);
        assert(negRes.plan->relocatedBlocks[1].finalAddress == 0x00051EU);
        assert(negRes.plan->relocatedBlocks[2].finalAddress == 0x00052EU);
        assert(negRes.plan->relocatedModifications[0].finalAddress == 0x000517U);
        recordTest("T05 LoadPlanner: relocacao com delta positivo e delta negativo");
    }

    // T06: Rejeicao de sobreposicao e violacoes de limites
    {
        const ObjectParseResult overlapParse = parseObjectFile("examples/error_overlap.obj");
        assert(overlapParse.success());
        const LoadPlanResult overlapPlan =
            planLoad(*overlapParse.object, 0x001000U, kDefaultMemorySize);
        assert(!overlapPlan.success());
        assert(hasLoaderDiag(overlapPlan.diagnostics, "overlaps"));

        const ObjectParseResult boundsParse = parseObjectFile("examples/error_bounds.obj");
        assert(boundsParse.success());
        const LoadPlanResult boundsPlan =
            planLoad(*boundsParse.object, 0x001000U, kDefaultMemorySize);
        assert(!boundsPlan.success());
        assert(hasLoaderDiag(boundsPlan.diagnostics, "T record lies outside"));
        assert(hasLoaderDiag(boundsPlan.diagnostics, "E record entry address lies outside"));

        // Excede memoria disponivel na Machine
        const LoadPlanResult smallMemPlan = planLoad(validObj, 0x001000U, 0x001020U);
        assert(!smallMemPlan.success());
        assert(hasLoaderDiag(smallMemPlan.diagnostics, "exceeds available memory"));

        // Excede espaco de enderecamento de 24 bits
        const LoadPlanResult limit24Plan = planLoad(validObj, 0xFFFFE0U, 0x2000000U);
        assert(!limit24Plan.success());
        assert(hasLoaderDiag(limit24Plan.diagnostics, "24-bit address space"));
        recordTest("T06 LoadPlanner: rejeicao de sobreposicao e limites de H/memoria/24 bits");
    }
}

// ====================================================================
// 3. CARGA ABSOLUTA VS RELOCADORA NO SIMULADOR
// ====================================================================
void testAbsoluteVsRelocatedLoad() {
    std::cout << "\n=== 3. Carga Absoluta vs Relocadora ===\n";

    // T07: Comparacao direta do mesmo examples/valid.obj em 0x1000 (absoluta) e 0x2000 (relocadora)
    {
        Machine absMachine(kDefaultMemorySize);
        Machine relMachine(kDefaultMemorySize);

        // Preenche as lacunas esperadas com sentinela para garantir que nao sao sobrescritas
        fillRegion(absMachine, 0x001027, 7, kSentinelByte);
        fillRegion(relMachine, 0x002027, 7, kSentinelByte);

        const ObjectLoadResult absRes = loadObject("examples/valid.obj", absMachine);
        const ObjectLoadResult relRes = loadObject("examples/valid.obj", relMachine, 0x002000U);

        assert(absRes.success());
        assert(relRes.success());
        assert(absRes.baseAddress == 0x001000U);
        assert(relRes.baseAddress == 0x002000U);
        assert(absRes.bytesLoaded == 46); // 30 + 9 + 7
        assert(relRes.bytesLoaded == 46);
        assert(absRes.executionAddress == std::optional<std::uint32_t>(0x001000U));
        assert(relRes.executionAddress == std::optional<std::uint32_t>(0x002000U));
        assert(absMachine.getRegisters().PC == 0x001000U);
        assert(relMachine.getRegisters().PC == 0x002000U);
        assert(absMachine.getStepCount() == 0);
        assert(relMachine.getStepCount() == 0);

        // Posicao dos 3 blocos e preservacao das lacunas
        assertRegionFilled(absMachine, 0x001027, 7, kSentinelByte);
        assertRegionFilled(relMachine, 0x002027, 7, kSentinelByte);

        // Bytes de instrucoes sem marcacao M permanecem identicos nos dois carregamentos:
        // CLEAR X (B4 10), CLEAR A (B4 00), LDA #5 (01 00 05) e J HALT (3F 2F FD)
        const std::vector<std::uint8_t> expectedPrefix = {0xB4, 0x10, 0xB4, 0x00, 0x01, 0x00, 0x05};
        assert(absMachine.dump(0x001000, 7) == expectedPrefix);
        assert(relMachine.dump(0x002000, 7) == expectedPrefix);

        const std::vector<std::uint8_t> expectedJump = {0x3F, 0x2F, 0xFD};
        assert(absMachine.dump(0x00101A, 3) == expectedJump);
        assert(relMachine.dump(0x00201A, 3) == expectedJump);

        // Bloco 2 (SUBR + PTR = 0x0003E8 = 1000) e Bloco 3 (C'EOF', X'F1', WORD 42) sem marcacao
        assert(absMachine.dump(0x00101E, 9) == relMachine.dump(0x00201E, 9));
        const std::vector<std::uint8_t> expectedDataBlock = {0x45, 0x4F, 0x46, 0xF1, 0x00, 0x00, 0x2A};
        assert(absMachine.dump(0x00102E, 7) == expectedDataBlock);
        assert(relMachine.dump(0x00202E, 7) == expectedDataBlock);

        // Campo marcado por M00101705 (+JSUB SUBR):
        // Original:  4B 10 10 1D (nibble xbpe=1, endereco 0x0101D)
        // Relocado:  4B 10 20 1D (nibble xbpe=1 preservado, endereco 0x0201D)
        const std::vector<std::uint8_t> expectedAbsJsub = {0x4B, 0x10, 0x10, 0x1D};
        const std::vector<std::uint8_t> expectedRelJsub = {0x4B, 0x10, 0x20, 0x1D};
        assert(absMachine.dump(0x001016, 4) == expectedAbsJsub);
        assert(relMachine.dump(0x002016, 4) == expectedRelJsub);
        recordTest("T07 Carga absoluta vs relocadora: blocos, lacuna, campo M e bytes sem M");
    }

    // T08: Carga com delta negativo de examples/valid.obj (base 0x0500, delta = -0x0B00)
    {
        Machine machine(kDefaultMemorySize);
        fillRegion(machine, 0x000527, 7, kSentinelByte);

        const ObjectLoadResult res = loadObject("examples/valid.obj", machine, 0x000500U);
        assert(res.success());
        assert(res.baseAddress == 0x000500U);
        assert(res.bytesLoaded == 46);
        assert(res.executionAddress == std::optional<std::uint32_t>(0x000500U));
        assert(machine.getRegisters().PC == 0x000500U);

        // +JSUB SUBR: 0x0101D - 0x0B00 = 0x0051D
        const std::vector<std::uint8_t> expectedNegJsub = {0x4B, 0x10, 0x05, 0x1D};
        assert(machine.dump(0x000516, 4) == expectedNegJsub);
        assertRegionFilled(machine, 0x000527, 7, kSentinelByte);
        const std::vector<std::uint8_t> expectedDataBlock = {0x45, 0x4F, 0x46, 0xF1, 0x00, 0x00, 0x2A};
        assert(machine.dump(0x00052E, 7) == expectedDataBlock);
        recordTest("T08 Carga relocadora com delta negativo (0x1000 -> 0x0500)");
    }
}

// ====================================================================
// 4. RELOCACAO DE FORMATO 4, CAMPOS PARES, FRONTEIRA DE BLOCOS E E SEM PC
// ====================================================================
void testFormat4AndEdgeRelocations() {
    std::cout << "\n=== 4. Relocacao de Formato 4 e Casos de Borda ===\n";

    // T09: Formato 4 com flags complexas (x=1, b=0, p=0, e=1 => nibble 0x9) e campo de 6 meios bytes
    {
        // Instrucao: 4B 90 10 10 (formato 4 indexado), WORD 0x001020 em 0x1004
        const std::string objText =
            "HFMT4  001000000007\n"
            "T001000074B901010001020\n"
            "M00100105\n"
            "M00100406\n"
            "E001000\n";

        Machine machine(kDefaultMemorySize);
        const ObjectLoadResult res = loadObjectText(objText, machine, 0x003000U, "fmt4.obj");
        assert(res.success());
        assert(res.bytesLoaded == 7);
        assert(machine.getRegisters().PC == 0x003000U);

        // Delta = +0x2000:
        // - Campo de 5 meios bytes em 0x3001 preserva nibble alto 9 -> 4B 90 30 10
        // - Campo de 6 meios bytes em 0x3004 -> 00 30 20
        const std::vector<std::uint8_t> expected = {0x4B, 0x90, 0x30, 0x10, 0x00, 0x30, 0x20};
        assert(machine.dump(0x003000, 7) == expected);
        recordTest("T09 Relocacao de formato 4 (nibble alto 0x90 preservado) e campo de 6 half-bytes");
    }

    // T10: Registro M cruzando dois registros T contiguos
    {
        // T1: 0x1000..0x1001 = 4B 10
        // T2: 0x1002..0x1003 = 10 20
        // M00100105 atravessa a fronteira entre T1 (byte 0x1001) e T2 (bytes 0x1002..0x1003)
        const std::string objText =
            "HSPLIT 001000000004\n"
            "T001000024B10\n"
            "T001002021020\n"
            "M00100105\n"
            "E001000\n";

        Machine machine(kDefaultMemorySize);
        const ObjectLoadResult res = loadObjectText(objText, machine, 0x002500U, "split.obj");
        assert(res.success());
        // 0x01020 + 0x1500 = 0x02520
        const std::vector<std::uint8_t> expected = {0x4B, 0x10, 0x25, 0x20};
        assert(machine.dump(0x002500, 4) == expected);
        recordTest("T10 Relocacao com registro M cruzando dois registros T contiguos");
    }

    // T11: Registro E sem endereco preserva PC anterior da Machine
    {
        const std::string objText =
            "HNOENT 001000000003\n"
            "T00100003010007\n"
            "E\n";

        Machine machine(kDefaultMemorySize);
        assert(machine.setPC(0x000123U));
        const ObjectLoadResult res = loadObjectText(objText, machine, 0x002000U, "no_entry.obj");
        assert(res.success());
        assert(!res.executionAddress.has_value());
        assert(machine.getRegisters().PC == 0x000123U);
        const std::vector<std::uint8_t> expected = {0x01, 0x00, 0x07};
        assert(machine.dump(0x002000, 3) == expected);
        recordTest("T11 Registro E sem endereco carrega bytes e preserva PC anterior");
    }
}

// ====================================================================
// 5. PRESERVACAO DE MEMORIA E PC EM CASO DE FALHA
// ====================================================================
void testFailurePreservesMemoryAndPC() {
    std::cout << "\n=== 5. Preservacao de Memoria e PC em Falhas ===\n";

    // T12: Falhas em parsing, planejamento, limites de memoria, lacuna e estouro de M
    {
        constexpr int kRegionStart = 0x001000;
        constexpr int kRegionLen = 64;
        constexpr std::uint32_t kInitialPc = 0x000321U;

        Machine machine(kDefaultMemorySize);
        fillRegion(machine, kRegionStart, kRegionLen, kSentinelByte);
        assert(machine.setPC(kInitialPc));
        const std::vector<std::uint8_t> expectedMem = machine.dump(kRegionStart, kRegionLen);

        // 1) Erro de parsing
        assertFailurePreservesState(
            loadObject("examples/error_syntax.obj", machine),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 2) Sobreposicao de T
        assertFailurePreservesState(
            loadObject("examples/error_overlap.obj", machine),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 3) Fora dos limites declarados em H
        assertFailurePreservesState(
            loadObject("examples/error_bounds.obj", machine),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 4) Registro M apontando para lacuna entre T
        assertFailurePreservesState(
            loadObject("examples/error_gap_modification.obj", machine),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 5) Estouro positivo de largura do campo M (5 meios bytes: 0xFFFF0 + 0x100 > 0xFFFFF)
        const std::string overflowObj =
            "HOVF   001000000004\n"
            "T001000044B1FFFF0\n"
            "M00100105\n"
            "E001000\n";
        assertFailurePreservesState(
            loadObjectText(overflowObj, machine, 0x001100U, "overflow.obj"),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 6) Estouro negativo (underflow) do campo M (0x00100 - 0x0800 < 0)
        const std::string underflowObj =
            "HUND   001000000004\n"
            "T001000044B100100\n"
            "M00100105\n"
            "E001000\n";
        assertFailurePreservesState(
            loadObjectText(underflowObj, machine, 0x000800U, "underflow.obj"),
            machine, kInitialPc, kRegionStart, kRegionLen, expectedMem);

        // 7) Limite da memoria da Machine (maquina pequena)
        Machine smallMachine(0x001010);
        fillRegion(smallMachine, 0x000800, 32, kSentinelByte);
        assert(smallMachine.setPC(0x000055U));
        const std::vector<std::uint8_t> smallExpected = smallMachine.dump(0x000800, 32);
        assertFailurePreservesState(
            loadObject("examples/valid.obj", smallMachine),
            smallMachine, 0x000055U, 0x000800, 32, smallExpected);

        recordTest("T12 Falhas em parser, planner, relocacao e memoria preservam RAM e PC");
    }
}

} // namespace

int main() {
    std::cout << "====================================================\n";
    std::cout << " SUITE DE TESTES DO CARREGADOR SIC/XE (CKPT 4)      \n";
    std::cout << "====================================================\n";

    testObjectParser();
    testLoadPlanner();
    testAbsoluteVsRelocatedLoad();
    testFormat4AndEdgeRelocations();
    testFailurePreservesMemoryAndPC();

    std::cout << "\n====================================================\n";
    std::cout << " SUCESSO: Todos os " << g_passedTests << " testes foram aprovados!\n";
    std::cout << "====================================================\n";
    return 0;
}
