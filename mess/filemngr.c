#include "driver.h"
#include <signal.h>
#include "utils.h"

static int count_chars_entered;
static char *enter_string;
static int enter_string_size;
static int enter_filename_mode;

static char entered_filename[512];

static void start_enter_string(char *string_buffer, int max_string_size, int filename_mode)
{
	enter_string = string_buffer;
	count_chars_entered = strlen(string_buffer);
	enter_string_size = max_string_size;
	enter_filename_mode = filename_mode;
}


/* code, lower case (w/o shift), upper case (with shift), control */
static int code_to_char_table[] =
{
	KEYCODE_0, '0', ')', 0,
	KEYCODE_1, '1', '!', 0,
	KEYCODE_2, '2', '"', 0,
	KEYCODE_3, '3', '#', 0,
	KEYCODE_4, '4', '$', 0,
	KEYCODE_5, '5', '%', 0,
	KEYCODE_6, '6', '^', 0,
	KEYCODE_7, '7', '&', 0,
	KEYCODE_8, '8', '*', 0,
	KEYCODE_9, '9', '(', 0,
	KEYCODE_A, 'a', 'A', 1,
	KEYCODE_B, 'b', 'B', 2,
	KEYCODE_C, 'c', 'C', 3,
	KEYCODE_D, 'd', 'D', 4,
	KEYCODE_E, 'e', 'E', 5,
	KEYCODE_F, 'f', 'F', 6,
	KEYCODE_G, 'g', 'G', 7,
	KEYCODE_H, 'h', 'H', 8,
	KEYCODE_I, 'i', 'I', 9,
	KEYCODE_J, 'j', 'J', 10,
	KEYCODE_K, 'k', 'K', 11,
	KEYCODE_L, 'l', 'L', 12,
	KEYCODE_M, 'm', 'M', 13,
	KEYCODE_N, 'n', 'N', 14,
	KEYCODE_O, 'o', 'O', 15,
	KEYCODE_P, 'p', 'P', 16,
	KEYCODE_Q, 'q', 'Q', 17,
	KEYCODE_R, 'r', 'R', 18,
	KEYCODE_S, 's', 'S', 19,
	KEYCODE_T, 't', 'T', 20,
	KEYCODE_U, 'u', 'U', 21,
	KEYCODE_V, 'v', 'V', 22,
	KEYCODE_W, 'w', 'W', 23,
	KEYCODE_X, 'x', 'X', 24,
	KEYCODE_Y, 'y', 'Y', 25,
	KEYCODE_Z, 'z', 'Z', 26,
	KEYCODE_OPENBRACE, '[', '{', 27,
	KEYCODE_BACKSLASH, '\\', '|', 28,
	KEYCODE_CLOSEBRACE, ']', '}', 29,
	KEYCODE_TILDE, '^', '~', 30,
	KEYCODE_BACKSPACE, 127, 127, 31,
	KEYCODE_COLON, ':', ';', 0,
	KEYCODE_EQUALS, '=', '+', 0,
	KEYCODE_MINUS, '-', '_', 0,
	KEYCODE_STOP, '.', '<', 0,
	KEYCODE_COMMA, ',', '>', 0,
	KEYCODE_SLASH, '/', '?', 0,
	KEYCODE_ENTER, 13, 13, 13,
	KEYCODE_ESC, 27, 27, 27
};

/*
 * For now I use a lookup table for valid filename characters.
 * Maybe change this for different platforms?
 * Put it to osd_cpu? Make it an osd_... function?
 */
static char valid_filename_char[256] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 00-0f */
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 10-1f */
	1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 	/*	!"#$%&'()*+,-./ */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 	/* 0123456789:;<=>? */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* @ABCDEFGHIJKLMNO */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 	/* PQRSTUVWXYZ[\]^_ */
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/* `abcdefghijklmno */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 	/* pqrstuvwxyz{|}~	*/
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 80-8f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 90-9f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* a0-af */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* b0-bf */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* c0-cf */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* d0-df */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* e0-ef */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0		/* f0-ff */
};

