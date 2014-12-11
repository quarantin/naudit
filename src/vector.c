#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "vector.h"
#define BLOCK 255


/*
 *
 */
vector_t *new_vector (free_t *ffunc)
{
	vector_t *vector = NULL;

	vector = malloc(sizeof(vector_t));
	if (!vector)
		return NULL;

	vector->size = 0;
	vector->free = ffunc;
	vector->capacity = BLOCK;
	vector->data = calloc(BLOCK + 1, sizeof(void *));
	if (!vector->data) {
		del_vector(vector);
		return NULL;
	}

	return vector;
}


/*
 *
 */
void del_vector (vector_t *vector)
{
	if (!vector)
		return;

	free_array_fn(vector->data, vector->size, vector->free);
	free(vector);
}


/*
 *
 */
void vector_add (vector_t *vector, void *value)
{
	void **ptr = NULL;

	if (!vector)
		return;

	if (vector->size >= vector->capacity) {
		vector->capacity += BLOCK;
		ptr = realloc(vector->data, (vector->capacity + 1) * sizeof(void *));
		if (!ptr) {
			err("realloc failed: %s\n", strerror(errno));
			return;
		}
		memset(&ptr[vector->size], 0, BLOCK + 1);
		vector->data = ptr;
	}

	vector->data[vector->size] = value;
	vector->size++;
}

/*
 *
 */
int *vector_to_int_array (vector_t *vector)
{
	int *array;
	unsigned int i;

	array = malloc(vector->size * sizeof(int));
	if (!array) {
		err("malloc failed\n");
		return NULL;
	}

	for (i = 0; i < vector->size; i++)
		array[i] = (int)(long)vector->data[i];

	return array;
}

