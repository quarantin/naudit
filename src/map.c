#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "map.h"
#include "util.h"
#include "config.h"


/*
 *
 */
line_t *new_line (const char *line)
{
	line_t *newline;
	vector_t *tokens;

	newline = calloc(1, sizeof(line_t));
	if (!newline) {
		err("calloc failed: %s\n", strerror(errno));
		return NULL;
	}

	tokens = split_max(line, ":", 4);
	if (!tokens || tokens->size != 4) {
		err("invalid line: `%s'\n", line);
		free(newline);
		del_vector(tokens);
		return NULL;
	}

	newline->tag  =   atoi((char *)tokens->data[0]);
	newline->file = strdup((char *)tokens->data[1]);
	newline->num  =   atoi((char *)tokens->data[2]);
	newline->data = strdup((char *)tokens->data[3]);

//	if (strstr(newline->data, "memcpy"))
//		err("DEBUG NEW_LINE: tag: %u | line: %u | file: %s | data: %s\n", newline->tag, newline->line, newline->file, newline->data);

	del_vector(tokens);
	return newline;
}


/*
 *
 */
void del_line (line_t *line)
{
	if (!line)
		return;

	free(line->file);
	free(line);
}


/*
 *
 */
int compare_line (void *lptr, void *rptr)
{
	int ret;
	line_t *left_line, *right_line;

	left_line = lptr;
	right_line = rptr;

	ret = strcmp(left_line->file, right_line->file);
	if (!ret) {

		if (left_line->num == right_line->num)
			return 0;

		return (left_line->num< right_line->num? -1 : 1);
	}

	return ret;
}


/*
 *
 */
map_t *new_map (const char *path, vector_t *lines)
{
	map_t *map;
	char string[BUFSIZ];
	config_t *config = get_config();
	const char *name = basename(path);

	map = calloc(1, sizeof(map_t));
	if (!map) {
		err("calloc failed: %s\n", strerror(errno));
		return NULL;
	}

	snprintf(string, BUFSIZ, "%s/%s", config->session, name);
	map->path = strdup(string);

	snprintf(string, BUFSIZ, "%s/%s\n", basename(config->session), name);
	map->title = strdup(string);

	if (!map->path || !map->title) {
		err("strdup failed\n");
		del_map(map);
		return NULL;
	}

	map->lines = lines;
	map->count = map->lines->size;
	map->empty = !map->count;
	sort_map(map);

	return map;
}


/*
 *
 */
map_t *load_map (const char *path)
{
	char *buf;
	unsigned int i;
	line_t *newline;
	vector_t *lines, *strlines;

	strlines = read_file(path);
	if (!strlines) {
		err("read_file failed: %s\n", strerror(errno));
		return NULL;
	}

	lines = new_vector((free_t *)&del_line);
	if (!lines) {
		err("new_vector failed: %s\n", strerror(errno));
		del_vector(strlines);
		return NULL;
	}

	for (i = 0; i < strlines->size; i++) {

		buf = (char *)strlines->data[i];
		trim_end(buf, " \t\r\n");

		newline = new_line(buf);
		if (!newline) {
			err("new_line failed\n");
			continue;
		}

		vector_add(lines, newline);
	}

	return new_map(path, lines);
}


/*
 *
 */
void del_map (map_t *map)
{
	if (!map)
		return;

	free(map->path);
	free(map->title);

	free(map->values->data);
	free(map->values);

	del_vector(map->lines);
	del_vector(map->raw_lines);
	free(map);
}


/*
 *
 */
void write_map_file (const char *path, vector_t *matches)
{
	FILE *fp;
	size_t i;

	fp = fopen(path, "w");
	if (!fp) {
		err("fopen failed\n");
		return;
	}

	array_sort(matches->data, matches->size, compare_line);

	for (i = 0; i < matches->size; i++) {
		line_t *l = matches->data[i];
		fprintf(fp, "%u:%s:%u:%s\n", l->tag, l->file, l->num, l->data); 
	}

	fclose(fp);
}


/*
 *
 */
void update_map (map_t *map)
{
	line_t *line;
	unsigned int i;
	char buf[BUFSIZ];
	vector_t *values, *raw_lines;

	if (map->values && map->raw_lines && !map->updated)
		return;

	values = new_vector(NULL);
	raw_lines = new_vector(free);
	if (!values || !raw_lines) {
		err("new_vector failed\n");
		del_vector(values);
		del_vector(raw_lines);
		return;
	}

	for (i = 0; i < map->lines->size; i++) {

		line = map->lines->data[i];
		snprintf(buf, BUFSIZ, "%s:%u:%s", line->file, line->num, line->data);

		vector_add(values, (void *)(long)line->tag);
		vector_add(raw_lines, strdup(buf));
	}

	del_vector(map->values);
	map->values = values;

	del_vector(map->raw_lines);
	map->raw_lines = raw_lines;

	map->updated = 0;
}


/*
 *
 */
void save_map (map_t *map, int *values)
{
	FILE *fp;
	unsigned int i;
	char path[BUFSIZ];

	if (!map || !map->updated || !map->lines || !map->lines->data[0])
		return;

	if (!values)
		values = (int *)map->values->data;

	snprintf(path, BUFSIZ, "%s.tmp", map->path);
	fp = fopen(path, "w");
	if (!fp) {
		err("fopen failed: %s: `%s'\n", strerror(errno), path);
		return;
	}

	for (i = 0; i < map->raw_lines->size; i++)
		fprintf(fp, "%u:%s\n", values[i], (char *)map->raw_lines->data[i]);

	fclose(fp);
	map->updated = 0;
	unlink(map->path);
	rename(path, map->path);
}


/*
 *
 */
void sort_map (map_t *map)
{
	int modified = array_sort(map->lines->data, map->lines->size, compare_line);
	map->updated = (map->updated || modified);
	update_map(map);
}


static void merge_line (vector_t *list, line_t *left_line, line_t *right_line, unsigned int *pl, unsigned int *pr)
{
	int ret;

	ret = strcmp(left_line->file, right_line->file);
	if (!ret) {

		if (left_line->num == right_line->num) {
			vector_add(list, left_line);
			(*pl)++; (*pr)++;
		}
		else if (left_line->num < right_line->num) {
			vector_add(list, left_line);
			(*pl)++;
		}
		else if (left_line->num > right_line->num) {
			vector_add(list, right_line);
			(*pr)++;
		}
	}

	else if (ret < 0) {
		vector_add(list, left_line);
		(*pl)++;
	}

	else if (ret > 0) {
		vector_add(list, right_line);
		(*pr)++;
	}
}

/*
 *
 */
map_t *merge_map (map_t *lmap, map_t *rmap)
{
	vector_t *list;
	unsigned int l, r;
	line_t *left_line, *right_line;

	list = new_vector((free_t *)&del_line);
	if (!list) {
		err("new_vector failed\n");
		return NULL;
	}

	sort_map(lmap);
	sort_map(rmap);

	for (l = 0, r = 0;;) {

		if (l >= lmap->lines->size || r >= rmap->lines->size)
			break;

		left_line = lmap->lines->data[l];
		right_line = rmap->lines->data[r];

		merge_line(list, left_line, right_line, &l, &r);
	}

	for (; l < lmap->lines->size; l++)
		vector_add(list, lmap->lines->data[l]);

	for (; r < rmap->lines->size; r++)
		vector_add(list, rmap->lines->data[r]);

	free(lmap->lines->data);
	free(lmap->lines);

	lmap->lines = list;
	lmap->updated = 1;
	update_map(lmap);

	return lmap;
}