static char code_to_ascii(InputCode code)
{
	int i;

	for (i = 0; i < (sizeof (code_to_char_table) / (sizeof (int) * 4)); i++)

	{
		if (code_to_char_table[i * 4] == code)
		{
			if (keyboard_pressed(KEYCODE_LCONTROL) ||
				keyboard_pressed(KEYCODE_RCONTROL))
				return code_to_char_table[i * 4 + 3];
			if (keyboard_pressed(KEYCODE_LSHIFT) ||
				keyboard_pressed(KEYCODE_RSHIFT))
				return code_to_char_table[i * 4 + 2];
			return code_to_char_table[i * 4 + 1];
		}
	}

	return -1;
}

static char *update_entered_string(void)
{
	InputCode code;
	int ascii_char;

	/* get key */
	code = keyboard_read_async();

	/* key was pressed? */
	if (code == CODE_NONE)
		return NULL;

	ascii_char = code_to_ascii(code);

	switch (ascii_char)
	{
		/* char could not be converted to ascii */
	case -1:
		return NULL;

	case 13:	/* Return */
		return enter_string;

	case 25:	/* Ctrl-Y (clear line) */
		count_chars_entered = 0;
		enter_string[count_chars_entered] = '\0';
		break;

	case 27:	/* Escape */
		return NULL;

		/* delete */
	case 127:
		count_chars_entered--;
		if (count_chars_entered < 0)
			count_chars_entered = 0;
		enter_string[count_chars_entered] = '\0';
		break;

		/* got a char - add to string */
	default:
		if (count_chars_entered < enter_string_size)
		{
			if ((enter_filename_mode && valid_filename_char[(unsigned)ascii_char]) ||
				!enter_filename_mode)
			{
				/* store char */
				enter_string[count_chars_entered] = ascii_char;
				/* update count of chars entered */
				count_chars_entered++;
				/* add null to end of string */
				enter_string[count_chars_entered] = '\0';
			}
		}
		break;
	}

	return NULL;
}


char current_filespecification[32] = "*.*";
const char fs_directory[] = "[DIR]";
const char fs_device[] = "[DRIVE]";
const char fs_file[] = "[FILE]";
/*const char fs_archive[] = "[ARCHIVE]"; */

static const char **fs_item;
static const char **fs_subitem;
static char *fs_flags;
static int *fs_types;
static int *fs_order;
static int fs_chunk;
static int fs_total;

enum {
	FILESELECT_NONE,
	FILESELECT_QUIT,
	FILESELECT_FILESPEC,
	FILESELECT_DEVICE,
	FILESELECT_DIRECTORY,
	FILESELECT_FILE
} FILESELECT_ENTRY_TYPE;


char *fs_dupe(const char *src, int len)
{
	char *dst;
	int display_length;
	int display_width;

	display_width = (Machine->uiwidth / Machine->uifontwidth);
	display_length = len;

	if (display_length>display_width)
		display_length = display_width;

	/* malloc space for string + NULL char + extra char.*/
	dst = malloc(len+2);
	if (dst)
	{
		strcpy(dst, src);
		/* copy old char to end of string */
		dst[len+1]=dst[display_length];
		/* put in NULL to cut string. */
		dst[len+1]='\0';
	}
	return dst;

}


void fs_free(void)
{
	if (fs_chunk > 0)
	{
		int i;
		/* free duplicated strings of file and directory names */
		for (i = 0; i < fs_total; i++)
		{
			if (fs_types[i] == FILESELECT_FILE ||
				fs_types[i] == FILESELECT_DIRECTORY)
				free((char *)fs_item[i]);
		}
		free(fs_item);
		free(fs_subitem);
		free(fs_flags);
		free(fs_types);
		free(fs_order);
		fs_chunk = 0;
		fs_total = 0;
	}
}

