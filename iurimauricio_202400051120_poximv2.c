#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> 

#define MEM_SIZE (32 * 1024)
#define MTIME_ADDR 0x0200bff8
#define MTIMECMP_ADDR 0x02004000
#define DEBUG_PLIC 1 

#define PLIC_PENDING_ADDR   0x0C001000
#define PLIC_ENABLE_ADDR    0x0C002000
#define PLIC_THRESHOLD_ADDR 0x0C200000
#define PLIC_CLAIM_ADDR     0x0C200004
#define MIP_MTIP (1 << 7)

#define MSIP_ADDR 0x02000000
#define UART_BASE_ADDR   0x10000000
#define UART_THR_ADDR    (UART_BASE_ADDR) // Transmit Holding Register (escrita)
#define UART_RHR_ADDR    (UART_BASE_ADDR) // Receive Holding Register (leitura)
#define UART_LSR_ADDR    (UART_BASE_ADDR + 0x05) // Line Status Register
#define UART_IER_ADDR    (UART_BASE_ADDR + 0x01) // Interrupt Enable Register
#define UART_IIR_ADDR    (UART_BASE_ADDR + 0x02) // Interrupt Identification Register
#define NUM_INTERRUPTS 32

uint32_t plic_priority[NUM_INTERRUPTS] = {0};
uint8_t uart_thr = 0; // Transmit Holding Register
uint8_t uart_rhr = 0; // Receive Holding Register
uint8_t uart_lsr = 0x60; // Line Status Register: THR empty (bit 5), line idle (bit 6)
uint8_t uart_ier = 0; // Interrupt Enable Register
uint8_t uart_iir = 1; // Interrupt Identification Register (bit 0 = 1: no interrupt)
uint32_t prev_mstatus_mie = 0;
uint32_t msip = 0;
uint32_t plic_pending = 0;
uint32_t plic_enable = 0;
uint32_t plic_threshold = 0;
uint32_t plic_claim = 0;

uint32_t mstatus = 0, mepc = 0, mcause = 0, mtvec = 0,mtval = 0,mie = 0, mip = 0,mtime = 0, mtimecmp = 0;

typedef enum {
    INSTRUCTION_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    LOAD_ACCESS_FAULT = 5,
    STORE_ACCESS_FAULT = 7,
    ENV_CALL_MMODE = 11
} ExceptionCode;

