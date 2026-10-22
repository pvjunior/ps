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

## Estado da integração

Na versão integrada à branch `main` pelo **Pull Request #4**, a incompatibilidade de `ObjectBlock` foi resolvida: `ObjectProgram.h` reutiliza a definição compartilhada de `PassTwo.h`, sem declarar a estrutura novamente. As instruções SIC/XE de **formato 1** (`FIX`, `FLOAT`, `HIO`, `NORM`, `SIO` e `TIO`) também foram alinhadas entre a tabela de opcodes, a primeira passagem e o encoder.

Essas correções foram realizadas colaborativamente nos módulos dos respectivos integrantes. A **parte 6 permanece responsável somente pela CLI, pela orquestração, pelo Makefile e por esta documentação**.

A integração foi verificada em **Windows/MSYS2 UCRT64, G++ 16.2.0**, com `make`, `make test`, montagem válida, geração de H/T/M/E, `--load`, detecção de símbolo inexistente e preservação do objeto em caso de erro. Também foi realizado um teste específico das seis instruções de formato 1. Esses testes comprovam os cenários exercitados; a verificação mais abrangente dos requisitos permanece sob responsabilidade dos testes da equipe.

## Demonstração reproduzível da CLI

No terminal **MSYS2 UCRT64**, dentro de `checkpoint_2`, execute `make` e prepare os arquivos de demonstração abaixo. Eles são independentes dos exemplos gerados por `make test`:

```bash
mkdir -p build/video
cat > build/video/valid.asm <<'EOF'
COPY START 1000
FIRST LDA #3
      CLEAR X
      +JSUB SUBR
      STA BUFFER,X
      BYTE C'EOF'
BUFFER RESW 1
SUBR  RSUB
      END FIRST
EOF
cat > build/video/invalid.asm <<'EOF'
BAD START 1000
    LDA MISSING
    END
EOF
```

Demonstração de sucesso: o arquivo objeto é criado e carregado na memória do simulador, sem executar as instruções ou abrir a TUI.

```bash
./assembler.exe build/video/valid.asm -o build/video/video.obj --load
cat build/video/video.obj
```

Demonstração de erro: copie o objeto válido, tente montar uma fonte com símbolo inexistente usando a **mesma saída**, e confira que a montagem falha com código 1 sem alterar o objeto anterior.

```bash
cp build/video/video.obj build/video/copia.obj
./assembler.exe build/video/invalid.asm -o build/video/video.obj
echo $?
cmp -s build/video/video.obj build/video/copia.obj && echo "Objeto anterior preservado"
```

Argumentos ausentes também são tratados pela CLI: `./assembler.exe` imprime a sintaxe de uso e retorna **código 2**.

A explicação do integrante 6 deve mostrar **como as etapas são chamadas e como o programa é executado**, sem atribuir a ele as implementações internas do parser, das passagens, da codificação, da geração dos registros ou da ponte do simulador.