int fs_alloc(void)
{
	if (fs_total < fs_chunk)
	{
		fs_order[fs_total] = fs_total;
		return (fs_total += 1) - 1;
	}
	if (fs_chunk)
	{
		fs_chunk += 256;
		logerror("fs_alloc() next chunk (total %d)\n", fs_chunk);
		fs_item = realloc(fs_item, fs_chunk * sizeof(char **));
		fs_subitem = realloc(fs_subitem, fs_chunk * sizeof(char **));
		fs_flags = realloc(fs_flags, fs_chunk * sizeof(char *));
		fs_types = realloc(fs_types, fs_chunk * sizeof(int));
		fs_order = realloc(fs_order, fs_chunk * sizeof(int));
	}
	else
	{
		fs_chunk = 512;
		logerror("fs_alloc() first chunk %d\n", fs_chunk);
		fs_item = malloc(fs_chunk * sizeof(char **));
		fs_subitem = malloc(fs_chunk * sizeof(char **));
		fs_flags = malloc(fs_chunk * sizeof(char *));
		fs_types = malloc(fs_chunk * sizeof(int));
		fs_order = malloc(fs_chunk * sizeof(int));
	}

	/* what do we do if reallocation fails? raise(SIGABRT) seems a way outa here */
	if (!fs_item || !fs_subitem || !fs_flags || !fs_types || !fs_order)
	{
		logerror("failed to allocate fileselect buffers!\n");
		raise(SIGABRT);
	}

	fs_order[fs_total] = fs_total;
	return (fs_total += 1) - 1;
}

static int DECL_SPEC fs_compare(const void *p1, const void *p2)
{
	const int i1 = *(int *)p1;
	const int i2 = *(int *)p2;

	if (fs_types[i1] != fs_types[i2])
		return fs_types[i1] - fs_types[i2];
	return strcmp(fs_item[i1], fs_item[i2]);

}

#define MAX_ENTRIES_IN_MENU (SEL_MASK-1)

int fs_init_done = 0;

