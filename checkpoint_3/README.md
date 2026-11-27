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

A fachada `runMacroAssembler`, em `MacroAssemblerPipeline`, coordena esse fluxo. Se ocorrer erro durante o processamento de macros, a montagem não deve ser iniciada. Os diagnósticos são exibidos com a localização do problema. O montador só é acionado após a gravação bem-sucedida do fonte expandido. Se a expansão falhar, o fonte expandido anterior é preservado; se a montagem falhar, o objeto anterior é preservado.

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
| `tests/*.cpp` | Caminho utilizado pelo Makefile para localizar os testes C++ dos módulos e da integração. |

Os arquivos de implementação estão em `checkpoint_3/src/`, os cabeçalhos públicos em `checkpoint_3/include/`. A montagem reaproveita os módulos de `checkpoint_2/`, sem executar a interface de terminal do simulador.

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

Por exemplo, após `make test` (ou `make smoke`), é possível executar o exemplo de macro criado pelos testes básicos:

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

Nesse exemplo, a chamada `SOMAUM #3` é substituída pelas instruções `LDA #3` e `ADD #1`. O argumento `#3` ocupa o lugar do parâmetro formal `&NUM`, e o rótulo `FIRST` é associado à primeira instrução expandida. As diretivas de definição não são passadas ao montador. O assembly expandido pode então ser transformado em registros de programa objeto SIC/XE.

Para conferir o resultado do comando de demonstração da seção anterior, execute:

```bash
cat build/smoke/demo_expandido.asm
cat build/smoke/demo.obj
```

O primeiro comando mostra o fonte após a expansão; o segundo apresenta o programa objeto. Para esse exemplo, o objeto esperado é:

```text
HCOPY  00100000000C
T001000090100031900010F2000
E001000
```

- `H` (Header): identifica o programa, seu endereço inicial e seu tamanho.
- `T` (Text): contém o endereço, o tamanho e os bytes das instruções e dos dados gerados.
- `E` (End): informa o endereço de entrada do programa.

O formato também admite registros `M` (Modification), utilizados quando a montagem identifica campos que precisam ser ajustados na relocação.

## Testes e tratamento de erros

### Testes automatizados

```bash
make test
```

Esse comando executa os testes básicos de integração (*smoke tests*) definidos no Makefile e compila e executa os arquivos de teste C++ encontrados em `tests/`, quando disponíveis. Os testes C++ são vinculados aos módulos do projeto sem incluir o `main.cpp` da CLI.

Para executar somente os testes básicos de integração:

```bash
make smoke
```

Os testes básicos verificam:

- montagem de um fonte sem macros;
- definição e expansão de uma macro simples;
- substituição dos parâmetros e remoção das diretivas de macro do fonte expandido;
- geração dos registros `H`, `T` e `E` do programa objeto;
- códigos de retorno para argumentos ausentes e erros de montagem;
- preservação de um arquivo objeto anterior quando a montagem falha.

Os exemplos e arquivos de saída desses testes são gerados em `build/smoke/`. A mensagem final `Smoke tests: CLI, macro simples, montagem, codigos de saida e objeto preservado: OK` indica que as verificações básicas foram aprovadas.

### Roteiro de validação dos recursos

Além dos testes básicos, os seguintes cenários permitem avaliar os módulos individualmente e o fluxo completo do macro-montador:

| Cenário | Comportamento a verificar |
|---|---|
| Leitura e parser | Separação de rótulo, operação, operandos e comentário; localização dos diagnósticos. |
| Tabela de macros | Registro, consulta e redefinição de macros. |
| Parâmetros e argumentos | Associação de argumentos posicionais e nomeados, valores padrão e substituição dos parâmetros. |
| Expansão simples e aninhada | Substituição das chamadas por instruções, inclusive chamadas e definições aninhadas. |
| Rótulos com `.SER` | Geração de rótulos distintos para expansões diferentes. |
| Entradas inválidas | Diagnósticos para erros de definição, argumentos ou ciclos de chamadas. |
| Integração com o montador | Geração do fonte expandido antes do objeto e preservação das saídas anteriores em caso de falha na etapa correspondente. |

Para testar manualmente outros cenários, salve o código assembly em um arquivo `.asm`, execute o macro-montador e compare o fonte expandido e o arquivo objeto com os resultados esperados.

### Testes manuais de erro

Após executar `make test`, é possível reutilizar o exemplo inválido gerado pelo Makefile:

```bash
./macro_assembler build/smoke/invalid.asm -o build/smoke/erro.obj --expanded build/smoke/erro_expandido.asm
echo $?
```

O fonte contém uma referência a um símbolo não definido (`MISSING`), portanto a montagem deve falhar, apresentar um diagnóstico e retornar `1`.

Para verificar o tratamento de argumentos ausentes:

```bash
./macro_assembler
echo $?
```

A CLI deve apresentar a sintaxe de uso e retornar `2`. O comando `echo $?` exibe o código de retorno do último comando executado no terminal (Bash/MSYS2).

A CLI utiliza os seguintes códigos de retorno:

| Código | Significado |
|---|---|
| `0` | Processamento e montagem concluídos. |
| `1` | Falha de leitura, expansão, gravação ou montagem. |
| `2` | Argumentos da linha de comando ausentes ou inválidos. |

Em caso de sucesso, a saída informa a quantidade de macros definidas, a quantidade de expansões executadas e os caminhos dos arquivos gerados. Em caso de erro, os diagnósticos indicam arquivo, linha e mensagem.

## Comandos disponíveis no Makefile

| Comando | Finalidade |
|---|---|
| `make` | Compilar o macro-montador. |
| `make test` | Executar os testes de integração e os testes C++ encontrados em `tests/`. |
| `make smoke` | Executar somente os testes básicos de integração. |
| `make run INPUT=arquivo.asm` | Executar o macro-montador com o fonte informado. |
| `make status` | Conferir os arquivos de implementação necessários à compilação. |
| `make clean` | Remover o executável e os arquivos gerados na pasta `build/`. |
