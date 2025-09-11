#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

struct compilation_result compile(char *source) {
	// stub
	size_t length = 2;
	uint8_t *bytecode = (uint8_t*)malloc(sizeof(uint8_t) * length);
	bytecode[0] = 0xFF;
	bytecode[1] = 0xC0;
	if (bytecode == NULL) {
		fprintf(stderr, "Could not allocate memory for bytecode");
		exit(EXIT_FAILURE);
	}
	struct compilation_result result = { 
		.bytecode = bytecode, .length = length
	};
	return result;
}

