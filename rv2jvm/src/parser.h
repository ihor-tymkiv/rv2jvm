#ifndef RV2JVM_PARSER_H
#define RV2JVM_PARSER_H

#include "codegen.h"
#include "tokens.h"

void parse(struct tokens tokens, struct ir_element **res);

#endif
