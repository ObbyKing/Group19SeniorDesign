#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct ConditionCodes {
	uint8_t		c:1; // Carry flag
	uint8_t		z:1; // Zero flag
	uint8_t		s:1; // Sign flag
	uint8_t		v:1; // Overflow flag
	uint8_t		d:1; // Decimal adjust flah
	uint8_t		h:1; // Half carry flag
} ConditionCodes;

typedef	struct State8002{
	uint16_t	R0;
	uint16_t	R1;
	uint16_t	R2;
	uint16_t 	R3;
	uint16_t 	R4;
	uint16_t 	R5;
	uint16_t 	R6;
	uint16_t 	R7;
	uint16_t 	R8;
	uint16_t 	R9;
	uint16_t 	R10;
	uint16_t 	R11;
	uint16_t 	R12;
	uint16_t	R13;
	uint16_t 	R14;
	uint16_t 	sp;	// Register 15 is the stack pointer
	uint8_t	pc; // Program counter
	uint16_t 	*memory; // 64k memory space the Z8002 does not use segmented mode
	struct 		ConditionCodes	cc;
} State8002;

int parity(int x, int size){
	int i;
	int p = 0;
	x = (x & ((1<<size)-1));
	for (i=0; i<size; i++){
		if (x & 0x1) p++;
		x = x >> 1;
	}
	return (0 == (p & 0x1));
}

void findByteRegister(uint8_t regToFind){
	switch(regToFind){
		case 0x00: printf("RH0"); break;
		case 0x01: printf("RH1"); break;
		case 0x02: printf("RH2"); break;
		case 0x03: printf("RH3"); break;
		case 0x04: printf("RH4"); break;
		case 0x05: printf("RH5"); break;
		case 0x06: printf("RH6"); break;
		case 0x07: printf("RH7"); break;
		case 0x08: printf("RL0"); break;
		case 0x09: printf("RL1"); break;
		case 0x0a: printf("RL2"); break;
		case 0x0b: printf("RL3"); break;
		case 0x0c: printf("RL4"); break;
		case 0x0d: printf("RL5"); break;
		case 0x0e: printf("RL6"); break;
		case 0x0f: printf("RL7"); break;
		default: printf("WRONG"); break;
	}
	return;
}

uint8_t* returnByteRegisterPointer(uint8_t regToFind, State8002* state){
	uint8_t* bytePointer;
	switch(regToFind){
		case 0x00: bytePointer = &state->R0; bytePointer += 1; break; //Higher
		case 0x01: bytePointer = &state->R1; bytePointer += 1; break;
		case 0x02: bytePointer = &state->R2; bytePointer += 1; break;
		case 0x03: bytePointer = &state->R3; bytePointer += 1; break;
		case 0x04: bytePointer = &state->R4; bytePointer += 1; break;
		case 0x05: bytePointer = &state->R5; bytePointer += 1; break;
		case 0x06: bytePointer = &state->R6; bytePointer += 1; break;
		case 0x07: bytePointer = &state->R7; bytePointer += 1; break;
		case 0x08: &state->R0; break; //Lower
		case 0x09: &state->R1; break;
		case 0x0a: &state->R2; break;
		case 0x0b: &state->R3; break;
		case 0x0c: &state->R4; break;
		case 0x0d: &state->R5; break;
		case 0x0e: &state->R6; break;
		case 0x0f: &state->R7; break;
		default: printf("WRONG"); return bytePointer; break;
	}

	return bytePointer;
}

