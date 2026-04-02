.section .rodata
    msg_comma:      .string ","
    msg_newline:    .string "\n"

.section .data
    input_array:    .space 4000 # Espaço para até 1000 inteiros de 32 bits

.equ UART_BASE, 0x10000000 # Endereço base da UART
.equ UART_RX, 0            # Offset para data register (leitura)
.equ UART_TX, 0            # Offset para data register (escrita)
.equ UART_STATUS, 5        # Offset para status register

.section .text
.globl main

#-------------------------------------------------------------------------------
# main: Função principal
#-------------------------------------------------------------------------------
main:
    # Prólogo
    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   s0, 8(sp)          # s0 = N (número de elementos)
    sw   s1, 4(sp)          # s1 = ponteiro para input_array
    sw   s2, 0(sp)          # s2 = contador de loop

    # Lê o primeiro número (N) da UART
    li   a0, UART_BASE
    call read_one_number
    mv   s0, a0

    # Valida N (deve estar entre 0 e 1000)
    li   t0, 1000
    blez s0, set_n_zero
    bgt  s0, t0, set_n_1000
    j    read_start

set_n_zero:
    li   s0, 0
    j    read_start
set_n_1000:
    li   s0, 1000

read_start:
    la   s1, input_array    # s1 = endereço de input_array
    li   s2, 0              # Inicia o contador do loop

# Loop para ler N números do array
read_loop:
    bge  s2, s0, read_loop_done
    li   a0, UART_BASE
    call read_one_number
    slli t1, s2, 2
    add  t1, s1, t1
    sw   a0, 0(t1)          # input_array[contador] = número lido
    addi s2, s2, 1
    j    read_loop

read_loop_done:
    # Chama Insertion Sort
    mv   a0, s1             # endereço do array
    mv   a1, s0             # tamanho do array (N)
    call sort

    # Imprime vetor ordenado
    li   a0, UART_BASE
    mv   a1, s1
    mv   a2, s0
    call imprime_vetor

    # Epílogo
    lw   ra, 12(sp)
    lw   s0, 8(sp)
    lw   s1, 4(sp)
    lw   s2, 0(sp)
    addi sp, sp, 16
    li   a0, 0              # Retorno de sucesso
    ret

#-------------------------------------------------------------------------------
# sort: Insertion Sort otimizado
# a0: endereço do array, a1: tamanho do array
#-------------------------------------------------------------------------------
sort:
    addi sp, sp, -20
    sw   ra, 16(sp)
    sw   s0, 12(sp) # Array
    sw   s1, 8(sp)  # n
    sw   s2, 4(sp)  # i
    sw   s3, 0(sp)  # key
    
    mv   s0, a0
    mv   s1, a1
    li   s2, 1      # i = 1

outer_loop:
    bge  s2, s1, sort_done
    slli t0, s2, 2
    add  t0, s0, t0
    lw   s3, 0(t0)  # key = arr[i]
    addi t1, s2, -1 # j = i - 1

inner_loop:
    blt  t1, zero, end_inner
    slli t2, t1, 2
    add  t2, s0, t2
    lw   t3, 0(t2)  # arr[j]
    ble  t3, s3, end_inner
    sw   t3, 4(t2)  # arr[j+1] = arr[j]
    addi t1, t1, -1 # j--
    j    inner_loop

end_inner:
    # Insere key na posição correta
    addi t1, t1, 1
    slli t2, t1, 2
    add  t2, s0, t2
    sw   s3, 0(t2)
    addi s2, s2, 1 # i++
    j    outer_loop

sort_done:
    lw   s3, 0(sp)
    lw   s2, 4(sp)
    lw   s1, 8(sp)
    lw   s0, 12(sp)
    lw   ra, 16(sp)
    addi sp, sp, 20
    ret

#-------------------------------------------------------------------------------
# read_one_number: Leitura otimizada com tratamento de sinal
# a0: endereço da UART
# Retorna em a0 o número lido
#-------------------------------------------------------------------------------
read_one_number:
    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   s0, 8(sp)  # UART base
    sw   s1, 4(sp)  # Número acumulado
    sw   s2, 0(sp)  # Flag negativo
    mv   s0, a0
    li   s1, 0
    li   s2, 0      # 0 = positivo, 1 = negativo

skip_whitespace:
    # Verifica se há dado disponível (bit 0 do status)
    lb   t0, UART_STATUS(s0)
    andi t0, t0, 1
    beqz t0, skip_whitespace
    
    # Lê caractere
    lb   t1, UART_RX(s0)
    li   t0, ' '
    beq  t1, t0, skip_whitespace
    li   t0, '\n'
    beq  t1, t0, skip_whitespace
    li   t0, '\t'
    beq  t1, t0, skip_whitespace
    li   t0, ','           # Trata vírgula como whitespace
    beq  t1, t0, skip_whitespace
    
    # Verifica sinal negativo
    li   t0, '-'
    bne  t1, t0, check_digit
    li   s2, 1
    j    read_next

check_digit:
    # Verifica se é dígito válido
    li   t0, '0'
    blt  t1, t0, number_done
    li   t0, '9'
    bgt  t1, t0, number_done
    addi t1, t1, -48  # char para int
    mv   s1, t1
    j    read_next

