# Projeto SIC/XE

Projeto desenvolvido para implementar um simulador e um montador de duas
passagens para a arquitetura SIC/XE.

## Checkpoint 1 — Simulador SIC/XE

### Parte 1 — Introdução e arquitetura

**Responsável:** Pablo Vicondo

Foi criada a estrutura inicial do simulador e o ponto de entrada do programa.
Esta parte organiza a integração geral entre memória, registradores, carregador,
CPU, máquina e interface de terminal.

### Parte 2 — Memória e registradores

**Responsável:** Vinicius Lameirão

Foram implementados os componentes responsáveis por armazenar os bytes na
memória do simulador e representar os registradores da arquitetura SIC/XE,
incluindo as operações de leitura, escrita e validação de acesso.

### Parte 3 — Loader

**Responsável:** Samuel Bottermund Flores

Foi implementado o carregador de bytecode, responsável por interpretar os bytes
de entrada e gravá-los na memória nos endereços apropriados para execução.

### Parte 4 — CPU: busca e decodificação

**Responsável:** Nickolas Werhli Barbosa

Foi implementada a etapa de busca e decodificação da CPU, incluindo a leitura
da instrução no endereço indicado pelo contador de programa, a identificação do
formato e a extração dos campos de opcode, registradores e flags `nixbpe`.

### Parte 5 — CPU: execução das instruções

**Responsável:** Dionathan Liermann Kringel

Foi implementada a execução do conjunto de instruções do simulador, incluindo
operações de carga, armazenamento, aritmética, comparação e salto, com suas
alterações correspondentes na memória, nos registradores e no contador de
programa.

### Parte 6 — Machine

**Responsável:** Gabriel M. Meireles

Foi implementada a classe que integra memória, registradores, Loader e CPU em
uma única fachada, facilitando a inicialização, o carregamento e a execução dos
programas no simulador.

### Parte 7 — Interface de terminal

**Responsável:** Pedro Paulo Moreira

Foi implementada a interface de terminal usada para interagir com o simulador,
carregar programas, executar instruções e visualizar o estado da memória e dos
registradores.

## Checkpoint 2 — Montador SIC/XE

### Parte 1 — Contratos, tipos compartilhados e opcodes

**Responsável:** Pablo Vicondo

Foram definidos os tipos compartilhados entre os módulos do montador, incluindo
linhas analisadas, diagnósticos, linhas intermediárias e tabela de símbolos.
Também foram implementadas a normalização de identificadores e a tabela de
opcodes com os formatos aceitos por cada instrução.

### Parte 2 — Leitura e análise léxica

**Responsável:** Vinicius Lameirão

Foi implementado o parser do código-fonte assembly, responsável por separar
rótulo, operação, operando e comentário. O parser preserva operandos imediatos,
indiretos, indexados e constantes, além de produzir diagnósticos sem interromper
a análise das demais linhas.

### Parte 3 — Primeira passagem

**Responsável:** Samuel Bottermund Flores

Foi implementada a primeira passagem do montador, responsável por calcular os
endereços das linhas, atualizar o contador de localização, construir a tabela de
símbolos e validar diretivas, instruções, rótulos e operandos.

### Parte 4 — Segunda passagem e codificação

**Responsável:** Nickolas Werhli Barbosa

Foi implementada a segunda passagem do montador e a codificação das instruções
nos formatos 1, 2, 3 e 4. Esta parte resolve símbolos, modos simples, imediatos,
indiretos e indexados, empacota os bits `nixbpe` e calcula endereçamento relativo
ao PC, relativo à base ou estendido.

### Parte 5 — Programa objeto e integração

**Responsável:** Dionathan Liermann Kringel

Foi implementada a geração do programa objeto nos registros `H`, `T`, `M` e
`E`, incluindo a divisão dos registros de texto por tamanho e lacunas. Também
foi criada a ponte que carrega no simulador os bytes produzidos pelo montador.

### Parte 6 — CLI e build

**Responsável:** Gabriel M. Meireles

Foram implementados o executável do montador e o processo de build do
Checkpoint 2. A interface de linha de comando integra parser, primeira passagem,
segunda passagem, geração do arquivo objeto, exibição de diagnósticos e
carregamento opcional no simulador.

### Parte 7 — Testes e exemplos

**Responsável:** Pedro Paulo Moreira

Foram criados os testes e exemplos que verificam o parser, as duas passagens, os
formatos e modos de endereçamento, a geração dos registros objeto e a integração
entre o montador e o simulador.
