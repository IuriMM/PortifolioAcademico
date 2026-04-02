#include <stdint.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#define MEM_SIZE (32 * 1024)

uint32_t mstatus = 0, mepc = 0, mcause = 0, mtvec = 0,mtval = 0;

typedef enum {
    INSTRUCTION_ACCESS_FAULT = 1,
    ILLEGAL_INSTRUCTION = 2,
    LOAD_ACCESS_FAULT = 5,
    STORE_ACCESS_FAULT = 7,
    ENV_CALL_MMODE = 11
} ExceptionCode;

void csrw(uint32_t *csr, uint32_t value) { *csr = value; }
uint32_t csrr(uint32_t *csr) { return *csr; }

void trigger_exception(uint32_t cause, uint32_t tval, uint32_t* pc) {

    mepc = *pc;

    // 3. Registrar a causa da exceção [cite: 512]
    mcause = cause;

    // 4. Salvar informação extra sobre a falha (endereço ou instrução) [cite: 345, 353]
    mtval = tval;
    
    // 5. Calcular o novo PC, pulando para a rotina de tratamento definida em mtvec [cite: 510]
    //    Por simplicidade, usaremos o modo direto (MODE = 00) [cite: 339]
    *pc = mtvec; 
}

void ecall(uint32_t* pc) {
    trigger_exception(ENV_CALL_MMODE, 0, pc);
}

void ecall(uint32_t pc) {
    handle_exception(11, pc); 
}

uint32_t mret() {
    return mepc; 
}

