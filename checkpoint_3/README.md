# Checkpoint 3 — Processador de macros e macro-montador SIC/XE

**Parte 6:** Módulo principal, CLI e build  

> **Estado de integração (13/11/2026):** estão publicados o parser de macros
> (Parte 2), a saída `MacroOutput` e a fachada `MacroAssemblerPipeline` (Parte 5),
> além da CLI e do build desta Parte 6. **Ainda faltam** os tipos compartilhados
> (Parte 1), a tabela e associação de argumentos (Parte 3), o processador de
> macros (Parte 4) e os testes/exemplos (Parte 7). O build completo permanece
> bloqueado até que os módulos obrigatórios cheguem; o Checkpoint 2 não é alterado.

## Objetivo desta parte

Disponibilizar o ponto de entrada `macro_assembler`, que recebe o arquivo
assembly, solicita o processamento das macros **antes** da montagem e informa
os caminhos das saídas e as estatísticas. A lógica de reconhecer/expandir
macros não é duplicada no `main.cpp`: fica nos módulos próprios dos colegas.

Os arquivos desta parte são apenas:

- `checkpoint_3/main.cpp`: valida CLI, aciona a fachada e mostra diagnósticos e resumo;
- `checkpoint_3/Makefile`: compila o Checkpoint 3 isoladamente com C++17 e
  vincula as bibliotecas já existentes do Checkpoint 2;
- `checkpoint_3/README.md`: documentação, execução e dependências entre módulos.

O Checkpoint 2 não é alterado; a interface TUI do simulador não é aberta.

## Fluxo de execução

```text
arquivo.asm
    |
    v
macro_assembler (main.cpp)       [Parte 6: valida argumentos]
    |
    v
runMacroAssembler(...)           [Parte 5: integra etapas]
    |
    +--> Leitura / parser de macros [Parte 2]
    |         |
    |         v
    |    Processamento em uma passagem [Parte 4]
    |    (contratos da Parte 1, tabela e argumentos da Parte 3)
    |         |
    |         v
    +--> MASMAPRG.ASM (ou --expanded) [Parte 5]
              |
              v
        Montador do Checkpoint 2:
        parseFile -> runPassOne -> runPassTwo -> ObjectProgram
              |
              v
        programa.obj (ou -o)
```

O `main.cpp` chama a fachada uma única vez; a fachada é responsável por
**ler, expandir, gravar a saída expandida e só então montar**. Um erro na
expansão impede a montagem. O fluxo deve preservar objetos válidos anteriores
quando uma nova montagem falhar. **A fachada da Parte 5 já foi publicada** e
sua assinatura corresponde à chamada da CLI; a execução completa ainda depende
dos contratos e do motor de macros das Partes 1, 3 e 4.

## Compilar e testar

Requer GCC/G++ com C++17, GNU Make e um shell compatível com os comandos do
Makefile (Linux ou MSYS2 UCRT64 no Windows). Não há bibliotecas externas.

Abra um terminal na pasta `checkpoint_3`:

```bash
make status
make clean
make
make test
```

- `make status`: lista os arquivos de integração já presentes e os pendentes,
  mesmo antes de o projeto estar completo.
- `make`: gera `macro_assembler` (ou `macro_assembler.exe` no Windows) somente
  quando os módulos necessários estiverem disponíveis.
- `make test`: executa os testes C++ em `tests/*.cpp` quando estiverem disponíveis
  (Parte 7) e testes de fumaça pela CLI com fonte válida, fonte inválida,
  preservação do objeto e códigos de saída.
- `make clean`: apaga somente o executável e a pasta `build/` desta etapa.
- `make run INPUT=arquivo.asm`: compila e executa o macro-montador.

Arquivos `.o` e `.d` da compilação ficam em `checkpoint_3/build/`. O Makefile
reutiliza os `.cpp` do montador do Checkpoint 2, **excluindo seu `main.cpp`**,
para evitar duas funções `main()`. Também vincula o núcleo do simulador exigido
por `SimulatorBridge.cpp`, mas não compila `TerminalUI.cpp`.

