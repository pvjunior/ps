# Checkpoint 3 — Processador de macros e macro-montador SIC/XE

**Integrante 6 — Módulo principal, interface de linha de comando (CLI) e build**  
**Disciplina:** Programação de Sistemas — UFPel (2026/2)

## Escopo e responsabilidade

Esta parte fornece o ponto de entrada do macro-montador e o processo de compilação
independente do Checkpoint 3. Ela não reimplementa o parser, a tabela de macros,
a associação de argumentos, o expansor nem a montagem SIC/XE.

Arquivos sob responsabilidade do integrante 6:

- `checkpoint_3/main.cpp` — interpreta os argumentos da CLI, chama a fachada de
  integração, apresenta diagnósticos e imprime o resumo da execução;
- `checkpoint_3/Makefile` — compila o executável isolado do Checkpoint 3,
  reaproveitando os módulos do Checkpoint 2, e disponibiliza testes básicos;
- `checkpoint_3/README.md` — documenta compilação, execução e integração.

A interface gráfica é uma contribuição opcional separada e **não está incluída
nesta entrega**. A CLI não abre a TUI do simulador.

## Fluxo do macro-montador

```text
arquivo.asm
    |
    v
main.cpp — Parte 6: interpreta as opções da CLI
    |
    v
runMacroAssembler(...) — Parte 5: coordena as etapas
    |
    +--> Parser — Parte 2
    |      |
    |      v
    |    Expansor em uma passagem — Parte 4
    |    (contratos da Parte 1; tabela e argumentos da Parte 3)
    |      |
    |      v
    +--> MASMAPRG.ASM (ou caminho definido por --expanded)
              |
              v
      Montador SIC/XE existente — Checkpoint 2
              |
              v
      programa.obj (ou caminho definido por -o)
```

A CLI chama apenas a API pública `runMacroAssembler`, implementada pelo
integrante 5. A expansão e a escrita do arquivo intermediário ocorrem antes
da montagem. A CLI não duplica as regras de macros nem altera os módulos dos
checkpoints anteriores.

## Estado de integração verificado (14/11/2026)

No ZIP recebido com as Partes 1 a 5, os seguintes módulos estão presentes:

| Parte | Módulo | Situação |
|---|---|---|
| 1 | `MacroTypes.h` e `MacroTypes.cpp` | Presente |
| 2 | `MacroParser.h` e `MacroParser.cpp` | Presente |
| 3 | `MacroTable` e `MacroArguments` (`.h`/`.cpp`) | Presente |
| 4 | `MacroProcessor.h` e `MacroProcessor.cpp` | Presente |
| 5 | `MacroOutput` e `MacroAssemblerPipeline` (`.h`/`.cpp`) | Presente |
| 6 | `main.cpp`, `Makefile` e este `README.md` | Presente |
| 7 | Testes e exemplos próprios do integrante 7 | Ainda não presentes no ZIP recebido |

**Verificação realizada com esses módulos:** `make clean && make` compilou
sem warnings, e `make test` passou nos testes básicos de CLI e integração
(incluindo expansão de uma macro simples até o objeto SIC/XE). Os testes
existentes do Checkpoint 2 haviam sido aprovados na integração anterior.
A suíte completa do integrante 7, quando publicada, deverá ser executada
separadamente pela mesma meta `make test`.

**Limitação conhecida fora da Parte 6:** em um caso com rótulo na chamada da
macro e outro rótulo já presente na primeira instrução expandida, o expansor
da Parte 4 pode gerar um rótulo isolado no fonte intermediário, causando erro
na montagem. Isso não foi corrigido por este pacote; deve ser verificado
pelo integrante 4 e coberto pelos testes do integrante 7. Portanto, o sucesso
da compilação e dos testes básicos não prova que todos os cenários de macro
estejam livres de falhas.

## Requisitos

- Compilador `g++` com suporte a C++17;
- GNU Make e shell compatível com os comandos do Makefile;
- Linux ou Windows com MSYS2 (recomendado: terminal UCRT64);
- Nenhuma biblioteca externa adicional.

O Makefile usa `-std=c++17 -Wall -Wextra -Wpedantic`, grava arquivos de
compilação em `checkpoint_3/build/` e gera `macro_assembler` (ou
`macro_assembler.exe` no Windows). Os headers do montador anterior são
localizados com `-I../checkpoint_2/include`, como solicitado na integração.
O Makefile exclui o `main.cpp` do Checkpoint 2 e não compila `TerminalUI.cpp`.

