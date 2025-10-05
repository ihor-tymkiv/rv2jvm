#ifndef RV2JVM_CODEGEN_H
#define RV2JVM_CODEGEN_H

#include <stddef.h>
#include <stdint.h>

enum ir_element_type {
	IR_INSTRUCTION, 
	IR_LABEL, 
	IR_DIRECTIVE
};

enum ir_instruction_type { 
	TYPE_R3, 
	TYPE_R2_IMM12, 
	TYPE_R1_IMM20 
};

enum ir_instruction_register {
	X0,
	X1,
	X2,
	X3,
	X4,
	X5,
	X6,
	X7,
	X8,
	X9,
	X10,
	X11,
	X12,
	X13,
	X14,
	X15,
	X16,
	X17,
	X18,
	X19,
	X20,
	X21,
	X22,
	X23,
	X24,
	X25,
	X26,
	X27,
	X28,
	X29,
	X30,
	X31,
	PC
};

enum ir_instruction_mnemonic {
	// chapter 2.4.1
	ADDI,
	MV,
	SLTI,
	SLTIU,
	SEQZ,
	ANDI,
	ORI,
	XORI,
	NOT,
	SLLI,
	SRLI,
	SRAI,
	LUI,
	AUIPC,

	// chapter 2.4.2
	ADD,
	SUB,
	SLT,
	SLTU,
	SNEZ,
	AND,
	OR,
	XOR,
	SLL,
	SRL,
	SRA,

	// chapter 2.4.3
	NOP,

	// chapter 2.5.1
	JAL,
	J,
	JALR,
	JR,
	RET,

	// chapter 2.5.2
	BEQ,
	BNE,
	BLT,
	BLTU,
	BGE,
	BGEU,

	// chapter 2.6
	LW,
	LH,
	LHU,
	LB,
	LBU,
	SW,
	SH,
	SB,

	// chapter 2.7
	FENCE,

	// chapter 2.8
	ECALL,
	EBREAK
};

struct ir_instruction_r3 {
	enum ir_instruction_register rd;
	enum ir_instruction_register rs1;
	enum ir_instruction_register rs2;
};

struct ir_instruction_r2imm12 {
	enum ir_instruction_register rd;
	enum ir_instruction_register rs1;
	int16_t imm : 12;
};

struct ir_instruction_r1imm20 {
	enum ir_instruction_register rd;
	int32_t imm : 20;
};

struct ir_instruction {
	enum ir_instruction_type type;
	enum ir_instruction_mnemonic mnemonic;
	union {
		struct ir_instruction_r3 r3;
		struct ir_instruction_r2imm12 r2imm12;
		struct ir_instruction_r1imm20 r1imm20;
	} as;
};

struct ir_label {
	char *name;
};

struct ir_directive {
	char *name;
	char **operands;
	size_t operands_length;
};

struct ir_element {
	enum ir_element_type type;
	union {
		struct ir_instruction instruction;
		struct ir_label label;
		struct ir_directive directive;
	} as;
};

void generate_bytecode(struct ir_element *ir, uint8_t **res);

#endif
