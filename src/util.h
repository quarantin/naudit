#ifndef UTIL_H
#define UTIL_H

#include "map.h"
#include "naudit.h"
#include "vector.h"

#define err(...) _err(__func__, __FILE__, __LINE__, __VA_ARGS__)

typedef int (*compare_t) (void *lptr, void *rptr);

void open_log (void);

void close_log (void);

void _err (const char *func, const char *file, int line, char *format, ...);

const char *basename (const char *path);

char *strdup (const char *str);

char *strndup (const char *str, unsigned int n);

int array_sort (void **array, unsigned int size, compare_t compare);

void free_array (void **array, unsigned int size);

void free_array_fn (void **array, unsigned int size, free_t *fn);

vector_t *split (const char *line, const char *seps);

vector_t *split_max (const char *line, const char *seps, unsigned int max);

void trim_end (char *line, const char *chars);

int file_exists (const char *path);

void write_file (const char *path, vector_t *content);

vector_t *read_file (const char *file);

char *canonicalize_label (const char *str);

char *canonicalize_ext (const char *ext);

char *canonicalize_path (const char *path);

char *canonicalize_default (const char *str);

void exec_cmd (char **argv);

void edit (const char *cmd, char type);

#endif /* UTIL_H */
