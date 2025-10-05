#ifndef RV2JVM_DARRAY_H
#define RV2JVM_DARRAY_H

#define darray_append(darr, item) \
	do { \
		if (darr.capacity == 0) { \
			darr.capacity = 64; \
		} \
		if (darr.capacity <= darr.size) { \
			darr.capacity *= 2; \
			darr.items = realloc(darr.items, \
					     darr.capacity * sizeof(*darr.items)); \
		} \
		darr[darr.size] = item; \
		darr.size++; \
	} while (0)

#endif
