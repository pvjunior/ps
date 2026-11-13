# Checkpoint 3 — Processador de macros e macro-montador SIC/XE

**Disciplina:** Programação de Sistemas — UFPel — 2026/2  
**Parte 6:** Módulo principal, CLI e build  
**Responsável:** Gabriel M. Meireles

> **Estado de integração (12/11/2026):** a pasta `checkpoint_3` já foi
> criada na `main` e o integrante 2 publicou `MacroParser.h` e
> `MacroParser.cpp`. Ainda faltam os tipos compartilhados da Parte 1, a
> tabela/argumentos da Parte 3, o expansor da Parte 4 e a fachada de saída e
> montagem da Parte 5. A CLI não pode montar um programa com macros até a
> integração desses módulos; isso não altera a funcionalidade do Checkpoint 2.

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
quando uma nova montagem falhar. O parser do integrante 2 agora está disponível,
mas ainda precisa dos tipos da Parte 1 para compilar.

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

**Situação da atualização:** o Makefile já inclui automaticamente
`src/MacroParser.cpp` no build por meio de `$(wildcard src/*.cpp)`, sem precisar
adicionar o parser ao `main.cpp`. Como `MacroParser.h` inclui `MacroTypes.h`, o
parser **não compila isoladamente** enquanto a Parte 1 não publicar esse header.
O comando `make status` mostra as dependências presentes e pendentes, e `make`
interrompe antes de iniciar qualquer compilação caso existam arquivos ausentes.
Isso evita uma sequência confusa de erros por falta de headers.

O parser disponível expõe `parseMacroFile(path)` e
`parseMacroText(text, sourceName)`, retornando `MacroParseResult` com `lines` e
`diagnostics`; essa API deve ser consumida pela fachada da Parte 5 ou pelo
expansor da Parte 4, e **não diretamente pela CLI da Parte 6**. A implementação
atual também usa o campo `MacroLine.comment`, que precisa existir no contrato
publicado pela Parte 1 (além de `location`, `originalText`, `label`, `operation`
e `operands`).

**Atenção:** o build completo depende da publicação dos demais módulos e da
compatibilidade entre suas APIs. O `make status` verifica presença de arquivos,
**não** comprova compatibilidade C++ nem funcionamento do processador de macros.
Não há um processador fictício no código entregue.

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

## Contrato de integração pendente: Partes 1, 4, 5 e 6

A interface proposta no README da Parte 5 não fornece o caminho de `-o`, e
os resultados propostos pelas Partes 1 e 4 não especificam as estatísticas
solicitadas à CLI. **Antes de integrar, alinhar com os integrantes 4 e 5**
o seguinte contrato público em `MacroAssemblerPipeline.h`:

```cpp
// Exemplo de contrato a ser combinado com a Parte 5:
struct MacroAssemblerResult {
    std::vector<MacroDiagnostic> macroDiagnostics;
    std::vector<Diagnostic> assemblerDiagnostics;
    std::size_t macrosDefined = 0;
    std::size_t expansionsPerformed = 0;
    bool success() const noexcept; // verdadeiro somente sem falhas
};

MacroAssemblerResult runMacroAssembler(
    const std::string& inputPath,
    const std::string& expandedPath = "MASMAPRG.ASM",
    const std::string& objectPath = "");
```

Os erros devem conter `where.file`, `where.line` e `message`, como os tipos de
diagnóstico do Checkpoint 2. O caminho vazio para objeto indica que a fachada
pode aplicar um padrão quando usada fora da CLI; **a CLI sempre fornece o caminho
resolvido**, inclusive quando `-o` é omitido. As contagens devem vir do motor de
expansão (Parte 4), não ser estimadas pela CLI analisando o arquivo gerado.

Esse código é uma **proposta de contrato**, não uma declaração de que a Parte 5
já o implementou. Quando o arquivo real estiver disponível, os nomes dos campos
precisam coincidir; caso haja um contrato definitivo diferente, ajustar apenas
a comunicação na CLI após acordo com os responsáveis, sem editar seus módulos.

## Testes e critérios de aceite

1. `make status` mostra os dois arquivos do parser como presentes e indica os
   módulos ainda ausentes; `make clean && make` termina sem warnings **após a
   chegada das dependências e alinhamento das APIs**.
2. A entrada com macros gera o fonte expandido antes de iniciar o montador.
3. Definições de macro não são enviadas ao montador como instruções assembly.
4. O objeto produzido corresponde ao assembly expandido.
5. Um erro na expansão não dispara a montagem; objetos válidos anteriores são
   preservados em caso de falha.
6. `make test` inclui os cenários do integrante 7: macros simples, definições
   aninhadas, chamadas aninhadas, redefinição, `.SER`, erros e integração real.

Os requisitos 2–6 dependem da implementação dos demais módulos e não podem ser
considerados validados apenas com testes isolados da CLI.

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
