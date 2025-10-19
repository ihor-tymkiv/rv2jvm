#include "codegen.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "darray.h"
#include "ir.h"
#include "table.h"

#define DEBUG

#define CONSTANT_POOL_LIMIT 65536

#define THIS_CLASS_NAME "RvRuntime"
#define THIS_CLASS "this_class"
#define SUPER_CLASS_NAME "java/lang/Object"
#define SUPER_CLASS "super_class"

#define THREAD_LOCAL "java/lang/ThreadLocal"
#define THREAD_LOCAL_CLASS "thread_local"
#define REGISTERS_FIELD "registers_field"
#define REGISTERS_FIELD_NAME "registers"
#define REGISTERS_FIELD_DESCRIPTOR "L" THREAD_LOCAL ";"
#define REGISTERS_FIELD_SIGNATURE "L" THREAD_LOCAL "<[J>;"
#define REGISTERS_FIELD_NAMEANDTYPE "registers_nameandtype"
#define REGISTERS_FIELDREF "registers_fieldref"

#define MEMORY_FIELD "memory_field"
#define MEMORY_FIELD_NAME "memory"

#define LONG_ARRAY_CLASS "long_array_class"
#define LONG_ARRAY_DESCRIPTOR "[J"
#define STRING_CLASS_NAME "java/lang/String"
#define STRING_ARRAY_CLASS "string_array_class"
#define STRING_ARRAY_DESCRIPTOR "[L" STRING_CLASS_NAME ";"

#define INIT_METHOD_NAME "<init>"
#define CLINIT_METHOD_NAME "<clinit>"
#define MAIN_METHOD_NAME "main"
#define MAIN_METHOD_DESCRIPTOR "(" STRING_ARRAY_DESCRIPTOR ")V"
#define NO_ARGS_VOID_DESCRIPTOR "()V"
#define INIT_METHOD_NAMEANDTYPE "init_method_nameandtype"

#define THREAD_LOCAL_GET_METHODREF "thread_local_get_methodref"
#define THREAD_LOCAL_GET_METHOD_NAMEANDTYPE "thread_local_get_method_nameandtype"
#define THREAD_LOCAL_GET_METHOD_NAME "get"
#define THREAD_LOCAL_GET_METHOD_DESCRIPTOR "()L" SUPER_CLASS_NAME ";"
#define THREAD_LOCAL_INIT_METHODREF "thread_local_init_methodref"
#define THREAD_LOCAL_SET_METHODREF "thread_local_set_methodref"
#define THREAD_LOCAL_SET_METHOD_NAMEANDTYPE "thread_local_set_method_nameandtype"
#define THREAD_LOCAL_SET_METHOD_NAME "set"
#define THREAD_LOCAL_SET_METHOD_DESCRIPTOR "(L" SUPER_CLASS_NAME  ";)V"

#define SIGNATURE "Signature"
#define CODE "Code"
#define STACK_MAP_TABLE "StackMapTable"

enum jvm_constant_tag {
	JVM_CONSTANT_UTF8 = 1,
	JVM_CONSTANT_INTEGER = 3,
	JVM_CONSTANT_CLASS = 7,
	JVM_CONSTANT_STRING = 8,
	JVM_CONSTANT_FIELDREF = 9,
	JVM_CONSTANT_METHODREF = 10,
	JVM_CONSTANT_NAMEANDTYPE = 12
};

enum jvm_access_flag {
	JVM_ACC_PUBLIC = 0x0001,
	JVM_ACC_PRIVATE = 0x0002,
	JVM_ACC_STATIC = 0x0008,
	JVM_ACC_FINAL = 0x0010,
	JVM_ACC_SYNTHETIC = 0x1000
};

enum jvm_atype {
	JVM_T_LONG = 11,
};

enum jvm_item {
	JVM_ITEM_LONG = 4,
	JVM_ITEM_OBJECT = 7,
};

enum jvm_frame_type {
	JVM_SAME_FRAME_EXTENDED = 251
};

