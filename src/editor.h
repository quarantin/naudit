#ifndef EDITOR_H
#define EDITOR_H

#include "naudit.h"
#include "vector.h"

typedef struct {
	char *name;
	void (*open) (char *file, char *line);
} editor_t;

editor_t *get_hex_editor (void);

editor_t *get_text_editor (void);

#endif /* EDITOR_H */
