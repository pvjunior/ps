# Plano de Testes — Checkpoint 3 (Integrante 7)

## 1. Estratégia e escopo

A suíte de validação em [`tests/test_macro_processor.cpp`](../tests/test_macro_processor.cpp) usa exclusivamente **C++17 padrão** e **`assert`**, consumindo apenas as APIs públicas definidas em `checkpoint_3/include/`:

- `MacroTypes.h`
- `MacroParser.h`
- `MacroTable.h`
- `MacroArguments.h`
- `MacroProcessor.h`
- `MacroOutput.h`
- `MacroAssemblerPipeline.h`

Nenhum arquivo de produção ou do `Makefile` foi modificado. Os testes unitários, de integração e de CLI são executados por `make test` dentro de `checkpoint_3/`.

---

## 2. Exemplos reproduzíveis em `checkpoint_3/examples/`

| Arquivo | Objetivo | Macros definidas / Expansões |
|---|---|---|
| [`examples/simple_macro.asm`](../examples/simple_macro.asm) | Macro simples com parâmetros posicionais, nomeados, padrão, `.SER` e código SIC/XE preservado (incluindo formato 4 `+JSUB`). | 2 definições / 4 expansões |
| [`examples/nested_definitions.asm`](../examples/nested_definitions.asm) | Definição de macro dentro de macro (`FABRICA` gerando `SOMA_M` e `SUB_M`), com nome de parâmetro `&X` coincidente nos níveis 1 e 2 e `.SER` preservado para expansão da macro interna. | 3 definições / 5 expansões |
| [`examples/nested_calls.asm`](../examples/nested_calls.asm) | Chamada aninhada em 3 níveis (`CALCULA -> ACUMULA -> CARREGA`), propagação de rótulo de chamada (`FIRST`), `.SER` e redefinição encadeada de `CARREGA` com corpo maior na mesma passagem. | 4 definições / 10 expansões |
| [`examples/errors.asm`](../examples/errors.asm) | Cenário de falhas acumuladas: `MCEND` órfão, falta/excesso/desconhecido de argumentos, ciclo mútuo (`CICLOA <-> CICLOB`) e definição sem `MCEND`. Valida que nenhuma saída válida anterior é sobrescrita. | Falha em `MacroExpansion` |

---

## 3. Casos de teste registrados

