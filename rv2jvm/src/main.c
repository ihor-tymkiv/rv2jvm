#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "codegen.h"
#include "compiler.h"
#include "darray.h"
#include "file.h"

static void compile_files(int filepaths_n, char **filepaths)
{
	struct bytecode bytecode = { 0 };
	compile(filepaths_n, filepaths, &bytecode);
	char *compiled_filename = "RvRuntime.class";
	write_file(compiled_filename, bytecode.items, bytecode.size);

	darray_free(bytecode);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file...\n", argv[0]);
		exit(EX_USAGE);
	} else {
		compile_files(argc - 1, &argv[1]);
	}
	return 0;
}