enum jvm_opcode {
	JVM_BIPUSH = 16,
	JVM_LDC_W = 19,
	JVM_LLOAD_2 = 32,
	JVM_ALOAD_1 = 43,
	JVM_LALOAD = 47,
	JVM_LSTORE_2 = 65,
	JVM_ASTORE_1 = 76,
	JVM_LASTORE = 80,
	JVM_POP2 = 88,
	JVM_DUP = 89,
	JVM_LADD = 97,
	JVM_I2L = 133,
	JVM_LCMP = 148,
	JVM_IFEQ = 153,
	JVM_IFGT = 157,
	JVM_RETURN = 177,
	JVM_GETSTATIC = 178,
	JVM_PUTSTATIC = 179,
	JVM_INVOKEVIRTUAL = 182,
	JVM_INVOKESPECIAL = 183,
	JVM_NEW = 187,
	JVM_NEWARRAY = 188,
	JVM_CHECKCAST = 192,
	JVM_GOTO_W = 200,
};

struct constant_pool_index {
	uint16_t index;
};

struct code_label_offset {
	uint16_t offset;
};

struct label_reference {
	uint16_t opcode_offset;
	uint16_t branch_offset;
};

struct label_references {
	struct label_reference *items;
	size_t size;
	size_t capacity;
};

struct local {
	enum jvm_item tag;
	uint16_t constant_pool_index;
};

struct stack_map_frame {
	uint8_t frame_type;
	uint16_t target_offset;
	struct local locals[2];
};

struct stack_map_frames {
	struct stack_map_frame *items;
	size_t size;
	size_t capacity;
};

struct code {
	uint16_t max_stack;
	uint16_t max_locals;
	struct bytecode *code;
	uint16_t exception_table_length;
	uint16_t attributes_count;
	struct stack_map_frames *stack_map_frames;
};

struct codegen {
	struct ir_element *ir;
	struct bytecode *res;
	struct table *constant_map;
	struct table *code_label_offsets;
	struct table *label_references;
};

static struct code create_code()
{
	struct code c = { 0 };
	c.code = malloc(sizeof(*c.code));
	c.code->capacity = 0;
	c.code->size = 0;
	c.code->items = NULL;
	c.stack_map_frames = malloc(sizeof(*c.stack_map_frames));
	c.stack_map_frames->items = NULL;
	c.stack_map_frames->size = 0;
	c.stack_map_frames->capacity = 0;
	return c;
}

static void free_code(struct code *c)
{
	darray_free((*c->code));
	free(c->code);
	darray_free((*c->stack_map_frames));
	free(c->stack_map_frames);
}

static void init_codegen(struct codegen *c, struct ir_element *ir,
			 struct bytecode *res)
{
	c->res = res;
	c->ir = ir;
	c->constant_map = table_create();
	c->code_label_offsets = table_create();
	c->label_references = table_create();
}

static void free_codegen(struct codegen *c)
{
	table_free(c->constant_map);
	table_free(c->code_label_offsets);
	table_free(c->label_references);
}

static bool add_constant(struct codegen *c, struct table_key key)
{
	struct constant_pool_index *value = malloc(sizeof(*value));
	if (value == NULL) {
		fprintf(stderr, "Failed to allocate memory for constant_pool_index.\n");
		exit(EXIT_FAILURE);
	}
	value->index = c->constant_map->size + 1;
	bool added = table_get(c->constant_map, key) == NULL;
	if (added) {
		table_set(c->constant_map, key, value);
#ifdef DEBUG
		printf("%d: ", value->index);
		switch (key.type) {
		case TABLE_KEY_STRING:
			printf("'%s'", key.as.string);
			break;
		case TABLE_KEY_NUMBER:
			printf("%d", key.as.number);
			break;
		}
		printf("\n");
#endif
	}
	return added;
}

static struct constant_pool_index *get_constant(struct codegen *c,
						struct table_key key)
{
	struct table_value *table_value = table_get(c->constant_map, key);
	if (table_value == NULL) {
		return NULL;
	}
	return (struct constant_pool_index*)table_value->value;
}

static uint16_t get_constant_index(struct codegen *c, struct table_key key)
{
	struct constant_pool_index *index = get_constant(c, key);
	if (index == NULL) {
		fprintf(stderr, "Failed to retrieve constant index using key ");
		switch (key.type) {
		case TABLE_KEY_NUMBER:
			fprintf(stderr, "%d", key.as.number);
		case TABLE_KEY_STRING:
			fprintf(stderr, "'%s'", key.as.string);
		}
		fprintf(stderr, ".\n");
		exit(EXIT_FAILURE);
	}
	return index->index;
}

