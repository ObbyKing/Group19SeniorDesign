#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
//#include <winsock2.h>


typedef int bool;
#define true 1
#define false 0


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
	uint16_t		pc; // Program counter
	uint8_t 	*memory; // 64k memory space the Z8002 does not use segmented mode
	struct ConditionCodes* cc;
	uint32_t instructionsRun;
} State8002;

bool checkConditionCode(uint8_t cc,ConditionCodes *o){
	cc = cc & 0x0F;
	switch (cc) {
		case 0b0000: return false; break;
		case 0b0001: return (o->s ^ o->v); break;
		case 0b0010: return (o->z || (o->s ^ o->v)); break;
		case 0b0011: return (o->c || o->z); break;
		case 0b0100: return o->v == 1; break;
		case 0b0101: return o->s == 1; break;
		case 0b0110: return o->z == 1; break;
		case 0b0111: return o->c == 1; break;
		case 0b1000: return true; break;
		case 0b1001: return !(o->s ^ o->v); break;
		case 0b1010: return !(o->z || (o->s ^ o->v)); break;
		case 0b1011: return !(o->c || o->z); break;
		case 0b1100: return o->v == 0; break;
		case 0b1101: return o->s == 0; break;
		case 0b1110: return o->z == 0; break;
		case 0b1111: return o->c == 0; break;
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
		case 0x00: regPointer = &state->R0;  break; //Higher
		case 0x01: regPointer = &state->R1;  break;
		case 0x02: regPointer = &state->R2;  break;
		case 0x03: regPointer = &state->R3;  break;
		case 0x04: regPointer = &state->R4;  break;
		case 0x05: regPointer = &state->R5;  break;
		case 0x06: regPointer = &state->R6;  break;
		case 0x07: regPointer = &state->R7;  break;
		case 0x08: regPointer = &state->R0 + 1;  break; //Higher
		case 0x09: regPointer = &state->R1 + 1;  break;
		case 0x0a: regPointer = &state->R2 + 1;  break;
		case 0x0b: regPointer = &state->R3 + 1;  break;
		case 0x0c: regPointer = &state->R4 + 1;  break;
		case 0x0d: regPointer = &state->R5 + 1;  break;
		case 0x0e: regPointer = &state->R6 + 1;  break;
		case 0x0f: regPointer = &state->R7 + 1;  break;
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

int Disassemble8002(uint8_t *codebuffer, int pc){
	uint16_t* code = (&codebuffer[pc]);
	uint8_t upperHalf = code[0] >> 8;
	uint8_t lowerHalf = code[0];
	uint8_t field1 = (code[0] >> 4) & (0x0F);
	uint8_t field2 = code[0] & 0x0F;
	uint8_t opcode = upperHalf & (0x3F);
	uint8_t adMode = upperHalf & (0xC0);

	// printf("%02x \n", upperHalf);
	// printf("%02x \n", lowerHalf);
	// printf("%01x \n", field1);
	// printf("%01x \n", field2);
	// printf("%02x \n", opcode);
	// printf("%01x \n", adMode);

	int opwords = 1;

	printf("OPcode: %x, PC: %x\n",upperHalf,pc );
	switch (upperHalf)
	{
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
						case 0x00:  printf("ADD ");				//ADD Rd, #data
									findRegister(field2);
									printf(", #%02x", code[1]);
									opwords = 2;
									break;
						default:	printf("ADD ");				//ADD Rd, @Rs
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
		case 0x0f:	printf("LD EPU"); break;					//TODO: IMPLEMENT THIS!

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
					printf(field2);
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
		case 0x22:	switch(field1){	//TODO
						case 0x00:	printf("RESB ");				//RESB Rbd, Rs
									findRegister(field2);
									printf(", #%02x", code[1]);
									opwords = 2;
									break;

						default:	printf("RESB ");				//RESB @Rd, #b
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;

					}	break;
		case 0x23:	switch(field1){	//TODO
						case 0x00:	printf("RES ");				//RES Rd, Rs
									findRegister(field2);
									printf(", #%02x", code[1]);
									opwords = 2;
									break;

						default:	printf("RES ");				//RES @Rd, #b
									findRegister(field2);
									printf(", @");
									findRegister(field1);
									break;

					}	break;
		case 0x24:	switch(field1){
						case 0x00:	printf("SETB ");			//TODO: Implement
									break;
						default:	printf("SETB @");			//SETB @Rd, #b
									findregRegister(field1);
									printf(", #%01x", field2);
									break;
					} break;
		case 0x25:	switch(field1){
						case 0x00:	printf("SET ");				//TODO: Implement
									break;
						default:	printf("SET @");			//SET @Rd, #b
									findRegister(field1);
									printf(", #%01x", field2);
									break;
					} break;
		case 0x26:	switch(field1){
						case 0x00:	printf("BITB ");			//TODO: Implement
									break;
						default:	printf("BITB @");			//BITB @Rd, #b
									findregRegister(field1);
									printf(", #%01x", field2);
									break;
					} break;
		case 0x27:	switch(field1){
						case 0x00:	printf("BIT ");				//TODO: Implement
									break;
						default:	printf("BIT @");			//BIT @Rd, #b
									findRegister(field1);
									printf(", #%01x", field2);
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
						case 0x00:	printf("LDR ");				//LDR Rd, address TODO
									findRegister(field2);
									printf(", %04x", code[1]);
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
						case 0x00:	printf("LDRB %04x, ", code[1]);	//LDRB address, Rbs TODO
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
						case 0x00:	printf("LDR %04x, ", code[1]); //LDR address, Rs TODO
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
						case 0x00:	printf("LDAR ");			// LDAR Rd, address TODO
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
						case 0x00:	printf("LDRL ");			//LDRL RRd, address TODO
									findLongRegister(field2);
									printf(", %04x", code[1]);
									opwords = 2;
									break;
						default:	printf("LDL ");				//LDL RRd, Rs(#disp)
									findLongRegister(field2);
									printf(", ");
									findLongRegister(field1);
									printf("(#%04x)");
					} break;
		case 0x36:	printf("Reserved"); break;
		case 0x37:	switch(field1){
						case 0x00:	printf("LDRL %04x, ", code[1]);	//LDRL address, RRs TODO
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
						case 0x00:	printf("LDPS @");			//LDPS @Rs TODO
									findRegister(field1);
									break;
						default:	printf("Reserved");
									break;
					} break;
		case 0x3a:	printf("TODO: Hardware Instructions"); break;
		case 0x3b:	printf("TODO: Hardware Instructions"); break;
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
									printf(", %04x(");
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
									printf(", %04x(");
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
										case 0x05:	printf("LDB %04x");				//TODO
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
										case 0x00:	printf("COMB %04x(");			//COMB addr(Rd)
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
										case 0x05:	printf("LDB %04x(",code[1]);	//TODO
													findregRegister(field1);
													printf("), TODO");
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
		default: printf("%02x not implemented in dissasembler", upperHalf); break;
	}

	printf("||||");
	return opwords;
}

int Emulate8002(State8002* state){

	uint16_t *opcode = &state->memory[state->pc];

	int done = 0;

	uint8_t upperHalf = opcode[0] >> 8;
	uint8_t lowerHalf = opcode[0];
	uint8_t upperFour = opcode[0] >> 12;
	uint8_t upperTwo = opcode[0] >> 14;
	uint8_t secondNibble = upperHalf & 0x0F;
	uint8_t field1 = (opcode[0] >> 4) & (0x0F);
	uint8_t field2 = opcode[0] & 0x0F;

	//Disassemble8002(state->memory, state->pc);

	//state->R0 = 0x1111;
	//state->R3 = 0x1111;
	uint8_t  *memptr8  = state->memory;
	uint16_t *memptr16 = state->memory;
	uint64_t *memptr64 = state->memory;
	state->memory[1] = fix_8(0x01);
	state->memory[3] = fix_8(0x01);
	state->memory[5] = fix_8(0xFF);


	//printf("point to memory plus 1 is :%x\n",(*(memptr8 + 2) ));
	state->pc += 2;

	state->instructionsRun += 1;

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
				case 0b00000000:{ //ADDB,  IM or IR

					if(field1 == 0){	//ADDB (IM)
						state->pc+=2;
						uint8_t* destinationReg8 = returnByteRegisterPointer(field2, state);
						*destinationReg8 = *destinationReg8 + (opcode[1] >> 8); //assuming we only use the top bits
					}
					else{ //ADDB (IR)
						uint8_t* destinationReg8 = returnByteRegisterPointer(field2, state);
						uint8_t* sourceReg8 = returnByteRegisterPointer(field1, state);
						uint8_t* memptr8 = state->memory;
						uint8_t memData8 = *(memptr8 + *sourceReg8);
						*destinationReg8 = *destinationReg8 + memData8;
					}
				} break;
				case 0b00000001:{//ADD IM or IR
					if(field1 == 0){	//ADD (IM)
						state->pc+=2;
						uint16_t* destinationReg16 = returnByteRegisterPointer(field2, state);
						*destinationReg16 = fix_16( fix_16(*destinationReg16) + opcode[1] ); //assuming we only use the top bits
					}
					else{ //ADDB (IR)
						uint16_t* destinationReg16 = returnByteRegisterPointer(field2, state);
						uint16_t* sourceReg16 = returnByteRegisterPointer(field1, state);
						uint16_t* memptr16 = state->memory;
						uint16_t memData16 = fix_16(*(memptr16 + fix_16(*sourceReg16)));
						*destinationReg16 = fix_16(fix_16(*destinationReg16) + memData16);
					}
				}break;
				case 0b00010110:{//ADDL IM or IR
					if(field1 == 0){	//ADDB (IM)
						state->pc += 2 ;
						uint32_t* destinationReg32 = returnLongRegisterPointer(field2, state);
						uint32_t immediate = (opcode[1] << 16) & opcode[2];
						*destinationReg32 = fix_32( fix_32(*destinationReg32) + immediate ); //assuming we only use the top bits
					}
					else{ //ADDB (IR)
						uint32_t* destinationReg32 = returnLongRegisterPointer(field2, state);
						uint32_t* sourceReg32 = returnLongRegisterPointer(field1, state);
						uint32_t* memptr32 = state->memory;
						uint32_t memData32 = fix_32(*(memptr32 + fix_32(*sourceReg32)));
						*destinationReg32 = fix_32(*destinationReg32) + memData32;
					}
				}break;
				case 0b10000000:{ // ADDB (R)
		      uint8_t* destinationReg8 = returnByteRegisterPointer(field2, state);
		      uint8_t* sourceReg8 = returnByteRegisterPointer(field1, state);
		      *destinationReg8 = *destinationReg8 + *sourceReg8;
		    }break;
		    case 0b10000001:{ // ADD (R)
		      uint16_t* destinationReg16 = returnWordRegisterPointer(field2, state);
		      uint16_t* sourceReg16 = returnWordRegisterPointer(field1, state);
		      *destinationReg16 = fix_16(fix_16(*destinationReg16) + fix_16(*sourceReg16));
		    }break;
				case 0b10010110:{//ADDL, R
					uint32_t* destinationReg32 = returnLongRegisterPointer(field2, state);
		      uint32_t* sourceReg32 = returnLongRegisterPointer(field1, state);
		      *destinationReg32 = fix_32(fix_32(*destinationReg32) + fix_32(*sourceReg32));
				}break;
				case 0b01000000:{//ADDB, DA or X
					uint8_t* destinationReg8 = returnByteRegisterPointer(field2, state);
					state->pc+=2;
					uint8_t* memptr8 = state->memory;

					if(field1 == 0){	//ADDB (DA)
						*destinationReg8 = *destinationReg8 + *(memptr8 + opcode[1]); //assuming we only use the top bits
					}
					else{ //ADDB (IR)
						uint8_t* sourceReg8 = returnByteRegisterPointer(field1, state);
						*destinationReg8 = *destinationReg8 + *(memptr8 + opcode[1] + *sourceReg8); //assuming we only use the top bits
					}
				}break;
				case 0b01000001:{//ADD, DA or X
					uint16_t* destinationReg16 = returnWordRegisterPointer(field2, state);
					state->pc+=2;
					uint16_t* memptr16 = state->memory;

					if(field1 == 0){	//ADD (DA)
						*destinationReg16 = fix_16(fix_16(*destinationReg16) + fix_16(*(memptr16 + opcode[1]))); //assuming we only use the top bits
					}
					else{ //ADD (IR)
						uint16_t* sourceReg16 = returnWordRegisterPointer(field1, state);
						*destinationReg16 = fix_16(fix_16(*destinationReg16) + fix_16(*(memptr16 + opcode[1] + fix_16(*sourceReg16)))); //assuming we only use the top bits
					}
				}break;
				case 0b01010110:{//ADDL, DA or X
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
				}break;
				case 0b10001011:{//CP R  needs work on C and V flags
					uint16_t* destinationReg16 = returnWordRegisterPointer(field2, state);
				 	uint16_t* sourceReg16 = returnWordRegisterPointer(field1, state);
				 	uint32_t result = fix_16(*destinationReg16) - fix_16(*sourceReg16);
					ConditionCodes* cc = state->cc;
					if(result & 0x80 == fix_16(*destinationReg16) & 0x80){ // MSB has no borrow
						cc->c = 0;
					}
					else{
						cc->c = 1;
					}

					if(result == 0){
						state->cc->z = 1;
					}
					else{
						state->cc->z = 0;
					}

					if(result < 0){
						state->cc->s = 1;
					}
					else{
						state->cc->s = 0;
					}

					if(result & 0x100 > 0){
						state->cc->v = 1;
					}
					else{
						state->cc->s = 0;
					}
				}break;


				default: printf("dicks2");done=1; break;
			}
		}
	}

	//printf("\t");
	printf("R1: %i, R2: %i, R7: %i, R8: %i  ||| OP: %x, ||| PC: %x\n", fix_16(state->R1),fix_16(state->R2),fix_16(state->R7),fix_16(state->R8),upperHalf,state->pc-3);


	if(upperFour == 0xFF){
		return 1;
	}
	return done;
}



void ReadFileIntoMemoryAt(State8002* state, char* filename, uint32_t offset){
	state->pc = offset;
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
	state->cc = calloc(1,sizeof(ConditionCodes));
	state->cc->c = 1;
	state->cc->v = 1;
	state->cc->s = 1;
	state->cc->z = 1;
	state->cc->d = 1;
	state->cc->h = 1;
	state->memory = malloc(0x10000); // 64k
	state->instructionsRun = 0;




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
	// 	printf("%x\n",*(p8+i));
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

	ReadFileIntoMemoryAt(state, "jr.t", 255*8);

	while (done == 0){
		done = Emulate8002(state);
	}
	return 0;
}
