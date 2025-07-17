.section .rodata
msg_comma:    .string ","

.section .data
input_buffer: .zero 8192
number_buffer: .zero 12
input_array:  .space 4000

.section .text
.globl main

# ==============================================================================
# Funções de I/O de Baixo Nível
# ==============================================================================
putchar:
    li   t0, 0x10000000
    sb   a0, 0(t0)
    ret

getchar:
    li   t0, 0x10000005
getchar_wait:
    lb   t1, 0(t0)
    andi t1, t1, 1
    beqz t1, getchar_wait
    li   t0, 0x10000000
    lb   a0, 0(t0)
    ret

# ==============================================================================
# Funções de Alto Nível
# ==============================================================================
print_string:
    addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); mv s0, a0
print_string_loop:
    lb a0, 0(s0); beqz a0, print_string_end; call putchar
    addi s0, s0, 1; j print_string_loop
print_string_end:
    lw ra, 12(sp); lw s0, 8(sp); addi sp, sp, 16; ret

# --- read_line: VERSÃO CORRIGIDA E ROBUSTA ---
# Usa registadores sX para guardar o estado, tornando-a imune à corrupção
# por parte de funções que ela chama (como getchar).
read_line:
    addi sp, sp, -16
    sw   ra, 12(sp)
    sw   s0, 8(sp)   # Salva o valor original de s0
    sw   s1, 4(sp)   # Salva o valor original de s1
    sw   s2, 0(sp)   # Salva o valor original de s2

    mv   s0, a0      # s0 = ponteiro para o buffer (seguro)
    mv   s1, a1      # s1 = tamanho máximo (seguro)
    li   s2, 0       # s2 = contador de caracteres (seguro)
read_line_loop:
    call getchar     # getchar pode usar t0-t6 livremente, s0-s2 estão seguros.
    li   t5, -1
    beq  a0, t5, read_line_end
    li   t3, 10
    beq  a0, t3, read_line_end
    li   t4, 13
    beq  a0, t4, read_line_end
    
    sb   a0, 0(s0)
    addi s0, s0, 1
    addi s2, s2, 1
    
    blt  s2, s1, read_line_loop # Compara com o tamanho máximo
read_line_end:
    sb   zero, 0(s0)
    
    lw   ra, 12(sp) # Restaura os registadores na ordem inversa
    lw   s0, 8(sp)
    lw   s1, 4(sp)
    lw   s2, 0(sp)
    addi sp, sp, 16
    ret

atoi:
    mv   t0, a0; li   t1, 0; li   t2, 1
atoi_skip_whitespace:
    lb   t3, 0(t0); li   t4, 32; bne  t3, t4, atoi_check_sign
    addi t0, t0, 1; j    atoi_skip_whitespace
atoi_check_sign:
    lb   t3, 0(t0); li   t4, 45; bne  t3, t4, atoi_loop
    li   t2, -1; addi t0, t0, 1
atoi_loop:
    lb   t3, 0(t0); li   t4, 48; blt  t3, t4, atoi_end; li   t4, 57; bgt  t3, t4, atoi_end
    addi t3, t3, -48; slli t4, t1, 3; slli t5, t1, 1; add  t1, t4, t5; add  t1, t1, t3
    addi t0, t0, 1; j    atoi_loop
atoi_end:
    li   t3, -1; bne  t2, t3, atoi_positive; neg  t1, t1
atoi_positive:
    mv   a0, t1; mv   a1, t0; ret

itoa:
    addi sp, sp, -48; sw ra, 44(sp); sw s0, 40(sp); sw s1, 36(sp); sw s2, 32(sp); sw s3, 28(sp); sw s4, 24(sp)
    mv s0, a0; mv s1, a1; li s4, 0; addi s2, sp, 23; sb zero, 0(s2); addi s2, s2, -1
    bnez s0, itoa_check_negative; li t0, 48; sb t0, 0(s1); addi t0, s1, 1; sb zero, 0(t0); j itoa_end
itoa_check_negative:
    bge s0, zero, itoa_convert_loop; li s4, 1; neg s0, s0
itoa_convert_loop:
    li t0, 10; rem s3, s0, t0; div s0, s0, t0; addi s3, s3, 48
    sb s3, 0(s2); addi s2, s2, -1; bnez s0, itoa_convert_loop
    li t0, 1; bne s4, t0, itoa_get_start_ptr; li t0, 45; sb t0, 0(s2); addi s2, s2, -1
itoa_get_start_ptr:
    addi s2, s2, 1
itoa_copy_loop:
    lb t0, 0(s2); sb t0, 0(s1); beqz t0, itoa_end; addi s2, s2, 1; addi s1, s1, 1; j itoa_copy_loop
itoa_end:
    lw ra, 44(sp); lw s0, 40(sp); lw s1, 36(sp); lw s2, 32(sp); lw s3, 28(sp); lw s4, 24(sp)
    addi sp, sp, 48; ret

main:
    addi sp, sp, -32; sw ra, 28(sp); sw s0, 24(sp); sw s1, 20(sp); sw s2, 16(sp); sw s3, 12(sp)
    
    la a0, input_buffer; li a1, 8192; call read_line
    la a0, input_buffer; call atoi; mv s0, a0

    la a0, input_buffer; li a1, 8192; call read_line
    
    la s2, input_array; la s3, input_buffer; li s1, 0
main_parse_loop:
    bge s1, s0, main_sort; mv a0, s3; call atoi
    sw a0, 0(s2); mv s3, a1; addi s2, s2, 4; addi s1, s1, 1; j main_parse_loop
main_sort:
    la a0, input_array; mv a1, s0; call bubblesort
main_print_loop:
    la s2, input_array; li s1, 0
main_print_loop_start:
    bge s1, s0, main_end; beqz s1, print_number
    la a0, msg_comma; call print_string
print_number:
    lw a0, 0(s2); la a1, number_buffer; call itoa
    la a0, number_buffer; call print_string
    addi s2, s2, 4; addi s1, s1, 1; j main_print_loop_start
main_end:
    lw ra, 28(sp); lw s0, 24(sp); lw s1, 20(sp); lw s2, 16(sp); lw s3, 12(sp)
    addi sp, sp, 32
    
    li a0, 0 # Define o código de retorno como 0 (sucesso)
    ret  

bubblesort:
    addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); sw s1, 4(sp); sw s2, 0(sp)
    mv t0, a1; li s0, 0
outer_loop:
    addi t1, t0, -1; bge s0, t1, end_bubblesort; mv s2, a0; li s1, 0; sub t3, t1, s0
inner_loop:
    bge s1, t3, end_inner_loop; lw t4, 0(s2); lw t5, 4(s2); ble t4, t5, no_swap
    sw t5, 0(s2); sw t4, 4(s2)
no_swap:
    addi s1, s1, 1; addi s2, s2, 4; j inner_loop
end_inner_loop:
    addi s0, s0, 1; j outer_loop
end_bubblesort:
    lw ra, 12(sp); lw s0, 8(sp); lw s1, 4(sp); lw s2, 0(sp); addi sp, sp, 16; ret
