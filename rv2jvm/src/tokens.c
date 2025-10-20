#include "tokens.h"

#include <stddef.h>
#include <stdlib.h>

#include "darray.h"

void free_tokens(struct tokens *tokens)
{
	if (tokens == NULL) {
		return;
	}
	for (size_t i = 0; i < tokens->size; i++) {
		free(tokens->items[i].lexeme);
	}
	darray_free((*tokens));
}