static void set_code_label_offset(struct codegen *c, struct table_key key,
				  uint16_t offset)
{
	struct code_label_offset *value = malloc(sizeof(*value));
	if (value == NULL) {
		fprintf(stderr, "Failed to allocate memory for code_label_offset.\n");
		exit(EXIT_FAILURE);
	}
#ifdef DEBUG
	printf("set_code_label_offset: '%s' %d\n", key.as.string, offset);
#endif
	value->offset = offset;
	table_set(c->code_label_offsets, key, value);
}

static uint16_t get_code_label_offset(struct codegen *c, char *label)
{
	struct table_value *table_value = table_get(c->code_label_offsets,
						    to_string_key(label));
	if (table_value == NULL) {
		fprintf(stderr, "Failed to retrieve code label offset for label '%s'.\n",
			label);
		exit(EXIT_FAILURE);
	}
	return ((struct code_label_offset*)table_value->value)->offset;
}

static struct label_references *get_label_references(struct codegen *c,
						     char *label)
{
	struct table_value *table_value = table_get(c->label_references,
						    to_string_key(label));
	if (table_value == NULL) {
		return NULL;
	}
	return table_value->value;
}

static void add_label_reference(struct codegen *c, char *label,
				uint16_t opcode_offset, uint16_t branch_offset)
{
	struct label_references *label_references = get_label_references(c, label);
	if (label_references == NULL) {
		label_references = malloc(sizeof(*label_references));
		if (label_references == NULL) {
			fprintf(stderr, "Failed to allocate memory for label_reference.\n");
			exit(EXIT_FAILURE);
		}
		table_set(c->label_references, to_string_key(label),
			  label_references);
	}
	struct label_reference reference = {
		.opcode_offset = opcode_offset,
		.branch_offset = branch_offset
	};
#ifdef DEBUG
	printf("add_label_reference: '%s' %d %d\n", label, opcode_offset, branch_offset);
#endif
	darray_append((*label_references), reference);
}

static void add_stack_frame(struct codegen *c, struct code *code,
			    uint16_t target_offset)
{
#ifdef DEBUG
	printf("add_stack_frame: target_offset %d\n", target_offset);
#endif
	struct stack_map_frame frame = {
		.target_offset = target_offset,
	};
	darray_append((*code->stack_map_frames), frame);
}

static void write_byte(struct bytecode *bytecode, uint8_t byte)
{
	darray_append((*bytecode), byte);
}

static void write_bytes(struct bytecode *bytecode, int64_t bytes,
			int8_t bytes_n)
{
	for (int8_t i = bytes_n - 1; i >= 0; i--) {
		write_byte(bytecode, bytes >> (i * 8));
	}
}

static void overwrite_bytes(struct bytecode *bytecode, size_t start,
			    int64_t bytes, int8_t bytes_n)
{
	size_t bytecode_idx = start;
	for (int8_t i = bytes_n - 1; i >= 0; i--) {
		bytecode->items[bytecode_idx] = bytes >> (i * 8);
		bytecode_idx++;
	}
}

static void write(struct codegen *c, uint8_t byte)
{
	write_byte(c->res, byte);
	//darray_append((*c->res), byte);
}

static void write_int(struct codegen *c, uint64_t bytes, size_t bytes_n)
{
	write_bytes(c->res, bytes, bytes_n);
}

static void magic(struct codegen *c)
{
	write_int(c, 0xCAFEBABE, 4);
}

static void minor_version(struct codegen *c)
{
	write_int(c, 0, 2);
}

static void major_version(struct codegen *c)
{
	write_int(c, 52, 2);
}

static void constant_integer_info(struct codegen *c, uint32_t value)
{
	write(c, JVM_CONSTANT_INTEGER);
	write_int(c, value, 4);
}

static void constant_utf8_info(struct codegen *c, uint16_t length, char *bytes)
{
	write(c, JVM_CONSTANT_UTF8);
	write_int(c, length, 2);

	for (uint16_t i = 0; i < length; i++) {
		write(c, (uint8_t)bytes[i]);
	}
}

