#ifndef RV2JVM_CODEGEN_H
#define RV2JVM_CODEGEN_H

#include <stddef.h>
#include <stdint.h>

#include "ir.h"

struct bytecode {
	uint8_t *items;
	size_t size;
	size_t capacity;
};

void generate_bytecode(struct ir_element *ir, struct bytecode *res);

#endif
