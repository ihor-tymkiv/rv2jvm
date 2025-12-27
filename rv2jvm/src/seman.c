#include "seman.h"

#include "ir.h"
#include "table.h"

void referenced_labels_exist(struct ir_element *ir)
{
	struct table *declared_labels = table_create();
	for (struct ir_element *it = ir; it->type != IR_EOF; it++) {
		if (it->type == IR_LABEL) {
			bool new = table_set(declared_labels, 
					     to_string_key(it->as.label.name), it);
			if (!new) {
				fprintf(stderr, "Error: Label '%s' already declared.\n", it->as.label.name);
				exit(EXIT_FAILURE);
			}
		}	
    	}

	for (struct ir_element *it = ir; it->type != IR_EOF; it++) {
		if (it->type != IR_INSTRUCTION) {
			continue;
		}
		struct ir_instruction instruction = it->as.instruction;
		char *label;
		switch (instruction.type) {
		case TYPE_R2_OP:
			if (instruction.as.r2op.op_type != OPERAND_LABEL) {
				continue;
			}
			label = instruction.as.r2op.op.label;
			break;
		case TYPE_R1_OP:
			if (instruction.as.r1op.op_type != OPERAND_LABEL) {
				continue;
			}
			label = instruction.as.r1op.op.label;
			break;
		default:
			continue;
		}
		if (table_get(declared_labels, to_string_key(label)) == NULL) {
			fprintf(stderr, "Error: Referenced label '%s' not found.\n", label);
			exit(EXIT_FAILURE);
		}
	}
}

void seman(struct ir_element *ir)
{
	referenced_labels_exist(ir);
}