static void constant_class_info(struct codegen *c, uint16_t name_index)
{
	write(c, JVM_CONSTANT_CLASS);
	write_int(c, name_index, 2);
}

static void constant_nameandtype_info(struct codegen *c, uint16_t name_idx,
				      uint16_t descriptor_idx)
{
	write(c, JVM_CONSTANT_NAMEANDTYPE);
	write_int(c, name_idx, 2);
	write_int(c, descriptor_idx, 2);
}

static void constant_methodref_info(struct codegen *c, uint16_t class_idx,
				    uint16_t nameandtype_idx)
{
	write(c, JVM_CONSTANT_METHODREF);
	write_int(c, class_idx, 2);
	write_int(c, nameandtype_idx, 2);
}

static void constant_fieldref_info(struct codegen *c, uint16_t class_idx,
				   uint16_t nameandtype_idx)
{
	write(c, JVM_CONSTANT_FIELDREF);
	write_int(c, class_idx, 2);
	write_int(c, nameandtype_idx, 2);
}

static uint16_t add_utf8_to_pool(struct codegen *c, char *string)
{
	struct table_key key = to_string_key(string);
	struct constant_pool_index *index = get_constant(c, key);
	if (index == NULL) {
		add_constant(c, to_string_key(string));
		constant_utf8_info(c, strlen(string), string);
		return c->constant_map->size;
	}
	return index->index;
}

static void add_class_to_pool(struct codegen *c, char *class_name, char *key)
{
	uint16_t idx = add_utf8_to_pool(c, class_name);
	constant_class_info(c, idx);
	add_constant(c, to_string_key(key));
}

static void add_nameandtype_to_pool(struct codegen *c, char *key,
				    char *name, char *descriptor)
{
	uint16_t name_idx = add_utf8_to_pool(c, name);
	uint16_t descriptor_idx = add_utf8_to_pool(c, descriptor);
	constant_nameandtype_info(c, name_idx, descriptor_idx);
	add_constant(c, to_string_key(key));
}

static void add_methodref_to_pool(struct codegen *c, char *class_key,
				  char *method_key, char *nameandtype_key,
				  char *name, char *descriptor)
{
	add_nameandtype_to_pool(c, nameandtype_key, name, descriptor);
	uint16_t nameandtype_idx = c->constant_map->size;
	uint16_t class_idx = get_constant_index(c, to_string_key(class_key));
	constant_methodref_info(c, class_idx, nameandtype_idx);
	add_constant(c, to_string_key(method_key));
}

static void add_fieldref_to_pool(struct codegen *c, char *class_key,
				 char *field_key, char *nameandtype_key,
				 char *name, char *descriptor)
{
	add_nameandtype_to_pool(c, nameandtype_key, name, descriptor);
	uint16_t nameandtype_idx = c->constant_map->size;
	uint16_t class_idx = get_constant_index(c, to_string_key(class_key));
	constant_fieldref_info(c, class_idx, nameandtype_idx);
	add_constant(c, to_string_key(field_key));
}

static void load_constant_from_instruction_at(struct codegen *c, size_t idx)
{
	struct ir_instruction instruction = c->ir[idx].as.instruction;
	int32_t imm;
	switch (instruction.type) {
	case TYPE_R1_OP:
		if (instruction.as.r1op.op_type == OPERAND_IMM) {
			imm = instruction.as.r1op.op.imm;
		}
		break;
	case TYPE_R2_OP:
		if (instruction.as.r2op.op_type == OPERAND_IMM) {
			imm = instruction.as.r2op.op.imm;
		}
		break;
	default:
		return;
	}

	if (add_constant(c, to_number_key(imm))) {
		constant_integer_info(c, imm);
	}
}

