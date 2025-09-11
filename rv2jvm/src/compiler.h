#ifndef RV2JVM_COMPILER_H
#define RV2JVM_COMPILER_H

#include <stdint.h>
#include <stdlib.h>

struct compilation_result {
	uint8_t *bytecode;
	size_t length;
};

struct compilation_result compile(char *source);

#endif
