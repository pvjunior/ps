# Projeto SIC/XE

Projeto desenvolvido para implementar um simulador, um montador de duas
passagens e um processador de macros para a arquitetura SIC/XE.

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

## Checkpoint 3 — Processador de macros e macro-montador SIC/XE

Nesta etapa, a divisão de responsabilidades segue as partes abaixo. Na versão
atual do repositório, estão presentes o parser de macros e a estrutura de CLI e
build; os demais módulos e a validação integrada ainda estão pendentes.

### Parte 1 — Contratos e tipos compartilhados

**Responsável:** Pablo Vicondo

Esta parte contempla a definição dos tipos compartilhados do processador de
macros, incluindo linhas de fonte, localização, diagnósticos, parâmetros,
definições e resultados de expansão. Os contratos devem permitir a integração
dos módulos e representar parâmetros posicionais, nomeados e valores padrão.

### Parte 2 — Leitura e análise das linhas de macro

**Responsável:** Vinicius Lameirão

Foi implementado o parser de macros, responsável por ler arquivos ou textos e
separar rótulo, operação, operandos e comentário, preservando o texto original e
a localização de cada linha. O parser reconhece os delimitadores `MCDEFN` e
`MCEND`, preserva parâmetros, constantes e referências como `.SER`, além de
gerar diagnósticos para aspas não fechadas sem interromper a leitura.

### Parte 3 — Tabela de macros e associação de argumentos

**Responsável:** Samuel Bottermund Flores

Esta parte contempla o armazenamento e a busca de definições de macros, com
suporte a redefinições encadeadas, e a associação de argumentos posicionais e
nomeados aos parâmetros formais. Também abrange valores padrão, validação de
argumentos, substituição de parâmetros por nível e posição e tratamento de
`.SER` para gerar rótulos únicos.

### Parte 4 — Motor de expansão em uma passagem

**Responsável:** Nickolas Werhli Barbosa

Esta parte contempla o processamento do fonte em uma única passagem,
registrando definições e expandindo chamadas conforme aparecem. O motor deve
controlar definições e chamadas aninhadas, os níveis dos parâmetros formais e
os números seriais das expansões, além de diagnosticar delimitadores
incorretos, definições incompletas e ciclos de chamadas.

### Parte 5 — Saída e integração com o montador

**Responsável:** Dionathan Liermann Kringel

Esta parte contempla a geração do arquivo expandido `MASMAPRG.ASM` e a fachada
que integra o processador de macros ao montador do Checkpoint 2. A integração
deve executar a montagem somente após a expansão e a gravação bem-sucedidas,
propagar os diagnósticos de ambas as etapas e preservar saídas válidas em caso
de erro.

### Parte 6 — Módulo principal, CLI e build

**Responsável:** Gabriel M. Meireles

Foram implementados o módulo principal, a interface de linha de comando e o
Makefile do executável `macro_assembler`. A CLI valida argumentos e caminhos de
entrada e saída, prevê a chamada à fachada de integração, exibe diagnósticos e
um resumo das expansões. O build reutiliza o montador do Checkpoint 2 e depende
da entrega dos demais módulos para compilar e executar o fluxo completo.

### Parte 7 — Testes e exemplos

**Responsável:** Pedro Paulo Moreira

Esta parte contempla os testes e exemplos do processador de macros, cobrindo
parser, tabela, argumentos, expansão simples, aninhamento, redefinições e
rótulos únicos com `.SER`. Também abrange os casos de erro, a preservação das
saídas anteriores e a validação do fluxo completo entre fonte com macros,
arquivo expandido e programa objeto SIC/XE.