**Situação em 13/11/2026:** os seguintes módulos já estão no repositório:

| Parte | Arquivos publicados | Estado |
|---|---|---|
| 2 — Parser | `MacroParser.h`, `MacroParser.cpp` | Presente |
| 5 — Saída | `MacroOutput.h`, `MacroOutput.cpp` | Presente |
| 5 — Integração | `MacroAssemblerPipeline.h`, `MacroAssemblerPipeline.cpp` | Presente |
| 6 — CLI/build | `main.cpp`, `Makefile`, `README.md` | Presente |
| 1, 3, 4 | Tipos, tabela, argumentos e expansor | **Pendentes** |
| 7 | Testes e exemplos | **Pendentes** |

O Makefile inclui automaticamente `src/MacroParser.cpp`, `src/MacroOutput.cpp`
e `src/MacroAssemblerPipeline.cpp` por meio de `$(wildcard src/*.cpp)`.
A fachada é utilizada diretamente pelo `main.cpp`, **sem** duplicar código dos
colegas. Atualmente `make status` identifica os oito arquivos obrigatórios
faltantes (`MacroTypes`, `MacroTable`, `MacroArguments` e `MacroProcessor`,
respectivos `.h` e `.cpp`). Enquanto faltarem, `make` informa os nomes e
interrompe antes da compilação.

O parser da Parte 2 utiliza `MacroLine.comment`, que deve constar no contrato
final `MacroTypes.h` da Parte 1. A saída/integração da Parte 5 também utiliza
`MacroDiagnostic.where`, `MacroLine.originalText` e `MacroLine.operation`;
esses nomes precisam permanecer coerentes entre as implementações. O uso de
`MacroOutput.h` com estruturas provisórias e o *fallback* presente na fachada
da Parte 5 servem à implementação isolada daquele colega: **não representam
uma expansão de macros completa**. O Makefile desta parte exige o verdadeiro
processador antes de liberar o executável.

**Verificação feita com este ZIP:** `main.cpp` compilou isoladamente com
`-std=c++17 -Wall -Wextra -Wpedantic`, e `make status`/`make` identificaram os
módulos pendentes. Isso **não** equivale a um teste de integração ou à aprovação
das macros aninhadas, que dependem das demais partes.

## Uso do executável

```text
./macro_assembler <entrada.asm> [-o programa.obj] [--expanded caminho]
```

Exemplos (no Windows/MSYS2, também é possível usar `./macro_assembler.exe`):

```bash
./macro_assembler examples/simple_macro.asm
./macro_assembler examples/simple_macro.asm -o build/programa.obj
./macro_assembler examples/simple_macro.asm --expanded build/expandido.asm
./macro_assembler examples/simple_macro.asm -o build/programa.obj --expanded build/expandido.asm
make run INPUT=examples/simple_macro.asm
make run INPUT=examples/simple_macro.asm ARGS='-o build/programa.obj'
```

Sem `-o`, o objeto fica ao lado do fonte e recebe extensão `.obj`. Sem
`--expanded`, a saída expandida é `MASMAPRG.ASM` no diretório de execução.
A CLI rejeita a utilização do mesmo caminho para fonte, saída expandida e
objeto, evitando sobrescrita acidental.

| Código | Significado |
|---|---|
| `0` | Macroprocessamento e montagem concluídos com sucesso |
| `1` | Erro de leitura, processamento, gravação ou montagem |
| `2` | Ausência de entrada ou parâmetros inválidos da CLI |

Os diagnósticos são impressos em `stderr` no formato `arquivo:linha: mensagem`.
Em caso de sucesso, a CLI mostra as macros definidas, as expansões realizadas,
o arquivo expandido e o objeto gerado.

