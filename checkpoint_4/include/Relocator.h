#pragma once

#include "LoaderTypes.h"

// Relocador SIC/XE (Integrante 4).
//
// Recebe um plano de carga já validado por planLoad() e produz o plano final de
// escrita, com os campos dos registros M ajustados. Todo o trabalho é feito em
// memória temporária: o plano de entrada nunca é alterado e nenhuma Machine é
// tocada aqui. A escrita atômica na memória é responsabilidade do ObjectLoader.
//
// Nomes alternativos usados pelo README da parte 4; são apenas apelidos dos
// tipos públicos de LoaderTypes.h.
using LoadPlan = ValidatedLoadPlan;
using RelocationResult = LoadWritePlanResult;

// Separa dois efeitos distintos da relocação:
//   1. POSIÇÃO: onde cada byte/campo é carregado. Já foi resolvido pelo planner
//      (RelocatedBlock::finalAddress e RelocatedModification::finalAddress =
//      endereço original + delta). Esta função só usa esses endereços finais
//      para localizar os bytes; ela não recalcula posições.
//   2. VALOR: o conteúdo guardado em cada campo marcado por M. É aqui que o
//      delta (plan.relocationDelta, com sinal) é somado ao valor do campo.
//
// Regras:
//   - Só os campos apontados por registros M mudam; os demais bytes (instruções
//     e dados sem marcação) saem idênticos aos de entrada.
//   - A largura do campo é dada em meios bytes (1..8). Com largura ímpar o campo
//     começa no nibble BAIXO do primeiro byte, e o nibble ALTO desse byte é
//     preservado (em formato 4, 5 meios bytes em endereço+1 mantêm as flags
//     x/b/p/e intactas).
//   - Todos os bytes do campo precisam existir nos blocos T. Um campo pode
//     cruzar a fronteira de dois registros T contíguos, mas não uma lacuna.
//   - Vários registros M sobre o mesmo campo acumulam (o delta é somado a cada
//     um, na ordem do plano).
//
// REGRA DE ESTOURO (acordada com a Parte 1): sem "wrap-around". Se
// valor_do_campo + delta ficar fora de [0, 2^(4*largura) - 1], o campo não cabe
// no novo valor e é gerado um diagnóstico de erro. Qualquer diagnóstico
// invalida o resultado inteiro (plan fica vazio), então nada é carregado.
// Todos os problemas encontrados são reportados de uma vez, não só o primeiro.
//
// Em caso de sucesso, executionAddress é plan.finalExecutionAddress.
LoadWritePlanResult relocate(const ValidatedLoadPlan& plan);
