/*********************************************************************

	messtest.c

	MESS testing code

*********************************************************************/

#include <stdio.h>
#include <ctype.h>

#include "messtest.h"
#include "osdepend.h"
#include "pool.h"
#include "inputx.h"

#include "expat/expat.h"

/* ----------------------------------------------------------------------- */



enum messtest_phase
{
	STATE_ROOT,
	STATE_TEST,
	STATE_COMMAND,
	STATE_ABORTED
};

enum blobparse_state
{
	BLOBSTATE_INITIAL,
	BLOBSTATE_AFTER_0,
	BLOBSTATE_HEX,
	BLOBSTATE_INQUOTES
};

struct messtest_state
{
	XML_Parser parser;
	enum messtest_phase phase;
	memory_pool pool;

	const char *script_filename;
	struct messtest_testcase testcase;
	int command_count;

	struct messtest_command current_command;

	enum blobparse_state blobstate;

	int test_flags;
	int test_count;
	int failure_count;
};



static const XML_Char *find_attribute(const XML_Char **attributes, const XML_Char *seek_attribute)
{
	int i;
	for (i = 0; attributes[i] && strcmp(attributes[i], seek_attribute); i += 2)
		;
	return attributes[i] ? attributes[i+1] : NULL;
}



static int append_command(struct messtest_state *state)
{
	struct messtest_command *new_commands;

	new_commands = (struct messtest_command *) pool_realloc(&state->pool,
		state->testcase.commands, (state->command_count + 1) * sizeof(state->testcase.commands[0]));
	if (!new_commands)
		return FALSE;

	state->testcase.commands = new_commands;
	state->testcase.commands[state->command_count++] = state->current_command;
	return TRUE;
}



static void report_parseerror(struct messtest_state *state, const char *fmt, ...)
{
	char buf[1024];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, va);
	va_end(va);

	fprintf(stderr, "%s:(%i:%i): %s\n",
		state->script_filename,
		XML_GetCurrentLineNumber(state->parser),
		XML_GetCurrentColumnNumber(state->parser),
		buf);
	state->phase = STATE_ABORTED;
}



static void parse_offset(const char *s, offs_t *result)
{
	if ((s[0] == '0') && (tolower(s[1]) == 'x'))
		sscanf(&s[2], "%x", result);
	else
		*result = atoi(s);
}



static mame_time parse_time(const char *s)
{
	double d = atof(s);
	return double_to_mame_time(d);
}



