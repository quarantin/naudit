#include <getopt.h>
#include <string.h>
#include <cdk/cdk.h>
#include <curses.h>
#include "naudit.h"
#include "config.h"
#include "editor.h"
#include "find.h"
#include "util.h"
#include "map.h"


int batchmode = 0;
unsigned int screen_index = 0;

CDKSCREEN *scr = NULL;
CDKSELECTION **screens = NULL;

char *empty_tags[] = { "", NULL };

static struct option long_options[] =
{
	{"help",       no_argument,       0, 'h'},
	{"version",    no_argument,       0, 'v'},
	{"batch",      no_argument,       0, 'b'},
	{"config",     required_argument, 0, 'c'},
	{"ext",        required_argument, 0, 'e'},
	{"ext-file",   required_argument, 0, 'E'},
	{"path",       required_argument, 0, 'p'},
	{"path-file",  required_argument, 0, 'P'},
	{"regex",      required_argument, 0, 'r'},
	{"regex-file", required_argument, 0, 'R'},
	{"session",    required_argument, 0, 's'},
	{"tag",        required_argument, 0, 't'},
	{"tag-file",   required_argument, 0, 'T'},
	{0, 0, 0, 0}
};


/*
 *
 */
static void usage (char *name)
{
	if (!name || !*name)
		name = "naudit";

	fprintf(stderr, "Usage:\n\t%s <options> [path...]\n\n\t\t"
			"-h, --help                   \tPrint this help and exit\n\t\t"
			"-v, --version                \tPrint naudit version and exit\n\t\t"
			"-b, --batch                  \tBatch mode\n\t\t"
			"-c, --config <folder>        \tUse <folder> as config folder\n\t\t"
			"-e, --ext <list>             \tList of file extensions to search for\n\t\t"
			"-E, --ext-file <file>        \tUse <file> to get a list of file extensions to search for, one extension per line\n\t\t"
			"-i, --ignore-ext <list>      \tList of file extensions to ignore\n\t\t"
			"-I, --ignore-ext-file <file> \tUse <file> to get a list of file extensions to ignore, one extension per line\n\t\t"
			"-p, --path <list>            \tList of path to search for files to audit\n\t\t"
			"-P, --path-file <file>       \tUse <file> to get a list of path to search for files to audit, one path per line\n\t\t"
			"-r, --regex [label:]<regex>  \tUse <regex> as a pattern to match lines of code and store them in a file called label in the current session\n\t\t"
			"-R, --regex-file <file>      \tUse <file> to get a list of regex to match lines of code to audit, one [label:]<regex> per line. See --regex\n\t\t"
			"-s, --session <session>      \tSession name to work on. Creates a new session if required\n\t\t"
			"-t, --tag <list>             \tList of tags for the current session\n\t\t"
			"-T, --tag-file <file>        \tUse <file> to get a list of tags for the current session, one tag per line\n\n", name);
	fflush(stdout);
	exit(EXIT_FAILURE);
}


/*
 *
 */
static void quit (void)
{
	unsigned int i;

	save_config(screens);

	for (i = 0; i < get_config()->maps->size; i++)
		destroyCDKSelection(screens[i]);

	destroyCDKScreen(scr);
	endCDK();
	endwin();
	free(screens);
	del_config();
	close_log();
	exit(EXIT_SUCCESS);
}


/*
 *
 */
static void rotate_screen (int index)
{
	unsigned int size = get_config()->maps->size;

	eraseCDKSelection(screens[screen_index]);
	screen_index = (size + screen_index + index) % size;
	activateCDKSelection(screens[screen_index], NULL);
}


/*
 *
 */
static int handle_key (EObjectType obj, void *data1, void *data2, chtype ch)
{
	unsigned int index;
	map_t *map = NULL;
	config_t *config = data2;

	if (obj || data1) {;}

	switch ((int)ch) {
	case ' ':
		map = config->maps->data[screen_index];
		map->updated = 1;
		break;

	case 'q':
		quit();
		return 0;

	case 's':
		save_config(screens);
		return 0;

	case KEY_ESC:
		return 0;

	case KEY_TAB:
	case KEY_TAB_SHIFTED:
		rotate_screen((ch == KEY_TAB) ? 1 : -1);
		return 0;

	case 'b':
	case KEY_ENTER:
		map = config->maps->data[screen_index];
		index = getCDKSelectionCurrent(screens[screen_index]);
		edit(map->raw_lines->data[index], (char)ch);
		return 0;
	}

	return 1;
}


static void process_opts_add_regex (char *regex)
{
	search_t *search;

	search = new_search(regex);
	if (!search) {
		err("new_search failed\n");
		quit();
	}

	vector_add(get_config()->regex, search);
}


static void process_opts_split (vector_t *list, char *line, char *seps, canonicalize_t fn)
{
	unsigned int i;
	char *ptr;
	vector_t *vector;

	vector = split(line, seps);
	if (!vector) {
		err("split failed: `%s'\n", line);
		quit();
	}

	for (i = 0; i < vector->size; i++) {

		ptr = vector->data[i];
		if (!ptr || !*ptr)
			continue;

		ptr = fn(ptr);
		if (!ptr) {
			err("allocation failure: %s\n", strerror(errno));
			del_vector(vector);
			quit();
		}

		vector_add(list, ptr);
	}

	del_vector(vector);
}