void fs_generate_filelist(void)
{
	void *dir;
	int qsort_start, count, i, n;
	const char **tmp_menu_item;
	const char **tmp_menu_subitem;
	char *tmp_flags;
	int *tmp_types;

	/* should be moved inside mess.c ??? */
	if (fs_init_done==0)
	{
		/* this will not work if roms is not a sub-dir of mess, and
		   will also not work if we are not in the mess dir */
		/* go to initial roms directory */
		osd_change_directory("software");
		osd_change_directory(Machine->gamedrv->name);
		fs_init_done = 1;
	}

	/* just to be safe */
	fs_free();

	/* quit back to main menu option at top */
	n = fs_alloc();
	fs_item[n] = "Quit Fileselector";
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_QUIT;
	fs_flags[n] = 0;

	/* insert blank line */
	n = fs_alloc();
	fs_item[n] = "-";
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_NONE;
	fs_flags[n] = 0;

	/* current directory */
	n = fs_alloc();
	fs_item[n] = osd_get_cwd();
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_NONE;
	fs_flags[n] = 0;

	/* blank line */
	n = fs_alloc();
	fs_item[n] = "-";
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_NONE;
	fs_flags[n] = 0;


	/* file specification */
	n = fs_alloc();
	fs_item[n] = "File Specification";
	fs_subitem[n] = current_filespecification;
	fs_types[n] = FILESELECT_FILESPEC;
	fs_flags[n] = 0;

	/* insert blank line */
	n = fs_alloc();
	fs_item[n] = "-";
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_NONE;
	fs_flags[n] = 0;

	qsort_start = fs_total;

	/* devices first */
	count = osd_num_devices();
	if (count > 0)
	{
		logerror("fs_generate_filelist: %d devices\n", count);
		for (i = 0; i < count; i++)
		{
			if (fs_total >= MAX_ENTRIES_IN_MENU)
				break;
			n = fs_alloc();
			fs_item[n] = osd_get_device_name(i);
			fs_subitem[n] = fs_device;
			fs_types[n] = FILESELECT_DEVICE;
			fs_flags[n] = 0;
		}
	}

	/* directory entries */
	dir = osd_dir_open(".", current_filespecification);
	if (dir)
	{
		int len, filetype;
		char filename[260];
		len = osd_dir_get_entry(dir, filename, sizeof(filename), &filetype);
		while (len > 0)
		{
			if (fs_total >= MAX_ENTRIES_IN_MENU)
				break;
			n = fs_alloc();
			fs_item[n] = fs_dupe(filename,len);
			if (filetype)
			{
				fs_types[n] = FILESELECT_DIRECTORY;
				fs_subitem[n] = fs_directory;
			}
			else
			{
				fs_types[n] = FILESELECT_FILE;
				fs_subitem[n] = fs_file;
			}
			fs_flags[n] = 0;
			len = osd_dir_get_entry(dir, filename, sizeof(filename), &filetype);
		}
		osd_dir_close(dir);
	}

	n = fs_alloc();
	fs_item[n] = 0; 		 /* terminate array */
	fs_subitem[n] = 0;
	fs_types[n] = FILESELECT_NONE;
	fs_flags[n] = 0;

	logerror("fs_generate_filelist: sorting %d entries\n", n - qsort_start);
	qsort(&fs_order[qsort_start], n - qsort_start, sizeof(int), fs_compare);

	tmp_menu_item = malloc(n * sizeof(char *));
	tmp_menu_subitem = malloc(n * sizeof(char *));
	tmp_flags = malloc(n * sizeof(char));
	tmp_types = malloc(n * sizeof(int));

	/* no space to sort? have to leave now... */
	if (!tmp_menu_item || !tmp_menu_subitem || !tmp_flags || !tmp_types )
		return;

	/* copy items in original order */
	memcpy(tmp_menu_item, fs_item, n * sizeof(char *));
	memcpy(tmp_menu_subitem, fs_subitem, n * sizeof(char *));
	memcpy(tmp_flags, fs_flags, n * sizeof(char));
	memcpy(tmp_types, fs_types, n * sizeof(int));

	for (i = qsort_start; i < n; i++)
	{
		int j = fs_order[i];
		fs_item[i] = tmp_menu_item[j];
		fs_subitem[i] = tmp_menu_subitem[j];
		fs_flags[i] = tmp_flags[j];
		fs_types[i] = tmp_types[j];
	}

	free(tmp_menu_item);
	free(tmp_menu_subitem);
	free(tmp_flags);
	free(tmp_types);
}

#define UI_SHIFT_PRESSED		(keyboard_pressed(KEYCODE_LSHIFT) || keyboard_pressed(KEYCODE_RSHIFT))
#define UI_CONTROL_PRESSED		(keyboard_pressed(KEYCODE_LCONTROL) || keyboard_pressed(KEYCODE_RCONTROL))
/* and mask to get bits */
#define SEL_BITS_MASK			(~SEL_MASK)

