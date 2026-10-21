# Montador SIC/XE — checkpoint 2

**Responsabilidade do integrante 6:** interface por linha de comando, orquestração e compilação. O simulador do checkpoint 1 permanece separado e inalterado.

## Compilar e executar

Execute os comandos **dentro de `checkpoint_2`** (Linux, WSL, Git Bash com Make e g++ instalados):

```bash
make clean && make
./assembler examples/valid.asm
./assembler examples/valid.asm -o examples/saida.obj
./assembler examples/valid.asm --load
make run INPUT=examples/valid.asm
make test
make clean
```

**Atenção:** em 20/10/2026, o ZIP fornecido ainda não tinha os módulos `PassOne`, `PassTwo`, `ObjectProgram` e `SimulatorBridge`. A compilação final e a montagem funcional dependem da integração dos commits dos integrantes responsáveis. Também aguardamos `examples/valid.asm` e `tests/test_assembler.cpp` do integrante 7. `make test` falha explicitamente se nenhum teste existir, em vez de apresentar falso sucesso.

## Interface

```text
./assembler <entrada.asm> [-o saida.obj] [--load]
```

- Entrada obrigatória: um arquivo-fonte `.asm`.
- `-o`: define o arquivo objeto. Por padrão, usa o mesmo caminho do fonte com extensão `.obj`.
- `--load`: opcionalmente carrega o programa objeto em uma instância de `Machine`; não inicia a interface visual.
- Retorno `0`: montagem bem-sucedida; `1`: erro de leitura, montagem, gravação ou carga; `2`: argumentos inválidos.
- Diagnósticos têm formato `arquivo:linha: mensagem` e vão para `stderr`.
- Se o parser ou alguma passagem encontrar erros, nenhum arquivo objeto é sobrescrito.
- O executável é `assembler` e os intermediários de compilação ficam em `build/`, exclusivos da segunda etapa.

## Fluxo e responsabilidades

```text
arquivo.asm
    | SourceParser::parseFile (integrante 2)
    v
ParsedLine[]
    | runPassOne (integrante 3)
    v
enderecos + SymbolTable
    | runPassTwo (integrante 4)
    v
blocos de bytes + relocacoes
    | buildObjectProgram (integrante 5)
    v
arquivo.obj  --[--load/SimulatorBridge]--> Machine (simulador)
```

O `main.cpp` apenas chama interfaces públicas. O Makefile identifica automaticamente novos arquivos em `src/`, sem modificar códigos de terceiros. O alvo de teste compila os testes em `tests/` com os módulos de produção, mas **não** vincula `main.cpp` para evitar dois pontos de entrada.

## Contratos a confirmar antes da integração final

O ZIP analisado continha somente o parser e os tipos básicos, portanto as assinaturas de alguns módulos ainda não estavam disponíveis. Para permitir trabalho paralelo, o `main.cpp` foi preparado usando as propostas abaixo. **Os responsáveis pelos módulos 3, 4 e 5 devem confirmar ou fornecer os nomes definitivos das APIs; somente os pontos de integração do `main.cpp` devem ser ajustados, sem alterar os arquivos deles.**

1. Integrante 3: `PassOneResult runPassOne(const std::vector<ParsedLine>&)` com `startAddress`, `programLength`, `symbols`, `diagnostics`.
2. Integrante 4: `PassTwoResult runPassTwo(const PassOneResult&)` com `diagnostics` e dados necessários ao objeto.
3. Integrante 5: `ObjectProgram buildObjectProgram(const PassOneResult&, const PassTwoResult&)` com `toText()`; construtor `SimulatorBridge(const ObjectProgram&)`; `loadInto(Machine&)` retornando algo compatível com `LoadResult {ok, error}`.
4. Integrante 7: `tests/test_assembler.cpp` com `int main()` e `examples/valid.asm`.

## Fala breve para o vídeo (integrante 6)

> Minha parte é a integração do montador e sua interface no terminal. O executável `assembler` recebe um arquivo Assembly, permite escolher o nome da saída e aciona, em ordem, o parser, a primeira passagem, a segunda passagem e a geração dos registros objeto. Se algum módulo informar um erro, a CLI mostra `arquivo:linha: mensagem`, retorna código 1 e não sobrescreve o arquivo objeto anterior. Configurei também um Makefile separado com compilação C++17, comandos de testes e a opção `--load`, que carrega no simulador sem abrir a interface do primeiro checkpoint.

Para a gravação, mostrar o terminal compilando, um exemplo válido, um exemplo com erro e entregar a fala seguinte ao integrante 7, responsável pelos testes. A fala acima deve ser adaptada às funcionalidades **efetivamente demonstradas** após a integração.
