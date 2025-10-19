#include "compiler.h"

#include "codegen.h"
#include "parser.h"

#include <stdio.h>

void compile(int filepaths_n, char **filepaths, struct bytecode *res)
{
	struct ir_element *ir;
	parse(filepaths_n, filepaths, &ir);
	generate_bytecode(ir, res);
}