int Disassemble8002(unsigned short *codebuffer, int pc){
	unsigned short *code = &codebuffer[pc];

	uint8_t upperHalf = code[0] >> 8;
	uint8_t lowerHalf = code[0];
	uint8_t field1 = (code[0] >> 4) & (0x0F);
	uint8_t field2 = code[0] & 0x0F;
	uint8_t opCode = upperHalf & (0x3F);
	uint8_t adMode = upperHalf & (0xC0);

	// printf("%02x \n", upperHalf);
	// printf("%02x \n", lowerHalf);
	// printf("%01x \n", field1);
	// printf("%01x \n", field2);
	// printf("%02x \n", opCode);
	// printf("%01x \n", adMode);

	int opwords = 1;
	printf("%04x ", pc);

	switch (upperHalf)
	{
		case 0x00: switch(field1){ // ADDB
						case 0x00:  printf("ADDB ");			//ADDB Rbd, #data
									findByteRegister(field2);
									printf(", #%02x", code[1]);
									opwords = 2;
									break;
									//print instruction
						default: 	printf("ADDB ");			//ADDB Rbd, @Rs
									findByteRegister(field2);
									printf(", @");
									findByteRegister(field1);
									opwords = 2;
									break;
					} break;
		case 0x12: printf("Gay cam"); break;
		default: printf("What the fuck happened"); break;
	}

	printf("\n");
	return opwords;
}

int Emulate8002(State8002* state){
	unsigned short *opcode = &state->memory[state->pc];

	int done = 0;

	uint8_t upperHalf = opcode[0] >> 8;
	uint8_t lowerHalf = opcode[0];
	uint8_t field1 = (opcode[0] >> 4) & (0x0F);
	uint8_t field2 = opcode[0] & 0x0F;

	Disassemble8002(state->memory, state->pc);

	state->R0 = 0x0134;
	state->R3 = 0x1078;

	state->pc+=1;
	done = 1;

	switch(upperHalf)
	{
		case 0x00: switch(field1){
						case 0x00:	{
									state->pc += 1;
									uint32_t destinationReg = returnByteRegisterPointer(field2, state);
									//uint8_t *destinationReg = returnByteRegisterPointer(field2, state);
									uint8_t* lowerBits = destinationReg+8;
									uint8_t* higherBits = destinationReg;
									printf("%04x\n", destinationReg);
									printf("haha %04x\n", &state->R0);
									printf("%02x\n", higherBits);
									printf("%04x\n", opcode[1]);
									// printf("%04x\n", state->R0);
									// uint8_t* testPointer = &state->R0;
									// *testPointer = 0;
									if(field2 <= 0x7){
										//Higher bits
										*higherBits = (*higherBits + (opcode[1] >> 8));
			
									}
									else{
										//Lower bits
										*lowerBits = (*lowerBits + (opcode[1]));
									}
									break; //ADDB Rbd, #data
								}
						default:    {
									uint8_t* destinationRegs = returnByteRegisterPointer(field2, state);
									uint8_t* sourceRegs = returnByteRegisterPointer(field1, state);
									// printf("%02x\n", destinationRegs);
									// printf("%02x\n", sourceRegs);
									// printf("%04x\n", &state->R0);
									// printf("%04x\n", &state->R3);
									// printf("%02x\n", *(destinationRegs));
									// printf("%02x\n", *(sourceRegs));
									*destinationRegs = *destinationRegs + *sourceRegs;
								 	break;
								 }
				} break;
		case 0x12: break;
		default: printf("dicks"); break;
	}

	//printf("\t");
	printf("R0 %04x\n", state->R0);
	return done;
}

void ReadFileIntoMemoryAt(State8002* state, char* filename, uint32_t offset){
	FILE *f= fopen(filename, "rb");
	if(f==NULL){
		printf("error: Couldn't open %s\n", filename);
		exit(1);
	}
	fseek(f, 0L, SEEK_END);
	int fsize = ftell(f);
	fseek(f, 0L, SEEK_SET);


	uint8_t *buffer = malloc(0x10000);
	fread(buffer, fsize, 1, f);
	fclose(f);
	uint16_t *memptr = &state->memory[offset];

	for(uint16_t i = 0; i < fsize/2; i++){
		memptr[i] = buffer[i*2+1] + (buffer[i*2] << 8);
	}
}

State8002* Init8002(void){
	State8002* state = calloc(1, sizeof(State8002));
	state->memory = malloc(0x10000); // 64k
	return state;
}

int main (int argc, char**argv){
	int done = 0;
	State8002* state = Init8002();

	ReadFileIntoMemoryAt(state, "test.t", 0);

	while (done == 0){
		done = Emulate8002(state);
	}
	return 0;
}
