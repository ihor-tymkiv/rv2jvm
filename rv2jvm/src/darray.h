#ifndef RV2JVM_DARRAY_H
#define RV2JVM_DARRAY_H

#include <stdlib.h>
#include <stdio.h>

#define darray_grow(darr)						   \
	do {								   \
		if (darr.capacity == 0) {				   \
			darr.capacity = 64;				   \
		} else {						   \
			darr.capacity *= 2;				   \
		}							   \
		darr.items = realloc(darr.items,			   \
				     darr.capacity * sizeof(*darr.items)); \
	} while (0)

#define darray_append(darr, item)			\
	do {						\
		if (darr.capacity <= darr.size) {	\
			darray_grow(darr);		\
		}					\
		darr.items[darr.size] = item;		\
		darr.size++;				\
	} while (0)

#endif
