#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <linux/limits.h>
#include "editor.h"
#include "util.h"


static editor_t *hex_editor = NULL;
static char hep[BUFSIZ];

static editor_t *text_editor = NULL;
static char tep[BUFSIZ];


/*
 *
 */
static void vim_open (char *file, char *line)
{
	char *argv[] = { tep, file, line, NULL };
	exec_cmd(argv);
}


/*
 *
 */
static void nano_open (char *file, char *line)
{
	char *argv[] = { tep, line, file, NULL };
	exec_cmd(argv);
}


/*
 *
 */
static void emacs_open (char *file, char *line)
{
	char *argv[] = { tep, "-nw", line, file, NULL };
	exec_cmd(argv);
}


/*
 *
 */
static void hexedit_open (char *file, char *line)
{
	char *argv[] = { hep, file, NULL };
	// Avoid compiler warning
	if (line) {;}
	exec_cmd(argv);
}


/*
 *
 */
static editor_t editor_list[] = {
	{ .name = "vi", .open = vim_open },
	{ .name = "vim", .open = vim_open },
	{ .name = "nano", .open = nano_open },
	{ .name = "emacs", .open = emacs_open },
	{ .name = "hexedit", .open = hexedit_open }
};


/*
 *
 */
static editor_t *find_editor (const char *path)
{
	unsigned int i;
	const char *name;
	editor_t *editor = NULL;

	name = strrchr(path, '/');
	name = (name ? name + 1 : path);

	for (i = 0; i < sizeof(editor_list)/sizeof(void *); i++) {

		if (!strcmp(name, editor_list[i].name)) {
			editor = &editor_list[i];
			break;
		}
	}

	return editor;
}


/*
 *
 */
editor_t *get_hex_editor (void)
{
	char *editor;

	if (hex_editor)
		return hex_editor;

	editor = getenv("HEX_EDITOR");
	if (!editor || !*editor)
		editor = "hexedit";

	snprintf(hep, BUFSIZ, "%s", editor);
	hex_editor = find_editor(editor);
	return hex_editor;
}


/*
 *
 */
editor_t *get_text_editor (void)
{
	char *editor;

	if (text_editor)
		return text_editor;

	editor = getenv("TEXT_EDITOR");
	if (!editor || !*editor) {
		editor = getenv("EDITOR");
		if (!editor || !*editor)
			editor = "vim";
	}

	snprintf(tep, BUFSIZ, "%s", editor);
	text_editor = find_editor(editor);
	return text_editor;
}

