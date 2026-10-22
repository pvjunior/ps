PROG    START   1000
. Inicializacao e operacoes imediatas
FIRST   CLEAR   X                   . Formato 2: limpa registrador X
        CLEAR   A                   . Formato 2: limpa acumulador A
        LDA     #5                  . Formato 3: carga imediata (#5)
        ADD     #10                 . Formato 3: soma imediata (#10)
        STA     TOTAL               . Formato 3: guarda resultado (PC-relative)
. Modos indireto e indexado
        LDA     @PTR                . Formato 3: indireto (@PTR)
        LDCH    BUFFER,X            . Formato 3: indexado (BUFFER,X)
        STCH    DEST,X              . Formato 3: armazena caractere (DEST,X)
. Chamada de sub-rotina em formato 4 (gera registro M)
        +JSUB   SUBR                . Formato 4 estendido (+JSUB)
. Laco de parada limpo (J *)
HALT    J       HALT                . Desvio incondicional para si mesmo
. --------------------------------------------------------------------
. Sub-rotina SUBR
. --------------------------------------------------------------------
SUBR    RMO     A,S                 . Formato 2: copia A para S
        ADDR    S,A                 . Formato 2: soma S em A
        RSUB                        . Formato 3: retorno de sub-rotina
. --------------------------------------------------------------------
. Area de constantes e variaveis
. --------------------------------------------------------------------
PTR     WORD    1000                . Constante numerica (endereco/valor)
TOTAL   RESW    1                   . Reserva 1 palavra (3 bytes) - causa quebra em T
DEST    RESB    4                   . Reserva 4 bytes para buffer de destino
BUFFER  BYTE    C'EOF'              . Constante caractere (3 bytes: 45 4F 46)
HEXVAL  BYTE    X'F1'               . Constante hexadecimal (1 byte: F1)
COUNT   WORD    42                  . Constante inteira de 3 bytes (00 00 2A)
        END     FIRST               . Ponto de entrada especificado em FIRST
