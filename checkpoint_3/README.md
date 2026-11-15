# Checkpoint 3 — Processador de Macros e Macro-montador SIC/XE
 
**Linguagem:** C++17

## Visão geral

Neste checkpoint, o projeto incorpora um **processador de macros em uma passagem** ao montador SIC/XE desenvolvido anteriormente. O usuário fornece um programa assembly que pode conter definições e chamadas de macros. O sistema interpreta essas macros, gera um **arquivo assembly expandido** e, em seguida, utiliza o montador do Checkpoint 2 para produzir o **programa objeto SIC/XE**.

O objetivo é reutilizar o montador existente e manter separadas as responsabilidades de leitura, armazenamento, expansão, geração de arquivos e execução.

## Fluxo de funcionamento

```text
Programa fonte (.asm)
         |
         v
CLI (main.cpp) — recebe entrada e opções
         |
         v
Parser de macros — interpreta as linhas do fonte
         |
         v
Processador de macros — definições, tabela, argumentos e expansão
         |
         v
MASMAPRG.ASM — fonte assembly expandido
         |
         v
Montador SIC/XE do Checkpoint 2 — Passagens 1 e 2
         |
         v
Programa objeto (.obj)
```

A fachada `runMacroAssembler`, em `MacroAssemblerPipeline`, coordena esse fluxo. Se ocorrer erro durante o processamento de macros, a montagem não deve ser iniciada. Os diagnósticos são exibidos com a localização do problema.

## Organização do código

| Arquivo ou módulo | Função no sistema |
|---|---|
| `include/MacroTypes.h` e `src/MacroTypes.cpp` | Tipos e contratos compartilhados, incluindo linhas, definições e diagnósticos. |
| `include/MacroParser.h` e `src/MacroParser.cpp` | Leitura e interpretação do texto fonte. |
| `include/MacroTable.h` e `src/MacroTable.cpp` | Armazenamento e consulta de definições de macros. |
| `include/MacroArguments.h` e `src/MacroArguments.cpp` | Associação e substituição de parâmetros e argumentos. |
| `include/MacroProcessor.h` e `src/MacroProcessor.cpp` | Expansão das macros em uma passagem, inclusive chamadas e definições aninhadas. |
| `include/MacroOutput.h` e `src/MacroOutput.cpp` | Gravação do assembly expandido. |
| `include/MacroAssemblerPipeline.h` e `src/MacroAssemblerPipeline.cpp` | Integração entre processamento de macros e montagem SIC/XE. |
| `main.cpp` | Interface de linha de comando (CLI), validação de opções e exibição de resultados. |
| `Makefile` | Compilação, execução e testes do Checkpoint 3. |

Os arquivos de implementação estão em `checkpoint_3/src/`, os cabeçalhos públicos em `checkpoint_3/include/`. A montagem aproveita os arquivos de `checkpoint_2/`, sem alterar o fluxo do simulador de interface de terminal dos checkpoints anteriores.

## Recursos principais

- Definições delimitadas por **`MCDEFN`** e **`MCEND`**.
- Chamadas de macro substituídas por instruções assembly no ponto da chamada.
- Parâmetros posicionais e nomeados, com tratamento de valores padrão.
- Definições e chamadas aninhadas, inclusive controle de nível de definição.
- Redefinição de macros e substituição de **`.SER`** para distinguir rótulos internos de expansões diferentes.
- Diagnósticos para problemas de sintaxe, argumentos e expansão, com proteção contra recursão de macros sem término.
- Geração do arquivo intermediário **`MASMAPRG.ASM`** e do arquivo objeto **`.obj`**.

## Compilação e execução

**Pré-requisitos:** `g++` com suporte a C++17 e GNU Make. Em Windows, utilizar um terminal do MSYS2 (por exemplo, UCRT64). Não são necessárias bibliotecas externas adicionais.

Abra um terminal na pasta `checkpoint_3` e execute:

```bash
make clean
make
make test
```

O comando `make` produz `macro_assembler` (`macro_assembler.exe` no Windows). Os arquivos intermediários de compilação ficam em `build/`.

A sintaxe de uso é:

```text
./macro_assembler <entrada.asm> [-o programa.obj] [--expanded caminho]
```

- **`entrada.asm`**: caminho do programa assembly original;
- **`-o`**: nome ou caminho do objeto gerado; por padrão, utiliza o nome do fonte com extensão `.obj`;
- **`--expanded`**: nome ou caminho do fonte expandido; por padrão, `MASMAPRG.ASM`.

Por exemplo, após `make test`, é possível executar o exemplo de macro criado pelos testes básicos:

```bash
./macro_assembler build/smoke/macro.asm -o build/smoke/demo.obj --expanded build/smoke/demo_expandido.asm
```

No Windows/MSYS2, use `./macro_assembler.exe` se necessário. Também estão disponíveis `make run INPUT=arquivo.asm`, `make status` e `make clean`.

## Exemplo de expansão

**Fonte original:**

```asm
COPY START 1000
      MCDEFN
SOMAUM &NUM
      LDA &NUM
      ADD #1
      MCEND
FIRST SOMAUM #3
      STA VALUE
VALUE RESW 1
      END FIRST
```

**Fonte após o processamento de macros:**

```asm
COPY START 1000
FIRST LDA #3
      ADD #1
      STA VALUE
VALUE RESW 1
      END FIRST
```

Nesse exemplo, a chamada `SOMAUM #3` é substituída pelas instruções `LDA #3` e `ADD #1`. As diretivas de definição não são passadas ao montador. O assembly expandido pode então ser transformado em registros de programa objeto SIC/XE.

## Testes e tratamento de erros

`make test` executa os testes C++ disponíveis na pasta `tests/`, quando presentes, e os testes básicos de integração definidos no `Makefile`. Esses testes básicos incluem execução sem macros, expansão simples, geração de objeto, parâmetros inválidos da CLI, retorno de erro de montagem e preservação de um objeto anterior quando a montagem falha.

A CLI utiliza os seguintes códigos de retorno:

| Código | Significado |
|---|---|
| `0` | Processamento e montagem concluídos. |
| `1` | Falha de leitura, expansão, gravação ou montagem. |
| `2` | Argumentos da linha de comando ausentes ou inválidos. |

Em caso de sucesso, a saída informa a quantidade de macros definidas, a quantidade de expansões executadas e os caminhos dos arquivos gerados. Em caso de erro, os diagnósticos indicam arquivo, linha e mensagem.

## Equipe e divisão do trabalho

| Integrante | Responsabilidade |
|---|---|
| Pablo Vicondo | Parte 1 — Tipos e contratos compartilhados |
| Vinícius Lameirão | Parte 2 — Parser de macros |
| Samuel Bottermund Flores | Parte 3 — Tabela e argumentos |
| Nickolas Werhli Barbosa | Parte 4 — Processador e expansão em uma passagem |
| Dionathan Liermann Kringel | Parte 5 — Saída expandida e integração com o montador |
| Gabriel M. Meireles | Parte 6 — Módulo principal, CLI e Makefile |
| Pedro Paulo Moreira | Parte 7 — Testes e exemplos |

