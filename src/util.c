#define _XOPEN_SOURCE 500
#include <limits.h>
#include <stdlib.h>
#include <curses.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "util.h"
#include "config.h"
#include "editor.h"
#include "vector.h"


#define LOG "error.log"
static FILE *fperr = NULL;


/*
 *
 */
void open_log (void)
{
	char path[BUFSIZ];
	config_t *config = get_config();

	snprintf(path, BUFSIZ, "%s/%s", config->root, LOG);
	fperr = freopen(path, "a", stderr);
	if (!fperr)
		err("open_log: freopen failed: %s: `%s'\n", strerror(errno), path);
}


/*
 *
 */
void close_log (void)
{
	if (fperr)
		fclose(fperr);
}


/*
 *
 */
void _err (const char *func, const char *file, int line, char *format, ...)
{
	va_list list;

	fprintf(stderr, "%s:%d:%s:", file, line, func);
	va_start(list, format);
	vfprintf(stderr, format, list);
	va_end(list);
	fflush(stderr);
}


/*
 *
 */
const char *basename (const char *path)
{
	const char *name;

	name = strrchr(path, '/');
	return (name ? name + 1 : path);
}


/*
 *
 */
char *strdup (const char *str)
{
	return strndup(str, strlen(str));
}


/*
 *
 */
char *strndup (const char *str, unsigned int n)
{
	char *copy;

	copy = malloc(n + 1);
	if (!copy)
		return NULL;

	memcpy(copy, str, n);
	copy[n] = 0;
	return copy;
}


/*
 *
 */
int array_sort (void **array, unsigned int size, compare_t compare)
{
	int ret;
	void *ptr;
	unsigned int i;
	int modified = 0;

	for (i = 1; i < size; i++) {

		ret = compare(array[i - 1], array[i]);
		if (!ret || ret < 0)
			continue;

		modified = 1;
		ptr = array[i - 1];
		array[i - 1] = array[i];
		array[i] = ptr;

		if (i > 2)
			i -= 2;
	}

	return modified;
}


/*
 *
 */
void free_array_fn (void **array, unsigned int size, free_t *ffn)
{
	unsigned int i;

	if (!array)
		return;

	if (ffn)
		for (i = 0; i < size; i++)
			ffn(array[i]);

	free(array);
}


/*
 *
 */
void free_array (void **array, unsigned int size)
{
	free_array_fn(array, size, free);
}


/*
 * split `line' into a list of tokens using `seps' as delimiter chars.
 */
vector_t *split (const char *line, const char *seps)
{
	return split_max(line, seps, 0);
}


/*
 * same as split() except it returns at most `max' tokens.
 */
vector_t *split_max (const char *line, const char *seps, unsigned int max)
{
	vector_t *list;
	char *copy, *token;
	unsigned int i, len;

	copy = token = strdup(line);
	if (!copy) {
		err("strdup failed: %s\n", strerror(errno));
		return NULL;
	}

	list = new_vector(free);
	if (!list) {
		err("new_vector failed: %s\n", strerror(errno));
		free(copy);
		return NULL;
	}

	if (!max)
		max = INT_MAX;

	len = strlen(copy);

	if (max != 1) {

		for (i = 0; i < len; i++) {

			if (!strchr(seps, copy[i]))
				continue;

			copy[i] = 0;
			if (*token)
				vector_add(list, strdup(token));

			token = &copy[i + 1];

			if (list->size >= max - 1)
				break;
		}
	}

	if (*token)
		vector_add(list, strdup(token));

	free(copy);
	return list;
}


/*
 * has side effects, modify the string in memory
 */
void trim_end (char *line, const char *chars)
{
	unsigned int i, len;

	if (!line || !line[0])
		return;

	len = strlen(line);
	for (i = len-1; i > 0; i--) {
		if (!strchr(chars, line[i]))
			return;
		line[i] = 0;
	}
}


/*
 *
 */
int file_exists (const char *path)
{
	struct stat info;
	return !stat(path, &info);
}


/*
 *
 */
void write_file (const char *path, vector_t *content)
{
	unsigned int i;
	FILE *fp = NULL;

	if (!path || !content)
		return;

	fp = fopen(path, "w");
	if (!fp) {
		err("fopen failed: %s: `%s'\n", strerror(errno), path);
		return;
	}

	for (i = 0; i < content->size; i++)
		fprintf(fp, "%s\n", (char *)content->data[i]);

	fclose(fp);
}


/*
 *
 */
vector_t *read_file (const char *file)
{
	FILE *fp;
	vector_t *vector;
	char buf[BUFSIZ];

	if (!file || !*file)
		return NULL;

	fp = fopen(file, "r");
	if (!fp) {
		err("fopen failed: %s: `%s'\n", strerror(errno), file);
		return NULL;
	}

	vector = new_vector(free);
	if (!vector) {
		err("new_vector failed: %s\n", strerror(errno));
		fclose(fp);
		return NULL;
	}

	while (fgets(buf, BUFSIZ, fp)) {

		trim_end(buf, " \t\r\n");
		vector_add(vector, strdup(buf));
	}

	fclose(fp);
	return vector;
}


/*
 *
 */
char *get_alnum (const char *str, const char *extra)
{
	char *alnum;
	unsigned int i, j, size;

	size = strlen(str) + 1;
	alnum = malloc(size);
	if (!alnum) {
		err("malloc failed\n");
		return NULL;
	}

	for (i = 0, j = 0; i < size; i++) {

		if (isalnum(str[i]))
			alnum[j++] = str[i];

		else if (extra && strchr(extra, str[i]))
			alnum[j++] = str[i];
	}

	alnum[j] = 0;
	return alnum;
}


/*
 *
 */
inline char *canonicalize_label (const char *str)
{
	return get_alnum(str, "_");
}


/*
 *
 */
inline char *canonicalize_ext (const char *ext)
{
	char *ptr;

	ptr = strrchr(ext, '.');
	return strdup((ptr ? ptr + 1 : ext));
}


/*
 *
 */
inline char *canonicalize_path (const char *path)
{
	return realpath(path, NULL);
}


/*
 *
 */
inline char *canonicalize_default (const char *str)
{
	return strdup(str);
}


/*
 *
 */
void exec_cmd (char **argv)
{
	pid_t pid;

	if (!argv || !argv[0])
		return;

	def_prog_mode();
	endwin();

	pid = fork();
	if (!pid) {
		execvp(argv[0], argv);
		err("execvp failed: %s\n", strerror(errno));
		return;
	}
	wait(NULL);

	reset_prog_mode();
	refresh();
}


/*
 *
 */
void edit (const char *cmd, char type)
{
	char ln[32];
	char *copy = NULL;
	char *file, *ptr;
	unsigned int num;
	editor_t *editor;

	if (!cmd|| !cmd[0])
		return;

	switch (type) {

		case 'h':
			editor = get_hex_editor();
			break;

		default:
			editor = get_text_editor();
			break;
	}

	if (!editor) {
		err("couldn't find editor for type: %u\n", type);
		return;
	}

	copy = strdup(cmd);
	if (!copy) {
		err("strdup failed: %s\n", strerror(errno));
		return;
	}

	file = copy;
	ptr = strchr(file, ':');
	if (!ptr) {
		err("strchr failed\n");
		free(copy);
		return;
	}
	*ptr++ = 0;

	num = atoi(ptr);
	snprintf(ln, 32, "+%u", num);
	editor->open(file, ln);
}

