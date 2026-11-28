SIMPLE  START   1000
. Exemplo simples de macros: posicionais, nomeados, padrao e .SER
        MCDEFN
SOMAR   &DEST,&ORIG,&INC=#1
        LDA     &ORIG               . Carrega valor de origem
        ADD     &INC                . Soma incremento (&INC ou padrao #1)
        STA     &DEST               . Armazena resultado em &DEST
        MCEND
        MCDEFN
PULAR   &ALVO
        COMP    #0                  . Compara acumulador com zero
        JEQ     FIM.SER             . Desvio usando rotulo unico por expansao
        LDA     &ALVO               . Carrega operando se diferente de zero
FIM.SER STA     RESULT              . Rotulo unico gerado a partir de .SER
        MCEND
. Fluxo principal com linhas assembly comuns preservadas
FIRST   CLEAR   A                   . Limpa acumulador antes das macros
        SOMAR   TOTAL,BASE          . Chamada posicional usando valor padrao #1
        SOMAR   INC=#5,DEST=TOTAL,ORIG=TOTAL . Argumentos nomeados fora de ordem
        PULAR   TOTAL               . Primeira chamada com .SER
        PULAR   BASE                . Segunda chamada com .SER distinto
        +JSUB   ROTINA              . Formato 4 preservado (gera registro M)
HALT    J       HALT
. Sub-rotina assembly sem macros
ROTINA  RMO     A,S
        ADDR    S,A
        RSUB
. Area de constantes e variaveis
BASE    WORD    10
TOTAL   RESW    1
RESULT  RESW    1
MSG     BYTE    C'OK'
        END     FIRST
