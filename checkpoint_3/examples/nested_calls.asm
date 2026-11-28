NSTCAL  START   1000
. Exemplo de chamadas de macros aninhadas em 3 niveis, .SER e redefinicao
        MCDEFN
CARREGA &ORIG
        LDA     &ORIG               . Versao 1: carga direta
        MCEND
        MCDEFN
ACUMULA &ORIG,&DELTA
        CARREGA &ORIG               . Chamada aninhada nivel 2 -> nivel 3
        ADD     &DELTA
        COMP    #0
        JEQ     ZERO.SER
        ADD     #1
ZERO.SER STA    TEMP
        MCEND
        MCDEFN
CALCULA &A,&B,&DEST
        ACUMULA &A,#2               . Chamada aninhada nivel 1 -> nivel 2
        ACUMULA &B,#4               . Segunda chamada interna na mesma expansao
        LDA     TEMP
        STA     &DEST
        MCEND
. Primeira chamada com rotulo propagado ate a instrucao mais interna
FIRST   CALCULA NUM1,NUM2,RES1
. Redefinicao encadeada da macro base com corpo maior na mesma passagem
        MCDEFN
CARREGA &ORIG
        CLEAR   A
        ADD     &ORIG               . Versao 2: limpa A antes de somar &ORIG
        MCEND
        CALCULA NUM1,NUM2,RES2      . Nova chamada reflete a versao ativa de CARREGA
HALT    J       HALT
NUM1    WORD    5
NUM2    WORD    7
TEMP    RESW    1
RES1    RESW    1
RES2    RESW    1
        END     FIRST
