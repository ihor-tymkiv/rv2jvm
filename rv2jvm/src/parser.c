#include "parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "darray.h"
#include "ir.h"
#include "tokens.h"

struct parser {
	struct {
		struct ir_element *items;
		size_t size;
		size_t capacity;
	} ir;
	struct tokens *tokens;
	struct token *current;
};

static void advance(struct parser *parser)
{
	parser->current++;
}

static void consume(struct parser *parser, enum token_type type,
		    char *message)
{
	if (parser->current->type == type) {
		advance(parser);
		return;
	}

	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

static bool check(struct parser *parser, enum token_type type)
{
	return parser->current->type == type;
}

static bool check_next(struct parser *parser, enum token_type type)
{
	return parser->current[1].type == type;
}

static bool match(struct parser *parser, enum token_type type)
{
	if (!check(parser, type)) return false;
	advance(parser);
	return true;
}

static void label(struct parser *parser)
{
	char *name = parser->current->lexeme;
	advance(parser);
	consume(parser, TOKEN_COLON, "':' expected");
	struct ir_label label = { .name = name };
	struct ir_element element = {
		.type = IR_LABEL,
		.as.label = label
	};
	darray_append((parser->ir), element);
}

static char *lower_str(char *str, size_t length) {
	for (size_t i = 0; i < length; i++) {
		str[i] = tolower(str[i]);
	}
	return str;
}

static enum ir_instruction_mnemonic check_mnemonic(char *lexeme, size_t start,
						   size_t rest_length,
						   char *rest,
						   size_t lexeme_length,
						   enum ir_instruction_mnemonic mnemonic)
{
	if (lexeme_length == start + rest_length && memcmp(lexeme + start,
							   rest,
							   rest_length) == 0) {
		return mnemonic;
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic a_trie(char *lexeme, size_t length)
{
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[1]) {
	case 'd':
		mnemonic = check_mnemonic(lexeme, 2, 2, "di", length, ADDI);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 2, 1, "d", length, ADD);
		}
		return mnemonic;
	case 'n':
		mnemonic = check_mnemonic(lexeme, 2, 2, "di", length, ANDI);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 2, 1, "d", length, AND);
		}
		return mnemonic;
	case 'u':
		return check_mnemonic(lexeme, 2, 3, "ipc", length, AUIPC);
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic b_trie(char *lexeme, size_t length)
{
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[1]) {
	case 'e':
		return check_mnemonic(lexeme, 2, 1, "q", length, BEQ);
	case 'g':
		mnemonic = check_mnemonic(lexeme, 2, 2, "eu", length, BGEU);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 2, 1, "e", length, BGE);
		}
		return mnemonic;
	case 'l':
		mnemonic = check_mnemonic(lexeme, 2, 2, "tu", length, BLTU);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 2, 1, "t", length, BLT);
		}
		return mnemonic;
	case 'n':
		return check_mnemonic(lexeme, 2, 1, "e", length, BNE);
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic j_trie(char *lexeme, size_t length)
{
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[1]) {
	case 'a':
		mnemonic = check_mnemonic(lexeme, 2, 2, "lr", length, JALR);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 2, 1, "l", length, JAL);
		}
		return mnemonic;
	case 'r':
		return check_mnemonic(lexeme, 2, 0, "", length, JR);
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic l_trie(char *lexeme, size_t length)
{
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[1]) {
	case 'b':
		if (length == 2) {
			return check_mnemonic(lexeme, 2, 0, "", length, LB);
		}
		return check_mnemonic(lexeme, 2, 1, "u", length, LBU);
	case 'h':
		if (length == 2) {
			return check_mnemonic(lexeme, 2, 0, "", length, LH);
		}
		return check_mnemonic(lexeme, 2, 1, "u", length, LHU);
	case 'u':
		return check_mnemonic(lexeme, 2, 1, "i", length, LUI);
	case 'w':
		if (length == 2) {
			return LW;
		}
		break;
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic s_trie(char *lexeme, size_t length)
{
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[1]) {
	case 'b':
		if (length == 2) {
			return SB;
		}
		break;
	case 'e':
		return check_mnemonic(lexeme, 2, 2, "qz", length, SEQZ);
	case 'h':
		if (length == 2) {
			return SH;
		}
		break;
	case 'l':
		if (length < 3) {
			break;
		}
		switch (lexeme[2]) {
		case 'l':
			if (length == 3) {
				return SLL;
			}
			return check_mnemonic(lexeme, 3, 1, "i", length, SLLI);
		case 't':
			if (length == 3) {
				return SLT;
			}
			if (length < 4) {
				break;
			}
			switch (lexeme[3]) {
			case 'i':
				if (length == 4) {
					return SLTI;
				}
				return check_mnemonic(lexeme, 4, 1, "u", length,
						      SLTIU);
			case 'u':
				if (length == 4) {
					return SLTU;
				}
			}
			break;
		}
		break;
	case 'n':
		return check_mnemonic(lexeme, 2, 2, "ez", length, SNEZ);
	case 'r':
		if (length < 3) {
			break;
		}
		switch (lexeme[2]) {
		case 'a':
			if (length == 3) {
				return SRA;
			}
			return check_mnemonic(lexeme, 3, 1, "i", length, SRAI);
		case 'l':
			if (length == 3) {
				return SRL;
			}
			return check_mnemonic(lexeme, 3, 1, "i", length, SRLI);
		}
		break;
	case 'u':
		return check_mnemonic(lexeme, 2, 1, "b", length, SUB);
	case 'w':
		if (length == 2) {
			return SW;
		}
		break;
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_mnemonic get_mnemonic(struct token token)
{
	size_t length = token.length;
	char *lexeme = lower_str(token.lexeme, length);
	enum ir_instruction_mnemonic mnemonic;
	switch (lexeme[0]) {
	case 'a':
		if (length < 2) {
			break;
		}
		return a_trie(lexeme, length);
	case 'b':
		if (length < 2) {
			break;
		}
		return b_trie(lexeme, length);
	case 'e':
		if (length < 2) {
			break;
		}
		switch (lexeme[1]) {
		case 'b':
			return check_mnemonic(lexeme, 2, 4, "reak", length,
					      EBREAK);
		case 'c':
			return check_mnemonic(lexeme, 2, 3, "all", length,
					      ECALL);
		}
		break;
	case 'f':
		return check_mnemonic(lexeme, 1, 4, "ence", length, FENCE);
	case 'j':
		if (length == 1) {
			return J;
		}
		return j_trie(lexeme, length);
	case 'l':
		if (length < 2) {
			break;
		}
		return l_trie(lexeme, length);
	case 'm':
		return check_mnemonic(lexeme, 1, 1, "v", length, MV);
	case 'n':
		mnemonic = check_mnemonic(lexeme, 1, 2, "op", length, NOP);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 1, 2, "ot", length, NOT);
		}
		return mnemonic;
	case 'o':
		mnemonic = check_mnemonic(lexeme, 1, 2, "ri", length, ORI);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 1, 1, "r", length, OR);
		}
		return mnemonic;
	case 'r':
		return check_mnemonic(lexeme, 1, 2, "et", length, RET);
	case 's':
		if (length < 2) {
			break;
		}
		return s_trie(lexeme, length);
	case 'x':
		mnemonic = check_mnemonic(lexeme, 1, 3, "ori", length, XORI);
		if (mnemonic == UNKNOWN_MNEMONIC) {
			return check_mnemonic(lexeme, 1, 2, "or", length, XOR);
		}
		return mnemonic;
	}
	return UNKNOWN_MNEMONIC;
}

