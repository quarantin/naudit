#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <dirent.h>
#include <regex.h>
#include "naudit.h"
#include "config.h"
#include "util.h"
#include "find.h"


/*
 *
 */
search_t *new_search (char *line)
{
	int ret;
	char *label, *regex;
	search_t *search = NULL;

	label = line;
	regex = strchr(line, ':');
	if (regex)
		*regex++ = 0;
	else
		regex = line;

	trim_end(regex, "\r\n");

	search = calloc(1, sizeof(search_t));
	if (!search) {
		err("calloc failed: %s\n", strerror(errno));
		return NULL;
	}

	search->label = canonicalize_label(label);
	search->regex = canonicalize_default(regex);
	search->matches = new_vector((free_t *)&del_line);
	search->compreg = calloc(1, sizeof(regex_t));
	if (!search->label || !search->regex || !search->matches || !search->compreg) {
		err("allocation failure: %s\n", strerror(errno));
		del_search(search);
		return NULL;
	}

	ret = regcomp(search->compreg, search->regex, REG_EXTENDED|REG_NOSUB);
	if (ret) {
		err("regcomp failed\n");
		del_search(search);
		return NULL;
	}

	return search;
}


/*
 *
 */
void del_search (search_t *search)
{
	if (!search)
		return;

	free(search->label);
	free(search->regex);
	regfree(search->compreg);
	free(search->compreg);
	del_vector(search->matches);
	free(search);
}


/*
 *
 */
static int has_valid_ext (char *file)
{
	unsigned int i;
	char *ext;
	config_t *config = get_config();

	ext = strrchr(file, '.');
	if (ext)
		ext++;

	for (i = 0; i < config->ignexts->size; i++) {

		if (!ext && !strcmp(config->ignexts->data[i], "$"))
			return 0;

		if (ext && !strcmp(config->ignexts->data[i], ext))
			return 0;
	}

	for (i = 0; i < config->exts->size; i++) {

		if (!strcmp(config->exts->data[i], "*"))
			return 1;

		if (!ext && !strcmp(config->exts->data[i], "$"))
			return 1;

		if (ext && !strcmp(config->exts->data[i], ext))
			return 1;
	}

	return 0;
}


/*
 *
 */
static void find_files (char *path, vector_t *files)
{
	int ret;
	char *ptr;
	DIR *dir = NULL;
	struct stat info;
	struct dirent *entry = NULL;
	char npath[BUFSIZ];

	memset(&info, 0, sizeof(struct stat));

	ret = stat(path, &info);
	if (ret == -1) {
		err("stat failed: %s: `%s'\n", strerror(errno), path);
		return;
	}

	if (!S_ISDIR(info.st_mode)) {
		ptr = strdup(path);
		if (ptr)
			vector_add(files, ptr);
		else
			err("strdup failed: %s\n", strerror(errno));
		return;
	}

	dir = opendir(path);
	if (!dir) {
		err("opendir failed: %s: `%s'\n", strerror(errno), path);
		return;
	}

	while ((entry = readdir(dir))) {

		if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
			continue;

		snprintf(npath, BUFSIZ, "%s/%s", path, entry->d_name);
		ret = stat(npath, &info);
		if (ret == -1) {
			err("stat failed: %s: `%s'\n", strerror(errno), npath);
			continue;
		}

		if (S_ISDIR(info.st_mode))
			find_files(npath, files);
		else
		if (S_ISREG(info.st_mode) && has_valid_ext(entry->d_name)) {
			ptr = strdup(npath);
			if (!ptr) {
				err("strdup failed: %s\n", strerror(errno));
				continue;
			}
			vector_add(files, ptr);
		}
	}

	closedir(dir);
}


/*
 *
 */
static void grep_files (vector_t *files, vector_t *searches)
{
	int ret;
	char *file;
	unsigned int i, j, ln;
	char buf[BUFSIZ];
	char line[BUFSIZ];
	FILE *fp = NULL;
	search_t *search;
	line_t *newline;

	for (i = 0; i < files->size; i++) {

		file = files->data[i];
		fp = fopen(file, "r");
		if (!fp) {
			err("fopen failed: %s: `%s'\n", strerror(errno), file);
			continue;
		}

		ln = 1;
		while (fgets(line, BUFSIZ, fp)) {

			for (j = 0; j < searches->size; j++) {
				search = searches->data[j];
				ret = regexec(search->compreg, line, 0, NULL, 0);
				if (ret != REG_NOMATCH) {
					snprintf(buf, BUFSIZ, "0:%s:%u:%s", file, ln, line);
					trim_end(buf, " \t\r\n");

					newline = new_line(buf);
					if (!newline) {
						err("new_line failed\n");
						continue;
					}

					vector_add(search->matches, newline);
				}
			}

			ln++;
		}

		fclose(fp);
	}
}


void merge_helper (const char *path, vector_t *lines)
{
	map_t *load, *map, *new;

	load = load_map(path);
	new = new_map(path, lines);
	if (!load || !new) {
		err("load_map or new_map failed\n");
		del_map(load);
		del_map(new);
		return;
	}

	map = merge_map(load, new);
	if (!map) {
		err("merge_map failed\n");
		return;
	}

	map->updated = 1;
	save_map(map, NULL);

// TODO delete map, new, load
}


int can_write (const char *path, vector_t *lines)
{
	char buf[BUFSIZ];
	const char *name;

	if (!file_exists(path))
		return 1;

	name = strrchr(path, '/');
	name = (name ? name + 1 : path);

	fprintf(stdout, "Label `%s' is already defined. How do you want to proceed?\nMerge, Overwrite, Skip: ", name);

	if (!fgets(buf, BUFSIZ, stdin))
		return can_write(path, lines);

	switch (buf[0]) {

		case 'o':
		case 'O':
			return 1;

		case 's':
		case 'S':
			return 0;

		case 'm':
		case 'M':
			merge_helper(path, lines);
			return 0;

		default:
			return can_write(path, lines);
	}
}


/*
 *
 */
void find (void)
{
	unsigned int i;
	vector_t *files;
	search_t *search;
	char path[BUFSIZ];
	config_t *config = get_config();

	files = new_vector(free);
	if (!files) {
		err("new_vector failed: %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < config->path->size; i++)
		find_files(config->path->data[i], files);

	grep_files(files, config->regex);

	del_vector(files);

	for (i = 0; i < config->regex->size; i++) {
		search = config->regex->data[i];
		snprintf(path, BUFSIZ, "%s/%s", config->session, basename(search->label));
		if (can_write(path, search->matches))
			write_map_file(path, search->matches);
	}
}