int fileselect(struct mame_bitmap *bitmap, int selected)
{
	int sel, total, arrowize;
	int visible;

	sel = selected - 1;

	/* generate menu? */
	if (fs_total == 0)
		fs_generate_filelist();

	total = fs_total - 1;

	if (total > 0)
	{
		/* make sure it is in range - might go out of range if
		 * we were stepping up and down directories */
		if ((sel & SEL_MASK) >= total)
			sel = (sel & SEL_BITS_MASK) | (total - 1);

		arrowize = 0;
		if (sel < total)
		{
			switch (fs_types[sel])
			{
				/* arrow pointing inwards (arrowize = 1) */
				case FILESELECT_QUIT:
				case FILESELECT_FILE:
					break;

				case FILESELECT_FILESPEC:
				case FILESELECT_DIRECTORY:
				case FILESELECT_DEVICE:
					/* arrow pointing to right -
					 * indicating more available if
					 * selected, or editable */
					arrowize = 2;
					break;
			}
		}

		if (sel & (1 << SEL_BITS))	/* are we waiting for a new key? */
		{
			char *name;

			/* change menu item to show this filename */
			fs_subitem[sel & SEL_MASK] = current_filespecification;

			/* display the menu */
			ui_displaymenu(bitmap, fs_item, fs_subitem, fs_flags, sel & SEL_MASK, 3);

			/* update string with any keys that are pressed */
			name = update_entered_string();

			/* finished entering filename? */
			if (name)
			{
				/* yes */
				sel &= SEL_MASK;

				/* if no name entered - go back to default all selection */
				if (strlen(name) == 0)
					strcpy(current_filespecification, "*");
				else
					strcpy(current_filespecification, name);
				fs_free();
			}

			schedule_full_refresh();
			return sel + 1;
		}


		ui_displaymenu(bitmap, fs_item, fs_subitem, fs_flags, sel, arrowize);

		/* borrowed from usrintrf.c */
		visible = Machine->uiheight / (3 * Machine->uifontheight /2) -1;

		if (input_ui_pressed_repeat(IPT_UI_DOWN, 8))
		{
			if (UI_CONTROL_PRESSED)
			{
				sel = total - 1;
			}
			else
			if (UI_SHIFT_PRESSED)
			{
				sel = (sel + visible) % total;
			}
			else
			{
				sel = (sel + 1) % total;
			}
		}

		if (input_ui_pressed_repeat(IPT_UI_UP, 8))
		{
			if (UI_CONTROL_PRESSED)
			{
				sel = 0;
			}
			if (UI_SHIFT_PRESSED)
			{
				sel = (sel + total - visible) % total;
			}
			else
			{
				sel = (sel + total - 1) % total;
			}
		}

		if (input_ui_pressed(IPT_UI_SELECT))
		{
			if (sel < SEL_MASK)
			{
				switch (fs_types[sel])
				{
				case FILESELECT_QUIT:
					sel = -1;
					break;

				case FILESELECT_FILESPEC:
					start_enter_string(current_filespecification, 32, 0);
					sel |= 1 << SEL_BITS; /* we'll ask for a key */
					schedule_full_refresh();
					break;

				case FILESELECT_FILE:
					/* copy filename */
					strncpyz(entered_filename, osd_get_cwd(), sizeof(entered_filename) / sizeof(entered_filename[0]));
					strncatz(entered_filename, fs_item[sel], sizeof(entered_filename) / sizeof(entered_filename[0]));

					fs_free();
					sel = -3;
					break;

				case FILESELECT_DIRECTORY:
					/*	fs_chdir(fs_item[sel]); */
					osd_change_directory(fs_item[sel]);
					fs_free();

					schedule_full_refresh();
					break;

				case FILESELECT_DEVICE:
					/*	 fs_chdir("/"); */
					osd_change_device(fs_item[sel]);
					fs_free();
					schedule_full_refresh();
					break;

				default:
					break;
				}
			}
		}

		if (input_ui_pressed(IPT_UI_CANCEL))
			sel = -1;

		if (input_ui_pressed(IPT_UI_CONFIGURE))
			sel = -2;
	}
	else
	{
		sel = -1;
	}

	if (sel == -1 || sel == -3)
		fs_free();

	if (sel == -1 || sel == -2 || sel == -3)
		schedule_full_refresh();

	return sel + 1;
}

