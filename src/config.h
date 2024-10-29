#ifndef CONFIG_H
#define CONFIG_H

#include <cdk.h>
#include "naudit.h"
#include "vector.h"

typedef struct {
	char *root;
	char *session;
	vector_t *exts;
	vector_t *ignexts;
	vector_t *maps;
	vector_t *path;
	vector_t *tags;
	vector_t *regex;
} config_t;

config_t *get_config (void);

int init_config (void);

void load_map_files (void);

void del_config (void);

void save_config (CDKSELECTION **screens);

#endif /* CONFIG_H */
