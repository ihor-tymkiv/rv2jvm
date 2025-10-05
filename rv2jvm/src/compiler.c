#include "compiler.h"

#include "codegen.h"
#include "parser.h"

void compile(char *source, struct bytecode *res) {
	struct ir_element *ir;
	parse(source, &ir);
	generate_bytecode(ir, res);
}