int filemanager(struct mame_bitmap *bitmap, int selected)
{
	static int previous_sel;
	const char *name;
	const char *menu_item[40];
	const char *menu_subitem[40];
	int types[40];
	int ids[40];
	char flag[40];

	int sel, total, arrowize, type, id;

	const struct IODevice *dev = Machine->gamedrv->dev;

	sel = selected - 1;


	total = 0;

	/* Cycle through all devices for this system */
	while(dev->type != IO_END)
	{
		type = dev->type;
		for (id = 0; id < dev->count; id++)
		{
			name = device_typename_id(type, id);

			menu_item[total] = (name) ? name : "---";

			name = device_filename(type, id);

			menu_subitem[total] = (name) ? name : "---";

			flag[total] = 0;
			types[total] = type;
			ids[total] = id;

			total++;

		}
		dev++;
	}


	/* if the fileselect() mode is active */
	if (sel & (2 << SEL_BITS))
	{
		sel = fileselect(bitmap, selected & ~(2 << SEL_BITS));
		if (sel != 0 && sel != -1 && sel!=-2)
			return sel | (2 << SEL_BITS);

		if (sel==-2)
		{
			/* selected a file */

			/* finish entering name */
			previous_sel = previous_sel & SEL_MASK;

			/* attempt a filename change */
			device_filename_change(types[previous_sel], ids[previous_sel], entered_filename);
		}

		sel = previous_sel;

		/* change menu item to show this filename */
		menu_subitem[sel & SEL_MASK] = entered_filename;

	}

	menu_item[total] = "Return to Main Menu";
	menu_subitem[total] = 0;
	flag[total] = 0;
	total++;
	menu_item[total] = 0;			   /* terminate array */
	menu_subitem[total] = 0;
	flag[total] = 0;

	arrowize = 0;
	if (sel < total - 1)
		arrowize = 2;

	if (sel & (1 << SEL_BITS))	/* are we waiting for a new key? */
	{
		/* change menu item to show this filename */
		menu_subitem[sel & SEL_MASK] = entered_filename;

		/* display the menu */
		ui_displaymenu(bitmap, menu_item, menu_subitem, flag, sel & SEL_MASK, 3);

		/* update string with any keys that are pressed */
		name = update_entered_string();

		/* finished entering filename? */
		if (name)
		{
			/* yes */
			sel &= SEL_MASK;
			if (strlen(name) == 0)
				device_filename_change(types[sel], ids[sel], NULL);
			else
				device_filename_change(types[sel], ids[sel], name);
		}

		schedule_full_refresh();
		return sel + 1;
	}

	ui_displaymenu(bitmap, menu_item, menu_subitem, flag, sel, arrowize);

	if (input_ui_pressed_repeat(IPT_UI_DOWN, 8))
		sel = (sel + 1) % total;

	if (input_ui_pressed_repeat(IPT_UI_UP, 8))
		sel = (sel + total - 1) % total;

	if (input_ui_pressed(IPT_UI_SELECT))
	{
		int os_sel;

		/* Return to main menu? */
		if (sel == total-1)
		{
			sel = -1;
			os_sel = -1;
		}
		/* no, let the osd code have a crack at changing files */
		else os_sel = osd_select_file (sel, entered_filename);

		if (os_sel != 0)
		{
			if (os_sel == 1)
			{
				/* attempt a filename change */
				device_filename_change(types[sel], ids[sel], entered_filename);
			}
		}
		/* osd code won't handle it, lets use our clunky interface */
		else if (!UI_SHIFT_PRESSED)
		{
			/* save selection and switch to fileselect() */
			previous_sel = sel;
			sel = (2 << SEL_BITS);
			fs_total = 0;
		}
		else
		{
			{
				if (strcmp(menu_subitem[sel], "---") == 0)
					entered_filename[0] = '\0';
				else
					strcpy(entered_filename, menu_subitem[sel]);
				start_enter_string(entered_filename, (sizeof(entered_filename) / sizeof(entered_filename[0])) - 1, 1);

				sel |= 1 << SEL_BITS;	/* we'll ask for a key */

				/* tell updatescreen() to clean after us (in case the window changes size) */
				schedule_full_refresh();
			}
		}
	}

	if (input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	if (input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if (sel == -1 || sel == -2)
		schedule_full_refresh();

	return sel + 1;
}

