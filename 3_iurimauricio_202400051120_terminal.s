# ==============================================================================
# iurimauricio_202400051120_terminal.s - Versão Final Altamente Otimizada
# A função 'partition' foi reescrita para usar aritmética de ponteiros.
# ==============================================================================

.section .rodata
msg_comma:    .string ","

.section .data
input_buffer: .zero 8192
number_buffer: .zero 12
input_array:  .space 4000

.section .text
.globl main

# ==============================================================================
# Funções de I/O e de Apoio (INTACTAS)
# ==============================================================================
putchar:
    li   t0, 0x10000000; sb a0, 0(t0); ret
getchar:
    li   t0, 0x10000005
getchar_wait:
    lb   t1, 0(t0); andi t1, t1, 1; beqz t1, getchar_wait
    li   t0, 0x10000000; lb a0, 0(t0); ret
print_string:
    addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); mv s0, a0
print_string_loop:
    lb a0, 0(s0); beqz a0, print_string_end; call putchar
    addi s0, s0, 1; j print_string_loop
print_string_end:
    lw ra, 12(sp); lw s0, 8(sp); addi sp, sp, 16; ret
read_line:
    addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); sw s1, 4(sp); sw s2, 0(sp)
    mv s0, a0; mv s1, a1; li s2, 0
read_line_loop:
    call getchar; li t5, -1; beq a0, t5, read_line_end
    li t3, 10; beq a0, t3, read_line_end
    li t4, 13; beq a0, t4, read_line_end
    sb a0, 0(s0); addi s0, s0, 1; addi s2, s2, 1
    blt s2, s1, read_line_loop
read_line_end:
    sb zero, 0(s0); lw ra, 12(sp); lw s0, 8(sp); lw s1, 4(sp); lw s2, 0(sp)
    addi sp, sp, 16; ret
atoi:
    mv t0, a0; li t1, 0; li t2, 1
atoi_skip_whitespace:
    lb t3, 0(t0); li t4, 32; bne t3, t4, atoi_check_sign
    addi t0, t0, 1; j atoi_skip_whitespace
atoi_check_sign:
    lb t3, 0(t0); li t4, 45; bne t3, t4, atoi_loop
    li t2, -1; addi t0, t0, 1
atoi_loop:
    lb t3, 0(t0); li t4, 48; blt t3, t4, atoi_end; li t4, 57; bgt t3, t4, atoi_end
    addi t3, t3, -48; slli t4, t1, 3; slli t5, t1, 1; add t1, t4, t5; add t1, t1, t3
    addi t0, t0, 1; j atoi_loop
atoi_end:
    li t3, -1; bne t2, t3, atoi_positive; neg t1, t1
atoi_positive:
    mv a0, t1; mv a1, t0; ret
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

# ==============================================================================
# Função Principal (INTACTA)
# ==============================================================================
main:
    addi sp, sp, -32; sw ra, 28(sp); sw s0, 24(sp); sw s1, 20(sp); sw s2, 16(sp); sw s3, 12(sp)
    
    la a0, input_buffer; li a1, 8192; call read_line
    la a0, input_buffer; call atoi; mv s0, a0

    li   t0, 25       

    # Compara a quantidade lida (s0) com o limite (t0).
    # Usa o menor dos dois valores.
    blt  t0, s0, set_limit # se LIMITE < n, então usa o limite
    j    limit_ok
set_limit:
    mv   s0, t0        
limit_ok:

    la a0, input_buffer; li a1, 8192; call read_line
    
    la s2, input_array; la s3, input_buffer; li s1, 0
main_parse_loop:
    bge s1, s0, main_sort; mv a0, s3; call atoi
    sw a0, 0(s2); mv s3, a1; addi s2, s2, 4; addi s1, s1, 1; j main_parse_loop
main_sort:
    la   a0, input_array; li   a1, 0; addi a2, s0, -1
    call quick_sort
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
    addi sp, sp, 32; li a0, 0; ret

# ==============================================================================
# Funções de Ordenação Otimizada: Quick Sort (INTACTAS)
# ==============================================================================
quick_sort:
    addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); sw s1, 4(sp); sw s2, 0(sp)
    mv s0, a0; mv s1, a1; mv s2, a2
    bge s1, s2, quick_sort_end
    addi sp, sp, -16; sw s0, 12(sp); sw s1, 8(sp); sw s2, 4(sp); sw a0, 0(sp)
    mv a0, s0; mv a1, s1; mv a2, s2
    call partition
    mv s1, a0
    lw s0, 12(sp); lw a1, 8(sp); addi a2, s1, -1; mv a0, s0
    call quick_sort
    lw s2, 4(sp); addi a1, s1, 1; mv a0, s0
    call quick_sort
    addi sp, sp, 16
quick_sort_end:
    lw ra, 12(sp); lw s0, 8(sp); lw s1, 4(sp); lw s2, 0(sp)
    addi sp, sp, 16; ret

# --- partition: Versão ALTAMENTE OTIMIZADA com ponteiros ---
# a0=&array, a1=low, a2=high. Retorna o índice do pivô em a0
partition:
    addi sp, sp, -32
    sw ra, 28(sp); sw s0, 24(sp); sw s1, 20(sp); sw s2, 16(sp)
    sw s3, 12(sp); sw s4, 8(sp); sw s5, 4(sp)

    mv s0, a0 # s0 = &array (base)
    
    # Calcula ponteiros para low e high
    slli s1, a1, 2; add s1, s0, s1 # s1 = &array[low]
    slli s2, a2, 2; add s2, s0, s2 # s2 = &array[high]

    # Pivô = *high_ptr
    lw s3, 0(s2) # s3 = pivot_value

    # i_ptr = low_ptr - 4
    addi s4, s1, -4 # s4 = i_ptr

    # j_ptr = low_ptr
    mv s5, s1 # s5 = j_ptr

partition_loop:
    beq s5, s2, partition_loop_end # se j_ptr == high_ptr, fim do loop
    
    lw t1, 0(s5) # t1 = *j_ptr
    bgt t1, s3, partition_increment_j # se *j_ptr > pivot, continua
    
    # se *j_ptr <= pivot, incrementa i_ptr e faz a troca
    addi s4, s4, 4 # i_ptr++
    
    # swap(*i_ptr, *j_ptr)
    lw t2, 0(s4) # t2 = *i_ptr
    sw t1, 0(s4) # *i_ptr = *j_ptr
    sw t2, 0(s5) # *j_ptr = *i_ptr

partition_increment_j:
    addi s5, s5, 4 # j_ptr++
    j partition_loop

partition_loop_end:
    # Troca final: swap(*(i_ptr+4), *high_ptr)
    addi s4, s4, 4
    lw t1, 0(s4) 
    sw s3, 0(s4) # *(i_ptr+4) = pivot
    sw t1, 0(s2) # *high_ptr = valor antigo de *(i_ptr+4)

    # Calcula e retorna o índice do pivô: ( (i_ptr+4) - &array_base ) / 4
    sub a0, s4, s0
    srli a0, a0, 2 # a0 = pivot_index

    lw ra, 28(sp); lw s0, 24(sp); lw s1, 20(sp); lw s2, 16(sp)
    lw s3, 12(sp); lw s4, 8(sp); lw s5, 4(sp)
    addi sp, sp, 32
    ret
