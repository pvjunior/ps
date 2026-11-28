ERRMAC  START   1000
. Exemplo com erros detectados pelo processador de macros (nao deve gerar saida nem montar)
. 1. MCEND sem MCDEFN correspondente
        MCEND
. 2. Definicao valida para testar erros de argumentos na chamada
        MCDEFN
PRECISA &VAL
        LDA     &VAL
        MCEND
        PRECISA                     . Erro: argumento ausente
        PRECISA #1,#2               . Erro: excesso de argumentos posicionais
        PRECISA OUTRO=#3            . Erro: argumento nomeado desconhecido
. 3. Ciclo mutuo de chamadas entre macros
        MCDEFN
CICLOA
        CICLOB
        MCEND
        MCDEFN
CICLOB
        CICLOA
        MCEND
        CICLOA                      . Erro: chamada ciclica CICLOA -> CICLOB -> CICLOA
. 4. Definicao de macro sem MCEND correspondente (ao final do arquivo)
        MCDEFN
ABERTA  &X
        LDA     &X
        END     ERRMAC
