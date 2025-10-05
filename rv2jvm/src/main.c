#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "codegen.h"
#include "compiler.h"

static char *read_file(const char *path) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\" for reading.\n", 
				path);
		exit(EX_IOERR);
	}

	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char *buffer = (char*)malloc(size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
		exit(EXIT_FAILURE);
	}

	size_t read = fread(buffer, sizeof(char), size, file);
	if (read < size) {
		fprintf(stderr, "Could not read file \"%s\".\n", path);
		exit(EX_IOERR);
	}
	buffer[read] = '\0';

	fclose(file);
	return buffer;
}

static void write_file(char *path, uint8_t *contents, size_t length) {
	FILE *file = fopen(path, "wb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\" for writing.\n", 
				path);
		exit(EX_IOERR);
	}

	size_t written = fwrite(contents, 1, length, file);
	if (written < length) {
		fprintf(stderr, "Could not write to file \"%s\".\n", path);
		exit(EX_IOERR);
	}

	fclose(file);
}

static char *get_compiled_filename(char *src_filename) {
	char *result = (char*)malloc(strlen(src_filename) + 7);
	if (result == NULL) {
		fprintf(stderr, "Could not allocate memory for compiled filename\n");
		exit(EXIT_FAILURE);
	}

	strcpy(result, src_filename);
	char *dot = strrchr(result, '.');
	if (dot == NULL) {
		strcat(result, ".class");
	} else {
		strcpy(dot, ".class");
	}
	return result;
}

static void compile_file(char *path) {
	struct bytecode bytecode = {0};
	char *src = read_file(path);
	compile(src, &bytecode);
	char *compiled_filename = get_compiled_filename(path);
	write_file(compiled_filename, bytecode.items, bytecode.size);

	free(compiled_filename);
	free(bytecode.items);
	free(src);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file...\n", argv[0]);
		exit(EX_USAGE);
	} else {
		for (int i = 1; i < argc; i++) {
			compile_file(argv[i]);
		}
	}
	return 0;
}
