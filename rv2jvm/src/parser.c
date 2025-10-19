#include "parser.h"

#include <stdlib.h>
#include <string.h>

#include "ir.h"

/**
 * stub
 *
 * RISC-V Equivalent:
 * addi x5, x0, 0      # i = 0
 * addi x6, x0, 5      # limit = 5
 * loop:
 * addi x5, x5, 1      # i++
 * add  x5, x5, x5     # i = i + i
 * blt  x5, x6, loop   # if (i < limit) goto loop
 * j    end
 * end:
 * # EOF
 */
void parse(int filepaths_n, char **filepaths, struct ir_element **res)
{
	*res = malloc(sizeof(struct ir_element) * 9);

	// 1. addi x5, x0, 0
	(*res)[0] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = ADDI,
				.type = TYPE_R2_OP,
				.as.r2op = {
					.rd = X5,
					.rs1 = X0,
					.op_type = OPERAND_IMM,
					.op.imm = 0
				}
			}
	};

	// 2. addi x6, x0, 5
	(*res)[1] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = ADDI,
				.type = TYPE_R2_OP,
				.as.r2op = {
					.rd = X6,
					.rs1 = X0,
					.op_type = OPERAND_IMM,
					.op.imm = 5
				}
			}
	};

	// 3. loop:
	(*res)[2] = (struct ir_element){
		.type = IR_LABEL,
			.as.label.name = "loop"
	};

	// 4. addi x5, x5, 1
	(*res)[3] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = ADDI,
				.type = TYPE_R2_OP,
				.as.r2op = {
					.rd = X5,
					.rs1 = X5,
					.op_type = OPERAND_IMM,
					.op.imm = 1
				}
			}
	};

	// 5. add x5, x5, x5
	(*res)[4] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = ADD,
				.type = TYPE_R3,
				.as.r3 = {
					.rd = X5,
					.rs1 = X5,
					.rs2 = X5
				}
			}
	};

	// 6. blt x5, x6, loop
	(*res)[5] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = BLT,
				.type = TYPE_R2_OP,
				.as.r2op = {
					.rs1 = X5,
					.rd = X6,
					.op_type = OPERAND_LABEL,
					.op.label = "loop"
				}
			}
	};

	// 7. j end
	(*res)[6] = (struct ir_element){
		.type = IR_INSTRUCTION,
			.as.instruction = {
				.mnemonic = J,
				.type = TYPE_R1_OP,
				.as.r1op = {
					.rd = X0,
					.op_type = OPERAND_LABEL,
					.op.label = "end"
				}
			}
	};

	// 8. end:
	(*res)[7] = (struct ir_element){
		.type = IR_LABEL,
			.as.label.name = "end"
	};


	// 9. EOF
	(*res)[8] = (struct ir_element) {
		.type = IR_EOF
	};
}