int main(int argc, char* argv[]) {

	printf("--------------------------------------------------------------------------------\n");

	for(uint32_t i = 0; i < argc; i++) {

		printf("argv[%i] = %s\n", i, argv[i]);
	}
	const uint32_t offset = 0x80000000;

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
			//printf("%02X ", byte); 
			
			endereço++;
			p += 2;  
		}
	}
	
	printf("\n--------------------------------------------------------------------------------\n");

	uint8_t run = 1;
	int c = 0;

	while(run) {

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
		
		c? printf("> 0x%02x %s\n",temph - 4,nomeInst):printf("> 0x%02x %s\n",pci,nomeInst);
		printf("--------------------------------------------------------------------------------\n");
		for (int r = 0; r < 32; r += 4){
    	printf("|%6s=0x%08x|%6s=0x%08x|%6s=0x%08x|%6s=0x%08x|",
        x_label[r],   x[r],
        x_label[r+1], x[r+1],
        x_label[r+2], x[r+2],
        x_label[r+3], x[r+3]);
		printf("\n");
		}
		printf("--------------------------------------------------------------------------------\n");

		char linha[256];

		switch(opcode) {
			case 0x01: // Instruction access fault
					handle_exception(INSTRUCTION_ACCESS_FAULT, pc);
					break;
				case 0x02: // Illegal instruction
					handle_exception(ILLEGAL_INSTRUCTION, pc);
					break;
				case 0x05: // Load access fault
					handle_exception(LOAD_ACCESS_FAULT, pc);
					break;
				case 0x07: // Store access fault
					handle_exception(STORE_ACCESS_FAULT, pc);
					break;
				case 0x0B: // ECALL (Environment call from M-mode)
					ecall(pc);
					break;
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
					if ((addr - offset) >= MEM_SIZE) { fprintf(stderr, "LB inválido: 0x%08x\n", addr); exit(1); }
					int8_t val = *(int8_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b001) { // lh
					strncpy(nomeInst, "lh", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 1) >= MEM_SIZE) { fprintf(stderr, "LH inválido: 0x%08x\n", addr); exit(1); }
					int16_t val = *(int16_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b010) { // lw
					strncpy(nomeInst, "lw", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 3) >= MEM_SIZE) { fprintf(stderr, "LW inválido: 0x%08x\n", addr); exit(1); }
					int32_t val = *(int32_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b100) { // lbu
					strncpy(nomeInst, "lbu", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset) >= MEM_SIZE) { fprintf(stderr, "LBU inválido: 0x%08x\n", addr); exit(1); }
					uint8_t val = *(uint8_t*)(mem + (addr - offset));
					x[rd] = val;
				} else if (funct3 == 0b101) { // lhu
					strncpy(nomeInst, "lhu", sizeof(nomeInst));
					int32_t addr = x[rs1] + imm_i;
					if ((addr - offset + 1) >= MEM_SIZE) { fprintf(stderr, "LHU inválido: 0x%08x\n", addr); exit(1); }
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

				// Breaking case
				break;
			case 0b1110011:
				// ebreak (funct3 == 000 and imm == 1)
				if(funct3 == 0b000 && imm_i == 1) {
					// Outputting instruction to console
					strncpy(nomeInst,"ebreak",sizeof(nomeInst));
					printf("> 0x%02x %s\n",temph,nomeInst);
					// Retrieving previous and next instructions
					const uint32_t previous = ((uint32_t*)(mem))[(pc - 4 - offset) >> 2];
					const uint32_t next = ((uint32_t*)(mem))[(pc + 4 - offset) >> 2];
					// Halting condition
            		if(previous == 0x01f01013 && next == 0x40705013) run = 0;
				}
				pc = pci + 4;
				// Breaking case
				break;
			case 0b1101111:
				// J type (1101111)
				//jal
    			strncpy(nomeInst, "jal", sizeof(nomeInst));
				if (rd != 0) x[rd] = pci + 4;
				// O imediato já está sinalizado em imm_j
				pc = pci + imm_j;
				temph = pc + 4;
				break;
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

				if(funct3 == 0b000){
					//sb
					strncpy(nomeInst,"sb",sizeof(nomeInst));
					uint32_t addr = x[rs1] + imm_s;
					if ((addr - offset) >= MEM_SIZE) {
   					 fprintf(stderr, "Memória inválida em SB: 0x%08x\n", addr);
    				exit(1);
					}					
					mem[addr - offset] = (uint8_t)(x[rs2] & 0b11111111);
				}
				else if(funct3 == 0b001){ 
					// sh
					strncpy(nomeInst,"sh",sizeof(nomeInst));
					uint32_t addr = x[rs1] + imm_s;
					printf("SH: rs1=%d (0x%08x), imm=%d, addr=0x%08x\n", rs1, x[rs1], imm_s, addr);
					if ((addr < offset) || (addr - offset + sizeof(uint16_t) > MEM_SIZE)) {
						fprintf(stderr, "Endereço inválido em SH: 0x%08x, pc: 0x%x", addr,pc);
						exit(1);
					}
					uint16_t value = (uint16_t)(x[rs2] & 0xFFFF);
					memcpy(mem + (addr - offset), &value, sizeof(uint16_t));
				}
				else if(funct3 == 0b010){
					//sw
					strncpy(nomeInst,"sw",sizeof(nomeInst));
					uint32_t addr = x[rs1] + imm_s;
					if ((addr < offset) || (addr - offset + sizeof(uint32_t) > MEM_SIZE)) {
						fprintf(stderr, "Endereço inválido em SW: 0x%08x, pc: 0x%x\n", addr,pc);
						exit(1);
					}
					*(uint32_t*)(mem + (addr - offset)) = x[rs2];
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
			    pc = (x[rs1] + (int32_t)imm_i) & ~1;
				if ((pc < offset) || (pc - offset + 3) >= MEM_SIZE) {
					fprintf(stderr, "JALR para endereço inválido: 0x%08x\n", pc);
					printf("JALR: rs1=%s (0x%08x), imm_i=%d, pc=0x%08x\n", x_label[rs1], x[rs1], imm_i, pc);
					exit(1);
				}
			    if (rd != 0) x[rd] = temp;
				break;
			}
			default:
				// Unknown
				// Outputting error message
				printf("error: unknown instruction, opcode at pc = 0x%08x opcode = 0x%x instruction = 0x%x\n", pc, opcode, instruction);
				handle_exception(ILLEGAL_INSTRUCTION, pc);
				// Halting simulation
		}
	
		//Escrita no arquivo .out
		if (strcmp(nomeInst, "jal") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:jal    %s,%#07x          pc=0x%08x,%s=0x%08x\n",
			pci, x_label[rd], (int32_t)((imm_j >> 1) & 0xFFFFF), pci + imm_j, x_label[rd], pci + 4);
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
		else if (strcmp(nomeInst, "sw") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:sw     %s,0x%03x(%s)        mem[0x%08x]=0x%08x\n",
				pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2]);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "jalr") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:jalr   %s,%s,0x%03x       pc=0x%08x+0x%08x,%s=0x%08x\n",
				pci, x_label[rd], x_label[rs1], imm_i & 0xFFF, x[rs1], (int32_t)imm_i, x_label[rd], pci + 4);
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
		else if (strcmp(nomeInst, "ebreak") == 0) {
			snprintf(linha, sizeof(linha),
				"0x%08x:ebreak\n", pci);
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
		}else if (strcmp(nomeInst, "sb") == 0) {
   			 snprintf(linha, sizeof(linha),
        	"0x%08x:sb     %s,0x%03x(%s)        mem[0x%08x]=0x%02x\n",
        	pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2] & 0xFF);
    		fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sh") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:sh     %s,0x%03x(%s)        mem[0x%08x]=0x%04x\n",
			pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2] & 0xFFFF);
			fputs(linha, output);
		}
		else if (strcmp(nomeInst, "sw") == 0) {
			snprintf(linha, sizeof(linha),
			"0x%08x:sw     %s,0x%03x(%s)        mem[0x%08x]=0x%08x\n",
			pci, x_label[rs2], imm_s & 0xFFF, x_label[rs1], x[rs1] + imm_s, x[rs2]);
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
		else {
			// fallback para instruções não tratadas
			snprintf(linha, sizeof(linha),
			"0x%08x:%s\n", pc, nomeInst);
			fputs(linha, output);
		}

	c = 1;
	temph = pc;
	x[0] = 0;
	}

	fclose(input);
	fclose(output);
	printf("--------------------------------------------------------------------------------\n");
	return 0;
}