static enum ir_instruction_register parse_register(char *reg_str)
{
	if (tolower(reg_str[0]) != 'x') {
		return -1; // invalid
	}
	
	char *endptr;
	long num = strtol(reg_str + 1, &endptr, 10);
	
	if (*endptr != '\0' || num < 0 || num > 31) {
		return -1; // invalid
	}
	
	return (enum ir_instruction_register)num;
}

static int32_t number(struct parser *parser)
{
	bool negative = match(parser, TOKEN_MINUS);
	char *decimal = parser->current->lexeme;
	consume(parser, TOKEN_DECIMAL, "decimal number expected");
	
	char *endptr;
	long value = strtol(decimal, &endptr, 10);
	
	if (*endptr != '\0') {
		fprintf(stderr, "Invalid decimal number: %s at %s line %lu\n",
			decimal, parser->current->file, parser->current->line);
		exit(EXIT_FAILURE);
	}
	
	if (negative) {
		value = -value;
	}
	
	return (int32_t)value;
}

static enum ir_instruction_register reg(struct parser *parser)
{
	char *register_str = parser->current->lexeme;
	enum ir_instruction_register r = parse_register(register_str);
	
	if (r == (enum ir_instruction_register)-1) {
		fprintf(stderr, "Invalid register: %s at %s line %lu\n",
			register_str, parser->current->file,
			parser->current->line);
		exit(EXIT_FAILURE);
	}
	