static void constant_pool(struct codegen *c)
{
	size_t pool_size_idx = c->res->size;
	write_int(c, 0, 2);

	add_class_to_pool(c, SUPER_CLASS_NAME, SUPER_CLASS);
	add_class_to_pool(c, THIS_CLASS_NAME, THIS_CLASS);
	add_class_to_pool(c, THREAD_LOCAL, THREAD_LOCAL_CLASS);
	add_class_to_pool(c, LONG_ARRAY_DESCRIPTOR, LONG_ARRAY_CLASS);
	add_class_to_pool(c, STRING_ARRAY_DESCRIPTOR, STRING_ARRAY_CLASS);
	add_utf8_to_pool(c, REGISTERS_FIELD_NAME);
	add_utf8_to_pool(c, REGISTERS_FIELD_DESCRIPTOR);
	add_utf8_to_pool(c, REGISTERS_FIELD_SIGNATURE);
	add_utf8_to_pool(c, REGISTERS_FIELD);
	add_utf8_to_pool(c, MEMORY_FIELD_NAME);
	add_utf8_to_pool(c, MEMORY_FIELD);
	add_utf8_to_pool(c, SIGNATURE);
	add_utf8_to_pool(c, INIT_METHOD_NAME);
	add_utf8_to_pool(c, CLINIT_METHOD_NAME);
	add_utf8_to_pool(c, MAIN_METHOD_NAME);
	add_utf8_to_pool(c, MAIN_METHOD_DESCRIPTOR);
	add_utf8_to_pool(c, NO_ARGS_VOID_DESCRIPTOR);
	add_utf8_to_pool(c, CODE);
	add_utf8_to_pool(c, STACK_MAP_TABLE);
	add_methodref_to_pool(c, THREAD_LOCAL_CLASS, THREAD_LOCAL_INIT_METHODREF,
			      INIT_METHOD_NAMEANDTYPE, INIT_METHOD_NAME,
			      NO_ARGS_VOID_DESCRIPTOR);
	add_methodref_to_pool(c, THREAD_LOCAL_CLASS, THREAD_LOCAL_SET_METHODREF,
			      THREAD_LOCAL_SET_METHOD_NAMEANDTYPE,
			      THREAD_LOCAL_SET_METHOD_NAME,
			      THREAD_LOCAL_SET_METHOD_DESCRIPTOR);
	add_methodref_to_pool(c, THREAD_LOCAL_CLASS, THREAD_LOCAL_GET_METHODREF,
			      THREAD_LOCAL_GET_METHOD_NAMEANDTYPE,
			      THREAD_LOCAL_GET_METHOD_NAME,
			      THREAD_LOCAL_GET_METHOD_DESCRIPTOR);
	add_fieldref_to_pool(c, THIS_CLASS, REGISTERS_FIELDREF,
			     REGISTERS_FIELD_NAMEANDTYPE, REGISTERS_FIELD_NAME,
			     REGISTERS_FIELD_DESCRIPTOR);

	for (size_t i = 0; c->ir[i].type != IR_EOF; i++) {
		switch (c->ir[i].type) {
		case IR_INSTRUCTION:
			load_constant_from_instruction_at(c, i);
			break;
		default:
			break;
		}
	}

	overwrite_bytes(c->res, pool_size_idx, c->constant_map->size + 1, 2);
}

static void access_flags(struct codegen *c)
{
	write_int(c, JVM_ACC_PUBLIC | JVM_ACC_SYNTHETIC, 2);
}

static void this_class(struct codegen *c)
{
	uint16_t idx = get_constant_index(c, to_string_key(THIS_CLASS));
	write_int(c, idx, 2);
}

static void super_class(struct codegen *c)
{
	uint16_t idx = get_constant_index(c, to_string_key(SUPER_CLASS));
	write_int(c, idx, 2);
}

static void interfaces(struct codegen *c)
{
	write_int(c, 0, 2);
}

static void add_field(struct codegen *c, uint16_t access_mask, char *name,
		      char *descriptor, char *signature)
{

	uint16_t name_idx = get_constant_index(c, to_string_key(name));
	uint16_t descriptor_idx = get_constant_index(c, to_string_key(descriptor));
	write_int(c, access_mask, 2);
	write_int(c, name_idx, 2);
	write_int(c, descriptor_idx, 2);

	if (signature != NULL) {
		write_int(c, 1, 2);
		uint16_t idx = get_constant_index(c, to_string_key(SIGNATURE));
		write_int(c, idx, 2);
		write_int(c, 2, 4);
		idx = get_constant_index(c, to_string_key(REGISTERS_FIELD_SIGNATURE));
		write_int(c, idx, 2);
	} else {
		write_int(c, 0, 2);
	}
}