| ID | Requisito validado | Arquivo ou entrada usada | Resultado esperado | Diagnóstico esperado | Módulo responsável |
|---|---|---|---|---|---|
| `T01` | 1. Parser: linhas comuns, `MCDEFN`, `MCEND`, parâmetros e comentários | Texto em memória (`parser.asm`) com comentário de linha inteira, linha vazia, `\r\n`, `MCDEFN`, `MCEND`, parâmetros `&ORIG,&DEST=BUFFER`, rótulo `ST.SER`, `C'A.B'` e comentário inline | `parseMacroText` retorna sucesso (`success() == true`), separa rótulo, operação, operandos e comentário e identifica `MCDEFN`/`MCEND` como operações com localização correta | Nenhum | `MacroParser` / `MacroTypes` |
| `T02` | 1. Parser: aspas inválidas e leitura de arquivo | Texto em memória (`aspas.asm`) com `BYTE C'SEM_FECHAMENTO` e chamada a `parseMacroFile` em caminho inexistente | `success() == false`; análise continua até a linha seguinte (`lines.size() == 3`) | `"unterminated single quote"` na linha 2; `"could not open macro source file"` para arquivo inexistente | `MacroParser` |
| `T03` | 2. Tabela: inserção e busca | Inserção de `MacroDefinition` com nome vazio e com nome `"  soma_um  "`, busca por `"SOMA_UM"` e `"INEXISTENTE"` | Nome vazio retorna `false`; busca normalizada retorna a definição inserida e `nullptr` para ausente | `"macro definition has an empty name"` para definição sem nome | `MacroTable` / `MacroTypes` |
| `T04` | 2. Tabela: redefinições encadeadas | Inserção de `CARREGA` v1 (1 linha) seguida de redefinição `carrega` v2 (3 linhas) em `MacroTable` | `find("CARREGA")` passa a apontar para v2 sem invalidar ou destruir o conteúdo da versão anterior v1 | Nenhum | `MacroTable` |
| `T05` | 3. Argumentos: posicionais, nomeados, padrões e substituição isolada | Definição `OPERA &A,&B=#1,&C=BUF` chamada com `"X1"` e `"TOTAL,C=SAIDA,&B=#9"`; substituição em linha com `&AB`, `X&A`, `C'&A'`, `C'L.SER'` e comentário | Associação exata dos posicionais, nomeados fora de ordem e valores padrão; `.SER` só é substituído quando `serial` é informado; constantes e comentários permanecem intactos | Nenhum | `MacroArguments` |
| `T06` | 3. Argumentos: ausentes e excedentes (além de desconhecido e repetido) | Definição `DUO &P1,&P2` chamada com `"#1"`, `"#1,#2,#3"`, `"#1,FOO=#2"` e `"#1,P1=#9"` | Todas as chamadas inválidas retornam `success() == false` em `MacroBindingResult` | `"argument ausente"`, `"excesso de argumentos posicionais"`, `"argument nomeado desconhecido"` e `"argumento repetido"` | `MacroArguments` |
| `T07` | 3, 8. Argumentos e níveis: parâmetros coincidentes em diferentes níveis via `#(d,i)` | `MacroDefinition` com `&X` em `definitionLevel = 1` e `&X` em `definitionLevel = 2` | Chamada nomeada por `X=INNER` falha por ambiguidade; chamada posicional associa `#(1,1) = OUTER` e `#(2,1) = INNER` e substitui corretamente cada nível | `"argumento nomeado ambiguo"` apenas na chamada nomeada | `MacroArguments` |
| `T08` | 4. Expansão simples com uma e várias chamadas | Fonte em memória (`simple.asm`) com macro `SOMA &REG,&VAL=#1` chamada 3 vezes (com rótulo `FIRST`, nomeado e padrão) | 1 definição, 3 expansões; `FIRST` propagado para a primeira linha emitida e diretivas `MCDEFN`/`MCEND` removidas | Nenhum | `MacroProcessor` |
| `T09` | 5, 8. Definição aninhada, contador de nível, pilha de formais e `#(d,i)` | Fonte em memória (`nest_def.asm`) com `EXTERNA &X,&Y` contendo `INTERNA &X,&Z=#7`, chamada antes e depois de `EXTERNA`, com `setTraceStream` | `INTERNA` antes de `EXTERNA` não expande; `trace` registra níveis `1 -> 2 -> 0`; `&X` de nível 1 é restaurado após `MCEND` interno e `INTERNA` usa `&Y` do nível 1 e seu próprio `&X` + padrão `#7` | Nenhum | `MacroProcessor` / `MacroArguments` |
| `T10` | 6. Chamada de macro dentro de outra macro | Fonte em memória (`nest_call.asm`) com cadeia `NIVEL1 -> NIVEL2 -> NIVEL3` | 3 definições, 3 expansões; rótulo `INICIO` propagado do nível 1 até a instrução `LDA #3` do nível 3 | Nenhum | `MacroProcessor` |
| `T11` | 7, 8. Combinação de definição e chamada aninhadas em vários níveis | Fonte em memória (`multi_combo.asm`) com `M1 -> M2 -> M3` (3 níveis de definição e chamada aninhados, todos usando parâmetro `&X`) | 3 definições, 3 expansões em uma passagem; cada nível resolve `#(1,1)`, `#(2,1)` e `#(3,1)` sem colisão | Nenhum | `MacroProcessor` / `MacroArguments` |
| `T12` | 9. Geração de rótulos únicos por `.SER` em chamadas repetidas | Fonte em memória (`ser.asm`) com `FABRICA` definindo `PASSO` contendo `FIM.SER`, chamada 2 vezes para `PASSO` | `.SER` preservado na definição interna e expandido como `FIM2` e `FIM3`; `.SER` em comentário e `C'X.SER'` permanece intacto | Nenhum | `MacroProcessor` / `MacroArguments` |
| `T13` | 10. Redefinição de macros e rejeição de referências adiante | Fonte em memória (`redef_fwd.asm`) chamando `DEPOIS #9` antes do `MCDEFN`, definindo v1, chamando `#1`, redefinindo v2 maior e chamando `#2` | Chamada anterior à definição permanece sem expansão (`DEPOIS #9`); primeira expansão usa v1 (`LDA #1`) e segunda usa v2 (`CLEAR A`, `ADD #2`) | Nenhum | `MacroProcessor` / `MacroTable` |
| `T14` | 11. Detecção de `MCEND` incorreto, definição não terminada, ciclo e profundidade | Fontes `struct_err.asm`, `cycle.asm` (`CA <-> CB`) e `depth.asm` (`M1 -> M2 -> M3` com limite 2) | Todos os erros são acumulados sem loop infinito ou queda abrupta | `"MCEND sem MCDEFN"`, `"MCDEFN sem prototipo"`, `"definicao de macro sem MCEND"`, `"chamada ciclica de macro"` e `"limite de profundidade de expansao excedido"` | `MacroProcessor` |
| `T15` | 12, 13. Preservação de linhas assembly e geração exata de `MASMAPRG.ASM` | Fonte SIC/XE sem macros (`plain.asm`), chamada a `writeExpandedSource(expanded)` sem 2º argumento, simulação de falha de expansão e `MacroExpansionResult` manual com `MCDEFN`/`MCEND` | `MASMAPRG.ASM` é criado com conteúdo idêntico e `\n` final; diretivas residuais são filtradas; resultado com erro aborta escrita preservando o `MASMAPRG.ASM` anterior | `"expansao de macros contem erros fatais"` no cenário de falha | `MacroProcessor` / `MacroOutput` |
| `T16` | 10, 14. Garantia de que o processador de macros roda antes do montador | Fontes `bad_both.asm` (erro macro + opcode inválido), `bad_quote.asm`, `ok_macro.asm` com caminho de saída inexistente e `forward_ref.asm` | Erros de parser/expansão ou falha ao gravar `MASMAPRG.ASM` mantêm `assemblerInvoked == false` e preservam `MASMAPRG.ASM`/`.obj` anteriores; referência adiante segue ao montador apenas após `MASMAPRG.ASM` gravado e falha na montagem | `"[Expansao de Macros]"` nos 3 primeiros; `"[Montagem] ... unknown opcode or directive"` na referência adiante | `MacroAssemblerPipeline` / `PassOne` |
| `T17` | 4, 9, 12, 13, 15. Integração completa: `simple_macro.asm` | `examples/simple_macro.asm` via `runMacroAssembler` | `success() == true`, `macrosDefined == 2`, `expansionsPerformed == 4`, `MASMAPRG.ASM` sem `MCDEFN`/`MCEND` com `FIM3`/`FIM4` e `.obj` com registros `H`, `T`, `M`, `E` | Nenhum | `MacroAssemblerPipeline` + Checkpoint 2 |
| `T18` | 5, 8, 9, 13, 15. Integração completa e determinismo: `nested_definitions.asm` | `examples/nested_definitions.asm` executado duas vezes via `runMacroAssembler` | `success() == true`, `macrosDefined == 3`, `expansionsPerformed == 5`, saídas idênticas nas duas execuções, zero `MCDEFN`/`MCEND` e `.obj` válido (`H`, `T`, `E`) | Nenhum | `MacroProcessor` / `MacroAssemblerPipeline` |
| `T19` | 6, 9, 10, 13, 15. Integração completa e determinismo: `nested_calls.asm` | `examples/nested_calls.asm` executado duas vezes via `runMacroAssembler` | `success() == true`, `macrosDefined == 4`, `expansionsPerformed == 10`, saídas idênticas nas duas execuções, redefinição aplicada na 2ª chamada, zero `MCDEFN`/`MCEND` e `.obj` válido | Nenhum | `MacroProcessor` / `MacroAssemblerPipeline` |
| `T20` | 11, 14. Erro de integração e preservação de saída válida anterior | `examples/errors.asm` executado apontando para os arquivos válidos gerados por `examples/simple_macro.asm` | `success() == false`, `failedStage == PipelineStage::MacroExpansion`, `assemblerInvoked == false`; `MASMAPRG.ASM` e `.obj` anteriores permanecem byte a byte intactos | Diagnósticos acumulados: `MCEND sem MCDEFN`, `ausente`, `excesso`, `desconhecido`, `chamada ciclica de macro`, `sem MCEND` | `MacroProcessor` / `MacroOutput` / `MacroAssemblerPipeline` |
| `T21` | 13, 15. Geração padrão de `MASMAPRG.ASM` pelo pipeline | Cópia de `examples/simple_macro.asm` em diretório isolado via `runMacroAssembler("simple_macro.asm")` | Cria literalmente `MASMAPRG.ASM` e `simple_macro.obj` no diretório corrente com conteúdo idêntico ao de `T17` | Nenhum | `MacroAssemblerPipeline` / `MacroOutput` |