	consume(parser, TOKEN_IDENTIFIER, "identifier expected");
	return r;
}

static char *identifier(struct parser *parser)
{
	char *identifier = parser->current->lexeme;
	consume(parser, TOKEN_IDENTIFIER, "identifier expected");
	return identifier;
}

static struct ir_instruction create_r3_instruction(enum ir_instruction_mnemonic mnemonic,
						   enum ir_instruction_register rd,
						   enum ir_instruction_register rs1,
						   enum ir_instruction_register rs2)
{
	struct ir_instruction inst;
	inst.type = TYPE_R3;
	inst.mnemonic = mnemonic;
	inst.as.r3.rd = rd;
	inst.as.r3.rs1 = rs1;
	inst.as.r3.rs2 = rs2;
	return inst;
}

static struct ir_instruction create_r2op_imm_instruction(enum ir_instruction_mnemonic mnemonic,
							 enum ir_instruction_register rd,
							 enum ir_instruction_register rs1,
							 int16_t imm)
{
	struct ir_instruction inst;
	inst.type = TYPE_R2_OP;
	inst.mnemonic = mnemonic;
	inst.as.r2op.rd = rd;
	inst.as.r2op.rs1 = rs1;
	inst.as.r2op.op_type = OPERAND_IMM;
	inst.as.r2op.op.imm = imm;
	return inst;
}

static struct ir_instruction create_r2op_label_instruction(enum ir_instruction_mnemonic mnemonic,
							   enum ir_instruction_register rd,
							   enum ir_instruction_register rs1,
							   char *label)
{
	struct ir_instruction inst;
	inst.type = TYPE_R2_OP;
	inst.mnemonic = mnemonic;
	inst.as.r2op.rd = rd;
	inst.as.r2op.rs1 = rs1;
	inst.as.r2op.op_type = OPERAND_LABEL;
	inst.as.r2op.op.label = label;
	return inst;
}

static struct ir_instruction create_r1op_imm_instruction(enum ir_instruction_mnemonic mnemonic,
							 enum ir_instruction_register rd,
							 int32_t imm)
{
	struct ir_instruction inst;
	inst.type = TYPE_R1_OP;
	inst.mnemonic = mnemonic;
	inst.as.r1op.rd = rd;
	inst.as.r1op.op_type = OPERAND_IMM;
	inst.as.r1op.op.imm = imm;
	return inst;
}

static struct ir_instruction create_r1op_label_instruction(enum ir_instruction_mnemonic mnemonic,
							   enum ir_instruction_register rd,
							   char *label)
{
	struct ir_instruction inst;
	inst.type = TYPE_R1_OP;
	inst.mnemonic = mnemonic;
	inst.as.r1op.rd = rd;
	inst.as.r1op.op_type = OPERAND_LABEL;
	inst.as.r1op.op.label = label;
	return inst;
}

