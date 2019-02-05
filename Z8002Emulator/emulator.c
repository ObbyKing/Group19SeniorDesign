#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
//#include <winsock2.h>


typedef int bool;
#define true 1
#define false 0

uint8_t fix_8(uint8_t i){
	return i;
}
uint16_t fix_16(uint16_t i){
	return htons(i);
}
uint32_t fix_32(uint32_t i){
	return htonl(i);
}
uint64_t fix_64(uint64_t i){
	uint64_t upperHalf = (i & 0xFFFFFFFF00000000) >> 32;
	uint64_t lowerHalf = i & 0x00000000FFFFFFFF;
	return ((uint64_t)htonl(lowerHalf) << 32) + (uint64_t)htonl(upperHalf);
}




typedef struct ConditionCodes {
	uint8_t		c; // Carry flag
	uint8_t		z; // Zero flag
	uint8_t		s; // Sign flag
	uint8_t		v; // Overflow flag
	uint8_t		d; // Decimal adjust flah
	uint8_t		h; // Half carry flag
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
	struct ConditionCodes cc;

	uint16_t 	*dataSpaceNM; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*dataSpaceSM; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*instructionSpaceNM; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*instructionSpaceSM; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*stackNM; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*stackSM; // 64k memory space the Z8002 does not use segmented mode

	uint16_t 	*memory; // 64k memory space the Z8002 does not use segmented mode
	uint16_t 	*memory2; // 64k memory space the Z8002 does not use segmented mode
	uint8_t status;

} State8002;

bool checkConditionCode(uint8_t cc,ConditionCodes o){
	cc = cc & 0x0F;
	switch (cc) {
		case 0b0000: return false; break;
		case 0b0001: return (o.s ^ o.v); break;
		case 0b0010: return (o.z || (o.s ^ o.v)); break;
		case 0b0011: return (o.c || o.z); break;
		case 0b0100: return o.v == 1; break;
		case 0b0101: return o.s == 1; break;
		case 0b0110: return o.z == 1; break;
		case 0b0111: return o.c == 1; break;
		case 0b1000: return true; break;
		case 0b1001: return !(o.s ^ o.v); break;
		case 0b1010: return !(o.z || (o.s ^ o.v)); break;
		case 0b1011: return !(o.c || o.z); break;
		case 0b1100: return o.v == 0; break;
		case 0b1101: return o.s == 0; break;
		case 0b1110: return o.z == 0; break;
		case 0b1111: return o.c == 0; break;
	}
}

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

void findregRegister(uint8_t regToFind){
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
		default: printf("Couldn't find reg register"); break;
	}
	return;
}

void findRegister(uint8_t regToFind){
	switch(regToFind){
		case 0x00: printf("R0"); break;
		case 0x01: printf("R1"); break;
		case 0x02: printf("R2"); break;
		case 0x03: printf("R3"); break;
		case 0x04: printf("R4"); break;
		case 0x05: printf("R5"); break;
		case 0x06: printf("R6"); break;
		case 0x07: printf("R7"); break;
		case 0x08: printf("R8"); break;
		case 0x09: printf("R9"); break;
		case 0x0a: printf("R10"); break;
		case 0x0b: printf("R11"); break;
		case 0x0c: printf("R12"); break;
		case 0x0d: printf("R13"); break;
		case 0x0e: printf("R14"); break;
		default: printf("Couldn't find register"); break;
	}
	return;
}

void findLongRegister(uint8_t regToFind){
	switch(regToFind){
		case 0x00: printf("RR0"); break;
		case 0x02: printf("RR2"); break;
		case 0x04: printf("RR4"); break;
		case 0x06: printf("RR6"); break;
		case 0x08: printf("RR8"); break;
		case 0x0a: printf("RR10"); break;
		case 0x0c: printf("RR12"); break;
		case 0x0e: printf("RR14"); break;
		default: printf("Couldn't find long register"); break;
	}
	return;
}

void findQuadRegister(uint8_t regToFind){
	switch(regToFind){
		case 0x00: printf("RQ0"); break;
		case 0x04: printf("RQ4"); break;
		case 0x08: printf("RQ8"); break;
		case 0x0c: printf("RQ12"); break;
		default: printf("Couldn't find quad register"); break;
	}
	return;
}


uint8_t* returnByteRegisterPointer(uint8_t regToFind, State8002* state){
	uint8_t* regPointer;
	switch(regToFind){
		case 0x00: regPointer = &state->R0; regPointer += 1; break; //Higher
		case 0x01: regPointer = &state->R1; regPointer += 1; break;
		case 0x02: regPointer = &state->R2; regPointer += 1; break;
		case 0x03: regPointer = &state->R3; regPointer += 1; break;
		case 0x04: regPointer = &state->R4; regPointer += 1; break;
		case 0x05: regPointer = &state->R5; regPointer += 1; break;
		case 0x06: regPointer = &state->R6; regPointer += 1; break;
		case 0x07: regPointer = &state->R7; regPointer += 1; break;
		case 0x08: regPointer = &state->R0;  break; // Lower
		case 0x09: regPointer = &state->R1;  break;
		case 0x0a: regPointer = &state->R2;  break;
		case 0x0b: regPointer = &state->R3;  break;
		case 0x0c: regPointer = &state->R4;  break;
		case 0x0d: regPointer = &state->R5;  break;
		case 0x0e: regPointer = &state->R6;  break;
		case 0x0f: regPointer = &state->R7;  break;
		default: printf("WRONG"); return regPointer; break;
	}
  return regPointer;
}
uint16_t* returnWordRegisterPointer(uint8_t regToFind, State8002* state){
  	uint16_t* regPointer;
  	switch(regToFind){
  		case 0x00: regPointer = &state->R0; break;
  		case 0x01: regPointer = &state->R1; break;
  		case 0x02: regPointer = &state->R2; break;
  		case 0x03: regPointer = &state->R3; break;
  		case 0x04: regPointer = &state->R4; break;
  		case 0x05: regPointer = &state->R5; break;
  		case 0x06: regPointer = &state->R6; break;
  		case 0x07: regPointer = &state->R7; break;
  		case 0x08: regPointer = &state->R8; break;
  		case 0x09: regPointer = &state->R9; break;
  		case 0x0a: regPointer = &state->R10; break;
  		case 0x0b: regPointer = &state->R11; break;
  		case 0x0c: regPointer = &state->R12; break;
  		case 0x0d: regPointer = &state->R13; break;
  		case 0x0e: regPointer = &state->R14; break;
  		case 0x0f: regPointer = &state->sp; break;
  		default: printf("WRONG"); return regPointer; break;
  	}

	return regPointer;
}
uint32_t* returnLongRegisterPointer(uint8_t regToFind, State8002* state){
  	uint32_t* regPointer;
  	switch(regToFind){
  		case 0x00: regPointer = &state->R0; break;
  		case 0x01: printf("Invalid address for long register"); return NULL; break;
  		case 0x02: regPointer = &state->R2; break;
  		case 0x03: printf("Invalid address for long register"); return NULL; break;
  		case 0x04: regPointer = &state->R4; break;
  		case 0x05: printf("Invalid address for long register"); return NULL; break;
  		case 0x06: regPointer = &state->R6; break;
  		case 0x07: printf("Invalid address for long register"); return NULL; break;
  		case 0x08: regPointer = &state->R8; break;
  		case 0x09: printf("Invalid address for long register"); return NULL; break;
  		case 0x0a: regPointer = &state->R10; break;
  		case 0x0b: printf("Invalid address for long register"); return NULL; break;
  		case 0x0c: regPointer = &state->R12; break;
  		case 0x0d: printf("Invalid address for long register"); return NULL; break;
  		case 0x0e: regPointer = &state->R14; break;
  		case 0x0f: printf("Invalid address for long register"); return NULL; break;
  		default: printf("WRONG"); return regPointer; break;
  	}

	return regPointer;
}

uint64_t* returnQuadRegisterPointer(uint8_t regToFind, State8002* state){
	uint64_t* regPointer;
	switch(regToFind){
		case 0x00: regPointer = &state->R0; break;
		case 0x01: printf("Invalid address for quad register");	return NULL; break;
		case 0x02: printf("Invalid address for quad register");	return NULL; break;
		case 0x03: printf("Invalid address for quad register");	return NULL; break;
		case 0x04: regPointer = &state->R4; break;
		case 0x05: printf("Invalid address for quad register");	return NULL; break;
		case 0x06: printf("Invalid address for quad register");	return NULL; break;
		case 0x07: printf("Invalid address for quad register");	return NULL; break;
		case 0x08: regPointer = &state->R8; break;
		case 0x09: printf("Invalid address for quad register");	return NULL; break;
		case 0x0a: printf("Invalid address for quad register");	return NULL; break;
		case 0x0b: printf("Invalid address for quad register");	return NULL; break;
		case 0x0c: regPointer = &state->R12; break;
		case 0x0d: printf("Invalid address for quad register");	return NULL; break;
		case 0x0e: printf("Invalid address for quad register");	return NULL; break;
		case 0x0f: printf("Invalid address for quad register");	return NULL; break;
		default: printf("WRONG"); return NULL; break;
	}

	return regPointer;
}


uint8_t readReg8(uint8_t regCode,State8002* state){
	return fix_8(*returnByteRegisterPointer(regCode, state));
}
uint16_t readReg16(uint8_t regCode,State8002* state){
	return fix_16(*returnWordRegisterPointer(regCode, state));
}
uint32_t readReg32(uint8_t regCode,State8002* state){
	return fix_32(*returnLongRegisterPointer(regCode, state));
}

uint64_t readReg64(uint8_t regCode,State8002* state){
	return fix_64(*returnQuadRegisterPointer(regCode, state));
}

void writeReg8(uint8_t regCode, State8002* state, uint8_t data){
	*returnByteRegisterPointer(regCode, state) = data;
}
void writeReg16(uint8_t regCode, State8002* state, uint16_t data){
	*returnWordRegisterPointer(regCode, state) = fix_16(data);
}
void writeReg32(uint8_t regCode, State8002* state, uint32_t data){
	*returnLongRegisterPointer(regCode, state) = fix_32(data);
}

void writeReg64(uint8_t regCode, State8002* state, uint64_t data){
	*returnQuadRegisterPointer(regCode, state) = data;
}

void writeZBus(uint16_t address, uint16_t data, State8002* state){
	uint8_t status = state->status;
	if (status = 0b1000) { // data space normal mode
		uint16_t* actualAddress = &state->dataSpaceNM + address;
		*actualAddress = data;
	}
	if (status = 0b1010) { // data space normal mode
		uint16_t* actualAddress = &state->dataSpaceSM + address;
		*actualAddress = data;
	}
}

