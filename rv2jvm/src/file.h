#ifndef RV2JVM_FILE_H
#define RV2JVM_FILE_H

#include <stddef.h>
#include <stdint.h>

char *read_file(const char *path);
void write_file(char *path, uint8_t *contents, size_t length);

#endif