---

## 4. Matriz de cobertura dos 15 requisitos

| Requisito | Casos que validam |
|---|---|
| 1. Parser (`MCDEFN`, `MCEND`, parâmetros, comentários, aspas) | `T01`, `T02` |
| 2. Tabela (inserção, busca, redefinições encadeadas) | `T03`, `T04` |
| 3. Argumentos (posicionais, nomeados, padrões, faltas, excessos, níveis) | `T05`, `T06`, `T07` |
| 4. Expansão simples (1 e várias chamadas) | `T08`, `T17` |
| 5. Definição de macro dentro de outra macro | `T09`, `T18` |
| 6. Chamada de macro dentro de outra macro | `T10`, `T19` |
| 7. Combinação de definição e chamada aninhadas em vários níveis | `T11` |
| 8. Contador de nível, pilha de formais e substituição `#(d,i)` | `T07`, `T09`, `T11`, `T18` |
| 9. Geração de rótulos únicos por `.SER` | `T05`, `T12`, `T17`, `T18`, `T19` |
| 10. Redefinição de macros e rejeição de referências adiante | `T04`, `T13`, `T16`, `T19` |
| 11. Detecção de `MCEND` incorreto, definição sem fim e ciclo | `T14`, `T20` |
| 12. Preservação de linhas assembly comuns | `T15`, `T17` |
| 13. Geração exata de `MASMAPRG.ASM` | `T15`, `T17`, `T18`, `T19`, `T21` |
| 14. Processador de macros antes do montador | `T16`, `T20` |
| 15. Integração completa (fonte -> expandido -> objeto SIC/XE) | `T17`, `T18`, `T19`, `T21` |

---

## 5. Como executar

Dentro de `checkpoint_3/`:

```bash
make test
```

Esse comando compila `macro_assembler` e `build/test_macro_processor`, executa os smoke tests do `Makefile` e roda os 21 casos da suíte `test_macro_processor.cpp`.

Para executar os exemplos individualmente via CLI (gravando em `build/demo/` para manter o diretório `examples/` limpo):

```bash
mkdir -p build/demo
./macro_assembler examples/nested_definitions.asm -o build/demo/nested_definitions.obj --expanded build/demo/MASMAPRG.ASM
./macro_assembler examples/nested_calls.asm -o build/demo/nested_calls.obj --expanded build/demo/nested_calls_MASMAPRG.ASM
```