uint16_t readZBus(uint16_t address, uint16_t data, State8002* state){
	uint8_t status = state->status;
	if (status = 0b1000) { // data space normal mode
		uint16_t* actualAddress = &state->dataSpaceNM + address;
		*actualAddress = data;
	}
	if (status = 0b1010) { // data space normal mode
		uint16_t* actualAddress = &state->dataSpaceSM + address;
		*actualAddress = data;
	}
}

int Disassemble8002(uint16_t *codebuffer, int pc){
	uint16_t *code = &codebuffer[pc/2];

	uint8_t upperHalf = code[0] >> 8;
	uint8_t lowerHalf = code[0];
	uint8_t field1 = (code[0] >> 4) & (0x0F);
	uint8_t field2 = code[0] & 0x0F;
	uint8_t opcode = upperHalf & (0x3F);
	uint8_t adMode = upperHalf & (0xC0);
	uint8_t	upperTwo = code[0] >> 14;
	uint8_t upperFour = code[0] >> 12;
	uint8_t secondNibble = upperHalf & 0x0F;


	 // printf("%02x \n", upperHalf);
	 // printf("%02x \n", lowerHalf);
	// printf("%01x \n", field1);
	// printf("%01x \n", field2);
	// printf("%02x \n", opcode);
	// printf("%01x \n", adMode);

	int opwords = 1;

	printf("%04x ", pc);
	switch (upperTwo){
		case 0x3:	switch(upperFour){
						case 0xc:	printf("LDB ");								//LDB Rbd, #data
									findregRegister(field1);
									printf(", #%02x", lowerHalf);
									break;
						case 0xd:	printf("CALR %04x", (code[0] & 0x0FFF));		//CALR address
									break;
						case 0xe:	printf("JR %01x, ", secondNibble);			//JR cc, address
									uint16_t addr = pc + (lowerHalf * 2);
									printf("%02x\t", addr);
									break;
						case 0xf:	switch((field1 & 0x8)){
										case 0x0:	printf("DBJNZ ");			//DBJNZ Rb, address
													findRegister(secondNibble);
													printf(", %02x", lowerHalf & 0x7F);
													break;
										case 0x1:	printf("DJNZ ");			//DJNZ R, address
													break;
										default:	("NOP from DJNZ");
													break;
									} break;
						default:	printf("I'm here now!"); break;
					} break;
		default:	switch (upperHalf){
						case 0x00: switch(field1){ // ADDB
										case 0x00:  printf("ADDB ");			//ADDB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
													//print instruction
										default: 	printf("ADDB ");			//ADDB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x01: switch(field1){
										case 0x00:  printf("ADD  ");				//ADD Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("ADD  ");				//ADD Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;

									} break;
						case 0x02:	switch(field1){
										case 0x00:	printf("SUBB ");			//SUBB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("SUBB ");			//SUBB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x03:	switch(field1){
										case 0x00:	printf("SUB ");				//SUB Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("SUB ");				//SUB Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;
									} break;

						case 0x04:	switch(field1){
										case 0x00:	printf("ORB ");				//ORB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("ORB ");				//ORB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x05:	switch(field1){
										case 0x00: 	printf("OR ");				//OR Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("OR ");				//OR Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;
									} break;
						case 0x06:	switch(field1){
										case 0x00:  printf("ANDB ");			//ANDB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("ANDB ");			//ANDB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x07:	switch(field1){
										case 0x00:	printf("AND ");				//AND Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("AND ");				//AND Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;
									} break;
						case 0x08:	switch(field1){
										case 0x00:  printf("XORB ");			//XORB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("XORB ");			//XORB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x09:	switch(field1){
										case 0x00:  printf("XOR ");				//XOR Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("XOR ");				//XOR Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;

									} break;
						case 0x0a:	switch(field1){
										case 0x00:  printf("CPB "); 			//CPB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("CPB ");				//CPB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;
						case 0x0b:	switch(field1){
										case 0x00:	printf("CP ");				//CP Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("CP ");				//CP Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;
									} break;
						case 0x0c:	switch(field2){
										case 0x00:	printf("COMB @");			//COMB @Rd
													findregRegister(field1);
													break;
										case 0x01:	printf("CPB @");			//CPB @Rd, #data
													findregRegister(field1);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										case 0x02:	printf("NEGB @");			//NEGB @Rd
													findregRegister(field1);
													break;
										case 0x04:	printf("TESTB @");			//TESTB @Rd
													findregRegister(field1);
													break;
										case 0x05:	printf("LDB @");			//LDB @Rd, #data
													findregRegister(field1);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										case 0x06:	printf("TSETB @");			//TSETB @Rd
													findregRegister(field1);
													break;
										case 0x08:	printf("CLRB @");			//CLRB @Rd
													findregRegister(field1);
													break;
										default:	printf("Whups!");
									} break;
						case 0x0d:	switch(field2){
										case 0x00:	printf("COM @");			//COM @Rd
													findRegister(field1);
													break;
										case 0x01:	printf("CP @");				//CP @Rd, #data
													findRegister(field1);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										case 0x02:	printf("NEG @");			//NEG @Rd
													findRegister(field1);
													break;
										case 0x04:	printf("TEST @");			//TEST @Rd
													findRegister(field1);
													break;
										case 0x05:	printf("LD @");				//LD @Rd, #data
													findRegister(field1);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										case 0x06:	printf("TSET @");			//TSET @Rd
													findRegister(field1);
													break;
										case 0x08:	printf("CLR @");			//CLR @Rd
													findRegister(field1);
													break;
										case 0x09:	printf("PUSH @");			//PUSH @Rd, #data
													findRegister(field1);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
									} break;
						case 0x0e:	printf("NOP"); break;
						case 0x0f:	printf("Hardware Instruction, Load from EPU, Opcode: 0x0F"); break;

						case 0x10:	switch(field1){
										case 0x00:	printf("CPL ");				//CPL RRd, #data
													findLongRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("CPL ");				//CPL RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;

						case 0x11:	printf("PUSHL @");							//PUSHL @Rd, @Rs
									findLongRegister(field1);
									printf(", @");
									findLongRegister(field2);
									break;
						case 0x12: 	switch(field1){
										case 0x00:	printf("SUBL ");			//SUBL RRd, #data
													findLongRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("SUBL ");			//SUBL RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;
						case 0x13:	printf("PUSH @");							//PUSH @Rd, @Rs
									findRegister(field1);
									printf(", @");
									findRegister(field2);
									break;
						case 0x14:  switch(field1){
										case 0x00:	printf("LDL ");				//LDL RRd, #data
													findLongRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("LDL ");				//LDL RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;
						case 0x15:	printf("POPL @");							//POPL @Rd, @Rs
									findLongRegister(field2);
									printf(", @");
									findLongRegister(field1);
									break;
						case 0x16:	switch(field1){
										case 0x00:	printf("ADDL ");			//ADDL RRd, #data
													findLongRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("ADDL ");			//ADDL RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;
						case 0x17:	printf("POP @");							//POP @Rd, @Rs
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;
						case 0x18:	switch(field1){
										case 0x00:	printf("MULTL ");			//MULTL RQd, #data
													findQuadRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("MULTL ");			//MULTL RQd, @Rs
													findQuadRegister(field2);
													printf(", @");
													findQuadRegister(field1);
													break;
									} break;
						case 0x19:	switch(field1){
										case 0x00:	printf("MULT ");			//MULT RRd, #data
													findLongRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("MULT ");			//MULT RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;
						case 0x1a:	switch(field1){
										case 0x00:	printf("DIVL ");			//DIVL RQd, #data
													findQuadRegister(field2);
													printf(", #%02x%02x", code[1], code[2]);
													opwords = 3;
													break;
										default:	printf("DIVL ");			//DIVL RQd, @Rs
													findQuadRegister(field2);
													printf(", @");
													findQuadRegister(field1);
													break;
									} break;
						case 0x1b:	switch(field1){
										case 0x00:	printf("DIV ");				//DIV RRd, #data
													findLongRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("DIV ");				//DIV RRd, @Rs
													findLongRegister(field2);
													printf(", @");
													findLongRegister(field1);
													break;
									} break;
						case 0x1c:	printf("TODO: Gotta implement");
						case 0x1d:	printf("LDL @");							//LDL @Rd, RRs
									findLongRegister(field1);
									printf(", ");
									findLongRegister(field2);
									break;
						case 0x1e:	printf("JP ");								//JP cc, @Rd
									printf("%02x", field2);
									printf(", @");
									findRegister(field1);
									break;
						case 0x1f:	printf("CALL @");
									findRegister(field1);
									break;

						case 0x20:	switch(field1){
										case 0x00:	printf("LDB ");				//LDB Rbd, #data
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
										default:	printf("LDB ");				//LDB Rbd, @Rs
													findregRegister(field2);
													printf(", @");
													findregRegister(field1);
													break;
									} break;

						case 0x21:	switch(field1){
										case 0x00:	printf("LD ");				//LD Rd, #data
													findRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("LD ");				//LD Rd, @Rs
													findRegister(field2);
													printf(", @");
													findRegister(field1);
													break;
									} break;
						case 0x22:	switch(field1){
										case 0x00:	printf("RESB ");				//RESB Rbd, Rs
													findregRegister(code[1] >> 8);
													printf(", ");
													findRegister(field2);
													opwords = 2;
													break;
										default:	printf("RESB @");				//RESB @Rd, #b
													findRegister(field2);
													printf(", #");
													findRegister(field1);
													break;
									}	break;
						case 0x23:	switch(field1){
										case 0x00:	printf("RES ");				//RES Rd, Rs
													findRegister(field2);
													printf(", ");
													printf(", #%02x", code[1]);
													opwords = 2;
													break;
										default:	printf("RES @");				//RES @Rd, #b
													findRegister(field1);
													printf(", #");
													findRegister(field1);
													break;
									}	break;
						case 0x24:	switch(field1){
										case 0x00:	printf("SETB @");			//SETB @Rbd, Rs
													findregRegister(code[1]>>8);
													printf(", ");
													findRegister(field2);
													break;
										default:	printf("SETB @");			//SETB @Rd, #b
													findregRegister(field1);
													printf(", #%02x", field2);
													break;
									} break;
						case 0x25:	switch(field1){
										case 0x00:	printf("SET ");				//SET Rd, Rs
													findRegister(code[1]>>8);
													printf(", ");
													findRegister(field2);
													break;
										default:	printf("SET @");			//SET @Rd, #b
													findRegister(field1);
													printf(", #%02x", field2);
													break;
									} break;
						case 0x26:	switch(field1){
										case 0x00:	printf("BITB ");			//BITB Rbd, Rs
													findRegister(code[1]>>8);
													printf(", ");
													findRegister(field2);
													break;
										default:	printf("BITB @");			//BITB @Rd, #b
													findregRegister(field1);
													printf(", #%02x", field2);
													break;
									} break;
						case 0x27:	switch(field1){
										case 0x00:	printf("BIT ");				//BIT Rd, Rs
													findRegister(code[1]>>8);
													printf(", ");
													findRegister(field2);
													break;
										default:	printf("BIT @");			//BIT @Rd, #b
													findRegister(field1);
													printf(", #%02x", field2);
													break;
									} break;
						case 0x28:	printf("INCB @");							//INCB @Rd, #n
									findregRegister(field1);
									printf(", #%01x", field2);
									break;
						case 0x29:	printf("INC @");							//INC @Rd, #n
									findRegister(field1);
									printf(", #%01x", field2);
									break;
						case 0x2a:	printf("DECB @");							//DECB @Rd, #n
									findregRegister(field1);
									printf(", #%01x", field2);
									break;
						case 0x2b:	printf("DEC @");							//DEC @Rd, #n
									findRegister(field1);
									printf(", #%01x", field2);
									break;
						case 0x2c:	printf("EXB ");								//EXB Rd, @Rs
									findregRegister(field2);
									printf(", @");
									findregRegister(field1);
									break;
						case 0x2d:	printf("EX ");								//EX Rd, @Rs
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;
						case 0x2e:	printf("LDB @");							//LDB @Rd, Rbs
									findregRegister(field1);
									printf(", ");
									findregRegister(field2);
									break;
						case 0x2f:	printf("LD @");								//LD @Rd, Rs
									findRegister(field1);
									printf(", ");
									findRegister(field2);
									break;
						case 0x30:	switch(field1){
										case 0x00:	printf("LRRB ");			//LDRB Rbd, address
													findregRegister(field2);
													printf(", #%02x", code[1]);
													opwords = 2;
													break;

										default:	printf("LDB ");				//LDB Rbd, Rs(#disp)
													findregRegister(field2);
													printf(", (");
													findregRegister(field1);
													printf(", #%02x", code[1]);
													printf(", )");
													opwords = 2;
													break;

									} break;
						case 0x31:	switch(field1){
										case 0x00:	printf("LDR ");				//LDR Rd, address
													findRegister(field2);
													printf(", @%04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LD ");				//LD Rd, Rs(#disp)
													findRegister(field2);
													printf(", ");
													findRegister(field1);
													printf("(#%04x)", code[1]);
													opwords = 2;
													break;
									} break;
						case 0x32:	switch(field1){
										case 0x00:	printf("LDRB %04x, ", code[1]);	//LDRB address, Rbs
													findregRegister(field2);
													opwords = 2;
													break;
										default:	printf("LDB ");				//LDB Rd(#disp), Rbs
													findregRegister(field1);
													printf("(#%04x), ", code[1]);
													findregRegister(field2);
													opwords = 2;
													break;
									} break;
						case 0x33:	switch(field1){
										case 0x00:	printf("LDR %04x, ", code[1]); //LDR address, Rs
													findRegister(field2);
													opwords = 2;
													break;
										default:	printf("LD ");				//LD Rd(#disp), Rbs
													findRegister(field1);
													printf("(#%04x), ", code[1]);
													findRegister(field2);
													opwords = 2;
													break;
									} break;
						case 0x34:	switch(field1){
										case 0x00:	printf("LDAR ");			// LDAR Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LDA ");				// LDA Rd, Rs(#disp)
													findRegister(field2);
													printf(", ");
													findRegister(field1);
													printf("(#%04x)", code[1]);
													opwords = 2;
													break;
									} break;
						case 0x35:	switch(field1){
										case 0x00:	printf("LDRL ");			//LDRL RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LDL ");				//LDL RRd, Rs(#disp)
													findLongRegister(field2);
													printf(", ");
													findLongRegister(field1);
													printf("(#%04x)", code[1]);
									} break;
						case 0x36:	printf("Reserved"); break;
						case 0x37:	switch(field1){
										case 0x00:	printf("LDRL %04x, ", code[1]);	//LDRL address, RRs
													findLongRegister(field2);
													opwords = 2;
													break;
										default:	printf("LDL ");				//LDL Rd(#disp), RRs
													findLongRegister(field1);
													printf("(#%04x), ", code[1]);
													findLongRegister(field2);
													opwords = 2;
													break;
									} break;
						case 0x38:	printf("Reserved"); break;
						case 0x39:	switch(field2){
										case 0x00:	printf("LDPS @");			//LDPS @Rs
													findRegister(field1);
													break;
										default:	printf("Reserved");
													break;
									} break;
						case 0x3a:	printf("Hardware Instruction INB, opcode 0x3a, not implemented"); break;
						case 0x3b:	printf("Special Input/Output instructions, opcode 0x3b, not implemented"); break;
						case 0x3c:	printf("INB ");								//INB Rbd, @Rs
									findregRegister(field2);
									printf(", @");
									findregRegister(field1);
									break;
						case 0x3d: 	printf("IN ");								//IN Rd, @Rs
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;
						case 0x3e:	printf("OUTB @");							//OUTB @Rd, Rbs
									findregRegister(field1);
									printf(", ");
									findregRegister(field2);
									break;
						case 0x3f:	printf("OUT @");
									findRegister(field1);
									printf(", ");
									findRegister(field2);
									break;

						case 0x40:	switch(field1){
										case 0x00:	printf("ADDB ");			//ADDB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("ADDB ");			//ADDB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x41:	switch(field1){
										case 0x00:	printf("ADD ");				//ADD Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("ADD ");				//ADD Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x42:	switch(field1){
										case 0x00:	printf("SUBB ");			//SUBB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("SUBB ");			//SUBB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x43:	switch(field1){
										case 0x00:	printf("SUB ");				//SUB Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("SUB ");				//SUB Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x44:	switch(field1){
										case 0x00:	printf("ORB ");				//ORB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("ORB ");				//ORB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x45:	switch(field1){
										case 0x00:	printf("OR ");				//OR Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("OR ");				//OR Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x46:	switch(field1){
										case 0x00:	printf("ANDB ");			//ANDB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("ANDB ");			//ANDB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x47:	switch(field1){
										case 0x00:	printf("AND ");				//AND Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("AND ");				//AND Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x48:	switch(field1){
										case 0x00:	printf("XORB ");			//XORB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default: 	printf("XORB ");			//XORB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;

									} break;
						case 0x49:	switch(field1){
										case 0x00:	printf("XOR ");				//XOR Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("XOR ");				//XOR Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x4a:	switch(field1){
										case 0x00:	printf("CPB ");				//CPB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("CPB ");				//CPB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x4b:	switch(field1){
										case 0x00:	printf("CP ");				//CP Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("CP ");				//CP Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x4c:	switch(field1){
										case 0x00:	switch(field2){
														case 0x00:	printf("COMB %04x", code[1]);	//COMB address
																	opwords = 2;
																	break;
														case 0x01:	printf("CPB %04x, #%04x", code[1], code[2]);	//CPB address, #data
																	opwords = 3;
																	break;
														case 0x02:	printf("NEGB %04x", code[1]);	//NEGB address
																	opwords = 2;
																	break;
														case 0x04:	printf("TESTB %04x", code[1]);	//TESTB address
																	opwords = 2;
																	break;
														case 0x05:	printf("LDB ");					//LDB address, Rs
																	break;
														case 0x06:	printf("TSETB %04x", code[1]);	//TSETB address
																	opwords = 2;
																	break;
														case 0x08:	printf("CLRB %04x", code[1]);	//CLRB address
																	opwords = 2;
																	break;
														default:	printf("Incorrect State!"); break;
													} break;
										default:	switch(field2){
														case 0x00:	printf("COMB %04x(", code[1]);			//COMB addr(Rd)
																	findregRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
														case 0x01:	printf("CPB (%04x)", code[1]);	//CPB (addr)Rd, #data
																	findregRegister(field1);
																	printf(", #%04x", code[2]);
																	opwords = 3;
																	break;
														case 0x02:	printf("NEGB %04x(", code[1]);	//NEGB addr(Rd)
																	findregRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
														case 0x04:	printf("TESTB %04x(", code[1]);	//TESTB addr(Rd)
																	findregRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
														case 0x05:	printf("LDB %04x(",code[1]);	//LDB addr(Rd), Rbs
																	findregRegister(field1);
																	printf("), ");
																	printf("Rbs");
																	opwords = 3;
																	break;
														case 0x06:	printf("TSETB %04x(", code[1]);	//TSETB addr(Rd)
																	findregRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
														case 0x08:	printf("CLRB %04x(", code[1]);	//CLRB addr(Rd)
																	findregRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
														default:	printf("Incorrect State!"); break;
													} break;
									} break;
						case 0x4d:	switch(field1){
										case 0x00:	switch(field2){
														case 0x08:	printf("CLR ");					//CLR address
																	printf("%04x", code[1]);
																	opwords = 2;
																	break;
														case 0x00:	printf("COM ");					//COM address TODO, there's two cases of 0x00
																	printf("%04x", code[1]);
																	opwords = 2;
																	break;
														case 0x01:	printf("CP ");					//CP address, #data
																	printf("%04x", code[1]);
																	printf(", ");
																	printf("#%04x", code[2]);
																	opwords = 3;
																	break;
														case 0x05:	printf("CLR ");					//LD address, Rs
																	printf("%04x", code[1]);
																	printf(", ");
																	findRegister(code[2]);
																	opwords = 3;
																	break;
														case 0x02:	printf("NEG ");					//NEG address
																	printf("%04x", code[1]);
																	opwords = 2;
																	break;
														// case 0x00:	printf("TEST ");					//TEST address
														// 			printf("%04x", code[1]);
														// 			opwords = 2;
																	break;
														case 0x06:	printf("TEST ");					//TEST address
																	printf("%04x", code[1]);
																	opwords = 2;
																	break;
														default: printf("Reserved Op Code: 0x4d"); break;
													} break;
										default:	switch(field2){

														case 0x04:	printf("TEST ");					//TEST address(Rd)
																	printf("%04x", code[1]);
																	printf("(");
																	findRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;

														default: printf("Reserved Op Code: 0x4d"); break;

													} break;
									} break;
						case 0x4e:	printf("NOP");
									break;
						case 0x4f:	printf("TODO");
									break;
						case 0x50:	switch(field1){
										case 0x00:	printf("CPL ");					//CPL RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("CPL ");					//CPL RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(", code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x51:	switch(field2){
										case 0x00:	printf("PUSHL @");				//PUSHL @Rd, address
													findLongRegister(field1);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("PUSHL @");				//PUSHL @Rd, addr(Rs)
													findLongRegister(field1);
													printf(", %04x(", code[1]);
													findLongRegister(field2);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x52:	switch(field1){
										case 0x00:	printf("SUBL ");				//SUBL RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("SUBL ");				//SUBL RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(",code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x53:	switch(field2){
										case 0x00:	printf("PUSH @");				//PUSH @Rd, address
													findRegister(field1);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("PUSH @");				//PUSH @Rd, addr(Rs)
													findRegister(field1);
													printf(", %04x(", code[1]);
													findRegister(field2);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x54:	switch(field1){
										case 0x00:	printf("LDL ");					//LDL RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LDL ");					//LDL RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(", code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x55:	switch(field2){
										case 0x00:	printf("POPL %04x, @", code[1]);//POPL address, @Rs
													findLongRegister(field1);
													opwords = 2;
													break;
										default:	printf("POPL %04x(", code[1]);	//POPL addr(Rd), @Rs
													findLongRegister(field2);
													printf("), @");
													findLongRegister(field1);
													opwords = 2;
													break;
									} break;
						case 0x56:	switch(field1){
										case 0x00:	printf("ADDL ");				//ADDL RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("ADDL ");				//ADDL RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(", code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x57:	switch(field2){
										case 0x00:	printf("POP %04x, @", code[1]);	//POP address, @Rs
													findRegister(field1);
													opwords = 2;
													break;
										default:	printf("POP %04x(", code[1]);	//POP addr(Rd), @Rs
													findRegister(field2);
													printf("), @");
													findRegister(field1);
													opwords = 2;
													break;
									} break;
						case 0x58:	switch(field1){
										case 0x00:	printf("MULTL ");				//MULTL RQd, address
													findQuadRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("MULTL ");				//MULTL RQd, addr(Rs)
													findQuadRegister(field2);
													printf(", %04x(", code[1]);
													findQuadRegister(field1);
													printf(")");
													break;
									} break;
						case 0x59:	switch(field1){
										case 0x00:	printf("MULT ");				//MULT RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("MULT ");				//MULT RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(", code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x5a:	switch(field1){
										case 0x00:	printf("DIVL ");				//DIVL RQd, address
													findQuadRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("DIVL ");				//DIVL RQd, addr(Rs)
													findQuadRegister(field2);
													printf(", %04x(", code[1]);
													findQuadRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x5b:	switch(field1){
										case 0x00:	printf("DIV ");					//DIV RRd, address
													findLongRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("DIV ");					//DIV RRd, addr(Rs)
													findLongRegister(field2);
													printf(", %04x(", code[1]);
													findLongRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x5c:	printf("TODO");
									break;
						case 0x5d:	switch(field1){
										case 0x00:	printf("LDL %04x, ", code[1]); //LDL address, RRs
													findLongRegister(field2);
													opwords = 2;
													break;
										default:	printf("LDL %04x(", code[1]); //LDL addr(Rd), RRs
													findLongRegister(field1);
													printf("), ");
													findLongRegister(field2);
													opwords = 2;
													break;

									} break;
						case 0x5e:	switch(field1){
										case 0x00:	printf("JP %02x, %04x", field2, code[1]);	//JP cc, address
													opwords = 2;
													break;
										default:	printf("JP %02x, %04x(", field2, code[1]);	//JP cc, address(Rd)
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x5f:	switch(field1){
										case 0x00:	printf("CALL %04x\t", code[1]);		//CALL address
													opwords = 2;
													break;
										default:	printf("CALL %04x(", code[1]);		//CALL address(Rd)
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x60:	switch(field1){
										case 0x00:	printf("LDB ");						//LDB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LDB ");						//LDB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x61:	switch(field1){
										case 0x00:	printf("LD ");						//LD Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("LD ");						//LD Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x62:	switch(field1){
										case 0x00:	printf("RESB %04x, #%02x", code[1], field2);	//RESB address, #b
													opwords = 2;
													break;
										default:	printf("RESB %04x(", code[1]);					//RESB addr(Rd), #b
													findregRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x63:	switch(field1){
										case 0x00:	printf("RES %04x, #%02x", code[1], field2);		//RES address, #b
													opwords = 2;
													break;
										default:	printf("RES %04x(", code[1]);					//RES addr(Rd), #b
													findRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x64:	switch(field1){
										case 0x00:	printf("SETB %04x, #%02x", code[1], field2);	//SETB address, #b
													opwords = 2;
													break;
										default:	printf("SETB %04x(", code[1]);					//SETB addr(Rd), #b
													findregRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x65:	switch(field1){
										case 0x00:	printf("SET %04x, #%02x", code[1], field2);		//SET address, #b
													opwords = 2;
													break;
										default:	printf("SET %04x(", code[1]);					//SET addr(Rd), #b
													findRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x66:	switch(field1){
										case 0x00:	printf("BITB %04x, #%02x", code[1], field2);	//BITB address, #b
													opwords = 2;
													break;
										default:	printf("BITB %04x(", code[1]);					//BITB addr(Rd), #b
													findregRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x67:	switch(field1){
										case 0x00:	printf("BIT %04x, #%02x", code[1], field2);		//BIT address, #b
													opwords = 2;
													break;
										default:	printf("BIT %04x(", code[1]);					//BIT addr(Rd), #b
													findRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x68:	switch(field1){
										case 0x00:	printf("INCB %04x, #%02x", code[1], field2);	//INCB address, #n
													opwords = 2;
													break;
										default:	printf("INCB %04x(", code[1]);					//INCB addr(Rd), #n
													findregRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x69:	switch(field1){
										case 0x00:	printf("INC %04x, #%02x", code[1], field2);		//INC address, #n
													opwords = 2;
													break;
										default:	printf("INC %04x(", code[1]);					//INC addr(Rd), #n
													findRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x6a:	switch(field1){
										case 0x00:	printf("DECB %04x, #%02x", code[1], field2);	//DECB address, #n
													opwords = 2;
													break;
										default:	printf("DECB %04x(", code[1]);					//DECB addr(Rd), #n
													findregRegister(field1);
													printf(", #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x6b:	switch(field1){
										case 0x00:	printf("DEC %04x, #%02x", code[1], field2);		//DEC address, #n
													opwords = 2;
													break;
										default:	printf("DEC %04x(", code[1]);					//DEC addr(Rd), #n
													findRegister(field1);
													printf("), #%02x", field2);
													opwords = 2;
													break;
									} break;
						case 0x6c:	switch(field1){
										case 0x00:	printf("EXB ");									//EXB Rbd, address
													findregRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("EXB ");									//EXB Rbd, addr(Rs)
													findregRegister(field2);
													printf(", %04x(", code[1]);
													findregRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x6d:	switch(field1){
										case 0x00:	printf("EX ");									//EX Rd, address
													findRegister(field2);
													printf(", %04x", code[1]);
													opwords = 2;
													break;
										default:	printf("EX ");									//EX Rd, addr(Rs)
													findRegister(field2);
													printf(", %04x(", code[1]);
													findRegister(field1);
													printf(")");
													opwords = 2;
													break;
									} break;
						case 0x6e:	switch(field1){
										case 0x00:	printf("LDB %04x, ", code[1]); //LDB address, Rbs
													findregRegister(field2);
													opwords = 2;
													break;
										default:	printf("LDB %04x(", code[1]); //LDB addr(Rd), Rbs
													findregRegister(field1);
													printf("), ");
													findregRegister(field2);
													opwords = 2;
													break;
									} break;
						case 0x6f:	switch(field1){
										case 0x00:	printf("LD %04x, ", code[1]);//LD address, Rs
													findRegister(field2);
													opwords = 2;
													break;
										default:	printf("LD %04x(", code[1]);//LD addr(Rd), Rs
													findRegister(field1);
													printf("), ");
													findRegister(field2);
													opwords = 2;
													break;
									} break;
						case 0x70:	printf("TODO");							//LDB Rbd, Rs(Rx)
						case 0x71:	printf("TODO");							//LD Rd, Rs(Rx)
						case 0x72:	printf("TODO");							//LDB Rd(Rx), Rbs
						case 0x73:	printf("TODO");							//LD Rd(Rx), Rs
						case 0x74:	printf("TODO");							//LDA Rd, Rs(Rx)
						case 0x75:	printf("TODO");							//LDL RRd, Rs(Rx)
						case 0x76:	printf("TODO");							//LDA Rd, address
						case 0x77:	printf("TODO");							//LDL Rd(Rx), RRs
						case 0x78:	printf("RESERVED");
						case 0x79:	switch(field2){
										case 0x00:	switch(field1){
														case 0x00:	printf("LDPS %04x", code[1]);	//LDPS address
																	opwords = 2;
																	break;
														default:	printf("LDPS %04x(", code[1]);	//LDPS addr(Rs)
																	findRegister(field1);
																	printf(")");
																	opwords = 2;
																	break;
													} break;
										default:	printf("RESERVED");
									} break;
						case 0x7a:	switch(lowerHalf){
										case 0x00:	printf("HALT");			//HALT
													break;
										default:	printf("TODO");
													break;
									} break;
						case 0x7b:	printf("TODO");
						case 0x7c:	printf("TODO");
						case 0x7d:	printf("TODO");
						case 0x7e:	printf("NOP");
						case 0x7f:	printf("SC #%04x", lowerHalf);			//SC #src
						case 0x80:	printf("ADDB ");						//ADDB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x81:	printf("ADD ");							//ADD Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x82:	printf("SUBB ");						//SUBB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x83:	printf("SUB ");							//SUB Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x84:	printf("ORB ");							//ORB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x85:	printf("OR ");							//OR Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x86:	printf("ANDB ");						//ANDB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x87:	printf("AND ");							//AND Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x88:	printf("XORB ");						//XORB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x89:	printf("XOR ");							//XOR Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x8a:	printf("CPB ");							//CPB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0x8b:	printf("CP ");
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x8c:	switch(field2){
										case 0x00:	printf("COMB ");		//COMB Rbd
													findregRegister(field1);
													break;
										case 0x01:	printf("LDCTBL ");		//LDCTBL Rbd, FLAGS TODO
													findregRegister(field1);
													printf(", FLAGS");
													break;
										case 0x02:	printf("NEGB ");		//NEGB Rbd
													findregRegister(field1);
													break;
										case 0x04:	printf("TESTB ");		//TESTB Rbd
													findregRegister(field1);
													break;
										case 0x06:	printf("TSETB ");		//TSETB Rbd
													findregRegister(field1);
													break;
										case 0x08:	printf("CLRB ");		//CLRB Rbd
													findregRegister(field1);
													break;
										case 0x09:	printf("LDCTLB FCW, ");	//LDCTLB FCW, Rbs
													findregRegister(field1);
													break;
										default:	printf("How did you get here?");
									} break;
						case 0x8d:	switch(field2){
										case 0x00:	printf("COM ");			//COM Rd
													findRegister(field1);
													break;
										case 0x01:	printf("SETFLG %02x", field1);	//SETFLG flags
													break;
										case 0x02:	printf("NEG ");			//NEG Rd
													findRegister(field1);
													break;
										case 0x03:	printf("RESFLG %02x", field1);	//RESFLG flags
													break;
										case 0x04:	printf("TEST ");		//TEST Rd
													findRegister(field1);
													break;
										case 0x05:	printf("COMFLG %02x", field1);	//COMFLG flags
													break;
										case 0x06:	printf("TSET ");		//TSET Rd
													findRegister(field1);
													break;
										case 0x07:	switch(field1){
														case 0x00:	printf("NOP");	//NOP
														default:	printf("How did you get here?");
													} break;
										case 0x08:	printf("CLR ");			//CLR Rd
													findRegister(field1);
													break;
										default:	printf("RESERVED");
									} break;
						case 0x8e:	printf("TODO");
						case 0x8f:	printf("TODO");
						case 0x90:	printf("CPL ");							//CPL RRd, RRs
									findLongRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x91:	printf("PUSHL @");						//PUSHL @Rd, RRs
									findLongRegister(field1);
									printf(", ");
									findLongRegister(field2);
									break;
						case 0x92:	printf("SUBL ");						//SUBL RRd, RRs
									findLongRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x93:	printf("PUSH @");						//PUSH @Rd, Rs
									findRegister(field1);
									printf(", ");
									findRegister(field2);
									break;
						case 0x94:	printf("LDL ");							//LDL RRd, RRs
									findLongRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x95:	printf("POPL ");						//POPL RRd, @Rs
									findLongRegister(field2);
									printf(", @");
									findLongRegister(field1);
									break;
						case 0x96:	printf("ADDL ");						//ADDL RRd, RRs
									findLongRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x97:	printf("POP ");							//POP Rd, @Rs
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;
						case 0x98:	printf("MULTL ");						//MULTL RQd, RRs
									findQuadRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x99:	printf("MULT ");						//MULT RRd, Rs
									findLongRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x9a:	printf("DIVL ");						//DIVL RQd, RRs
									findQuadRegister(field2);
									printf(", ");
									findLongRegister(field1);
									break;
						case 0x9b:	printf("DIV ");							//DIV RRd, Rs
									findLongRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0x9c:	printf("TESTL ");						//TESTL RRd
									findLongRegister(field1);
									break;
						case 0x9d:	printf("NOP");							//NO INSTRUCTION
									break;
						case 0x9e:	switch(field1){
										case 0x00:	printf("RET %01x\t", field2);	//RET cc
													break;
										default:	printf("How did you get here?");
													break;
									} break;
						case 0x9f:	printf("NOP");							//NO INSTRUCTION
									break;
						case 0xa0:	printf("LDB ");							//LDB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xa1:	printf("LD ");							//LD Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									printf("\t");
									break;
						case 0xa2:	printf("RESB ");						//RESB Rbd, #b
									findregRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa3:	printf("RES ");							//RES Rd, #b
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa4:	printf("SETB ");						//SETB Rbd, #b
									findregRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa5:	printf("SET ");							//SET Rd, #b
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa6:	printf("BITB ");						//BITB Rbd, #b
									findregRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa7:	printf("BIT ");							//BIT Rd, #b
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa8:	printf("INCB ");						//INCB Rbd, #n
									findregRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xa9:	printf("INC ");							//INC Rd, #n
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xaa:	printf("DECB ");						//DECB Rbd, #n
									findregRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xab:	printf("DEC ");							//DEC Rd, #n
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xac:	printf("EXB ");							//EXB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xad:	printf("EX ");							//EX Rd, Rs
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0xae:	printf("TCCB %02x, ", field2);			//TCCB cc, Rbd
									findregRegister(field1);
									break;
						case 0xaf:	printf("TCC %02x, ", field2);			//TCC cc, Rd
									findRegister(field1);
									break;
						case 0xb0:	printf("DAB ");							//DAB Rbd
									findregRegister(field1);
									break;
						case 0xb1:	switch(field2){
										case 0x00:	printf("EXTSB ");		//EXTSB Rd
													findRegister(field1);
													break;
										case 0x07:	printf("EXTS ");		//EXTS RRd
													findLongRegister(field1);
													break;
										case 0x0a:	printf("EXTSL ");		//EXTSL RQd
													findQuadRegister(field1);
													break;
										default:	printf("How did you get here?");
													break;
									} break;
						case 0xb2:	switch(field2){
										case 0x00:	printf("RLB ");			//RLB Rbd, #1
													findregRegister(field1);
													printf(", #1");
													break;
										case 0x01:	printf("TODO");			//SLLB Rbd, #b & SRLB Rbd, #b
										case 0x02:	printf("RLB ");			//RLB Rbd, #2
													findregRegister(field1);
													printf(", #1");
													break;
										case 0x03:	printf("SDLB ");		//SDLB Rbd, Rs
													findregRegister(field1);
													printf(", ");
													findRegister(code[1]>>8);	//TODO
													opwords = 2;
													break;
										case 0x04:	printf("RRB ");			//RRB Rbd, #1
													findregRegister(field1);
													printf(", #1");
													break;
										case 0x05:	printf("RESERVED");
													break;
										case 0x06:	printf("RRB ");			//RRB Rbd, #2
													findregRegister(field1);
													printf(", #2");
													break;
										case 0x07:	printf("RESERVED");
													break;
										case 0x08:	printf("RLCB ");		//RLCB Rbd, #1
													findregRegister(field1);
													printf(", #1");
													break;
										case 0x09:	printf("SLAB ");		//SLAB Rbd, #b
													findregRegister(field1);
													printf(", #b");			//TODO
													opwords = 2;
													break;
										case 0x0a:	printf("RLCB ");		//RLCB Rbd, #2
													findregRegister(field1);
													printf(", #2");
													break;
										case 0x0b:	printf("SDAB ");		//SDAB Rbd, Rs
													findregRegister(field1);
													printf(", ");
													findRegister(code[1]>>8);	//TODO
													break;
										case 0x0c:	printf("RRCB ");		//RRCB Rbd, #1
													findregRegister(field1);
													printf(", #1");
													break;
										case 0x0d:	printf("RESERVED");
													break;
										case 0x0e:	printf("RRCB ");		//RRCB Rbd, #2
													findregRegister(field1);
													printf(", #2");
													break;
										case 0x0f:	printf("RESERVED");
													break;
										default:	printf("How did you get here?");
													break;
									} break;
						case 0xb3:	switch(field2){
										case 0x00:	printf("RL ");			//RL Rd, #1
													findRegister(field1);
													printf(", #1");
													break;
										case 0x01:	printf("SLA ");			//SLA Rd, #b
													findRegister(field1);
													printf(", #%04x", code[1]);
													break;
										case 0x02:	printf("RL ");			//RL Rd, #2
													findRegister(field1);
													printf(", #2");
													break;
										case 0x03:	printf("SDL ");			//SDL Rd, Rs
													findRegister(field1);
													printf(", ");
													findRegister(code[1]>>8);	//TODO
													opwords = 2;
													break;
										case 0x04:	printf("RR ");			//RR Rd, #1
													findRegister(field1);
													printf(", #1");
													break;
										case 0x05:	printf("SLLL ");		//SLLL RRd, #b
													findLongRegister(field1);
													printf(", #%04x", code[1]);
													opwords = 2;
													break;
										case 0x06:	printf("RR ");			//RR Rd, #2
													findRegister(field1);
													printf(", #2");
													break;
										case 0x07:	printf("SDLL ");		//SDLL RRd, Rs
													findLongRegister(field1);
													printf(", ");
													findRegister(code[1]>>8); //TODO
													opwords = 2;
													break;
										case 0x08:	printf("RLC ");			//RLC Rd, #1
													findRegister(field1);
													printf(", #1");
													break;
										case 0x09:	printf("SLA ");			//SLA Rd, #b
													findRegister(field1);
													printf(", #%04x", code[1]);
													opwords = 2;
													break;
										case 0x0a:	printf("RLC");			//RLC Rd, #2
													findRegister(field1);
													printf(", #2");
													break;
										case 0x0b:	printf("SDA ");			//SDA Rd, Rs
													findRegister(field1);
													printf(", ");
													findRegister(code[1]>>8);
													opwords = 2;
													break;
										case 0x0c:	printf("RRC ");			//RRC Rd, #1
													findRegister(field1);
													printf(", #1");
													break;
										case 0x0d:	printf("SRAL ");		//SRAL RRd, #b
													findLongRegister(field1);
													printf(", #%04x", code[1]);
													opwords = 2;
													break;
										case 0x0e:	printf("RRC ");			//RRC Rd, #2
													findRegister(field1);
													printf(", #2");
													break;
										case 0x0f:	printf("SDAL ");		//SDAL RRd, Rs
													findLongRegister(field1);
													printf(", ");
													findRegister(code[1]>>8);
													opwords = 2;
													break;
									} break;
						case 0xb4:	printf("ADCB ");
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xb5:	printf("ADC ");
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;
						case 0xb6:	printf("SBCB ");
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xb7:	printf("SBC ");
									findRegister(field2);
									printf(", ");
									findRegister(field1);
									break;

						case 0xb8:	switch(field2){
										case(0x8):	printf("TRDB ");		//TRDB @RD, @RS, r
													findRegister(field1);
													printf(", ");
													printf("@%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0xc):	printf("TRRDB ");		//TRRDB @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("@%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0x0):	printf("TRIB ");		//TRIB @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("@%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0x4):	printf("TRIRB ");		//TRIRB @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0xa):	printf("TRTDB ");		//TRTDB @Rs1, @Rs2, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0xe):	printf("TRTDRB ");		//TRTDRB @Rs1, @Rs2, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0x2):	printf("TRTIB ");		//TRTIB @Rs1, @Rs2, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;


										case(0x6):	printf("TRTIRB ");		//TRTIRB @Rs1, @Rs2, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										default: 	printf("This is a reserved instruction // Comes from same op code as TRDB");
													break;

									} break;

						case 0xb9:	printf("Reserved Instruction 0xb9"); break;
						case 0xba:	switch(field2){
										case(0x8):	printf("CPDB ");		//CPDB Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xc):	printf("CPDRB ");		//CPDRB Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x0):	printf("CPIB ");		//CPIB Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x4):	printf("CPIRB ");		//CPIRB RD, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xa):	printf("CPSDB ");		//@Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xe):	printf("CPSDRB ");		//CPSDRB @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x2):	printf("CPSIB ");		//CPSIB @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x6):	printf("CPSIRB ");		//CPSIRB @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x9):	printf("LDDB ");		//LDDB @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0x1):	printf("LDIRB ");		//LDIRB @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										default: 	printf("NOP, location 0xba");
												 	break;
									} break;

						case 0xbb:  switch(field2){

										case(0x8):	printf("CPD ");		//CPD Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xc):	printf("CPDR ");		//CPDR Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x0):	printf("CPI ");		//CPI Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x4):	printf("CPIR ");		//CPIR Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xa):	printf("CPSD ");		//CPSD Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0xe):	printf("CPSDR ");		//CPSDR @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("@%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x2):	printf("CPSI ");		//CPSI @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x6):	printf("CPSIR ");		//CPSIR @Rd, @Rs, r, cc
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													printf(", ");
													printf("%04x", (code[1]) & 0x000F);
													break;

										case(0x9):	printf("LDD ");		//LDD @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										case(0x1):	printf("LDI ");		//LDI @Rd, @Rs, r
													findRegister(field1);
													printf(", ");
													printf("%04x", (code[1]>>4) & 0x000F);
													printf(", ");
													printf("%04x", code[1]>>8);
													break;

										default:	printf("NOP from op code 0xbb");
													break;

									} break;
						case 0xbc:	printf("RRDB ");					//RRDB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xbd:	printf("LDK ");						//LDK Rd, #data
									findRegister(field1);
									printf(", #%02x", field2);
									break;
						case 0xbe:	printf("RLDB ");					//RLDB Rbd, Rbs
									findregRegister(field2);
									printf(", ");
									findregRegister(field1);
									break;
						case 0xbf:	printf("RESERVED");
									break;

						default: printf("%02x not implemented in dissasembler", upperHalf); break;
					} break;
	}
	printf("\t");
	return opwords;
}

void UnimplementedInstruction(State8002* state){
	//pc will have advanced two, so undo that
	printf("\tError: Unimplemented instruction\n");
	state->pc-=2;
	Disassemble8002(state->memory, state->pc);
	printf("\n");
	exit(1);
}

int Emulate8002(State8002* state){
	uint16_t *opcode = &state->memory[state->pc/2];

	int done = 0;

	uint8_t upperHalf = opcode[0] >> 8;
	uint8_t lowerHalf = opcode[0];
	uint8_t upperFour = opcode[0] >> 12;
	uint8_t upperTwo = opcode[0] >> 14;
	uint8_t secondNibble = upperHalf & 0x0F;
	uint8_t field1 = (opcode[0] >> 4) & (0x0F);
	uint8_t field2 = opcode[0] & 0x0F;

	Disassemble8002(state->memory, state->pc);

	//state->memory[0] = 2;

	state->pc += 2;

	//state->status = 0b1000;
	//printf("point to memory plus 1 is :%x\n",(*(memptr8 + 2) ));


	switch(upperTwo){ //address mode/type of instruction
		case 0b11:{  //special instruction
			switch (upperFour){
				case 0b1110:{ //JR
					int8_t displacement = lowerHalf;
					if(checkConditionCode(secondNibble,state->cc)){
						state->pc = state->pc + (displacement * 2) - 2;
					}
				}break;

				default: printf("bad");done=1; break;

			}
		}break;

		default:{ //regular instruction
			switch(upperHalf){ //opcode
				case 0x00:{ //ADDB,  IM or IR
								if(field1 == 0){	// ADDB Rbd, #data
									uint8_t res = readReg8(field2, state);
									if(field2 <= 7){
										res += opcode[1] >> 8; //Higher
									} else{
										res += opcode[1]; //Lower
									}
									writeReg8(field2, state, res);
									state->pc += 2;
								}
								else{ 				// ADDB Rbd, @Rs
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									if(field2 <= 7){
										res += state->memory[offset] >> 8; //Higher
									} else{
										res += state->memory[offset]; //Lower
									}
									writeReg8(field2, state, res);
								}
							} break;
				case 0x01:{ //ADD IM or IR
								if(field1 == 0){	//ADD Rd, #data
									uint16_t res = readReg16(field2, state) + opcode[1];
									writeReg16(field2, state, res);
									state->cc.c = (readReg16(field2, state) + opcode[1] > 0xffff);
									state->pc += 2;
								}
								else{ 				//ADD Rd, @Rs
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res += state->memory[offset];
									writeReg16(field2, state, res);
								}
							} break;
				case 0x02:{
								if(field1 == 0){	// SUBB Rbd, #data
									uint8_t res = readReg8(field2, state);
									if(field2 <= 7){
										res -= opcode[1] >> 8; //Higher
									} else{
										res -= opcode[1]; //Lower
									}
									writeReg8(field2, state, res);
									state->pc += 2;
								} else {			//SUBB Rbd, @Rs
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									if(field2 <= 7){
										res -= state->memory[offset] >> 8; //Higher
									} else{
										res -= state->memory[offset];	//Lower
									}
									writeReg8(field2, state, res);
								}
							} break;
				case 0x03:{
								if(field1 == 0){	// SUB Rd, #data
									uint16_t res = readReg16(field2, state) - opcode[1];
									writeReg16(field2, state, res);
									state->pc += 2;
								} else {			// SUB Rd, @Rs
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res -= state->memory[offset];
									writeReg16(field2, state, res);
								}
							} break;
				case 0x04:{
								if(field1 == 0){	//ORB Rbd, #data
									uint8_t res = readReg8(field2, state);
									if(field2 <= 7){
										res = res | opcode[1] >> 8; //Higher
									} else{
										res = res | opcode[1]; //Lower
									}
									writeReg8(field2, state, res);
									state->pc += 2;
								} else{				//ORB Rbd, @Rs
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									if(field2 <= 7){
										res = res | (state->memory[offset] >> 8); //Higher
									} else{
										res = res | state->memory[offset];	//Lower
									}
									writeReg8(field2, state, res);
								}
							} break;
				case 0x05: {
								if(field1 == 0){	//OR Rd, #data
									uint16_t res = readReg16(field2, state) | opcode[1];
									writeReg16(field2, state, res);
									state->pc += 2;
								} else{				//OR Rd, @Rs
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res | state->memory[offset];
									writeReg16(field2, state, res);
								}
							}	break;
				case 0x06: {
								if(field1 == 0){	//ANDB Rbd, #data
									uint8_t res = readReg8(field2, state);
									if(field2 <= 7){
										res = res & opcode[1] >> 8; //Higher
									} else{
										res = res & opcode[1]; //Lower
									}
									writeReg8(field2, state, res);
									state->pc += 2;
								} else{				//ANDB Rbd, @Rs
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									if(field2 <= 7){
										res = res & (state->memory[offset] >> 8);	//Higher
									} else {
										res = res & state->memory[offset];	//Lower
									}
									writeReg8(field2, state, res);
								}
							}	break;
				case 0x07: {
								if(field1 == 0){	//AND Rd, #data
									uint16_t res = readReg16(field2, state) & opcode[1];
									writeReg16(field2, state, res);
									state->pc += 2;
								} else{				//AND Rd, @Rs
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res & state->memory[offset];
									writeReg16(field2, state, res);
								}
							}	break;
				case 0x08: {
								if(field1 == 0){	//XORB Rbd, #data
									uint8_t res = readReg8(field2, state);
									if(field2 <= 7){
										res = res ^ opcode[1] >> 8; //Higher
									} else{
										res = res ^ opcode[1]; //Lower
									}
									writeReg8(field2, state, res);
									state->pc += 2;
								} else{				//XORB Rbd, @Rs
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									if(field2 <= 7){
										res = res ^ (state->memory[offset] >> 8);	//Higher
									} else{
										res = res ^ state->memory[offset];	//Lower
									}
									writeReg8(field2, state, res);
								}
							}	break;
				case 0x09: {
								if(field1 == 0){	//XOR Rd, #data
									uint16_t res = readReg16(field2, state) ^ opcode[1];
									writeReg16(field2, state, res);
									state->pc += 2;
								} else{				//XOR Rd, @Rs
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res ^ state->memory[offset];
									writeReg16(field2, state, res);
								}
							}	break;
				case 0x0a: {
								if(field1 == 0){	//CPB Rbd, #data
									uint8_t* dReg8 = returnByteRegisterPointer(field2, state);
									uint8_t* res;
									if(field2 <= 7){
										*res = *dReg8 - (opcode[1] >> 8);	//Higher
										state->cc.c = (*dReg8 < opcode[1] >> 8);
									} else{
										*res = *dReg8 - (opcode[1]);			//Lower
										state->cc.c = (*dReg8 < opcode[1]);
									}
									state->cc.z = (*res == 0);
									state->cc.s = (0x80 == (*res & 0x80));
									//TODO state->cc.v
									state->pc += 2;
								} else{				//CPB Rbd, @Rs
									uint8_t* dReg8 = returnByteRegisterPointer(field2, state);
									uint8_t* sReg8 = returnByteRegisterPointer(field1, state);
									uint8_t* res = *dReg8 - state->memory[*sReg8];
									//TODO
								}
							}	break;
				case 0x0b: UnimplementedInstruction(state);	break; //TODO CP Rd, #data & CP Rd, @Rs
				case 0x0c: UnimplementedInstruction(state); break; //TODO
				case 0x0d: UnimplementedInstruction(state);	break; //TODO
				case 0x0e: break;	//NOP
				case 0x0f: UnimplementedInstruction(state); break;
				case 0x10: UnimplementedInstruction(state);	break; //TODO
				case 0x11: {		// PUSHL @Rd, @Rs TODO
								uint32_t* dReg32 = returnLongRegisterPointer(field1, state);
								uint32_t* sReg32 = returnLongRegisterPointer(field2, state);
								uint32_t* temp = state->memory[*sReg32];
								state->memory[*dReg32] = state->memory[*dReg32] - 4;
								state->memory[*dReg32] = *temp;
								//This might be wrong
							}	break;
				case 0x12: {
								if(field1 == 0){	//SUBL RRd, #data
									uint32_t res = readReg32(field2, state) - (opcode[1] | opcode [2]);
									writeReg32(field2, state, res);
									state->pc += 4;
								} else{				//SUBL RRd, @Rs
									uint32_t res = readReg32(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res - state->memory[offset];
									writeReg32(field2, state, res);
								}
							}	break;
				case 0x13: UnimplementedInstruction(state);	break; //TODO
				case 0x14: {
								if(field1 == 0){	//LDL RRd, #data
									writeReg32(field2, state, (opcode[1] | opcode[2]));
									state->pc += 4;
								} else{				//LDL RRd, @Rs
									uint16_t offset = readReg16(field1, state);
									writeReg32(field2, state, state->memory[offset]);
								}
							}	break;
				case 0x15: UnimplementedInstruction(state);	break; //TODO
				case 0x16: {//ADDL IM or IR
								if(field1 == 0){	//ADDL RRd, #data
									uint32_t res = readReg32(field2, state) + (opcode[1] | opcode [2]);
									writeReg32(field2, state, res);
									state->pc += 4;
								} else{ 			//ADDL RRd, @Rs
									uint32_t res = readReg32(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res + state->memory[offset];
									writeReg32(field2, state, res);
								}
							} break;
				case 0x17: UnimplementedInstruction(state);	break; //TODO
				case 0x18: {
								if(field1 == 0){	//MULTL RQd, #data
									uint64_t res = readReg64(field2, state) * (opcode[1] | opcode[2]);
									writeReg64(field2, state, res);
									state->pc += 4;
								} else{				//MULTL RQd, @Rs
									uint64_t res = readReg64(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res * state->memory[offset];
									writeReg64(field2, state, res);
								}
							} break;
				case 0x19: {
								if(field1 == 0){	//MULT RRd, #data
									uint32_t res = readReg32(field2, state) * opcode[1];
									writeReg32(field2, state, res);
									state->pc += 2;
								} else{				//MULT RRd, @Rs
									uint32_t res = readReg32(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res * state->memory[offset];
									writeReg32(field2, state, res);
								}
							}	break;
				case 0x1a: {
								if(field1 == 0){	//DIVL RQd, #data
									uint64_t res = readReg64(field2, state) / (opcode[1] | opcode[2]);
									writeReg64(field2, state, res);
									state->pc += 4;
								} else{				//DIVL RQd, @Rs
									uint64_t res = readReg64(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res / state->memory[offset];
									writeReg64(field2, state, res);
								}
							}	break;
				case 0x1b: {
								if(field1 == 0){	//DIV RRd, #data
									uint32_t res = readReg32(field2, state) / opcode[1];
									writeReg32(field2, state, res);
									state->pc += 2;
								} else{				//DIV RRd, @Rs
									uint32_t res = readReg32(field2, state);
									uint16_t offset = readReg16(field1, state);
									res = res / state->memory[offset];
									writeReg32(field2, state, res);
								}
							}	break;
				case 0x1c: UnimplementedInstruction(state);	break; //TODO
				case 0x1d: {	//LDL @Rd, RRs
								uint32_t source = readReg32(field2, state);
								uint16_t offset = readReg16(field1, state);
								//writeReg16()
							}	break;
				case 0x1e: {	//JP cc, @Rd
								uint16_t offset = readReg16(field1, state);
								if(checkConditionCode(field2, state->cc)){
									state->pc = state->memory[offset];
								}
							} break;
				case 0x1f: {	//CALL @Rd
								uint16_t dest = readReg16(field1, state);
								state->sp -= 2;
								state->memory[state->sp] = state->pc;
								state->pc = dest;
							}	break;
				case 0x20: {
								if(field1 == 0){	//LDB Rbd, #data
									if(field2 <= 7){
										writeReg8(field2, state, opcode[1]>>8); //Higher
									} else{
										writeReg8(field2, state, opcode[1]);	//Lower
									}
									state->pc += 2;
								} else{				//LDB Rbd, @Rs
									uint16_t offset = readReg16(field1, state);
									writeReg8(field2, state, state->memory[offset]);
								}
							}	break;
				case 0x21: {
								if(field1 == 0){	//LD Rd, #data
									writeReg16(field2, state, opcode[1]);
									state->pc += 2;
								} else{				//Ld Rd, @Rs
									uint16_t offset = readReg16(field1, state);
									writeReg16(field2, state, state->memory[offset]);
								}
							} break;
				case 0x22: {	//TODO
								if(field1 == 0){
									uint16_t offset = readReg16(field1, state);
									uint16_t temp = state->memory[offset];
								} else{

								}
							}	break;
				case 0x23: {	//TODO
								if(field1 == 0){

								} else{

								}
							}	break;
				case 0x24: UnimplementedInstruction(state); break; //TODO
				case 0x25: UnimplementedInstruction(state);	break; //TODO
				case 0x26: UnimplementedInstruction(state);	break; //TODO
				case 0x27: UnimplementedInstruction(state);	break; //TODO
				case 0x28: {	//INCB @Rd, #n
								uint16_t offset = readReg16(field1, state);
								//WRITE MEMORY
							}	break;
				case 0x29: UnimplementedInstruction(state);	break; //TODO
				case 0x2a: UnimplementedInstruction(state);	break; //TODO
				case 0x2b: UnimplementedInstruction(state);	break; //TODO
				case 0x2c: {	//EXB Rbd, @Rs
								uint8_t dest = readReg8(field2, state);
								uint16_t offset = readReg16(field1, state);
								uint16_t temp = state->memory[offset];
								state->memory[offset] = temp; //TODO?
								writeReg8(field2, state, temp);
							}	break; 
				case 0x2d: {	//EX Rd, @Rs
								uint16_t dest = readReg16(field2, state);
								uint16_t offset = readReg16(field1, state);
								uint16_t temp = state->memory[offset];
								state->memory[offset] = temp; //TODO?
								writeReg16(field2, state, temp);
							}	break; 
				case 0x2e: {	//LDB @Rd, Rbs
								uint8_t source = readReg8(field2, state);
								uint16_t offset = readReg16(field1, state);
								state->memory[offset] = source; //TODO?
							}	break;
				case 0x2f: {	//LD @Rd, Rs
								uint16_t source = readReg16(field2, state);
								uint16_t offset = readReg16(field1, state);
								state->memory[offset] = source; //TODO?
							}	break;
				case 0x30: UnimplementedInstruction(state); break; //TODO
				case 0x31: UnimplementedInstruction(state);	break; //TODO
				case 0x32: UnimplementedInstruction(state);	break; //TODO
				case 0x33: UnimplementedInstruction(state); break; //TODO
				case 0x34: UnimplementedInstruction(state);	break; //TODO
				case 0x35: UnimplementedInstruction(state);	break; //TODO
				case 0x36: break;
				case 0x37: UnimplementedInstruction(state);	break; //TODO
				case 0x38: break;
				case 0x39: {	//TODO
								if(field2 == 0){

								} else{

								}
							}	break;
				case 0x3a: UnimplementedInstruction(state); break; //TODO?
				case 0x3b: UnimplementedInstruction(state); break; //TOOD?
				case 0x3c: UnimplementedInstruction(state); break; //TODO?
				case 0x3d: UnimplementedInstruction(state); break; //TODO?
				case 0x3e: UnimplementedInstruction(state); break; //TOOD?
				case 0x3f: {	//CALL @Rd
								uint16_t offset = readReg16(field1, state);
								state->sp -= 2;
								state->memory[state->sp] = state->pc;
								state->pc = offset;	//Not entirely sure what gets stored for call
							} break;
				case 0x40: { //ADDB, DA or X
								if(field1 == 0){	//ADDB Rbd, address
									uint8_t res = readReg8(field2, state);
									res += state->memory[opcode[1]];
									writeReg8(field2, state, res);
								} else{				//ADDB Rbd, addr(Rs)
									uint8_t res = readReg8(field2, state);
									uint16_t offset = readReg16(field1, state);
									res += state->memory[offset + opcode[1]];
									writeReg8(field2, state, res);
								}
								state->pc += 2;
							} break;
				case 0x41: {//ADD, DA or X
								if(field1 == 0){	//ADD Rd, address
									uint16_t res = readReg16(field2, state);
									res += state->memory[opcode[1]];
									writeReg16(field2, state, res);
								} else{ 			//ADD Rd, addr(Rs)
									uint16_t res = readReg16(field2, state);
									uint16_t offset = readReg16(field1, state);
									res += state->memory[offset + opcode[1]];
									writeReg16(field2, state, res);
								}
								state->pc += 2;
							} break;
				case 0x42: UnimplementedInstruction(state);	break;
				case 0x43: UnimplementedInstruction(state);	break;
				case 0x44: UnimplementedInstruction(state);	break;
				case 0x45: UnimplementedInstruction(state);	break;
				case 0x46: UnimplementedInstruction(state);	break;
				case 0x47: UnimplementedInstruction(state);	break;
				case 0x48: UnimplementedInstruction(state); break;
				case 0x49: UnimplementedInstruction(state);	break;
				case 0x4a: UnimplementedInstruction(state);	break;
				case 0x4b: UnimplementedInstruction(state); break;
				case 0x4c: UnimplementedInstruction(state);	break;
				case 0x4d: UnimplementedInstruction(state);	break;
				case 0x4e: UnimplementedInstruction(state); break;
				case 0x4f: UnimplementedInstruction(state);	break;
				case 0x50: UnimplementedInstruction(state);	break;
				case 0x51: UnimplementedInstruction(state); break;
				case 0x52: UnimplementedInstruction(state);	break;
				case 0x53: UnimplementedInstruction(state);	break;
				case 0x54: UnimplementedInstruction(state); break;
				case 0x55: UnimplementedInstruction(state);	break;
				case 0x56: {//ADDL, DA or X
								uint32_t* destinationReg32 = returnLongRegisterPointer(field2, state);
								state->pc+=2;
								uint32_t* memptr32 = state->memory;

								if(field1 == 0){	//ADDL (DA)
									*destinationReg32 = fix_32(fix_32(*destinationReg32) + fix_32(*(memptr32 + opcode[1]))); //assuming we only use the top bits
								}
								else{ //ADDL (IR)
									uint32_t* sourceReg32 = returnLongRegisterPointer(field1, state);
									*destinationReg32 = fix_32(fix_32(*destinationReg32) + fix_32(*(memptr32 + opcode[1] + fix_32(*sourceReg32)))); //assuming we only use the top bits
								}
							} break;
				case 0x57: UnimplementedInstruction(state); break;
				case 0x58: UnimplementedInstruction(state);	break;
				case 0x59: UnimplementedInstruction(state);	break;
				case 0x5a: UnimplementedInstruction(state);	break;
				case 0x5b: UnimplementedInstruction(state);	break;
				case 0x5c: UnimplementedInstruction(state);	break;
				case 0x5d: UnimplementedInstruction(state); break;
				case 0x5e: UnimplementedInstruction(state);	break;
				case 0x5f: {	
								if(field1 == 0){	//CALL address
									state->sp -= 2;
									state->memory[state->sp] = state->pc;
									state->pc = opcode[1];
								} else{				//CALL address(Rd)
									uint16_t offset = readReg16(field1, state);
									state->pc -= 2;
									state->memory[state->sp] = state->pc;
									state->pc = state->memory[offset];	//TODO?
								}
							}	break;
				case 0x60: UnimplementedInstruction(state); break;
				case 0x61: UnimplementedInstruction(state);	break;
				case 0x62: UnimplementedInstruction(state);	break;
				case 0x63: UnimplementedInstruction(state); break;
				case 0x64: UnimplementedInstruction(state);	break;
				case 0x65: UnimplementedInstruction(state);	break;
				case 0x66: UnimplementedInstruction(state);	break;
				case 0x67: UnimplementedInstruction(state);	break;
				case 0x68: UnimplementedInstruction(state);	break;
				case 0x69: UnimplementedInstruction(state);	break;
				case 0x6a: UnimplementedInstruction(state);	break;
				case 0x6b: UnimplementedInstruction(state);	break;
				case 0x6c: UnimplementedInstruction(state); break;
				case 0x6d: UnimplementedInstruction(state);	break;
				case 0x6e: UnimplementedInstruction(state);	break;
				case 0x6f: UnimplementedInstruction(state);	break;
				case 0x70: UnimplementedInstruction(state);	break;
				case 0x71: UnimplementedInstruction(state);	break;
				case 0x72: UnimplementedInstruction(state); break;
				case 0x73: UnimplementedInstruction(state);	break;
				case 0x74: UnimplementedInstruction(state);	break;
				case 0x75: UnimplementedInstruction(state); break;
				case 0x76: UnimplementedInstruction(state);	break;
				case 0x77: UnimplementedInstruction(state);	break;
				case 0x78: UnimplementedInstruction(state); break;
				case 0x79: UnimplementedInstruction(state);	break;
				case 0x7a: UnimplementedInstruction(state);	break;
				case 0x7b: UnimplementedInstruction(state);	break;
				case 0x7c: UnimplementedInstruction(state);	break;
				case 0x7d: UnimplementedInstruction(state);	break;
				case 0x7e: UnimplementedInstruction(state); break;
				case 0x7f: UnimplementedInstruction(state);	break;
				case 0x80: { // ADDB (R)
						    	uint8_t* destinationReg8 = returnByteRegisterPointer(field2, state);
						    	uint8_t* sourceReg8 = returnByteRegisterPointer(field1, state);
						    	*destinationReg8 = *destinationReg8 + *sourceReg8;
		   					} break;
				case 0x81: { // ADD (R)
								uint16_t* destinationReg16 = returnWordRegisterPointer(field2, state);
						    	uint16_t* sourceReg16 = returnWordRegisterPointer(field1, state);
						    	*destinationReg16 = fix_16(fix_16(*destinationReg16) + fix_16(*sourceReg16));
		    				} break;
				case 0x82: UnimplementedInstruction(state);	break;
				case 0x83: UnimplementedInstruction(state);	break;
				case 0x84: UnimplementedInstruction(state);	break;
				case 0x85: UnimplementedInstruction(state);	break;
				case 0x86: UnimplementedInstruction(state);	break;
				case 0x87: UnimplementedInstruction(state);	break;
				case 0x88: UnimplementedInstruction(state);	break;
				case 0x89: UnimplementedInstruction(state);	break;
				case 0x8a: UnimplementedInstruction(state);	break;
				case 0x8b: {//CP R  needs work on C and V flags
								uint16_t* destinationReg16 = returnWordRegisterPointer(field2, state);
							 	uint16_t* sourceReg16 = returnWordRegisterPointer(field1, state);
							 	uint32_t result = fix_16(*destinationReg16) - fix_16(*sourceReg16);
								if(result & 0x80 == fix_16(*destinationReg16) & 0x80){ // MSB has no borrow
									state->cc.c = 0;
								}
								else{
									state->cc.c = 1;
								}

								if(result == 0){
									state->cc.z = 1;
								}
								else{
									state->cc.z = 0;
								}

								if(result < 0){
									state->cc.s = 1;
								}
								else{
									state->cc.s = 0;
								}

								if(result & 0x100 > 0){
									state->cc.v = 1;
								}
								else{
									state->cc.v = 0;
								}
							} break;
				case 0x8c: UnimplementedInstruction(state);	break;
				case 0x8d: UnimplementedInstruction(state);	break;
				case 0x8e: UnimplementedInstruction(state);	break;
				case 0x8f: UnimplementedInstruction(state);	break;
				case 0x90: UnimplementedInstruction(state); break;
				case 0x91: UnimplementedInstruction(state);	break;
				case 0x92: UnimplementedInstruction(state);	break;
				case 0x93: UnimplementedInstruction(state); break;
				case 0x94: UnimplementedInstruction(state);	break;
				case 0x95: UnimplementedInstruction(state);	break;
				case 0x96: {//ADDL, R
								uint32_t* destinationReg32 = returnLongRegisterPointer(field2, state);
				      			uint32_t* sourceReg32 = returnLongRegisterPointer(field1, state);
				      			*destinationReg32 = fix_32(fix_32(*destinationReg32) + fix_32(*sourceReg32));
							} break;
				case 0x97: UnimplementedInstruction(state);	break;
				case 0x98: UnimplementedInstruction(state);	break;
				case 0x99: UnimplementedInstruction(state); break;
				case 0x9a: UnimplementedInstruction(state);	break;
				case 0x9b: UnimplementedInstruction(state);	break;
				case 0x9c: UnimplementedInstruction(state); break;
				case 0x9d: UnimplementedInstruction(state);	break;
				case 0x9e: {	//RET cc
								if(checkConditionCode(field2, state->cc)){
									state->pc = state->memory[state->sp];
								}
								state->sp += 2;
							}	break;
				case 0x9f: UnimplementedInstruction(state); break;
				case 0xa0: UnimplementedInstruction(state);	break;
				case 0xa1: {	//LD Rd, Rs
								uint16_t source = readReg16(field1, state);
								writeReg16(field2, state, source);
							}	break;
				case 0xa2: UnimplementedInstruction(state);	break;
				case 0xa3: UnimplementedInstruction(state);	break;
				case 0xa4: UnimplementedInstruction(state);	break;
				case 0xa5: UnimplementedInstruction(state); break;
				case 0xa6: UnimplementedInstruction(state);	break;
				case 0xa7: UnimplementedInstruction(state);	break;
				case 0xa8: UnimplementedInstruction(state); break;
				case 0xa9: UnimplementedInstruction(state);	break;
				case 0xaa: UnimplementedInstruction(state);	break;
				case 0xab: UnimplementedInstruction(state); break;
				case 0xac: UnimplementedInstruction(state);	break;
				case 0xad: UnimplementedInstruction(state);	break;
				case 0xae: UnimplementedInstruction(state);	break;
				case 0xaf: UnimplementedInstruction(state);	break;
				case 0xb0: UnimplementedInstruction(state);	break;
				case 0xb1: UnimplementedInstruction(state);	break;
				case 0xb2: UnimplementedInstruction(state);	break;
				case 0xb3: UnimplementedInstruction(state);	break;
				case 0xb4: UnimplementedInstruction(state); break;
				case 0xb5: UnimplementedInstruction(state);	break;
				case 0xb6: UnimplementedInstruction(state);	break;
				case 0xb7: UnimplementedInstruction(state);	break;
				case 0xb8: UnimplementedInstruction(state);	break;
				case 0xb9: UnimplementedInstruction(state);	break;
				case 0xba: UnimplementedInstruction(state); break;
				case 0xbb: UnimplementedInstruction(state);	break;
				case 0xbc: UnimplementedInstruction(state);	break;
				case 0xbd: UnimplementedInstruction(state); break;
				case 0xbe: UnimplementedInstruction(state);	break;
				case 0xbf: UnimplementedInstruction(state);	break;
				default: printf("wrong place");done=1; break;
			}
		}
	}

	//printf("\t");
	printf("R0: %x R1: %x, R2: %x, R3: %x, SP: %x  ||| OP: %x\n",readReg16(0, state), readReg16(1, state), readReg16(2, state),readReg16(3, state), readReg16(15, state),upperHalf);

	if(upperFour == 0xFF){
		return 1;
	}
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


	uint8_t *buffer = &state->memory[offset];
	fread(buffer, fsize, 1, f);
	fclose(f);
	uint16_t *memptr = &state->memory[offset];

	for(uint16_t i = 0; i < fsize/2; i++){
	 	memptr[i] = buffer[i*2+1] + (buffer[i*2] << 8);
	}
}

State8002* Init8002(void){
	State8002* state = calloc(1, sizeof(State8002));
	// state->cc = calloc(1,sizeof(ConditionCodes));
	// state->cc.c = 1;
	// state->cc.v = 1;
	// state->cc.s = 1;
	// state->cc.z = 1;
	// state->cc.d = 1;
	// state->cc.h = 1;
	state->memory = malloc(0x10000); // 64k




	// uint8_t *p8 = &state->R0 + 0x0000;
	// uint16_t *p16 = &state->R0 + 0x0000;
	// uint32_t *p32 = &state->R0 + 0x0000;
	// uint64_t *p64 = &state->R0 + 0x0000;
	//
	//
	//
	// uint64_t ll = 0x123456789abcdeff;
	// *p64 = fix_64(ll);
	//
	// for(int i = 0; i<8; i++)
	// 	pr`intf("%x\n",*(p8+i));
	//
	// for(int i = 0; i<4; i++)
	// 	printf("%x\n",fix_16(*(p16+i)));
	//
	// for(int i = 0; i<2; i++)
	// 	printf("%x\n",fix_32(*(p32+i)));
	//
	// printf("%llx\n",fix_64(*(p64)));



	return state;
}

int main (int argc, char**argv){
	int done = 0;
	State8002* state = Init8002();

	ReadFileIntoMemoryAt(state, "testFormat.t", 0);
	//ReadFileIntoMemoryAt(state, "jr.t", 2);
	while (done == 0){
		done = Emulate8002(state);
	}
	return 0;
}
