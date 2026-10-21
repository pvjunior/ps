# Checkpoint 2 — montador SIC/XE (integrante 6)

**Responsabilidade:** `main.cpp` (CLI e orquestração), `Makefile` (compilação isolada), `README.md` (uso e demonstração). Nenhum arquivo do checkpoint 1 ou dos demais integrantes foi modificado nesta contribuição.

## Estado de integração — 21/10/2026

Foi conferido o bundle Git com `origin/main` no commit `e264d9a`:

- **Integrante 1 (Pablo):** `AssemblerTypes` e `OpcodeTable` presentes.
- **Integrante 2 (Vinícius):** `parseFile()` presente.
- **Integrante 3 (Samuel):** `runPassOne(const std::vector<ParsedLine>&)` presente e integrado à `main`.
- **Integrante 4 (Nickolas):** `runPassTwo(const PassOneResult&)` e `Encoder` presentes na `main`.
- **Integrante 5 (Dionatahan):** `ObjectProgram.*` e `SimulatorBridge.*` **ainda não constavam do bundle**, portanto os dois pontos de integração dependem dos headers finais dele.
- **Integrante 7 (Pedro Paulo):** exemplos e testes **ainda não constavam do bundle**.

Os nomes de funções e tipos das passagens 1 e 2 estão confirmados. Não é necessário editá-las nem alterá-las para adequação à CLI.

## Compilar (MSYS2 UCRT64, Linux ou similar)

Dentro de `checkpoint_2`:

```bash
make clean
make
```

O projeto usa C++17, `-Wall -Wextra -Wpedantic`; os `.o` ficam em `build/`. O executável será `assembler` (ou `assembler.exe` no Windows). A compilação usa os arquivos `.cpp` colocados na pasta `src/` e os componentes essenciais do simulador, mantendo o build do checkpoint 1 isolado. Enquanto os quatro arquivos da parte 5 estiverem ausentes, `make` informa claramente essa pendência; isso não é falha do GCC.

## Executar a CLI

```bash
./assembler examples/valid.asm
./assembler examples/valid.asm -o examples/saida.obj
./assembler examples/valid.asm --load
make run INPUT=examples/valid.asm
make test
make clean
```

**Sintaxe:** `./assembler <entrada.asm> [-o saida.obj] [--load]`

- O `.asm` é obrigatório; opções inválidas imprimem o uso e retornam código **2**.
- O destino padrão é o mesmo nome e diretório do `.asm`, com extensão `.obj`.
- A CLI percorre `parseFile` → `runPassOne` → `runPassTwo` → `buildObjectProgram`.
- Qualquer diagnóstico é exibido em `arquivo:linha: mensagem` no `stderr` e causa saída **1**.
- Somente se todas as etapas estiverem corretas o objeto é gravado. O texto é escrito em `.tmp` e renomeado depois da escrita, protegendo o arquivo anterior quando houver erro de análise/montagem.
- `--load` instancia `Machine` e usa `SimulatorBridge` para carregar bytes sem iniciar a interface gráfica (TUI).
- Saída **0** significa que a operação solicitada terminou com sucesso.

O comando `make test` compila testes C++ de `tests/*.cpp` quando existirem; se não houver nenhum, exibe uma pendência explícita, evitando falso resultado positivo.

## Contrato público e fluxo

```text
arquivo.asm
  → parseFile(...)                 [integrante 2]
  → runPassOne(parsed.lines)       [integrante 3]
  → runPassTwo(first)              [integrante 4]
  → buildObjectProgram(first, second) [integrante 5: pendente no bundle]
  → arquivo.obj
  → [--load] SimulatorBridge / Machine [integrante 5: pendente no bundle]
```

**PassTwoResult** já fornece `programName`, `startAddress`, `programLength`, `executionStart`, `blocks`, `modifications` e `diagnostics`. Isso é o contrato disponível para a geração dos registros H/T/M/E pelo integrante 5.

### Verificação de integração

Em laboratório separado (não incluído na contribuição Git), foram combinados os módulos reais 1–4 com uma implementação provisória **apenas** dos módulos ausentes 5. O exemplo com `LDA #3`, `CLEAR X`, `+JSUB`, `STA BUFFER,X`, `BYTE`, `RESW`, `RSUB` e `END` compilou sem warnings, gerou registros H/T/M/E e permitiu `--load`. Esse teste não substitui a verificação com o código final do integrante 5.

## Demonstração sugerida para o vídeo

1. Entrar em `checkpoint_2`, mostrar `make clean && make`.
2. Executar `./assembler examples/valid.asm` e mostrar o resumo e o `.obj`.
3. Explicar parser → passagem 1 → passagem 2 → programa objeto.
4. Executar um `.asm` inválido, mostrar `arquivo:linha: mensagem` e verificar que o objeto anterior não foi sobrescrito.
5. Opcionalmente executar `--load` se a ponte do integrante 5 estiver integrada.

Use exemplos e comandos que de fato funcionem com a versão definitiva do grupo. O foco da fala é a **integração/CLI/build**, sem tomar para si a explicação das técnicas de codificação, que pertencem aos demais integrantes.
