#ifndef VECTOR_H
#define VECTOR_H
#include "naudit.h"

typedef struct {
	unsigned int size;
	unsigned int capacity;
	void **data;
	free_t *free;
} vector_t;

vector_t *new_vector (free_t *ffunc);

void del_vector (vector_t *vector);

void vector_add (vector_t *vector, void *value);

int *vector_to_int_array (vector_t *vector);

#endif /* VECTOR_H */
