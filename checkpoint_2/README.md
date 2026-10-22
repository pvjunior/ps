# Checkpoint 2 — montador SIC/XE

**Integrante 6 — Gabriel M.** Responsável pela interface de linha de comando (`main.cpp`), pela compilação isolada (`Makefile`) e pela documentação deste ponto de entrada. O código do checkpoint 1 e os módulos dos outros integrantes não são alterados por esta contribuição.

## Compilação

Ambiente recomendado: **MSYS2 UCRT64**, com `g++` (C++17) e GNU Make. No terminal, dentro de `checkpoint_2`:

```bash
make clean
make
make test
```

O alvo principal é `assembler` (no Windows, normalmente `assembler.exe`). O `Makefile` compila os `.cpp` de `src/` e somente os componentes de núcleo necessários do simulador, sem compilar a interface TUI. Os objetos e dependências geradas ficam em `build/`, separados do checkpoint 1.

`make test` executa `tests/*.cpp`, se existirem. Se a equipe ainda não tiver disponibilizado testes C++, executa **testes de fumaça reais pela CLI**, gerando fontes temporários em `build/smoke/` para conferir montagem válida, registros H/T/M/E, carregamento opcional, diagnóstico e preservação do objeto anterior quando houver erro.

## Interface

```bash
./assembler arquivo.asm
./assembler arquivo.asm -o saida.obj
./assembler arquivo.asm --load
make run INPUT=arquivo.asm
```

Uso: `./assembler <entrada.asm> [-o saida.obj] [--load]`

- O argumento `.asm` é obrigatório. Se `-o` não for fornecido, o arquivo `.obj` é criado no mesmo diretório da fonte.
- Opções incorretas exibem a mensagem de uso e encerram com **código 2**.
- Diagnósticos do parser e de ambas as passagens são exibidos como `arquivo:linha: mensagem` em `stderr` e encerram com **código 1**. O arquivo `.obj` anterior permanece intacto nesses casos.
- Na montagem válida, o executável grava o objeto usando um arquivo temporário, exibe o endereço inicial, o comprimento e o total de símbolos e encerra com **código 0**.
- `--load` instancia `Machine`, passa o `ObjectProgram` à `SimulatorBridge` e reporta eventuais diagnósticos, sem abrir a interface de texto (TUI). Isso **carrega** a memória, mas não executa as instruções.

## Fluxo de orquestração

```text
arquivo.asm
   │
   ▼
parseFile(input)                  Integrante 2 — Vinícius
   │ ParseResult
   ▼
runPassOne(parsed.lines)          Integrante 3 — Samuel
   │ PassOneResult
   ▼
runPassTwo(first)                 Integrante 4 — Nickolas
   │ PassTwoResult: programa, blocos e modificações
   ▼
ObjectProgram(...)               Integrante 5 — Dionathan
  addBlock(...)
  addModification(...)
  toText() → arquivo.obj
   │
   └── --load → SimulatorBridge(object).loadInto(machine)
```

Os contratos da parte 5 utilizam um **construtor e métodos da classe** `ObjectProgram`, não a função `buildObjectProgram` da versão provisória. `loadInto()` retorna `std::vector<Diagnostic>`; vetor vazio significa ausência de erros informados pela ponte.

## Dependência de integração identificada em 21/10/2026

No bundle que contém a `main` no commit `d37febc`, tanto `PassTwo.h` quanto `ObjectProgram.h` declaram o tipo global `ObjectBlock`. Ao incluir ambas as interfaces no mesmo programa ocorre **redefinição de struct**. Essa questão **precisa ser corrigida na parte 5 pelo responsável**: `ObjectProgram.h` deve incluir `PassTwo.h` e reutilizar o tipo `ObjectBlock` existente em vez de declará-lo novamente.

Com **essa correção aplicada somente na cópia de integração**, os módulos reais compilam com `g++ -std=c++17 -Wall -Wextra -Wpedantic` sem warnings. Foram testados os registros H/T/M/E, uma fonte válida, uma fonte com símbolo inexistente, blocos maiores que 30 bytes, lacunas de memória e o comando `--load`. A correção não está dentro dos três arquivos do integrante 6 e deve entrar no histórico por meio da contribuição do integrante 5.

## Demonstração de integração

1. Executar `make clean && make && make test` na pasta `checkpoint_2`.
2. Executar `./assembler build/smoke/valid.asm -o build/smoke/demonstracao.obj --load` e mostrar o resumo.
3. Exibir o conteúdo do `.obj` (H/T/M/E) e explicar como a CLI encadeia as etapas.
4. Executar `./assembler build/smoke/invalid.asm -o build/smoke/demonstracao.obj` e explicar o diagnóstico, o retorno diferente de zero e a proteção da saída anterior.

A explicação do integrante 6 é sobre **orquestração, CLI e compilação**, mantendo as implementações das passagens e do objeto sob a responsabilidade dos outros integrantes.