## Contrato real de integração: Partes 5 e 6

Em 13/11/2026, a Parte 5 publicou `include/MacroAssemblerPipeline.h` e
`src/MacroAssemblerPipeline.cpp`. A assinatura pública está **compatível** com
os três parâmetros enviados pelo `main.cpp`, incluindo a opção `-o`:

```cpp
MacroAssemblerResult runMacroAssembler(
    const std::string& inputPath,
    const std::string& expandedPath = "MASMAPRG.ASM",
    const std::string& objectPath = "");
```

O resultado publicado contém `macroDiagnostics`, `assemblerDiagnostics`,
`macrosDefined`, `expansionsPerformed` e `success()`, todos consumidos pela CLI.
Também fornece `diagnostics` (com etapa de origem), `expandedPath`, `objectPath`
e sinalizadores das fases de expansão, escrita e montagem. Os diagnósticos da
CLI seguem `arquivo:linha: mensagem`, e a fachada acrescenta a etapa à mensagem.

**Ponto de atenção entre as Partes 1, 4 e 5:** a fachada tenta obter
`macrosDefined` e `expansionsPerformed` do `MacroExpansionResult` e usa zero
caso esses campos não existam. Para o resumo da CLI exibir contagens **reais**,
o contrato final da Parte 1 e a implementação da Parte 4 precisam fornecê-las.
Não é responsabilidade da CLI estimar esses valores lendo o arquivo gerado.

A Parte 5 já possui uma rotina que recebe resultados expandidos em memória
(`runMacroAssemblerFromExpandedResult`), útil para testes isolados. Entretanto,
para validar o comando público `runMacroAssembler` de ponta a ponta, o parser,
os contratos e o `MacroProcessor` precisam estar integrados de verdade.

## Testes e critérios de aceite

1. `make status` mostra os arquivos do parser e da fachada/saída como presentes
   e informa os oito arquivos obrigatórios ainda ausentes; `make clean && make`
   termina sem warnings **após a chegada das dependências e alinhamento das APIs**.
2. A entrada com macros gera o fonte expandido antes de iniciar o montador.
3. Definições de macro não são enviadas ao montador como instruções assembly.
4. O objeto produzido corresponde ao assembly expandido.
5. Um erro na expansão não dispara a montagem; objetos válidos anteriores são
   preservados em caso de falha.
6. `make test` inclui os cenários do integrante 7: macros simples, definições
   aninhadas, chamadas aninhadas, redefinição, `.SER`, erros e integração real.

Os requisitos 2–6 dependem da implementação dos demais módulos e não podem ser
considerados validados apenas com testes isolados da CLI. A disponibilidade
da fachada da Parte 5 confirma a compatibilidade da assinatura, **não** o
funcionamento completo do macroprocessador.

## Demonstração da Parte 6 no vídeo

No terminal aberto em `checkpoint_3`, mostrar o comando:

```bash
make clean && make
make test
./macro_assembler examples/nested_calls.asm -o build/demo.obj --expanded build/demo_expandido.asm
cat build/demo_expandido.asm
cat build/demo.obj
```

Explicar a função de `main.cpp`, a chamada à fachada, a sequência de geração
e o resumo exibido. Os exemplos pertencem ao integrante 7 e devem ser
confirmados após a integração; nenhum arquivo de exemplo é criado por esta
parte.

## Equipe

- Pablo Vicondo — Parte 1: contratos e tipos compartilhados.
- Vinícius Lameirão — Parte 2: parser de macros.
- Samuel Bottermund Flores — Parte 3: tabela e argumentos.
- Nickolas Werhli Barbosa — Parte 4: expansão em uma passagem.
- Dionathan Liermann Kringel — Parte 5: saída e integração.
- Gabriel M. Meireles — Parte 6: módulo principal, CLI e Makefile.
- Pedro Paulo Moreira — Parte 7: testes e exemplos.