static void fields(struct codegen *c)
{
	write_int(c, 2, 2);

	uint16_t mask = JVM_ACC_PRIVATE | JVM_ACC_FINAL | JVM_ACC_STATIC
			| JVM_ACC_SYNTHETIC;
	add_field(c, mask, REGISTERS_FIELD_NAME, REGISTERS_FIELD_DESCRIPTOR,
		  REGISTERS_FIELD_SIGNATURE);
	add_field(c, mask, MEMORY_FIELD_NAME, LONG_ARRAY_DESCRIPTOR, NULL);
}

static void sort_stack_map_frames(struct stack_map_frames *stack_map_frames)
{
	bool swapped;
	struct stack_map_frame *frames = stack_map_frames->items;
	do {
		swapped = false;
		for (size_t i = 1; i < stack_map_frames->size; i++) {
			if (frames[i - 1].target_offset > frames[i].target_offset) {
				struct stack_map_frame tmp = frames[i - 1];
				frames[i - 1] = frames[i];
				frames[i] = tmp;
				swapped = true;
			}
		}
	} while (swapped);
}

static uint32_t add_stack_table_attribute_entries(struct codegen *c,
						  struct stack_map_frames *stack_map_frames)
{
	uint32_t attribute_length = 2;
	sort_stack_map_frames(stack_map_frames);
	uint16_t previous_offset_deltas = 0;
	for (size_t i = 0; i < stack_map_frames->size; i++) {
		struct stack_map_frame frame = stack_map_frames->items[i];
		attribute_length += 1;
		uint16_t offset_delta;
		uint8_t frame_type;
		if (i != 0) {
			offset_delta = frame.target_offset - previous_offset_deltas - i;
			frame_type = JVM_SAME_FRAME_EXTENDED;
			write(c, frame_type);
			write_int(c, offset_delta, 2);
			attribute_length += 2;
		} else {
			offset_delta = frame.target_offset;
			frame_type = 251 + 2;
			frame.locals[0].tag = JVM_ITEM_OBJECT;
			frame.locals[0].constant_pool_index =
				get_constant_index(c, to_string_key(LONG_ARRAY_CLASS));
			frame.locals[1].tag = JVM_ITEM_LONG;
			write(c, frame_type);
			write_int(c, offset_delta, 2);
			write(c, frame.locals[0].tag);
			write_int(c, frame.locals[0].constant_pool_index, 2);
			write(c, frame.locals[1].tag);
			attribute_length += 2 + 1 + 2 + 1;
		}

		previous_offset_deltas += offset_delta;
	}
	return attribute_length;
}

static uint32_t add_stack_table_attribute(struct codegen *c,
					  struct stack_map_frames *stack_map_frames)
{
	uint16_t idx = get_constant_index(c, to_string_key(STACK_MAP_TABLE));
	write_int(c, idx, 2);
	uint16_t attribute_length_idx = c->res->size;
	write_int(c, 0, 4);
	write_int(c, stack_map_frames->size, 2);
	uint32_t attribute_length =
		add_stack_table_attribute_entries(c, stack_map_frames);
	overwrite_bytes(c->res, attribute_length_idx, attribute_length, 4);
	return attribute_length + 6;
}

static void add_code_attribute(struct codegen *c, struct code *code)
{
	uint16_t idx = get_constant_index(c, to_string_key(CODE));
	write_int(c, idx, 2);
	uint16_t attribute_length_idx = c->res->size;
	write_int(c, 0, 4);
	write_int(c, code->max_stack, 2);
	write_int(c, code->max_locals, 2);
	write_int(c, code->code->size, 4);
	for (size_t i = 0; i < code->code->size; i++) {
		write(c, code->code->items[i]);
	}
	write_int(c, code->exception_table_length, 2);
	if (code->stack_map_frames->size > 0) {
		code->attributes_count += 1;
	}
	write_int(c, code->attributes_count, 2);
	uint32_t attribute_length = code->code->size + 12;
	if (code->stack_map_frames->size > 0) {
		attribute_length +=
			add_stack_table_attribute(c, code->stack_map_frames);
	}
	overwrite_bytes(c->res, attribute_length_idx, attribute_length, 4);
}