int main(int argc, char* argv[]) {

	int trap_config_done = 0;

	printf("--------------------------------------------------------------------------------\n");

	for(uint32_t i = 0; i < argc; i++) {

		printf("argv[%i] = %s\n", i, argv[i]);
	}
	const uint32_t offset = 0x80000000;

	mtvec = offset + 0x20; // Inicializa mtvec com o valor de offset

	FILE* input = fopen(argv[1], "r");
	FILE* output = fopen(argv[2], "w");

	if (input == NULL){
		printf("Erro ao abrir o arquivo");
		exit(1);
	}
	
	uint32_t x[32] = { 0 };
	const char* x_label[32] = { "zero", "ra", "sp", "gp", "tp", "t0",
		"t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
		"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9",
		"s10", "s11", "t3", "t4", "t5", "t6" };
		
	uint32_t pc = offset;
	
	uint8_t* mem = (uint8_t*)(malloc(32 * 1024));

	memset(mem, 0, MEM_SIZE);
	
	uint32_t endereço = offset;
	
	char str[128];

	while (fgets(str, sizeof(str), input) != NULL) {
    
	    if(str[0] == '@') {
    	    sscanf(str + 1, "%x", &endereço);
        	continue;
    	}
    
		char *p = str;
		while(*p != '\0') {
			if(*p == ' ' || *p == '\n' || *p == '\r') {
				p++;
				continue;
			}
			
			uint8_t byte = (uint8_t)strtol(p, NULL, 16);
			
			if((endereço - offset) >= MEM_SIZE) {
				fprintf(stderr, "Endereço 0x%08x fora dos limites da memória!\n", endereço);
				exit(1);
			}
			
			mem[endereço - offset] = byte;			
			endereço++;
			p += 2;  
		}
	}
	
	printf("\n--------------------------------------------------------------------------------\n");


	void trigger_exception(uint32_t cause, uint32_t tval, uint32_t* pc) {
		const char* exception_name = "unknown_exception";
		mcause = cause;
		switch (cause) {
			case INSTRUCTION_ACCESS_FAULT:
				exception_name = "instruction_fault";
				break;
			case ILLEGAL_INSTRUCTION:
				exception_name = "illegal_instruction";
				break;
			case LOAD_ACCESS_FAULT:
				exception_name = "load_fault";
				break;
			case STORE_ACCESS_FAULT:
				exception_name = "store_fault";
				break;
			case ENV_CALL_MMODE:
				exception_name = "environment_call";
				break;
		}
		char linha[256] = {0}; // Clear the line buffer

		snprintf(linha,sizeof(linha),">exception:%s\tcause=0x%08x,epc=0x%08x,tval=0x%08x\n",
			exception_name,
			mcause,
			mepc,
			mtval);

		fputs(linha, output);

		*pc = mtvec;
	}

	void ecall(uint32_t* pc) {
		trigger_exception(ENV_CALL_MMODE, 0, pc);
	}
	uint8_t run = 1;
	int c = 0;
	clock_t start_time = clock();

	while(run) {
		mtime++;
		
		if ((double)(clock() - start_time) / CLOCKS_PER_SEC >= 0.01) {
			printf("\n--------------------------------------------------------------------------------\n");
			printf("SIMULATOR TIMEOUT: A simulação excedeu 0.01 segundos e foi encerrada.\n");
			break; // Sai do loop imediatamente
		}
		
        // --- ALTERAÇÃO INICIADA: Lógica de checagem de interrupção movida e ajustada ---
		if (mtime >= mtimecmp) {
			mip |= (1 << 7); // Seta o bit MTIP (Machine Timer Interrupt Pending)
		}
		
		// Lógica do PLIC para determinar se uma interrupção externa deve ser sinalizada
		int irq_id = -1;
		int highest_priority = 0;
		for (int i = 0; i < NUM_INTERRUPTS; i++) {
			// Verifica se a interrupção 'i' está pendente E habilitada
			if ((plic_pending & (1 << i)) && (plic_enable & (1 << i))) {
				// Verifica se a prioridade é maior que o limiar do core e a maior prioridade até agora
				if (plic_priority[i] > plic_threshold && plic_priority[i] > highest_priority) {
					highest_priority = plic_priority[i];
					irq_id = i;
				}
			}
		}

		// Se o PLIC encontrou uma interrupção válida, eleva a linha de interrupção externa para a CPU
		if (irq_id != -1) {
			mip |= (1 << 11); // Seta o bit MEIP (Machine External Interrupt Pending)
		} else {
			// Caso contrário, garante que a linha de interrupção externa esteja baixa
			mip &= ~(1 << 11); // Limpa o bit MEIP
		}
        // --- ALTERAÇÃO FINALIZADA ---

		uint32_t mstatus_mie = (mstatus >> 3) & 1;
		uint32_t mie_mtie = (mie >> 7) & 1;
		uint32_t mip_mtip = (mip >> 7) & 1;
		uint32_t mie_meie = (mie >> 11) & 1;
		uint32_t mip_meip = (mip >> 11) & 1;

		//timer
		if (trap_config_done && mstatus_mie && mie_mtie && mip_mtip) {
			mepc = pc;                   // Salva o PC da instrução que seria executada
			mcause = 0x80000007;         // Causa da interrupção: Machine Timer Interrupt
			mtval = 0;                   // tval não é usado para esta interrupção

			char linha[256] = {0};	
			snprintf(linha, sizeof(linha), ">interrupt:timer\tcause=0x%08x,epc=0x%08x,tval=0x%08x\n", mcause, mepc, mtval);
			fputs(linha, output); // <--- Aqui você escreve na saída (arquivo .out)
			printf("%s",linha);

			// Calcula o endereço do handler
			uint32_t handler_addr;
			if (mtvec & 1) { // Modo vetorado
				handler_addr = (mtvec & ~1) + 4 * (mcause & 0x1F);
			} else { // Modo direto
				handler_addr = mtvec & ~1;
			}

			if (handler_addr < offset || (handler_addr - offset + 3) >= MEM_SIZE) {
				fprintf(stderr, "handler_addr (0x%08x) fora do campo de instruções! Redirecionando para o início.\n", handler_addr);
				pc = offset;
			} else {
				pc = handler_addr;
			}

			mstatus = 0x1880;
			continue;             // Pula para a próxima iteração para executar o handler
		}
			
		
		printf("trap_config_done: %d, mstatus_mie: %d, prev_mstatus_mie: %d, mie_meie: %d, mip_meip: %d\n",
			trap_config_done, mstatus_mie, prev_mstatus_mie, mie_meie, mip_meip);

		// Se a configuração de trap estiver feita e interrupções estiverem habilitadas
		if (trap_config_done && mstatus_mie && mie_meie && mip_meip) {
			mepc = pc;                   // Salva o PC da instrução que seria executada
			mcause = 0x8000000b;         // Causa da interrupção: Machine External Interrupt
			mtval = 0;                   // tval não é usado para esta interrupção
			
			char linha[256] = {0};	
			snprintf(linha, sizeof(linha), ">interrupt:external\tcause=0x%08x,epc=0x%08x,tval=0x%08x\n", mcause, mepc, mtval);
			printf("[INTERRUPT] mcause=0x%08x, mepc=0x%08x, mtval=0x%08x\n", mcause, mepc, mtval);
			fputs(linha, output);

			uint32_t handler_addr;
			if (mtvec & 1) { // Modo vetorado
				handler_addr = (mtvec & ~1) + 4 * (mcause & 0x1F);
			} else { // Modo direto
				handler_addr = mtvec & ~1;
			}
			
			if (handler_addr < offset || (handler_addr - offset + 3) >= MEM_SIZE) {
				fprintf(stderr, "handler_addr (0x%08x) fora do campo de instruções! Redirecionando para o início.\n", handler_addr);
				pc = offset;
			} else {
				pc = handler_addr;
			}
			
			mstatus = 0x1880;			
			continue;             // Pula para a próxima iteração para executar o handler
		}
		
		uint32_t mie_msie = (mie >> 3) & 1;
		uint32_t mip_msip = (mip >> 3) & 1;

		if (mstatus_mie && mie_msie && mip_msip) {
			printf("[INTERRUPT] msip=0x%08x -> MSIP!\n", msip);
			mepc = pc;
			mcause = 0x80000003; // Código de interrupção de software
			mtval = 0;
			char linha[256] = {0};
			snprintf(linha,sizeof(linha),">interrupt:software\tcause=0x%08x,epc=0x%08x,tval=0x%08x\n", mcause, mepc, mtval);
			fputs(linha, output);

			uint32_t handler_addr;
			if (mtvec & 1) {
				handler_addr = (mtvec & ~1) + 4 * (mcause & 0x1F);
			} else {
				handler_addr = mtvec & ~1;
			}
			if (handler_addr < offset || (handler_addr - offset + 3) >= MEM_SIZE) {
				fprintf(stderr, "handler_addr (0x%08x) fora do campo de instruções! Redirecionando para o início.\n", handler_addr);
				pc = offset;
			} else {
				pc = handler_addr;
			}

			if (mstatus & (3 & 1)) { 
				mstatus |= (1 << 7);    // seta MPIE
			} else {
				mstatus &= ~(1 << 7);   // limpa MPIE
			}

			mstatus &= ~(1 << 3);
			continue;
		}

		uint32_t instruction = mem[pc - offset] | (mem[pc - offset + 1] << 8) |(mem[pc - offset + 2] << 16) |	
		(mem[pc - offset + 3] << 24);
		const uint8_t opcode = instruction & 0b1111111;
		const uint8_t rd     = (instruction >> 7)  & 0b11111;
		const uint8_t funct3 = (instruction >> 12) & 0b111;
		const uint8_t rs1    = (instruction >> 15) & 0b11111;
		const uint8_t rs2    = (instruction >> 20) & 0b11111;
		const uint8_t funct7 = (instruction >> 25) & 0b1111111;
		char nomeInst[10];
		int32_t pci = pc;
		
		// Imediatos sinalizados
		int32_t imm_i = (int32_t)instruction >> 20;
		int32_t imm_s = ((instruction >> 25) << 5) | ((instruction >> 7) & 0x1F);
		imm_s = (imm_s << 20) >> 20;
		int32_t imm_b = ((instruction >> 31) << 12) |
						(((instruction >> 7) & 0x1) << 11) |
						(((instruction >> 25) & 0x3F) << 5) |
						(((instruction >> 8) & 0xF) << 1);
		imm_b = (imm_b << 19) >> 19;
		int32_t imm_j = ((instruction >> 31) << 20) |
						(((instruction >> 12) & 0xFF) << 12) |
						(((instruction >> 20) & 0x1) << 11) |
						(((instruction >> 21) & 0x3FF) << 1);
		imm_j = (imm_j << 11) >> 11;
		uint32_t imm20 = instruction >> 12;
		uint32_t uimm = (instruction >> 20) & 0b11111;
		int32_t temph;

		uint32_t rs1_val = x[rs1];
        uint32_t rs2_val = x[rs2];

		char linha[256];
		printf("----------------------------------------------------------------------\n"
       "|   zero=%#010x|   ra=%#010x|   sp=%#010x|    gp=%#010x|\n"
       "|     tp=%#010x|   t0=%#010x|   t1=%#010x|    t2=%#010x|\n"
       "|     s0=%#010x|   s1=%#010x|   a0=%#010x|    a1=%#010x|\n"
       "|     a2=%#010x|   a3=%#010x|   a4=%#010x|    a5=%#010x|\n"
       "|     a6=%#010x|   a7=%#010x|   s2=%#010x|    s3=%#010x|\n"
       "|     s4=%#010x|   s5=%#010x|   s6=%#010x|    s7=%#010x|\n"
       "|     s8=%#010x|   s9=%#010x|  s10=%#010x|   s11=%#010x|\n"
       "|     t3=%#010x|   t4=%#010x|   t5=%#010x|    t6=%#010x|\n"
       "----------------------------------------------------------------------\n"
       "|     pc=%#010x|mtvec=%#010x| mepc=%#010x|mcause=%#010x|\n"
       "|mstatus=%#010x|  mie=%#010x|mtval=%#010x|   mip=%#010x|\n"
       "----------------------------------------------------------------------\n"
       "> 0x%08x <%s>:    %s   %s,%s,0x%x\n"
       "----------------------------------------------------------------------\n",
       x[0],  x[1],  x[2],  x[3],
       x[4],  x[5],  x[6],  x[7],
       x[8],  x[9],  x[10], x[11],
       x[12], x[13], x[14], x[15],
       x[16], x[17], x[18], x[19],
       x[20], x[21], x[22], x[23],
       x[24], x[25], x[26], x[27],
       x[28], x[29], x[30], x[31],
       pci, mtvec, mepc, mcause,
       mstatus, mie, mtval, mip,
       pci, x_label[rd], nomeInst, x_label[rs1], x_label[rs2], imm_i
	);

		switch(opcode) {
			case 0b1110011: // SYSTEM Opcode
			{
				const uint16_t csr_addr = instruction >> 20;

				if (funct3 == 0b000) {
					if (instruction == 0x00000073) { // ecall
						strncpy(nomeInst, "ecall", sizeof(nomeInst));
						snprintf(linha, sizeof(linha),
							"0x%08x:ecall\n", pci);
						fputs(linha, output);
						
						ecall(&pc); // Chama a nova função ecall
						continue; // Pula para a próxima iteração do loop com o novo pc

					}else if (instruction == 0x30200073) { // mret
					strncpy(nomeInst, "mret", sizeof(nomeInst));
					snprintf(linha, sizeof(linha),
						"0x%08x: mret   (mstatus: 0x%08x -> ", pci, mstatus);
					fputs(linha, output);

					uint32_t mpie = (mstatus >> 7) & 1;

					if (mpie) {
						mstatus |= (1 << 3);  // Habilita MIE
					} else {
						mstatus &= ~(1 << 3); // Desabilita MIE
					}

					mstatus |= (1 << 7);

					mstatus &= ~((1 << 11) | (1 << 12)); // Zera os bits 11 e 12 (MPP)

					snprintf(linha, sizeof(linha), "0x%08x, pc -> 0x%08x)\n", mstatus, mepc);
					fputs(linha, output);

					pc = mepc;
					continue;
				}
				}
				if(funct3 == 0b000 && uimm == 1) {
					// Outputting instruction to console
					printf("0x%08x:ebreak\n", pc);

					snprintf(linha, sizeof(linha),
					"0x%08x:ebreak\n", pci);
					fputs(linha, output);
		
					const uint32_t previous = ((uint32_t*)(mem))[(pc - 4 - offset) >> 2];
					const uint32_t next = ((uint32_t*)(mem))[(pc + 4 - offset) >> 2];
					// Halting condition	
            		if(previous == 0x01f01013 && next == 0x40705013){
					run = 0;
					continue;
					} 
				}
				uint32_t* csr = NULL;

				switch (csr_addr) {
					case 0x300: csr = &mstatus; break;
					case 0x305: csr = &mtvec;   break;
					case 0x304: csr = &mie; break; 
					case 0x341: csr = &mepc;    break;
					case 0x342: csr = &mcause;  break;
					case 0x343: csr = &mtval;   break;
					case 0x344: csr = &mip; break;  
					default: 
						trigger_exception(ILLEGAL_INSTRUCTION, instruction, &pc);
						continue;
				}

				uint32_t old_val = *csr; // Sempre lê o valor antigo

				switch (funct3) {
					case 0b001: // CSRRW
						strncpy(nomeInst, "csrrw", sizeof(nomeInst));
						*csr = x[rs1];
						if (csr_addr == 0x300 && funct3 == 0b001 && (*csr & (1 << 3))) {
							trap_config_done = 1;
						}
						break;
					case 0b010: // CSRRS
						strncpy(nomeInst, "csrrs", sizeof(nomeInst));
						*csr = old_val | x[rs1];
						break;
					case 0b011: // CSRRC
						strncpy(nomeInst, "csrrc", sizeof(nomeInst));
						*csr = old_val & (~x[rs1]);
						break;
					case 0b101: // CSRRWI (I for Immediate) 
						strncpy(nomeInst, "csrrwi", sizeof(nomeInst));
						*csr = rs1; 
						break;

					case 0b110: // CSRRSI (I for Immediate) 
						strncpy(nomeInst, "csrrsi", sizeof(nomeInst));
						*csr = old_val | rs1;
						break;

					case 0b111: // CSRRCI (I for Immediate) 
						strncpy(nomeInst, "csrrci", sizeof(nomeInst));
						*csr = old_val & (~rs1);
						break;
					default:
						trigger_exception(ILLEGAL_INSTRUCTION, instruction, &pc);
						continue;
					}

				if (rd != 0) {
					x[rd] = old_val; // Escreve o valor antigo em rd
				}
				
				pc = pci + 4;
				break;
			}
			case 0b0110011:
			//R-type
				switch (funct7){
				case 0b0000000:
					if(funct3 == 0b000) {
						//add
						strncpy(nomeInst,"add",sizeof(nomeInst));
						if(rd != 0) x[rd] = x[rs1] + x[rs2];
					}
					else if (funct3 == 0b001){
						//sll
						strncpy(nomeInst,"sll",sizeof(nomeInst));
						x[rd] = x[rs1] << (x[rs2] & 0b11111);
					}
					else if (funct3 == 0b010){
						//slt
						strncpy(nomeInst,"slt",sizeof(nomeInst));
						int32_t temp1 = (int32_t)x[rs1], temp2 = (int32_t)x[rs2];
						x[rd] = (temp1 < temp2)? 1 : 0;

					}
					else if (funct3 == 0b011) {
						// sltu
						strncpy(nomeInst, "sltu", sizeof(nomeInst));
						x[rd] = (x[rs1] < x[rs2]) ? 1 : 0;
					}
					else if (funct3 == 0b100){
						//xor
						strncpy(nomeInst,"xor",sizeof(nomeInst));
						x[rd] = x[rs1]^x[rs2];

					}
					else if (funct3 == 0b101){
						//srl
						strncpy(nomeInst,"srl",sizeof(nomeInst));
						x[rd] = x[rs1] >> (x[rs2] & 0b11111);

					}
						else if (funct3 == 0b110){
						//or
						strncpy(nomeInst,"or",sizeof(nomeInst));
						x[rd] = x[rs1] | x[rs2];

					}
						else if (funct3 == 0b111){
						//and
						strncpy(nomeInst,"and",sizeof(nomeInst));
						x[rd] = x[rs1] & x[rs2];

					}
					break;
				case 0b0100000:
					if (funct3 == 0b000) {
						//sub
						strncpy(nomeInst,"sub",sizeof(nomeInst));
						x[rd] = x[rs1] - x[rs2];	

					}
					else if (funct3 == 0b101){
						//sra
						strncpy(nomeInst,"sra",sizeof(nomeInst));
						int32_t temp1 = (int32_t)x[rs1];
						uint32_t temp2 = x[rs2] & 0b11111;
						x[rd] = temp1 >> temp2;

					}
					break;
				case 0b0000001:
					if (funct3 == 0b000) { // mul
						strncpy(nomeInst, "mul", sizeof(nomeInst));
						if (rd != 0) x[rd] = ((int32_t)x[rs1]) * ((int32_t)x[rs2]);
					}
					else if (funct3 == 0b001) { // mulh
						strncpy(nomeInst, "mulh", sizeof(nomeInst));
						int64_t a = (int64_t)(int32_t)x[rs1];
						int64_t b = (int64_t)(int32_t)x[rs2];
						if (rd != 0) x[rd] = (uint32_t)((a * b) >> 32);
					}
					else if (funct3 == 0b010) { // mulhsu
						strncpy(nomeInst, "mulhsu", sizeof(nomeInst));
						int64_t a = (int64_t)(int32_t)x[rs1];
						uint64_t b = (uint64_t)x[rs2];
						if (rd != 0) x[rd] = (uint32_t)((a * b) >> 32);
					}
					else if (funct3 == 0b011) { // mulhu
						strncpy(nomeInst, "mulhu", sizeof(nomeInst));
						uint64_t a = (uint64_t)x[rs1];
						uint64_t b = (uint64_t)x[rs2];
						if (rd != 0) x[rd] = (uint32_t)((a * b) >> 32);
					}
					else if (funct3 == 0b100) { // div
						strncpy(nomeInst, "div", sizeof(nomeInst));
						int32_t a = (int32_t)x[rs1], b = (int32_t)x[rs2];
						if (rd != 0) x[rd] = (b == 0) ? -1 : (a == INT32_MIN && b == -1) ? a : a / b;
					}
					else if (funct3 == 0b101) { // divu
						strncpy(nomeInst, "divu", sizeof(nomeInst));
						uint32_t a = x[rs1], b = x[rs2];
						if (rd != 0) x[rd] = (b == 0) ? 0xFFFFFFFF : a / b;
					}
					else if (funct3 == 0b110) { // rem
						strncpy(nomeInst, "rem", sizeof(nomeInst));
						int32_t a = (int32_t)x[rs1], b = (int32_t)x[rs2];
						if (rd != 0) x[rd] = (b == 0) ? a : (a == INT32_MIN && b == -1) ? 0 : a % b;
					}
					else if (funct3 == 0b111) { // remu
						strncpy(nomeInst, "remu", sizeof(nomeInst));
						uint32_t a = x[rs1], b = x[rs2];
						if (rd != 0) x[rd] = (b == 0) ? a : a % b;
					}
					break;
					break;
				default:
					break;
			}		
			pc = pci + 4;
			break;
			case 0b0000011: 
					// L-type
				if (funct3 == 0b000) { // lb
					strncpy(nomeInst, "lb", sizeof(nomeInst));
					
					int32_t addr = x[rs1] + imm_i;
					
					if (addr == UART_THR_ADDR) { 
						x[rd] = uart_rhr;
					} else if (addr == UART_LSR_ADDR) {
						x[rd] = uart_lsr;
					} else if (addr == UART_IER_ADDR) {
						x[rd] = uart_ier;
					} else if (addr == UART_IIR_ADDR) {
						x[rd] = uart_iir;
					} else if ((addr - offset + 3) >= MEM_SIZE || addr < offset) { 
						trigger_exception(LOAD_ACCESS_FAULT, addr, &pc);
						continue;
					}else {
						int8_t val = *(int8_t*)(mem + (addr - offset));
						x[rd] = val;
					}
					
					
				} else if (funct3 == 0b001) { // lh
					strncpy(nomeInst, "lh", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 3) >= MEM_SIZE || addr < offset) { 
						trigger_exception(LOAD_ACCESS_FAULT, addr, &pc);
						continue;
					}
					int16_t val = *(int16_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b010) { // lw
					int32_t addr = x[rs1] + imm_i;
					if (addr == PLIC_PENDING_ADDR) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = plic_pending;
					} else if (addr == MSIP_ADDR) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = msip;
   					} else if (addr == PLIC_ENABLE_ADDR) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = plic_enable;
					} else if (addr == PLIC_THRESHOLD_ADDR) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = plic_threshold;
					// --- ALTERAÇÃO INICIADA: Lógica de 'claim' do PLIC implementada ---
					}else if (addr == PLIC_CLAIM_ADDR) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						
						// Encontra a interrupção de maior prioridade que está pendente e habilitada
						int highest_prio_claim = 0;
						int claimed_irq = 0; // 0 significa 'sem interrupção'
						for (int i = 1; i < NUM_INTERRUPTS; i++) {
							if ((plic_pending & (1 << i)) && (plic_enable & (1 << i)) && (plic_priority[i] > plic_threshold)) {
								if (plic_priority[i] > highest_prio_claim) {
									highest_prio_claim = plic_priority[i];
									claimed_irq = i;
								}
							}
						}

						if (claimed_irq > 0) {
							// Limpa o bit de pendência para a IRQ que está sendo reivindicada
							plic_pending &= ~(1 << claimed_irq);
							#if DEBUG_PLIC
							printf("[DEBUG_PLIC] PC 0x%08x: lw de PLIC_CLAIM_ADDR. Reivindicando IRQ %d. plic_pending agora é 0x%08x\n", pci, claimed_irq, plic_pending);
							#endif
						}
						
						// O registrador de destino 'rd' recebe o ID da IRQ reivindicada
						if (rd != 0) {
							x[rd] = claimed_irq;
						}
						
						// A variável plic_claim pode ser usada para rastrear a IRQ reivindicada, se necessário
						plic_claim = claimed_irq;
					// --- ALTERAÇÃO FINALIZADA ---
					}else if (addr >= MTIME_ADDR && addr < MTIME_ADDR + 8) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = (uint32_t)mtime;
					} else if (addr >= MTIMECMP_ADDR && addr < MTIMECMP_ADDR + 8) {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						x[rd] = (uint32_t)mtimecmp;
					} else {
						strncpy(nomeInst, "lw", sizeof(nomeInst));
						int32_t addr_val = x[rs1] + imm_i;
						if ((addr_val - offset + 3) >= MEM_SIZE || addr_val < offset) { 
							trigger_exception(LOAD_ACCESS_FAULT, addr_val, &pc);
							continue;
							}
							int32_t val = *(int32_t*)(mem + (addr_val - offset));
							x[rd] = val;
					}
				} else if (funct3 == 0b100) { // lbu
					strncpy(nomeInst, "lbu", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 3) >= MEM_SIZE || addr < offset) { 
						trigger_exception(LOAD_ACCESS_FAULT, addr, &pc);
						continue;
					}
					uint8_t val = *(uint8_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b101) { // lhu
					strncpy(nomeInst, "lhu", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 3) >= MEM_SIZE || addr < offset) { 
						trigger_exception(LOAD_ACCESS_FAULT, addr, &pc);
						continue;
					}
					uint16_t val = *(uint16_t*)(mem + (addr - offset));
					x[rd] = val;
				}
				pc = pci + 4;
				break;
			case 0b0010011:
				//I-type
				if(funct3 == 0b000) {
					//addi
					strncpy(nomeInst,"addi",sizeof(nomeInst));
					if(rd != 0) x[rd] = x[rs1] + (int32_t)imm_i;
				}
				else if(funct3 == 0b010){
					//slti
					strncpy(nomeInst,"slti",sizeof(nomeInst));
					int32_t temp = (int32_t)x[rs1];
					x[rd] = (temp < (int32_t)imm_i)? 1 : 0;
				}
				else if (funct3 == 0b011) {
					//sltiu
					strncpy(nomeInst,"sltiu",sizeof(nomeInst));
					x[rd] = (x[rs1] < (uint32_t)imm_i) ? 1 : 0;
				}
				else if (funct3 == 0b100) {
					//xori
					strncpy(nomeInst,"xori",sizeof(nomeInst));
					x[rd] = x[rs1] ^ imm_i;
				}
				else if (funct3 == 0b110) {
					//ori
					strncpy(nomeInst,"ori",sizeof(nomeInst));
					x[rd] = x[rs1] | imm_i;
				}
				else if (funct3 == 0b111) {
					//andi
					strncpy(nomeInst,"andi",sizeof(nomeInst));
					x[rd] = x[rs1] & imm_i;
				}
				else if (funct3 == 0b001 && ((imm_i >> 5) & 0b1111111) == 0b0000000) {
					//slli
					strncpy(nomeInst,"slli",sizeof(nomeInst));
					const uint32_t data = x[rs1] << uimm;
					x[rd] = data;
				}
				else if (funct3 == 0b101 && ((imm_i >> 5) & 0b1111111) == 0b0000000) {
					//srli
					strncpy(nomeInst,"srli",sizeof(nomeInst));
					const uint32_t data = x[rs1] >> uimm;
					x[rd] = data;
				}
				else if (funct3 == 0b101 && ((imm_i >> 5) & 0b1111111) == 0b0100000) {
					// srai
					strncpy(nomeInst,"srai",sizeof(nomeInst));
					int32_t temp = (int32_t)x[rs1];
					x[rd] = temp >> (imm_i & 0b11111);
				}

				pc = pci + 4;

				break;
			// DENTRO DO SWITCH(OPCODE)
			case 0b1101111: // JAL
			{
				strncpy(nomeInst, "jal", sizeof(nomeInst));
				uint32_t target_pc = pci + imm_j;

				// VERIFICAÇÃO DE LIMITES (ADICIONADA)
				if ((target_pc < offset) || (target_pc - offset + 3) >= MEM_SIZE) {
					trigger_exception(INSTRUCTION_ACCESS_FAULT, target_pc, &pc);
					continue;
				}

				if (rd != 0) {
					x[rd] = pci + 4;
				}
				snprintf(linha, sizeof(linha),
					"0x%08x:jal    %s,%#07x          pc=0x%08x,%s=0x%08x\n",
					pci, x_label[rd], (int32_t)((imm_j >> 1) & 0xFFFFF), pci + imm_j, x_label[rd], pci + 4);
				fputs(linha, output);
				pc = target_pc;
				continue; // Use 'continue' para saltar a lógica de incremento padrão
			}
			case 0b0010111: 
				//auipc
				strncpy(nomeInst,"auipc",sizeof(nomeInst));
       			x[rd] = pci + (imm20 << 12);
				pc = pci + 4;

        		break;
			case 0b0110111:
				//lui
				strncpy(nomeInst,"lui",sizeof(nomeInst));
				x[rd] = imm20 << 12;
				pc = pci + 4;

        		break;
			case 0b0100011:
				//S-type
				if(funct3 == 0b000){
					//sb
					strncpy(nomeInst,"sb",sizeof(nomeInst));
					snprintf(linha, sizeof(linha),
					"0x%08x:sb     %s,0x%03x(%s)        mem[0x%08x]=0x%02x\n",
					pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2] & 0xFF);
					fputs(linha, output);

					uint32_t addr = x[rs1] + imm_s;
					
					if (addr == UART_THR_ADDR) { // escrita = THR
						uart_thr = x[rs2] & 0xFF;

						uart_lsr &= ~0x20;
						fputc(uart_thr, stdout);
						fflush(stdout);
						uart_lsr |= 0x20; // THR empty

						if (uart_ier & 0x02) {
							plic_pending |= (1 << 10); // IRQ 10
						}

					} else if (addr == UART_IER_ADDR) {
						uart_ier = x[rs2] & 0xFF;
					} else if (addr == UART_LSR_ADDR) {
						if (uart_ier & 0x02) {
						plic_pending |= 0x400;
						}
						// LSR geralmente é somente leitura, ignore escrita
					} else if ((addr - offset) >= MEM_SIZE) {
							trigger_exception(STORE_ACCESS_FAULT, addr, &pc);
							continue;					
					} else {
						mem[addr - offset] = (uint8_t)(x[rs2] & 0b11111111);
					}

				}
				else if(funct3 == 0b001){ 
					// sh
					strncpy(nomeInst,"sh",sizeof(nomeInst));
					uint32_t addr = x[rs1] + imm_s;
					if ((addr < offset) || (addr - offset + sizeof(uint16_t) > MEM_SIZE)) {
						trigger_exception(STORE_ACCESS_FAULT, addr, &pc);
						continue;
					}
					uint16_t value = (uint16_t)(x[rs2] & 0xFFFF);
					memcpy(mem + (addr - offset), &value, sizeof(uint16_t));
				}
				else if(funct3 == 0b010){
					//sw
					uint32_t addr = x[rs1] + imm_s;
					if (addr >= 0x0C000000 && addr < 0x0C000000 + NUM_INTERRUPTS * 4) {
						uint32_t irq_id_sw = (addr - 0x0C000000) / 4;
						plic_priority[irq_id_sw] = x[rs2];
						#if DEBUG_PLIC
						printf("[DEBUG_PLIC] PC 0x%08x: sw para plic_priority[%u]. Novo valor: %u\n", pci, irq_id_sw, x[rs2]);
						#endif
					} else if (addr == PLIC_PENDING_ADDR) {
						plic_pending = x[rs2];
						#if DEBUG_PLIC
						printf("[DEBUG_PLIC] PC 0x%08x: sw para PLIC_PENDING_ADDR. Novo valor: 0x%08x\n", pci, x[rs2]);
						#endif
					} else if (addr == MSIP_ADDR) {
						
					} else if (addr == PLIC_ENABLE_ADDR) {
						plic_enable = x[rs2];
						#if DEBUG_PLIC
						printf("[DEBUG_PLIC] PC 0x%08x: sw para PLIC_ENABLE_ADDR. Novo valor: 0x%08x\n", pci, x[rs2]);
						#endif
					} else if (addr == PLIC_THRESHOLD_ADDR) {
						plic_threshold = x[rs2];
						#if DEBUG_PLIC
						printf("[DEBUG_PLIC] PC 0x%08x: sw para PLIC_THRESHOLD_ADDR. Novo valor: %u\n", pci, x[rs2]);
						#endif
					} else if (addr == PLIC_CLAIM_ADDR) {
						uint32_t complete_id = x[rs2];
						#if DEBUG_PLIC
						printf("[DEBUG_PLIC] PC 0x%08x: sw para PLIC_CLAIM_ADDR. Completando IRQ %u.\n", pci, complete_id);
						#endif
						// A escrita em claim/complete sinaliza que a interrupção foi tratada.
						// A lógica de 'claim' na leitura já limpou o bit de pendência.
						// Esta parte é para sinalizar a conclusão. No nosso modelo simples,
						// não precisamos fazer nada, mas em um PLIC real, isso habilitaria
						// a mesma IRQ a ser sinalizada novamente se a condição persistir.
						plic_claim = 0; // Reseta o estado de claim.
					} else if (addr >= MTIME_ADDR && addr < MTIME_ADDR + 8) {
						mtime = (mtime & 0xFFFFFFFF00000000) | x[rs2];
					} else if (addr >= MTIMECMP_ADDR && addr < MTIMECMP_ADDR + 8) {
						mtimecmp = (mtimecmp & 0xFFFFFFFF00000000) | x[rs2];
						mip &= ~MIP_MTIP;
					}else if ((addr < offset) || (addr - offset + sizeof(uint32_t) > MEM_SIZE)) {
							trigger_exception(STORE_ACCESS_FAULT, addr, &pc);
							continue;
					}else {					
						*(uint32_t*)(mem + (addr - offset)) = x[rs2];
					}
					
					strncpy(nomeInst,"sw",sizeof(nomeInst));
					snprintf(linha, sizeof(linha),
						"0x%08x:sw     %s,0x%03x(%s)        mem[0x%08x]=0x%08x\n",
						pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2]);
					fputs(linha, output);
				}
				pc = pci + 4;
				break;
			case 0b1100011:	
			{
				//B-type
				int condição = 0;
				switch(funct3) {
					case 0b000: // beq
						strncpy(nomeInst,"beq",sizeof(nomeInst));
						condição = (x[rs1] == x[rs2]);
						break;
					case 0b001: // bne
						strncpy(nomeInst,"bne",sizeof(nomeInst));
						condição = (x[rs1] != x[rs2]);
						break;
					case 0b100: // blt
						strncpy(nomeInst,"blt",sizeof(nomeInst));
						condição = ((int32_t)x[rs1] < (int32_t)x[rs2]);
						break;
					case 0b101: // bge
						strncpy(nomeInst,"bge",sizeof(nomeInst));
						condição = ((int32_t)x[rs1] >= (int32_t)x[rs2]);
						break;
					case 0b110: // bltu
						strncpy(nomeInst,"bltu",sizeof(nomeInst));
						condição = (x[rs1] < x[rs2]);
						break;
					case 0b111: // bgeu
						strncpy(nomeInst,"bgeu",sizeof(nomeInst));
						condição = (x[rs1] >= x[rs2]);
						break;
				}
				if (condição) pc = pci + imm_b;
				
				else pc = pci + 4;

				break;
			}
			case 0b1100111: // JALR
			{
				strncpy(nomeInst, "jalr", sizeof(nomeInst));
				uint32_t temp = pci + 4;
				uint32_t target_pc = (x[rs1] + (int32_t)imm_i) & ~1;

				if ((target_pc < offset) || (target_pc - offset + 3) >= MEM_SIZE) {
					trigger_exception(INSTRUCTION_ACCESS_FAULT, target_pc, &pc);
					continue; // Pula para a rotina de tratamento
				}

				snprintf(linha, sizeof(linha),
					"0x%08x:jalr   %s,%s,0x%03x       pc=0x%08x+0x%08x,%s=0x%08x\n",
					pci, x_label[rd], x_label[rs1], imm_i & 0xFFF, x[rs1], (int32_t)imm_i, x_label[rd], pci + 4);
				fputs(linha, output);
			
				pc = target_pc;
				if (rd != 0) {
					x[rd] = temp;
				}
				continue; 
			}
			default:
				trigger_exception(ILLEGAL_INSTRUCTION, instruction, &pc);
				continue; // Pula para a próxima iteração com o PC do handler
		}
	
		//Escrita no arquivo .out
		if (strcmp(nomeInst, "jal") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:jal    %s,%#07x          pc=0x%08x,%s=0x%08x\n",
			pci, x_label[rd], (int32_t)((imm_j >> 1) & 0xFFFFF), pci + imm_j, x_label[rd], pci + 4);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrw") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:csrrw  %s,%#05x,%s          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, x_label[rs1], x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrs") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:csrrs  %s,%#05x,%s          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, x_label[rs1], x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrc") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:csrrc  %s,%#05x,%s          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, x_label[rs1], x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrwi") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:csrrwi %s,%#05x,%d          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, uimm, x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrsi") == 0) {
			snprintf(linha, sizeof(linha),	
				"0x%08x:csrrsi %s,%#05x,%d          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, uimm, x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "csrrci") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:csrrci %s,%#05x,%d          %s=0x%08x\n",
				pci, x_label[rd], rs1_val, uimm, x_label[rd], x[rd]);
			fputs(linha, output);
		}	
		else if (strcmp(nomeInst, "auipc") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:auipc  %s,0x%05x          %s=0x%08x+0x%08x=0x%08x\n",
				pci, x_label[rd], imm20, x_label[rd], pci, imm20 << 12, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "addi") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:addi   %s,%s,0x%03x         %s=0x%08x+0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF, x_label[rd], rs1_val, (int32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "lui") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lui    %s,0x%05x          %s=0x%08x\n",
				pci, x_label[rd], imm20, x_label[rd], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "add") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:add    %s,%s,%s          %s=0x%08x+0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sll") == 0) {
			uint32_t shift_amount = rs2_val & 0x1F;
			snprintf(linha, sizeof(linha),
				"0x%08x:sll    %s,%s,%s          %s=0x%08x<<%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, shift_amount, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "slli") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:slli   %s,%s,%d          %s=0x%08x<<%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], uimm, x_label[rd], x[rs1], uimm, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "mul") == 0) {
            snprintf(linha, sizeof(linha), 
				"0x%08x:mul    %s,%s,%s            %s=0x%08x*0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha,output);
		}else if (strcmp(nomeInst, "beq") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:beq    %s,%s,0x%03x       (0x%08x==0x%08x)=%d->pc=0x%08x\n",
				pci, x_label[rs1], x_label[rs2], (imm_b >> 1) & 0xFFF,
				x[rs1], x[rs2], (x[rs1] == x[rs2]) ? 1 : 0,
				(x[rs1] == x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "bne") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:bne    %s,%s,0x%03x       (0x%08x!=0x%08x)=%d->pc=0x%08x\n",
				pci, x_label[rs1], x_label[rs2], (imm_b >> 1) & 0xFFF,
				x[rs1], x[rs2], (x[rs1] != x[rs2]) ? 1 : 0,
				(x[rs1] != x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "blt") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:blt    %s,%s,0x%03x       (0x%08x<0x%08x)=%d->pc=0x%08x\n",
				pci, x_label[rs1], x_label[rs2], (uint32_t)(imm_b>>1) & 0xFFF,
				x[rs1], x[rs2], ((int32_t)x[rs1] < (int32_t)x[rs2]) ? 1 : 0,
				((int32_t)x[rs1] < (int32_t)x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "bge") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:bge    %s,%s,0x%03x       (0x%08x>=0x%08x)=%d->pc=0x%08x\n",
				pci, x_label[rs1], x_label[rs2], (imm_b >> 1) & 0xFFF,
				x[rs1], x[rs2], ((int32_t)x[rs1] >= (int32_t)x[rs2]) ? 1 : 0,
				((int32_t)x[rs1] >= (int32_t)x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "bltu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:bltu   %s,%s,0x%03x       (0x%08x<0x%08x)=%d->pc=0x%08x\n",
				pci, x_label[rs1], x_label[rs2], (imm_b >> 1) & 0xFFF,
				x[rs1], x[rs2], (x[rs1] < x[rs2]) ? 1 : 0,
				(x[rs1] < x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "bgeu") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:bgeu   %s,%s,0x%03x       (0x%08x>=0x%08x)=%d->pc=0x%08x\n",
			pci, x_label[rs1], x_label[rs2], (imm_b >> 1) & 0xFFF,
			x[rs1], x[rs2], (x[rs1] >= x[rs2]) ? 1 : 0,
			(x[rs1] >= x[rs2]) ? (pci + imm_b) : (pci + 4));
			fputs(linha, output);
		}else if (strcmp(nomeInst, "lb") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lb     %s,0x%03x(%s)         %s=mem[0x%08x]=0x%08x\n",
				pci, x_label[rd], imm_i, x_label[rs1], x_label[rd], x[rs1] + imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "lh") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lh     %s,0x%03x(%s)         %s=mem[0x%08x]=0x%08x\n",
				pci, x_label[rd], imm_i, x_label[rs1], x_label[rd], x[rs1] + imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "lw") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lw     %s,0x%03x(%s)         %s=mem[0x%08x]=0x%08x\n",
				pci, x_label[rd], imm_i & 0xFFF, x_label[rs1], x_label[rd], x[rs1] + imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "lbu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lbu    %s,0x%03x(%s)         %s=mem[0x%08x]=0x%08x\n",
				pci, x_label[rd], imm_i, x_label[rs1], x_label[rd], x[rs1] + imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "lhu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:lhu    %s,0x%03x(%s)         %s=mem[0x%08x]=0x%08x\n",
				pci, x_label[rd], imm_i, x_label[rs1], x_label[rd], x[rs1] + imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sh") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:sh     %s,0x%03x(%s)        mem[0x%08x]=0x%04x\n",
			pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2] & 0xFFFF);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "slti") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:slti   %s,%s,0x%03x         %s=(0x%08x<0x%08x)=%d\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF,
				x_label[rd], x[rs1], (int32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sltiu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:sltiu  %s,%s,0x%03x         %s=(0x%08x<0x%08x)=%d\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF,
				x_label[rd], x[rs1], (uint32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "xori") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:xori   %s,%s,0x%03x         %s=0x%08x^0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF,
				x_label[rd], x[rs1], (int32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "ori") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:ori    %s,%s,0x%03x         %s=0x%08x|0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF,
				x_label[rd], x[rs1], (int32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "andi") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:andi   %s,%s,0x%03x         %s=0x%08x&0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF,
				x_label[rd], rs1_val, (int32_t)imm_i, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "slli") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:slli   %s,%s,%d          %s=0x%08x<<%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], uimm,
				x_label[rd], x[rs1], uimm, (rs1_val << uimm));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "srli") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:srli   %s,%s,%d          %s=0x%08x>>%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], uimm,
				x_label[rd], rs1_val, uimm, (rs1_val >> uimm));
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "srai") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:srai   %s,%s,%d          %s=0x%08x>>>%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], uimm,
				x_label[rd], rs1_val, uimm, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sltu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:sltu   %s,%s,%s          %s=(0x%08x<0x%08x)=%d\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}else if (strcmp(nomeInst, "mulh") == 0) {
   		 snprintf(linha, sizeof(linha),
        	"0x%08x:mulh   %s,%s,%s            %s=0x%08x*0x%08x=0x%08x\n",
       		pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
   		 fputs(linha, output);
		}
		else if (strcmp(nomeInst, "mulhsu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:mulhsu %s,%s,%s            %s=0x%08x*0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "mulhu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:mulhu  %s,%s,%s            %s=0x%08x*0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "div") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:div    %s,%s,%s            %s=0x%08x/0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "divu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:divu   %s,%s,%s            %s=0x%08x/0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "rem") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:rem    %s,%s,%s            %s=0x%08x%%0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "remu") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:remu   %s,%s,%s            %s=0x%08x%%0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2], x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "srl") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:srl    %s,%s,%s          %s=0x%08x>>%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], x[rs1], x[rs2] & 0x1F, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "slt") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:slt    %s,%s,%s          %s=(0x%08x<0x%08x)=%d\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], (int32_t)rs1_val, (int32_t)rs2_val, x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sra") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:sra    %s,%s,%s          %s=0x%08x>>>%d=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], x[rs1], x[rs2] & 0x1F, x[rd]);
			fputs(linha, output);
		}else if (strcmp(nomeInst, "sub") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:sub    %s,%s,%s          %s=0x%08x-0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], rs1_val, rs2_val, x[rd]);
			fputs(linha, output);
		}else if (strcmp(nomeInst, "xor") == 0) {
		snprintf(linha, sizeof(linha),
			"0x%08x:xor    %s,%s,%s          %s=0x%08x^0x%08x=0x%08x\n",
			pci, x_label[rd], x_label[rs1], x_label[rs2],
			x_label[rd], x[rs1], x[rs2], x[rd]);
		fputs(linha, output);
		}
		else if (strcmp(nomeInst, "or") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:or     %s,%s,%s          %s=0x%08x|0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], x[rs1], x[rs2], x[rd]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "and") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:and    %s,%s,%s          %s=0x%08x&0x%08x=0x%08x\n",
				pci, x_label[rd], x_label[rs1], x_label[rs2],
				x_label[rd], x[rs1], x[rs2], x[rd]);
			fputs(linha, output);
		}

	c = 1;
	temph = pc;
	x[0] = 0;
	prev_mstatus_mie = mstatus_mie;
	}

	fclose(input);
	fclose(output);
	printf("--------------------------------------------------------------------------------\n");
	return 0;
}