## Compilar e testar

Abra o terminal na pasta `checkpoint_3` e execute:

```bash
make status
make clean
make
make test
```

- `make status`: lista a presença de arquivos obrigatórios dos módulos;
- `make`: compila o executável do Checkpoint 3;
- `make test`: executa os testes C++ da pasta `tests/`, caso existam, e os
  testes básicos de integração da Parte 6;
- `make clean`: remove o executável e os artefatos da pasta `build/`;
- `make run INPUT=arquivo.asm`: compila (se necessário) e executa o macro-montador.

Os testes básicos (*smoke tests*) verificam: fonte assembly sem macro,
macro simples com `MCDEFN`/`MCEND`, geração de programa objeto, opções
`-o`/`--expanded`, código de saída 1 para erro de montagem, código 2 para
argumentos ausentes e preservação do objeto anterior após uma falha.
O Makefile cria `build/smoke/macro.asm` durante `make test`, fornecendo uma
entrada reproduzível para a demonstração.

## Utilização da CLI

```text
./macro_assembler <entrada.asm> [-o programa.obj] [--expanded caminho]
```

- `entrada.asm`: arquivo fonte original;
- `-o`: caminho do objeto SIC/XE; sem a opção, usa o nome do fonte com `.obj`;
- `--expanded`: caminho do assembly expandido; o padrão é `MASMAPRG.ASM`.

Exemplos, após `make test`:

```bash
./macro_assembler build/smoke/macro.asm
./macro_assembler build/smoke/macro.asm -o build/smoke/demo.obj --expanded build/smoke/demo_expandido.asm
make run INPUT=build/smoke/macro.asm ARGS='-o build/smoke/programa.obj'
```

A CLI rejeita argumentos duplicados, opções inválidas e caminhos de saída
que coincidam com o fonte ou com a outra saída. Os diagnósticos são
apresentados em `stderr` como `arquivo:linha: mensagem`.

| Código de retorno | Significado |
|---|---|
| `0` | Macroprocessamento e montagem concluídos |
| `1` | Erro de processamento, leitura, saída ou montagem |
| `2` | Parâmetros ausentes ou inválidos |

Em caso de sucesso, são impressos a quantidade de macros definidas, a
quantidade de expansões realizadas e os caminhos dos dois arquivos gerados.
As estatísticas são fornecidas pela fachada e pelo expansor, não contadas
pela CLI por interpretação do arquivo intermediário.

## Contrato com a Parte 5

A fachada publicada pelo integrante 5 disponibiliza:

```cpp
MacroAssemblerResult runMacroAssembler(
    const std::string& inputPath,
    const std::string& expandedPath = "MASMAPRG.ASM",
    const std::string& objectPath = "");
```

O `main.cpp` consome `success()`, `macroDiagnostics`,
`assemblerDiagnostics`, `macrosDefined` e `expansionsPerformed`. A chamada
com os três argumentos permite respeitar `-o` e `--expanded`, sem alterar
nenhum módulo de processamento dos demais integrantes.

## Demonstração curta para o vídeo

Na pasta `checkpoint_3`, antes da gravação, execute `make clean`, `make` e
`make test`, confirmando que tudo termina normalmente. Durante a gravação,
abra `main.cpp` e `Makefile` e mostre:

```bash
cat build/smoke/macro.asm
./macro_assembler build/smoke/macro.asm -o build/smoke/demo.obj --expanded build/smoke/demo_expandido.asm
cat build/smoke/demo_expandido.asm
cat build/smoke/demo.obj
```

Explique que a Parte 6 recebe os argumentos, chama a fachada da Parte 5,
apresenta os diagnósticos e informa os resultados, enquanto as regras de
expansão e montagem pertencem aos colegas. No Windows/MSYS2, também é
possível executar `./macro_assembler.exe`.

**Não envie** `build/`, executáveis compilados nem arquivos `.asm`/`.obj`
gerados durante os testes. São artefatos locais, não código-fonte.

## Equipe e divisão das partes

- Pablo Vicondo — Parte 1: contratos e tipos compartilhados;
- Vinícius Lameirão — Parte 2: parser de macros;
- Samuel Bottermund Flores — Parte 3: tabela e argumentos;
- Nickolas Werhli Barbosa — Parte 4: expansor em uma passagem;
- Dionathan Liermann Kringel — Parte 5: saída e integração;
- Gabriel M. Meireles — Parte 6: módulo principal, CLI e Makefile;
- Pedro Paulo Moreira — Parte 7: testes e exemplos.