static void add_method(struct codegen *c, uint16_t mask, char *name,
		       char *descriptor, struct code *code)
{
	write_int(c, mask, 2);
	uint16_t name_index = get_constant_index(c, to_string_key(name));
	write_int(c, name_index, 2);
	uint16_t descriptor_index = get_constant_index(c, to_string_key(descriptor));
	write_int(c, descriptor_index, 2);
	write_int(c, 1, 2);
	add_code_attribute(c, code);
}

static void clinit_method_code(struct codegen *c, struct code *code)
{
	code->max_stack = 3;
	uint16_t idx;
	write_byte(code->code, JVM_NEW);
	idx = get_constant_index(c, to_string_key(THREAD_LOCAL_CLASS));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_DUP);
	write_byte(code->code, JVM_INVOKESPECIAL);
	idx = get_constant_index(c, to_string_key(THREAD_LOCAL_INIT_METHODREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_PUTSTATIC);
	idx = get_constant_index(c, to_string_key(REGISTERS_FIELDREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_GETSTATIC);
	idx = get_constant_index(c, to_string_key(REGISTERS_FIELDREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_BIPUSH);
	write_byte(code->code, 32);
	write_byte(code->code, JVM_NEWARRAY);
	write_byte(code->code, JVM_T_LONG);
	write_byte(code->code, JVM_INVOKEVIRTUAL);
	idx = get_constant_index(c, to_string_key(THREAD_LOCAL_SET_METHODREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_RETURN);
}

static void load_registers_into_local(struct codegen *c, struct code *code)
{
	write_byte(code->code, JVM_GETSTATIC);
	uint16_t idx = get_constant_index(c, to_string_key(REGISTERS_FIELDREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_INVOKEVIRTUAL);
	idx = get_constant_index(c, to_string_key(THREAD_LOCAL_GET_METHODREF));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_CHECKCAST);
	idx = get_constant_index(c, to_string_key(LONG_ARRAY_CLASS));
	write_bytes(code->code, idx, 2);
	write_byte(code->code, JVM_ASTORE_1);
}

static void write_label(struct codegen *c, size_t ir_idx, struct code *code)
{
	struct ir_label label = c->ir[ir_idx].as.label;
	switch (c->ir[ir_idx + 1].type) {
	case IR_EOF:
	case IR_INSTRUCTION:
		set_code_label_offset(c, to_string_key(label.name),
				      code->code->size);
		break;
	default:
		break;
	}
}

static void load_register(struct code *code, enum ir_instruction_register r)
{
	write_byte(code->code, JVM_ALOAD_1);
	write_byte(code->code, JVM_BIPUSH);
	write_byte(code->code, r);
	write_byte(code->code, JVM_LALOAD);
}

static void store_register(struct code *code, enum ir_instruction_register r)
{
	if (r == X0) {
		write_byte(code->code, JVM_POP2);
		return;
	}
	write_byte(code->code, JVM_LSTORE_2);
	write_byte(code->code, JVM_ALOAD_1);
	write_byte(code->code, JVM_BIPUSH);
	write_byte(code->code, r);
	write_byte(code->code, JVM_LLOAD_2);
	write_byte(code->code, JVM_LASTORE);
}

static void load_constant(struct codegen *c, struct code *code,
			  uint32_t constant)
{
	write_byte(code->code, JVM_LDC_W);
	write_bytes(code->code, get_constant_index(c,
						   to_number_key(constant)), 2);
	write_byte(code->code, JVM_I2L);
}

static void jump(struct codegen *c, struct code *code, char *label)
{
	add_label_reference(c, label, code->code->size, code->code->size + 1);
	write_byte(code->code, JVM_GOTO_W);
	write_bytes(code->code, 0, 4);
}

static void write_instruction(struct codegen *c, size_t ir_idx,
			      struct code *code)
{
	struct ir_instruction instr = c->ir[ir_idx].as.instruction;
	switch (instr.type) {
	case TYPE_R3:
		load_register(code, instr.as.r3.rs1);
		load_register(code, instr.as.r3.rs2);
		switch (instr.mnemonic) {
		case ADD:
			write_byte(code->code, JVM_LADD);
			break;
		default:
			break;
		}
		store_register(code, instr.as.r3.rd);
		break;

	case TYPE_R2_OP:
		switch (instr.mnemonic) {
		case ADDI:
			load_register(code, instr.as.r2op.rs1);
			load_constant(c, code, instr.as.r2op.op.imm);
			write_byte(code->code, JVM_LADD);
			store_register(code, instr.as.r2op.rd);
			break;
		case BNE:
			load_register(code, instr.as.r2op.rd);
			load_register(code, instr.as.r2op.rs1);
			write_byte(code->code, JVM_LCMP);
			add_stack_frame(c, code, code->code->size + 8);
			write_byte(code->code, JVM_IFEQ);
			write_bytes(code->code, 8, 2);
			jump(c, code, instr.as.r2op.op.label);
			break;
		case BLT:
			load_register(code, instr.as.r2op.rd);
			load_register(code, instr.as.r2op.rs1);
			write_byte(code->code, JVM_LCMP);
			add_stack_frame(c, code, code->code->size + 8);
			write_byte(code->code, JVM_IFGT);
			write_bytes(code->code, 8, 2);
			jump(c, code, instr.as.r2op.op.label);
		default:
			break;
		}
		break;

	case TYPE_R1_OP:
		switch (instr.mnemonic) {
		case J:
			jump(c, code, instr.as.r1op.op.label);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
}

static void update_label_reference(struct codegen *c, struct code *code,
				   char *label)
{
	struct label_references *references = get_label_references(c, label);
	if (references == NULL) {
		return;
	}
	for (size_t i = 0; i < references->size; i++) {
		struct label_reference reference = references->items[i];
		uint16_t label_offset = get_code_label_offset(c, label);
		int32_t offset = (int32_t)label_offset - reference.opcode_offset;
		overwrite_bytes(code->code, reference.branch_offset, offset, 4);
		add_stack_frame(c, code, label_offset);
	}
}

static void main_method_code(struct codegen *c, struct code *code)
{
	code->max_stack = 5;
	code->max_locals = 2 + 2;
	load_registers_into_local(c, code);

	// 1st pass: build bytecode with offset placeholders and a symbol map
	for (size_t i = 0; c->ir[i].type != IR_EOF; i++) {
		switch (c->ir[i].type) {
		case IR_DIRECTIVE:
			break;
		case IR_LABEL:
			write_label(c, i, code);
			break;
		case IR_INSTRUCTION:
			write_instruction(c, i, code);
			break;
		default:
			break;
		}
	}

	// 2nd pass: calculate actual offsets and replace placeholder values
	for (size_t i = 0; c->ir[i].type != IR_EOF; i++) {
		if (c->ir[i].type != IR_LABEL) {
			continue;
		}
		update_label_reference(c, code, c->ir[i].as.label.name);
	}

	write_byte(code->code, JVM_RETURN);
}

static void methods(struct codegen *c)
{
	write_int(c, 2, 2);

	uint16_t mask = JVM_ACC_PUBLIC | JVM_ACC_STATIC | JVM_ACC_SYNTHETIC;
	struct code clinit_code = create_code();
	clinit_method_code(c, &clinit_code);
	add_method(c, mask, CLINIT_METHOD_NAME, NO_ARGS_VOID_DESCRIPTOR,
		   &clinit_code);
	free_code(&clinit_code);
	struct code main_code = create_code();
	main_method_code(c, &main_code);
	add_method(c, mask, MAIN_METHOD_NAME, MAIN_METHOD_DESCRIPTOR,
		   &main_code);
	free_code(&main_code);
}

static void attributes(struct codegen *c)
{
	write_int(c, 0, 2);
}

void generate_bytecode(struct ir_element *ir, struct bytecode *res)
{
	struct codegen codegen;
	init_codegen(&codegen, ir, res);
	magic(&codegen);
	minor_version(&codegen);
	major_version(&codegen);
	constant_pool(&codegen);
	access_flags(&codegen);
	this_class(&codegen);
	super_class(&codegen);
	interfaces(&codegen);
	fields(&codegen);
	methods(&codegen);
	attributes(&codegen);
	free_codegen(&codegen);
}