static void process_opts_file (vector_t *list, char *path)
{
	unsigned int i;
	char *ptr;
	vector_t *lines;

	lines = read_file(path);
	if (!lines) {
		err("read_file failed: `%s'\n", path);
		quit();
	}

	for (i = 0; i < lines->size; i++) {

		ptr = strdup(lines->data[i]);
		if (!ptr) {
			err("strdup failed: %s\n", strerror(errno));
			del_vector(lines);
			quit();
		}

		vector_add(list, ptr);
	}

	del_vector(lines);
}


// TODO save the file in the session
static void process_opts_regex_file (char *path)
{
	unsigned int i;
	vector_t *lines;
	search_t *search;

	lines = read_file(path);
	if (!lines) {
		err("read_file failed: `%s'\n", path);
		quit();
	}

	for (i = 0; i < lines->size; i++) {
		search = new_search(lines->data[i]);
		if (!search) {
			fprintf(stdout, "Invalid argument for option --regex-file: a label is missing.\n"
					"Should be: `<label>:<regex>'. Given: `%s'.\n", (char *)lines->data[i]);
			quit();
		}

		vector_add(get_config()->regex, search);
	}

	del_vector(lines);
}


/*
 *
 */
static config_t *process_opts (int argc, char **argv)
{
	unsigned int i;
	int c, index = 0;
	config_t *config = get_config();

	if (!config)
		return NULL;

	while (1) {

		c = getopt_long (argc, argv, "bc:e:E:i:I:hp:P:r:R:s:t:T:v", long_options, &index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			usage(argv[0]);
			break;
		case 'b':
			batchmode = 1;
			break;
		case 'c':
			config->root = strdup(optarg);
			break;
		case 's':
			config->session = strdup(optarg);
			break;
		case 'r':
			process_opts_add_regex(optarg);
			break;
		case 'e':
			process_opts_split(config->exts, optarg, ",:", canonicalize_ext);
			break;
		case 'i':
			process_opts_split(config->ignexts, optarg, ",:", canonicalize_ext);
			break;
		case 'p':
			process_opts_split(config->path, optarg, ",:", canonicalize_path);
			break;
		case 't':
			process_opts_split(config->tags, optarg, ",:", canonicalize_default);
			break;
		case 'E':
			process_opts_file(config->exts, optarg);
			break;
		case 'I':
			process_opts_file(config->ignexts, optarg);
			break;
		case 'P':
			process_opts_file(config->path, optarg);
			break;
		case 'T':
			process_opts_file(config->tags, optarg);
			break;
		case 'R':
			process_opts_regex_file(optarg);
			break;
		case 'v':
			fprintf(stdout, "naudit %s\n", NAUDIT_VERSION); quit();
			break;
		default:
			usage(argv[0]);
		}
	}

	if (optind < argc) {
		unsigned int limit = argc - optind;
		for (i = 0; i < limit; i++, optind++) {
			char *path = canonicalize_path(argv[optind]);
			if (!path) {
				err("canonicalize_path failed: %s: %s\n", strerror(errno), argv[optind]);
				continue;
			}
			vector_add(config->path, path);
		}
	}

	if (!init_config()) {
		err("init_config failed\n");
		goto err;
	}

	load_map_files();
	if (!config->maps->size) {
		fprintf(stdout, "No map file selected. Quitting\n");
		goto err;
	}

	screens = calloc(config->maps->size, sizeof(void *));
	if (!screens) {
		err("calloc failed: %s\n", strerror(errno));
		goto err;
	}

	return config;

err:
	del_config();
	return NULL;
}


/*
 *
 */
int main (int argc, char **argv)
{
	unsigned int i;
	unsigned int ctags;
	char **tags;
	map_t *map = NULL;
	config_t *config = NULL;

	if (argc < 2)
		usage(argv[0]);

	config = process_opts(argc, argv);
	if (!config)
		usage(argv[0]);

	open_log();

	initscr();
	scr = initCDKScreen(stdscr);

	for (i = 0; i < config->maps->size; i++) {

		tags = (char **)config->tags->data;
		ctags = config->tags->size;
		map = config->maps->data[i];
		if (map->empty) {
			tags = empty_tags;
			ctags = 1;
		}

		screens[i] = newCDKSelection(scr,
				LEFT, TOP, RIGHT, LINES, COLS,
				map->title,
				(char **)map->raw_lines->data,
				map->raw_lines->size,
				tags, ctags,
				0, 1, 0);

		if (!screens[i]) {
			err("newCDKSelection failed\n");
			quit();
		}

		if (map->values)
			setCDKSelectionChoices(screens[i], vector_to_int_array(map->values));

		setCDKSelectionPreProcess(screens[i], handle_key, config);
	}

	save_config(screens);

	if (batchmode)
		quit();

	if (screens && screens[0])
		activateCDKSelection(screens[0], NULL);

	quit();
	return 0;
}