static enum ir_instruction_type get_instruction_type(enum ir_instruction_mnemonic mnemonic)
{
	switch (mnemonic) {
	case ADD:
	case SUB:
	case SLT:
	case SLTU:
	case AND:
	case OR:
	case XOR:
	case SLL:
	case SRL:
	case SRA:
		return TYPE_R3;
	
	case ADDI:
	case SLTI:
	case SLTIU:
	case ANDI:
	case ORI:
	case XORI:
	case SLLI:
	case SRLI:
	case SRAI:
	case LW:
	case LH:
	case LHU:
	case LB:
	case LBU:
	case SW:
	case SH:
	case SB:
	case BEQ:
	case BNE:
	case BLT:
	case BLTU:
	case BGE:
	case BGEU:
	case JALR:
		return TYPE_R2_OP;
	
	case LUI:
	case AUIPC:
	case JAL:
		return TYPE_R1_OP;
	
	default:
		return -1; // pseudoinstruction
	}
}

static void parse_r3_instruction(struct parser *parser,
				 enum ir_instruction_mnemonic mnemonic)
{
	enum ir_instruction_register rd, rs1, rs2;
	
	rd = reg(parser);
	consume(parser, TOKEN_COMMA, "Expected ',' after rd");
	
	rs1 = reg(parser);
	consume(parser, TOKEN_COMMA, "Expected ',' after rs1");
	
	rs2 = reg(parser);
	
	struct ir_instruction inst = create_r3_instruction(mnemonic, rd, rs1, rs2);
	struct ir_element element = {
		.type = IR_INSTRUCTION,
		.as.instruction = inst
	};
	darray_append((parser->ir), element);
}

static void parse_r2op_instruction(struct parser *parser,
				   enum ir_instruction_mnemonic mnemonic)
{
	enum ir_instruction_register rd, rs1;
	
	rd = reg(parser);
	consume(parser, TOKEN_COMMA, "Expected ',' after rd");
	
	rs1 = reg(parser);
	consume(parser, TOKEN_COMMA, "Expected ',' after rs1");
	
	struct ir_instruction inst;
	
	if (check(parser, TOKEN_DECIMAL) || check(parser, TOKEN_MINUS)) {
		int32_t imm = number(parser);
		inst = create_r2op_imm_instruction(mnemonic, rd, rs1, (int16_t)imm);
	} else if (check(parser, TOKEN_IDENTIFIER)) {
		char *label = identifier(parser);
		inst = create_r2op_label_instruction(mnemonic, rd, rs1, label);
	} else {
		fprintf(stderr, "Expected immediate or label at %s line %lu\n",
			parser->current->file, parser->current->line);
		exit(EXIT_FAILURE);
	}
	
	struct ir_element element = {
		.type = IR_INSTRUCTION,
		.as.instruction = inst
	};
	darray_append((parser->ir), element);
}

static void parse_r1op_instruction(struct parser *parser,
				   enum ir_instruction_mnemonic mnemonic)
{
	enum ir_instruction_register rd;
	
	rd = reg(parser);
	consume(parser, TOKEN_COMMA, "Expected ',' after rd");
	
	struct ir_instruction inst;
	
	if (check(parser, TOKEN_DECIMAL) || check(parser, TOKEN_MINUS)) {
		int32_t imm = number(parser);
		inst = create_r1op_imm_instruction(mnemonic, rd, imm);
	} else if (check(parser, TOKEN_IDENTIFIER)) {
		char *label = identifier(parser);
		inst = create_r1op_label_instruction(mnemonic, rd, label);
	} else {
		fprintf(stderr, "Expected immediate or label at %s line %lu\n",
			parser->current->file, parser->current->line);
		exit(EXIT_FAILURE);
	}
	
	struct ir_element element = {
		.type = IR_INSTRUCTION,
		.as.instruction = inst
	};
	darray_append((parser->ir), element);
}

static void parse_special_instruction(struct parser *parser,
				      enum ir_instruction_mnemonic mnemonic)
{
	struct ir_instruction inst;
	
