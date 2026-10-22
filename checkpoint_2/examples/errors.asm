ERRPROG START   1000
. 1. Erro de sintaxe: registrador inexistente em instrucao de Formato 2
        CLEAR   REG_INEXISTENTE

. 2. Erro semantico: simbolo nao declarado no escopo
        LDA     SIMBOLO_NAO_DECLARADO

. 3. Erro de combinacao: indexacao (,X) nao pode ser combinada com imediato (#)
        STA     #100,X

. 4. Erro de Formato 2: ADDR requer dois registradores
        ADDR    A

. 5. Erro da Passagem 1: opcode inexistente na tabela
        INVALID_OPCODE #42

. 6. Erro de definicao: simbolo duplicado (primeira declaracao)
DUPLICADO LDA   #1

. 7. Erro de diretiva: BYTE X'...' com quantidade impar de digitos hexadecimais
ERR_BYTE BYTE   X'ABC'

. 8. Erro de sintaxe lexica: aspas simples sem fechamento
ERR_QUOTE BYTE  C'ASPAS_SEM_FECHAR

. 9. Erro da Passagem 1: simbolo duplicado (segunda declaracao com mesmo nome)
DUPLICADO RESW  1

. Fim do arquivo
        END     ERRPROG
