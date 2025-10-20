#include "lexer.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>

#include "darray.h"
#include "file.h"
#include "tokens.h"

struct lexer {
	char *start;
	char *current;
	char *file;
	size_t line;
};

static void init_lexer(struct lexer *lexer, char *source)
{
	lexer->start = source;
	lexer->current = source;
	lexer->line = 1;
}

static bool is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

static bool is_at_end(struct lexer *lexer)
{
	return *lexer->current == '\0';
}

static char advance(struct lexer *lexer)
{
	lexer->current++;
	return lexer->current[-1];
}

static char peek(struct lexer *lexer)
{
	return *lexer->current;
}

static char peek_next(struct lexer *lexer)
{
	if (is_at_end(lexer)) return '\0';
	return lexer->current[1];
}

static bool match(struct lexer *lexer, char expected)
{
	if (is_at_end(lexer)) return false;
	if (*lexer->current != expected) return false;
	lexer->current++;
	return true;
}

static size_t create_lexeme(struct lexer *lexer, char **lexeme)
{
	size_t size = lexer->current - lexer->start;
	char *result = malloc(size + 1);
	strncpy(result, lexer->start, size);
	result[size] = '\0';
	*lexeme = result;
	return size;
}

static struct token create_token(struct lexer *lexer, enum token_type type)
{
	struct token token;
	token.type = type;
	token.length = create_lexeme(lexer, &token.lexeme);
	token.file = lexer->file;
	token.line = lexer->line;
	return token;
}

static void skip_whitespace(struct lexer *lexer)
{
	for (;;) {
		char c = peek(lexer);
		switch (c) {
			case ' ':
			case '\r':
			case '\t':
				advance(lexer);
				break;
			default:
				return;
		}
	}
}

static struct token identifier(struct lexer *lexer)
{
	while (is_alpha(peek(lexer)) || is_digit(peek(lexer))) {
		advance(lexer);
	}
	return create_token(lexer, TOKEN_IDENTIFIER);
}

static struct token number(struct lexer *lexer)
{
	while (is_digit(peek(lexer))) {
		advance(lexer);
	}
	return create_token(lexer, TOKEN_DECIMAL);
}


static struct token scan_token(struct lexer *lexer)
{
	skip_whitespace(lexer);
	lexer->start = lexer->current;
	if (is_at_end(lexer)) {
		return create_token(lexer, TOKEN_EOF);
	}

	char c = advance(lexer);
	if (is_alpha(c)) {
		return identifier(lexer);
	}
	if (is_digit(c)) {
		return number(lexer);
	}

	switch (c) {
		case ',': return create_token(lexer, TOKEN_COMMA);
		case '-': return create_token(lexer, TOKEN_MINUS);
		case ':': return create_token(lexer, TOKEN_COLON);
		case '\n':
			  lexer->line++;
			  return create_token(lexer, TOKEN_NEWLINE);
	}

	fprintf(stderr, "Unexpected character '%c' %s line %lu\n", c, lexer->file,
			lexer->line);
	exit(EXIT_FAILURE);
}

void lex(int filepaths_n, char **filepaths, struct tokens *res)
{
	struct lexer lexer;
	struct token token;
	for (int i = 0; i < filepaths_n; i++) {
		char *source = read_file(filepaths[i]);
		init_lexer(&lexer, source);
		while (true) {
			token = scan_token(&lexer);
			if (token.type == TOKEN_EOF) {
				break;
			}
			darray_append((*res), token);
		}
		free(source);
	}
	darray_append((*res), token); // token should contain token of type TOKEN_EOF
}
