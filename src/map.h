#ifndef MAP_H
#define MAP_H

#include "vector.h"

typedef struct {
	unsigned int tag;
	unsigned int num;
	char *file;
	char *data;
} line_t;

typedef struct {
	int empty;
	char updated;
	char *path;
	char *title;
	vector_t *lines;
	vector_t *values;
	vector_t *raw_lines;
	unsigned int count;
} map_t; 

line_t *new_line (const char *line);

void del_line (line_t *line);

int compare_line (void *lptr, void *rptr);

map_t *new_map (const char *path, vector_t *strlines);

void del_map (map_t *map);

void write_map_file (const char *path, vector_t *matches);

map_t *load_map (const char *path);

void update_map (map_t *map);

void save_map (map_t *map, int *values);

void sort_map (map_t *map);

map_t *merge_map (map_t *lmap, map_t *rmap);

#endif /* MAP_H */
