#ifndef RV2JVM_TABLE_H
#define RV2JVM_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "darray.h"

enum table_key_type {
	TABLE_KEY_NUMBER,
	TABLE_KEY_STRING
};

struct table_key {
	enum table_key_type type;
	uint32_t hash;
	union {
		int32_t number;
		char *string;
	} as;
};

struct table_value {
	struct table_key key;
	void *value;
};

struct table {
	struct table_value *values;
	size_t size;
	size_t capacity;
};


struct table_key to_number_key(int32_t number);
struct table_key to_string_key(char *string);
struct table *table_create();
bool table_set(struct table *table, struct table_key key, void *value);
struct table_value *table_get(struct table *table, struct table_key key);
void table_free(struct table *table);

#endif
