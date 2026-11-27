#pragma once

#include "LoaderTypes.h"
#include "Machine.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Resultado da carga de um programa objeto na memória da Machine.
// Compatível com a CLI do Checkpoint 4 (checkpoint_4/main.cpp) e com a suíte de testes.
struct ObjectLoadResult {
    std::uint32_t baseAddress = 0;
    std::size_t bytesLoaded = 0;
    std::optional<std::uint32_t> executionAddress;
    std::vector<LoaderDiagnostic> diagnostics;
    bool loaded = false;

    bool success() const noexcept {
        return loaded && diagnostics.empty();
    }

    explicit operator bool() const noexcept {
        return success();
    }
};

// Fachada principal do carregador (Integrante 5):
// 1. Lê o programa objeto em `path` via ObjectParser.
// 2. Define a base efetiva (`base` se fornecida, ou o endereço inicial do registro H) e valida
//    todo o plano de carga contra `machine.getMemorySize()` via LoadPlanner.
// 3. Aplica os registros de modificação (M) em memória temporária via Relocator.
// 4. Somente após sucesso integral de todas as etapas, escreve os bytes na memória da `Machine`
//    (usando `machine.loader().write`) e ajusta o PC para o endereço E relocado.
// Em caso de qualquer erro, preserva a memória e o PC anteriores da `Machine` intactos e não
// inicia a execução de instruções.
ObjectLoadResult loadObject(
    const std::string& path,
    Machine& machine,
    std::optional<std::uint32_t> base = std::nullopt);

// Sobrecarga auxiliar para carregar um programa objeto a partir de texto em memória
// (facilita testes independentes sem depender de arquivos em disco).
ObjectLoadResult loadObjectText(
    const std::string& text,
    Machine& machine,
    std::optional<std::uint32_t> base = std::nullopt,
    const std::string& sourceName = "<memory>");

// Executa planejamento, relocação e carga na `Machine` a partir de um `ParsedObject` já pronto.
ObjectLoadResult loadParsedObject(
    const ParsedObject& object,
    Machine& machine,
    std::optional<std::uint32_t> base = std::nullopt);

// Grava um `LoadWritePlan` já validado e relocado na `Machine` de forma atômica (tudo ou nada)
// e atualiza o PC somente após todas as escritas em memória serem concluídas com sucesso.
ObjectLoadResult applyLoadWritePlan(
    const LoadWritePlan& writePlan,
    std::uint32_t baseAddress,
    Machine& machine);
