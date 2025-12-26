#ifndef RV2JVM_TOKENS_H
#define RV2JVM_TOKENS_H

#include <stddef.h>

enum token_type {
	TOKEN_NEWLINE,
	TOKEN_COMMA,
	TOKEN_COLON,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_MINUS,
	TOKEN_IDENTIFIER,
	TOKEN_DECIMAL,
	TOKEN_EOF
};

struct token {
	enum token_type type;
	char *lexeme;
	size_t length;
	char *file;
	size_t line;
};

struct tokens {
	struct token *items;
	size_t size;
	size_t capacity;
};

void free_tokens(struct tokens *tokens);

#endif