read_next:
    # *** CORREÇÃO: Remove verificação de status aqui (causa perda de dados) ***
    # Move a verificação de status para DENTRO do loop, antes de ler cada byte
    lb   t0, UART_STATUS(s0)
    andi t0, t0, 1
    beqz t0, number_done   # Se não há dado, termina
    
    lb   t1, UART_RX(s0)   # Lê caractere
    li   t0, '0'
    blt  t1, t0, number_done
    li   t0, '9'
    bgt  t1, t0, number_done
    
    # Acumula dígito
    addi t1, t1, -48
    li   t0, 10
    mul  s1, s1, t0
    add  s1, s1, t1
    j    read_next         

number_done:
    # Aplica sinal negativo se necessário
    beqz s2, positive
    neg  s1, s1

positive:
    mv   a0, s1
    lw   ra, 12(sp)
    lw   s0, 8(sp)
    lw   s1, 4(sp)
    lw   s2, 0(sp)
    addi sp, sp, 16
    ret

#-------------------------------------------------------------------------------
# imprime_vetor: Imprime vetor formatado
# a0: UART base, a1: array, a2: tamanho
#-------------------------------------------------------------------------------
imprime_vetor:
    addi sp, sp, -20
    sw   ra, 16(sp)
    sw   s0, 12(sp) # UART
    sw   s1, 8(sp)  # Array
    sw   s2, 4(sp)  # Tamanho
    sw   s3, 0(sp)  # Contador
    mv   s0, a0
    mv   s1, a1
    mv   s2, a2
    li   s3, 0

print_loop:
    bge  s3, s2, print_end
    lw   a1, 0(s1)
    mv   a0, s0
    call imprime_numero
    
    # Imprime vírgula se não for último elemento
    addi t0, s3, 1
    bge  t0, s2, skip_comma
    la   a1, msg_comma
    mv   a0, s0
    call imprime_string

skip_comma:
    addi s1, s1, 4
    addi s3, s3, 1
    j    print_loop

print_end:
    la   a1, msg_newline
    mv   a0, s0
    call imprime_string
    lw   ra, 16(sp)
    lw   s0, 12(sp)
    lw   s1, 8(sp)
    lw   s2, 4(sp)
    lw   s3, 0(sp)
    addi sp, sp, 20
    ret

#-------------------------------------------------------------------------------
# imprime_string: Imprime string via UART
# a0: UART base, a1: endereço da string
#-------------------------------------------------------------------------------
imprime_string:
    addi sp, sp, -12
    sw   ra, 8(sp)
    sw   s0, 4(sp) # UART
    sw   s1, 0(sp) # String
    mv   s0, a0
    mv   s1, a1

next_char:
    lb   t0, 0(s1)
    beqz t0, done_print
    mv   a0, s0
    mv   a1, t0
    call write_uart_char
    addi s1, s1, 1
    j    next_char

done_print:
    lw   ra, 8(sp)
    lw   s0, 4(sp)
    lw   s1, 0(sp)
    addi sp, sp, 12
    ret

#-------------------------------------------------------------------------------
# imprime_numero: Impressão otimizada
# a0: UART base, a1: número
#-------------------------------------------------------------------------------
imprime_numero:
    addi sp, sp, -48      # Alocar mais espaço (ex: 48 bytes)
    sw   ra, 44(sp)
    sw   s0, 40(sp)
    sw   s1, 36(sp)
    sw   s2, 32(sp)      # Salva s2 em um local seguro
    sw   s3, 28(sp)      # Salva s3 em um local seguro
    
    # O buffer pode usar o espaço de sp+0 a sp+27 com segurança
    addi s3, sp, 8  # Buffer de 20 bytes (sp+8 a sp+27)
    
    mv   s0, a0
    mv   s1, a1
    li   s2, 0
    
    # Trata zero explicitamente
    bnez s1, non_zero
    li   a1, '0'
    call write_uart_char
    j    exit_num_print

non_zero:
    # Trata sinal negativo
    bgez s1, positive_num
    li   a1, '-'
    call write_uart_char
    neg  s1, s1

positive_num:
    # Extrai dígitos (armazena na pilha em ordem inversa)
    li   t0, 10
extract_loop:
    rem  t1, s1, t0       # t1 = dígito
    div  s1, s1, t0       # s1 /= 10
    addi t1, t1, 48       # to ASCII
    sb   t1, 0(s3)
    addi s3, s3, 1
    addi s2, s2, 1
    bnez s1, extract_loop

    # Imprime dígitos na ordem correta
    addi s3, s3, -1
print_digits:
    lb   a1, 0(s3)
    mv   a0, s0
    call write_uart_char
    addi s3, s3, -1
    addi s2, s2, -1
    bgtz s2, print_digits

exit_num_print:
    lw   ra, 44(sp)
    lw   s0, 40(sp)
    lw   s1, 36(sp)
    lw   s2, 32(sp)
    lw   s3, 28(sp)
    addi sp, sp, 48
    ret

#-------------------------------------------------------------------------------
# write_uart_char: Correção crítica para evitar loop infinito
# a0: UART base, a1: caractere
#-------------------------------------------------------------------------------
write_uart_char:
    # CORREÇÃO: Usa offset correto para status register (5)
    addi t2, a0, UART_STATUS # Calcula endereço do status
    
uart_tx_wait:
    lb   t0, 0(t2)          # Lê registrador de status
    andi t0, t0, 0x20       # Isola bit THRE (5)
    beqz t0, uart_tx_wait   # Espera até estar pronto
    
    sb   a1, UART_TX(a0)    # Escreve caractere
    ret
