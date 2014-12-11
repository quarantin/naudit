#ifndef FIND_H
#define FIND_H

#include <regex.h>
#include "vector.h"

typedef struct {
	char *label;
	char *regex;
	regex_t *compreg;
	vector_t *matches;
} search_t;

search_t *new_search (char *line);

void del_search (search_t *search);

void find (void);

#endif /* FIND_H */
