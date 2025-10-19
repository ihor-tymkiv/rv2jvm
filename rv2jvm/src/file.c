#include "file.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

char *read_file(const char *path)
{
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

void write_file(char *path, uint8_t *contents, size_t length)
{
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
