#include "table.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sysexits.h>

#define TABLE_MAX_LOAD 0.75

struct table_key to_number_key(int32_t number) {
	struct table_key res = {
		.type = TABLE_KEY_NUMBER,
		.as.number = number
	};
	return res;
}

struct table_key to_string_key(char *string) {
	struct table_key res = {
		.type = TABLE_KEY_STRING,
		.as.string = string
	};
	return res;
}

struct table *table_create() {
	struct table *t = malloc(sizeof(*t));
	if (t == NULL) {
		fprintf(stderr, "Failed to allocate memory for table.\n");
		exit(EXIT_FAILURE);
	}
	t->capacity = 0;
	t->size = 0;
	t->values = NULL;
	return t;
}

void table_free(struct table *table) {
	for (size_t i = 0; i < table->size; i++) {
		if (table->values[i].value == NULL) {
			continue;
		}
		free(table->values[i].value);
	}
	free(table->values);
	free(table);
}

/*
 * Compute 32 bit FNV-1a hash.
 */
static uint32_t hash_string(char *string) {
	uint32_t hash = 0x811c9dc5;
	while (*string != '\0') {
		hash ^= (uint8_t)*string;
		hash *= 0x01000193;
		string++;
	}
	return hash;
}

/*
 * Compute 32 bit Thomas Wang Bit Mix Hash.
 */
static uint32_t hash_number(int32_t number) {
	uint32_t hash = (uint32_t)number;
	hash = ~hash + (hash << 15);
	hash ^= hash >> 12;
	hash += hash << 2;
	hash ^= hash >> 4;
	hash *= 2057;
	hash ^= hash >> 16;
	return hash;
}

static uint32_t hash(struct table_key key) {
	switch (key.type) {
	case TABLE_KEY_NUMBER:
		return hash_number(key.as.number);
	case TABLE_KEY_STRING:
		return hash_string(key.as.string);
	}
}

static bool keys_equal(struct table_key *k1, struct table_key *k2) {
	if (k1->type != k2->type) {
		return false;
	}
	switch (k1->type) {
	case TABLE_KEY_NUMBER:
		return k1->as.number == k2->as.number;
	case TABLE_KEY_STRING:
		return k1->hash == k2->hash &&
			strcmp(k1->as.string, k2->as.string) == 0;
	}
}

static struct table_value *find_slot(struct table *table, 
				     struct table_key *key) {
	uint32_t idx = key->hash % table->capacity;
	while (table->values[idx].value != NULL) {
		if (keys_equal(key, &table->values[idx].key)) {
			break;
		}
		idx = (idx + 1) % table->capacity;
	}
	return &table->values[idx];
}

static void grow_capacity(struct table *table) {
	size_t old_capacity = table->capacity;
	if (old_capacity == 0) {
		table->capacity = 64;
	} else {
		table->capacity *= 2;
	}

	size_t alloc_size = sizeof(*table->values) * table->capacity;
	struct table_value *values = malloc(alloc_size);
	if (values == NULL) {
		fprintf(stderr, "Failed to allocate memory for table of capacity %lu.\n", table->capacity);
		exit(EXIT_FAILURE);
	}
	memset(values, 0, alloc_size);
	for (size_t i = 0; i < old_capacity; i++) {
		struct table_value *src = &table->values[i];
		if (src->value == NULL) {
			continue;
		}
		struct table_value *dst = find_slot(table, &src->key);
		dst->key = src->key;
		dst->value = src->value;
	}
	if (table->values != NULL) {
		free(table->values);
	}
	table->values = values;
}

void table_set(struct table *table, struct table_key key, void *value) {
	if (table->size + 1 > table->capacity * TABLE_MAX_LOAD) {
		grow_capacity(table);
	}
	key.hash = hash(key);
	struct table_value *slot = find_slot(table, &key);
	if (slot->value == NULL) {
		table->size++;
	}
	slot->key = key;
	slot->value = value;
}

struct table_value *table_get(struct table *table, struct table_key key) {
	if (table->size == 0) {
		return NULL;
	}
	key.hash = hash(key);
	struct table_value *slot = find_slot(table, &key);
	if (slot->value == NULL) {
		return NULL;
	}
	return slot;
}
