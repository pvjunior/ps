NSTDEF  START   1000
. Exemplo de definicao de macro dentro de outra macro com pilha de formais e .SER
        MCDEFN
FABRICA &NOME,&OP,&X
        LDA     &X                  . Usa &X do nivel 1 (definicao externa)
        MCDEFN
&NOME   &X,&DEST
        &OP     &X                  . &OP do nivel 1 e &X coincidente do nivel 2
        COMP    #0
        JEQ     IGN.SER             . .SER preservado ate a expansao da macro interna
        STA     &DEST
IGN.SER STA     ULTIMO              . Rotulo unico a cada chamada da macro interna
        MCEND
        STA     &X                  . Volta a usar &X do nivel 1 apos definicao interna
        MCEND
. Chamadas que geram novas definicoes e depois expandem as macros geradas
FIRST   CLEAR   A
        FABRICA SOMA_M,ADD,VALOR1   . Expansao 1: gera SOMA_M e opera em VALOR1
        FABRICA SUB_M,SUB,VALOR2    . Expansao 2: gera SUB_M e opera em VALOR2
        SOMA_M  #10,SAIDA1          . Expansao 3: chama macro definida dinamicamente
        SOMA_M  #20,SAIDA1          . Expansao 4: segundo uso com .SER distinto
        SUB_M   #5,SAIDA2           . Expansao 5: segunda macro definida dinamicamente
HALT    J       HALT
VALOR1  WORD    100
VALOR2  WORD    200
SAIDA1  RESW    1
SAIDA2  RESW    1
ULTIMO  RESW    1
        END     FIRST