static void start_handler(void *data, const XML_Char *tagname, const XML_Char **attributes)
{
	struct messtest_state *state = (struct messtest_state *) data;
	const XML_Char *s;
	const XML_Char *s1;
	const XML_Char *s2;
	const XML_Char *s3;
	const char *attr_name;
	struct messtest_command cmd;
	int region;
	int device_type;
	int preload;
	mame_time rate;

	switch(state->phase) {
	case STATE_ROOT:
		if (!strcmp(tagname, "tests"))
		{
			/* <tests> - used to group tests together */
		}
		else if (!strcmp(tagname, "test"))
		{
			/* <test> - identifies a specific test */

			memset(&state->testcase, 0, sizeof(state->testcase));

			/* 'name' attribute */
			attr_name = "name";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;
			state->testcase.name = pool_strdup(&state->pool, s);
			if (!state->testcase.name)
				goto outofmemory;

			/* 'driver' attribute */
			attr_name = "driver";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;
			state->testcase.driver = pool_strdup(&state->pool, s);
			if (!state->testcase.driver)
				goto outofmemory;

			/* 'ramsize' attribute */
			s = find_attribute(attributes, "ramsize");
			state->testcase.ram = s ? ram_parse_string(s) : 0;

			state->phase = STATE_TEST;
			state->testcase.commands = NULL;
			state->command_count = 0;
		}
		else
			goto unknowntag;
		break;

	case STATE_TEST:
		memset(&state->current_command, 0, sizeof(state->current_command));
		if (!strcmp(tagname, "wait"))
		{
			/* <wait> - waits for a duration of emulated time */
			attr_name = "time";
			s = find_attribute(attributes, attr_name);
			if (!s)
				goto missing_attribute;

			state->current_command.command_type = MESSTEST_COMMAND_WAIT;
			state->current_command.u.wait_time = mame_time_to_double(parse_time(s));
		}
		else if (!strcmp(tagname, "input"))
		{
			/* <input> - inputs natural keyboard data into a system */
			state->current_command.command_type = MESSTEST_COMMAND_INPUT;

			s = find_attribute(attributes, "rate");
			rate = s ? parse_time(s) : make_mame_time(0, 0);
			state->current_command.u.input_args.rate = rate;
		}
		else if (!strcmp(tagname, "rawinput"))
		{
			/* <rawinput> - inputs raw data into a system */
			state->current_command.command_type = MESSTEST_COMMAND_RAWINPUT;
		}
		else if (!strcmp(tagname, "switch"))
		{
			/* <switch> - switches a DIP switch/config setting */
			state->current_command.command_type = MESSTEST_COMMAND_SWITCH;

			/* 'name' attribute */
			attr_name = "name";
			s1 = find_attribute(attributes, attr_name);
			if (!s1)
				goto missing_attribute;

			/* 'value' attribute */
			attr_name = "value";
			s2 = find_attribute(attributes, attr_name);
			if (!s2)
				goto missing_attribute;

			state->current_command.u.switch_args.name =
				pool_strdup(&state->pool, s1);
			state->current_command.u.switch_args.value =
				pool_strdup(&state->pool, s2);
		}
		else if (!strcmp(tagname, "screenshot"))
		{
			/* <screenshot> - dumps a screenshot */
			state->current_command.command_type = MESSTEST_COMMAND_SCREENSHOT;
		}
		else if (!strcmp(tagname, "imagecreate") || !strcmp(tagname, "imageload"))
		{
			/* <imagecreate> - creates an image */
			/* <imageload> - loads an image */
			if (!strcmp(tagname, "imagecreate"))
				state->current_command.command_type = MESSTEST_COMMAND_IMAGE_CREATE;
			else
				state->current_command.command_type = MESSTEST_COMMAND_IMAGE_LOAD;

			/* 'preload' attribute */
			s = find_attribute(attributes, "preload");
			preload = s ? atoi(s) : 0;
			if (preload)
				state->current_command.command_type += 2;

			/* 'filename' attribute */
			s1 = find_attribute(attributes, "filename");

			/* 'type' attribute */
			attr_name = "type";
			s2 = find_attribute(attributes, attr_name);
			if (!s2)
				goto missing_attribute;
			device_type = device_typeid(s2);
			if (device_type < 0)
				goto bad_device_type;
			
			/* 'slot' attribute */
			s3 = find_attribute(attributes, "slot");

			state->current_command.u.image_args.filename =
				s1 ? pool_strdup(&state->pool, s1) : NULL;
			state->current_command.u.image_args.device_type = device_type;
			state->current_command.u.image_args.device_slot = s3 ? atoi(s3) : 0;
		}
		else if (!strcmp(tagname, "memverify"))
		{
			/* <memverify> - verifies that a range of memory contains specific data */
			attr_name = "start";
			s1 = find_attribute(attributes, attr_name);
			if (!s1)
				goto missing_attribute;

			attr_name = "end";
			s2 = find_attribute(attributes, attr_name);
			if (!s2)
				s2 = "0";

			s3 = find_attribute(attributes, "region");

			state->current_command.command_type = MESSTEST_COMMAND_VERIFY_MEMORY;
			state->blobstate = BLOBSTATE_INITIAL;
			parse_offset(s1, &state->current_command.u.verify_args.start);
			parse_offset(s2, &state->current_command.u.verify_args.end);

			if (s3)
			{
				region = memory_region_from_string(s3);
				if (region == REGION_INVALID)
					goto invalid_memregion;
				state->current_command.u.verify_args.mem_region = region;
			}
		}
		else
			goto unknowntag;
		state->phase = STATE_COMMAND;
		break;
	}

	return;

outofmemory:
	report_parseerror(state, "Out of memory");
	return;

missing_attribute:
	report_parseerror(state, "Missing attribute '%s'\n", attr_name);
	return;

unknowntag:
	report_parseerror(state, "Unknown tag '%s'\n", tagname);
	return;

invalid_memregion:
	report_parseerror(state, "Invalid memory region '%s'\n", s3);
	return;

bad_device_type:
	report_parseerror(state, "Bad device type '%s'\n", s2);
	return;
}