	switch (mnemonic) {
	case MV: {
		enum ir_instruction_register rd, rs1;
		rd = reg(parser);
		consume(parser, TOKEN_COMMA, "Expected ',' after rd");
		rs1 = reg(parser);
		inst = create_r2op_imm_instruction(ADDI, rd, rs1, 0);
		break;
	}
	
	case NOT: {
		enum ir_instruction_register rd, rs1;
		rd = reg(parser);
		consume(parser, TOKEN_COMMA, "Expected ',' after rd");
		rs1 = reg(parser);
		inst = create_r2op_imm_instruction(XORI, rd, rs1, -1);
		break;
	}
	
	case SEQZ: {
		enum ir_instruction_register rd, rs1;
		rd = reg(parser);
		consume(parser, TOKEN_COMMA, "Expected ',' after rd");
		rs1 = reg(parser);
		inst = create_r2op_imm_instruction(SLTIU, rd, rs1, 1);
		break;
	}
	
	case SNEZ: {
		enum ir_instruction_register rd, rs1;
		rd = reg(parser);
		consume(parser, TOKEN_COMMA, "Expected ',' after rd");
		rs1 = reg(parser);
		inst = create_r3_instruction(SLTU, rd, X0, rs1);
		break;
	}
	
	case J: {
		char *label = identifier(parser);
		inst = create_r1op_label_instruction(JAL, X0, label);
		break;
	}
	
	case JR: {
		enum ir_instruction_register rs1;
		rs1 = reg(parser);
		inst = create_r2op_imm_instruction(JALR, X0, rs1, 0);
		break;
	}
	
	case RET: {
		inst = create_r2op_imm_instruction(JALR, X0, X1, 0);
		break;
	}
	
	case NOP: {
		inst = create_r2op_imm_instruction(ADDI, X0, X0, 0);
		break;
	}
	
	case FENCE: {
		inst = create_r2op_imm_instruction(FENCE, X0, X0, 0);
		break;
	}
	
	case ECALL: {
		inst = create_r2op_imm_instruction(ECALL, X0, X0, 0);
		break;
	}
	
	case EBREAK: {
		inst = create_r2op_imm_instruction(EBREAK, X0, X0, 0);
		break;
	}
	
	default:
		fprintf(stderr, "Unhandled pseudoinstruction mnemonic: %d\n", mnemonic);
		exit(EXIT_FAILURE);
	}
	
	struct ir_element element = {
		.type = IR_INSTRUCTION,
		.as.instruction = inst
	};
	darray_append((parser->ir), element);
}

static void instruction(struct parser *parser)
{
	enum ir_instruction_mnemonic mnemonic = get_mnemonic(*parser->current);
	
	if (mnemonic == UNKNOWN_MNEMONIC) {
		fprintf(stderr, "Unknown instruction mnemonic: %s at %s line %lu\n",
			parser->current->lexeme, parser->current->file,
			parser->current->line);
		exit(EXIT_FAILURE);
	}
	
	advance(parser);
	
	enum ir_instruction_type type = get_instruction_type(mnemonic);
	
	switch (type) {
	case TYPE_R3:
		parse_r3_instruction(parser, mnemonic);
		break;
	case TYPE_R2_OP:
		parse_r2op_instruction(parser, mnemonic);
		break;
	case TYPE_R1_OP:
		parse_r1op_instruction(parser, mnemonic);
		break;
	default:
		// pseudoinstructions
		parse_special_instruction(parser, mnemonic);
		break;
	}
	
	consume(parser, TOKEN_NEWLINE, "NEWLINE expected after instruction");
}

void parse(struct tokens tokens, struct ir_element **res)
{
	struct parser parser = { 0 };
	parser.tokens = &tokens;
	parser.current = &tokens.items[0];

	while (!check(&parser, TOKEN_EOF)) {
		if (match(&parser, TOKEN_NEWLINE)) {
			continue;
		}
		if (check_next(&parser, TOKEN_COLON)) {
			label(&parser);
		} else {
			instruction(&parser);
		}
	}

	struct ir_element eof = { .type = IR_EOF };
	darray_append((parser.ir), eof);
	*res = parser.ir.items;
}
