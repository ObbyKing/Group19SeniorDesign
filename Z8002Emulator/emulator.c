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
	uint16_t	pc; // Program counter
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

int Disassemble8002(unsigned char *codebuffer, int pc){
	unsigned char *code = &codebuffer[pc];
	unsigned char upperEightBits = *code >> 8;
	unsigned char lowerEightBits = *code;
	unsigned char upperFourBits = lowerEightBits >> 4;
	unsigned char lowerFourBits = lowerEightBits & 0x0F;
	unsigned int test = code[1];
	
	//printf("%c ", test);
	printf("%02x ", code[0]);

	//printf(&codebuffer[pc]);
	
	int opwords = 1;
	printf("%04x ", pc);
	switch (*code)
	{
		case 0x00: switch(upperFourBits){
						case 0x00: printf("NOP");//figure out destination register
									//print instruction
						default: printf("How tf did we get here");//figure out source and desination register
									//print instruction
					}
		default: printf("What the fuck happened");
	}
	
	printf("\n");
	return opwords;
}

int Emulate8002(State8002* state){
	Disassemble8002(state->memory, state->pc);
	return 1;
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

	uint8_t *buffer = &state->memory[offset];
	fread(buffer, fsize, 1, f);
	fclose(f);
}

State8002* Init8002(void){
	State8002* state = calloc(1, sizeof(State8002));
	state->memory = malloc(0x10000); // 64k
	return state;
}

int main (int argc, char**argv){
	int done = 0;
	State8002* state = Init8002();

	ReadFileIntoMemoryAt(state, "test.h", 0);

	while (done == 0){
		done = Emulate8002(state);
	}
	return 0;
}