static void end_handler(void *data, const XML_Char *name)
{
	struct messtest_state *state = (struct messtest_state *) data;
	struct messtest_command *command = &state->current_command;

	switch(state->phase) {
	case STATE_TEST:
		/* append final end command */
		memset(&state->current_command, 0, sizeof(state->current_command));
		state->current_command.command_type = MESSTEST_COMMAND_END;
		if (!append_command(state))
			goto outofmemory;

		if (run_test(&state->testcase, state->test_flags, NULL))
			state->failure_count++;
		state->test_count++;
		state->phase = STATE_ROOT;
		break;

	case STATE_COMMAND:
		switch(command->command_type) {
		case MESSTEST_COMMAND_VERIFY_MEMORY:
			if (command->u.verify_args.end == 0)
			{
				command->u.verify_args.end = command->u.verify_args.start
					+ command->u.verify_args.verify_data_size - 1;
			}
			break;
		};

		if (!append_command(state))
			goto outofmemory;
		state->phase = STATE_TEST;
		break;
	}
	return;

outofmemory:
	fprintf(stderr, "Out of memory\n");
	state->phase = STATE_ABORTED;
	return;
}



static int hexdigit(char c)
{
	int result = 0;
	if (isdigit(c))
		result = c - '0';
	else if (isxdigit(c))
		result = toupper(c) - 'A' + 10;
	return result;
}



