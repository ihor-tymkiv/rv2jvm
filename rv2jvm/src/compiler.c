#include "compiler.h"

#include "codegen.h"
#include "parser.h"
#include "lexer.h"

#include <stdio.h>

void compile(int filepaths_n, char **filepaths, struct bytecode *res)
{
	struct tokens tokens = { 0 };
	lex(filepaths_n, filepaths, &tokens);
	struct ir_element *ir;
	parse(tokens, &ir);
	generate_bytecode(ir, res);
}
