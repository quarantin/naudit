#define _XOPEN_SOURCE 500
#include <cdk.h>
#include "naudit.h"
#include "config.h"
#include "vector.h"
#include "find.h"
#include "util.h"
#include "map.h"
#define TAG_FILE "tags.conf"


/*
 *
 */
static config_t *config = NULL;


/*
 *
 */
static char *default_tags[] = {
	"TODO",
	"Not vulnerable",
	"Need review",
	"Might be vulnerable",
	"Probably vulnerable",
	"Vulnerable",
};


/*
 *
 */
void load_map_files (void)
{
	char path[BUFSIZ];
	DIR *dir = NULL;
	map_t *map = NULL;
	struct dirent *entry = NULL;

	dir = opendir(config->session);
	if (!dir) {
		err("opendir failed: %s: `%s'\n", strerror(errno), config->session);
		return;
	}

	while ((entry = readdir(dir))) {

		if (!strncmp(entry->d_name, ".", 1) ||
				!strncmp(entry->d_name, "..", 2) ||
				!strncmp(entry->d_name, TAG_FILE, strlen(TAG_FILE)))
			continue;

		snprintf(path, BUFSIZ, "%s/%s", config->session, entry->d_name);
		map = load_map(path);
		vector_add(config->maps, map);
	}

	closedir(dir);
	return;
}


/*
 *
 */
config_t *get_config (void)
{
	if (config)
		return config;

	config = calloc(1, sizeof(config_t));
	if (!config) {
		err("calloc failed: %s\n", strerror(errno));
		return NULL;
	}

	config->exts = new_vector(free);
	config->path = new_vector(free);
	config->tags = new_vector(free);
	config->maps = new_vector((free_t *)&del_map);
	config->regex = new_vector((free_t *)&del_search);
	config->ignexts = new_vector(free);
	if (!config->exts || !config->maps || !config->path
	||  !config->tags || !config->regex || !config->ignexts) {
		err("allocation failure: %s\n", strerror(errno));
		del_config();
		return NULL;
	}

	return config;
}


/*
 *
 */
static int config_root (void)
{
	int ret;
	char *ptr;
	char path[BUFSIZ];

	if (!config->root) {
		ptr = getenv("HOME");
		if (!ptr) {
			ptr = ".";
			fprintf(stdout, "HOME not defined. Using `.' as root folder\n");
		}
		snprintf(path, BUFSIZ, "%s/.naudit", ptr);
		config->root = strdup(path);
		if (!config->root) {
			err("strdup failed: %s\n", strerror(errno));
			return 0;
		}
	}

	if (!file_exists(config->root)) {
		ret = mkdir(config->root, 0700);
		if (ret == -1) {
			err("mkdir failed: %s: `%s'\n", strerror(errno), config->root);
			return 0;
		}
	}

	return 1;
}


/*
 *
 */
static int config_session (void)
{
	int ret;
	char path[BUFSIZ];

	if (!config->session) {
		config->session = strdup("default");
		if (!config->session) {
			err("strdup failed: %s\n", strerror(errno));
			return 0;
		}
	}

	snprintf(path, BUFSIZ, "%s/%s", config->root, config->session);
	free(config->session);
	config->session = strdup(path);
	if (!config->session) {
		err("strdup failed: %s\n", strerror(errno));
		return 0;
	}

	if (!file_exists(config->session)) {
		ret = mkdir(config->session, 0700);
		if (ret == -1) {
			err("mkdir failed: %s: `%s'\n", strerror(errno), config->session);
			return 0;
		}
	}

	return 1;
}


/*
 *
 */
static int config_tags (void)
{
	unsigned int i = 1;
	char buf[BUFSIZ];
	char path[BUFSIZ];
	FILE *fp = NULL;
	vector_t *tags = NULL;

	snprintf(path, BUFSIZ, "%s/%s", config->session, TAG_FILE);

	if (config->tags->size) {

		if (file_exists(path)) {
			do
				snprintf(buf, BUFSIZ, "%s.%d", path, i++);
			while (file_exists(buf));
			rename(path, buf);
		}
		write_file(path, config->tags);
		return 1;
	}

	if (file_exists(path))
		goto end;

	snprintf(path, BUFSIZ, "%s/%s", config->root, TAG_FILE);
	if (file_exists(path))
		goto end;

	fp = fopen(path, "w");
	if (!fp) {
		err("fopen failed: %s: `%s'\n", strerror(errno), path);
		return 0;
	}

	for (i = 0; i < sizeof(default_tags)/sizeof(void *); i++)
		fprintf(fp, "%s\n", default_tags[i]);

	fclose(fp);

end:
	tags = read_file(path);
	if (!tags) {
		err("read_file failed: `%s'\n", path);
		return 0;
	}

	del_vector(config->tags);
	config->tags = tags;
	return 1;
}


/*
 *
 */
static int config_find (void)
{
	char *ptr;

	if (config->exts->size || config->path->size || config->regex->size) {

		if (!config->path->size) {
			ptr = realpath(".", NULL);
			if (!ptr) {
				err("realpath failed: %s: `.'\n", strerror(errno));
				return 0;
			}
			vector_add(config->path, ptr);
		}

		if (!config->exts->size) {
			ptr = strdup("*");
			if (!ptr) {
				err("strdup failed: %s\n", strerror(errno));
				return 0;
			}
			vector_add(config->exts, ptr);
		}

		if (!config->regex->size) {
			fprintf(stdout, "Option --regex or --regex-file missing\n");
			return 0;
		}

		find();
	}

	return 1;
}


/*
 *
 */
int init_config (void)
{
	if (!config_root())
		return 0;

	if (!config_session())
		return 0;

	if (!config_tags())
		return 0;

	if (!config_find())
		return 0;

	return 1;
}


/*
 *
 */
void del_config (void)
{
	if (!config)
		return;

	del_vector(config->exts);
	del_vector(config->path);
	del_vector(config->tags);
	del_vector(config->maps);
	del_vector(config->regex);
	del_vector(config->ignexts);
	free(config->root);
	free(config->session);
	free(config);
}


/*
 *
 */
void save_config (CDKSELECTION **screens)
{
	unsigned int i;
	map_t *map = NULL;
	int *values = NULL;

	if (!config->maps || !config->maps->size)
		return;

	for (i = 0; i < config->maps->size; i++) {
		map = config->maps->data[i];
		values = getCDKSelectionChoices(screens[i]);
		save_map(map, values);
	}
}