static void get_blob(struct messtest_state *state, const XML_Char *s, int len,
	 void **blob, size_t *blob_len)
{
	UINT8 *bytes = NULL;
	int bytes_len = 0;
	int i = 0;
	int j, k;

	while(i < len)
	{
		switch(state->blobstate) {
		case BLOBSTATE_INITIAL:
			if (isspace(s[i]))
			{
				/* ignore whitespace */
				i++;
			}
			else if (s[i] == '0')
			{
				state->blobstate = BLOBSTATE_AFTER_0;
				i++;
			}
			else if (s[i] == '\"')
			{
				state->blobstate = BLOBSTATE_INQUOTES;
				i++;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_AFTER_0:
			if (tolower(s[i]) == 'x')
			{
				state->blobstate = BLOBSTATE_HEX;
				i++;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_HEX:
			if (isspace(s[i]))
			{
				state->blobstate = BLOBSTATE_INITIAL;
				i++;
			}
			else if (isxdigit(s[i]))
			{
				/* count the number of hex digits available */
				for (j = i; (j < len) && isxdigit(s[j+0]) && isxdigit(s[j+1]); j += 2)
					;
				bytes_len = (j - i) / 2;
				
				/* allocate the memory */
				bytes = pool_realloc(&state->pool, *blob, *blob_len + bytes_len);
				if (!bytes)
					goto outofmemory;

				/* build the byte array */
				for (k = 0; k < bytes_len; k++)
				{
					bytes[k + *blob_len] =
						(hexdigit(s[i+k*2+0]) << 4) + hexdigit(s[i+k*2+1]);
				}

				i = j;
				*blob = bytes;
				*blob_len += bytes_len;
			}
			else
				goto parseerror;
			break;

		case BLOBSTATE_INQUOTES:
			if (s[i] == '\"')
			{
				state->blobstate = BLOBSTATE_INITIAL;
				i++;
			}
			else
			{
				/* count the number of quoted chars available */
				for (j = i; (j < len) && (s[j] != '\"'); j++)
					;
				bytes_len = j - i;
				
				/* allocate the memory */
				bytes = pool_realloc(&state->pool, *blob, *blob_len + bytes_len);
				if (!bytes)
					goto outofmemory;

				/* build the byte array */
				memcpy(&bytes[*blob_len], &s[i], bytes_len);

				i = j;
				*blob = bytes;
				*blob_len += bytes_len;
			}
			break;
		}
	}
	return;

outofmemory:
	fprintf(stderr, "Out of memory\n");
	state->phase = STATE_ABORTED;
	return;

parseerror:
	fprintf(stderr, "Invalid blob string specifier\n");
	state->phase = STATE_ABORTED;
	return;
}



static void data_handler(void *data, const XML_Char *s, int len)
{
	struct messtest_state *state = (struct messtest_state *) data;
	struct messtest_command *command = &state->current_command;
	char *str;
	int i, line_begin;
	int old_len;

	switch(state->phase) {
	case STATE_COMMAND:
		switch(command->command_type) {
		case MESSTEST_COMMAND_INPUT:
		case MESSTEST_COMMAND_RAWINPUT:
			str = (char *) command->u.input_args.input_chars;
			old_len = str ? strlen(str) : 0;
			str = pool_realloc(&state->pool, str, old_len + len + 1);
			if (!str)
				goto outofmemory;
			strncpyz(str + old_len, s, len + 1);
			command->u.input_args.input_chars = str;
			break;

		case MESSTEST_COMMAND_VERIFY_MEMORY:
			get_blob(state, s, len,
				(void **) &command->u.verify_args.verify_data,
				&command->u.verify_args.verify_data_size);
			break;
		}
	}
	return;

outofmemory:
	fprintf(stderr, "Out of memory\n");
	state->phase = STATE_ABORTED;
	return;
}



/* this external entity handler allows us to do things like this:
 *
 *	<!DOCTYPE tests
 *	[
 *		<!ENTITY mamekey_esc SYSTEM "http://www.mess.org/messtest/">
 *	]>
 */
static int external_entity_handler(XML_Parser parser,
	const XML_Char *context,
	const XML_Char *base,
	const XML_Char *systemId,
	const XML_Char *publicId)
{
	XML_Parser extparser = NULL;
	int rc = 0, i;
	char buf[256];
	char charbuf[UTF8_CHAR_MAX + 1];
	static const char *mamekey_prefix = "mamekey_";
	unicode_char_t c;

	buf[0] = '\0';

	/* only supportr our own schema */
	if (strcmp(systemId, "http://www.mess.org/messtest/"))
		goto done;

	extparser = XML_ExternalEntityParserCreate(parser, context, "UTF-8");
	if (!extparser)
		goto done;

	/* does this use the 'mamekey' prefix? */
	if ((strlen(context) > strlen(mamekey_prefix)) && !memcmp(context,
		mamekey_prefix, strlen(mamekey_prefix)))
	{
		context += strlen(mamekey_prefix);
		c = 0;

		/* this is interim until we can come up with a real solution */
		if (!strcmp(context, "esc"))
			c = UCHAR_MAMEKEY(ESC);
		else if (!strcmp(context, "up"))
			c = UCHAR_MAMEKEY(UP);

		if (c)
		{
			i = utf8_from_uchar(charbuf, sizeof(charbuf) / sizeof(charbuf[0]), c);
			if (i < 0)
				goto done;
			charbuf[i] = 0;

			snprintf(buf, sizeof(buf) / sizeof(buf[0]), "<%s%s>%s</%s%s>",
				mamekey_prefix, context,
				charbuf,
				mamekey_prefix, context);

			if (XML_Parse(extparser, buf, strlen(buf), 0) == XML_STATUS_ERROR)
				goto done;
		}
	}

	if (XML_Parse(extparser, NULL, 0, 1) == XML_STATUS_ERROR)
		goto done;

	rc = 1;
done:
	if (extparser)
		XML_ParserFree(extparser);
	return rc;
}



int messtest(const char *script_filename, int flags, int *test_count, int *failure_count)
{
	struct messtest_state state;
	char buf[1024];
	char saved_directory[1024];
	FILE *in;
	int len, done;
	int result = -1;
	char *script_directory;

	memset(&state, 0, sizeof(state));
	state.test_flags = flags;
	state.script_filename = script_filename;

	/* open the script file */
	in = fopen(script_filename, "r");
	if (!in)
	{
		fprintf(stderr, "%s: Cannot open file\n", script_filename);
		goto done;
	}

	/* save the current working directory, and change to the test directory */
	script_directory = osd_dirname(script_filename);
	if (script_directory)
	{
		osd_getcurdir(saved_directory, sizeof(saved_directory) / sizeof(saved_directory[0]));
		osd_setcurdir(script_directory);
		free(script_directory);
	}
	else
	{
		saved_directory[0] = '\0';
	}

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser)
	{
		fprintf(stderr, "Out of memory\n");
		goto done;
	}

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);
	XML_SetExternalEntityRefHandler(state.parser, external_entity_handler);

	do
	{
		len = (int) fread(buf, 1, sizeof(buf), in);
		done = feof(in);
		
		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR)
		{
			report_parseerror(&state, "%s",
				XML_ErrorString(XML_GetErrorCode(state.parser)));
			goto done;
		}
	}
	while(!done);

	result = 0;

done:
	/* restore the directory */
	if (saved_directory[0])
		osd_setcurdir(saved_directory);

	/* dispose of the parser */
	if (state.parser)
		XML_ParserFree(state.parser);

	/* write out test and failure counts */
	if (test_count)
		*test_count = state.test_count;
	if (failure_count)
		*failure_count = state.failure_count;
	return result;
}
