/*###################################################################################################
**
**
**      debugcmd.c
**      Debugger command interface engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#include "driver.h"
#include "debugcmd.h"
#include "debugcon.h"
#include "debugcpu.h"
#include "debugexp.h"
#include "debughlp.h"
#include "debugvw.h"
#include "artwork.h"
#include <stdarg.h>



/*###################################################################################################
**  DEBUGGING
**#################################################################################################*/

#define DEBUG			0



/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/



/*###################################################################################################
**  LOCAL VARIABLES
**#################################################################################################*/



/*###################################################################################################
**  PROTOTYPES
**#################################################################################################*/

static UINT64 execute_min(UINT32 ref, UINT32 params, UINT64 *param);
static UINT64 execute_max(UINT32 ref, UINT32 params, UINT64 *param);
static UINT64 execute_if(UINT32 ref, UINT32 params, UINT64 *param);

static void execute_help(int ref, int params, const char **param);
static void execute_printf(int ref, int params, const char **param);
static void execute_logerror(int ref, int params, const char **param);
static void execute_tracelog(int ref, int params, const char **param);
static void execute_quit(int ref, int params, const char **param);
static void execute_do(int ref, int params, const char **param);
static void execute_step(int ref, int params, const char **param);
static void execute_over(int ref, int params, const char **param);
static void execute_out(int ref, int params, const char **param);
static void execute_go(int ref, int params, const char **param);
static void execute_go_vblank(int ref, int params, const char **param);
static void execute_go_interrupt(int ref, int params, const char **param);
static void execute_focus(int ref, int params, const char **param);
static void execute_ignore(int ref, int params, const char **param);
static void execute_observe(int ref, int params, const char **param);
static void execute_next(int ref, int params, const char **param);
static void execute_bpset(int ref, int params, const char **param);
static void execute_bpclear(int ref, int params, const char **param);
static void execute_bpdisenable(int ref, int params, const char **param);
static void execute_bplist(int ref, int params, const char **param);
static void execute_wpset(int ref, int params, const char **param);
static void execute_wpclear(int ref, int params, const char **param);
static void execute_wpdisenable(int ref, int params, const char **param);
static void execute_wplist(int ref, int params, const char **param);
static void execute_save(int ref, int params, const char **param);
static void execute_dump(int ref, int params, const char **param);
static void execute_dasm(int ref, int params, const char **param);
static void execute_find(int ref, int params, const char **param);
static void execute_trace(int ref, int params, const char **param);
static void execute_traceover(int ref, int params, const char **param);
static void execute_snap(int ref, int params, const char **param);
static void execute_source(int ref, int params, const char **param);
static void execute_memdump(int ref, int params, const char **param);



/*###################################################################################################
**  CODE
**#################################################################################################*/

/*-------------------------------------------------
    debug_command_init - initializes the command
    system
-------------------------------------------------*/

void debug_command_init(void)
{
	struct symbol_table *global_table = debug_expression_get_global_symtable();

	/* add a few simple global functions */
	debug_symtable_add_function(global_table, "min", 0, 2, 2, execute_min);
	debug_symtable_add_function(global_table, "max", 0, 2, 2, execute_max);
	debug_symtable_add_function(global_table, "if", 0, 3, 3, execute_if);

	/* add all the commands */
	debug_console_register_command("help",      CMDFLAG_NONE, 0, 0, 1, execute_help);
	debug_console_register_command("printf",    CMDFLAG_NONE, 0, 1, MAX_COMMAND_PARAMS, execute_printf);
	debug_console_register_command("logerror",  CMDFLAG_NONE, 0, 1, MAX_COMMAND_PARAMS, execute_logerror);
	debug_console_register_command("tracelog",  CMDFLAG_NONE, 0, 1, MAX_COMMAND_PARAMS, execute_tracelog);
	debug_console_register_command("quit",      CMDFLAG_NONE, 0, 0, 0, execute_quit);
	debug_console_register_command("do",        CMDFLAG_NONE, 0, 1, 1, execute_do);
	debug_console_register_command("step",      CMDFLAG_NONE, 0, 0, 1, execute_step);
	debug_console_register_command("s",         CMDFLAG_NONE, 0, 0, 1, execute_step);
	debug_console_register_command("over",      CMDFLAG_NONE, 0, 0, 1, execute_over);
	debug_console_register_command("o",         CMDFLAG_NONE, 0, 0, 1, execute_over);
	debug_console_register_command("out" ,      CMDFLAG_NONE, 0, 0, 0, execute_out);
	debug_console_register_command("go",        CMDFLAG_NONE, 0, 0, 1, execute_go);
	debug_console_register_command("g",         CMDFLAG_NONE, 0, 0, 1, execute_go);
	debug_console_register_command("gvblank",   CMDFLAG_NONE, 0, 0, 0, execute_go_vblank);
	debug_console_register_command("gv",        CMDFLAG_NONE, 0, 0, 0, execute_go_vblank);
	debug_console_register_command("gint",      CMDFLAG_NONE, 0, 0, 1, execute_go_interrupt);
	debug_console_register_command("gi",        CMDFLAG_NONE, 0, 0, 1, execute_go_interrupt);
	debug_console_register_command("next",      CMDFLAG_NONE, 0, 0, 0, execute_next);
	debug_console_register_command("n",         CMDFLAG_NONE, 0, 0, 0, execute_next);
	debug_console_register_command("focus",     CMDFLAG_NONE, 0, 1, 1, execute_focus);
	debug_console_register_command("ignore",    CMDFLAG_NONE, 0, 0, MAX_COMMAND_PARAMS, execute_ignore);
	debug_console_register_command("observe",   CMDFLAG_NONE, 0, 0, MAX_COMMAND_PARAMS, execute_observe);

	debug_console_register_command("bpset",     CMDFLAG_NONE, 0, 1, 3, execute_bpset);
	debug_console_register_command("bp",        CMDFLAG_NONE, 0, 1, 3, execute_bpset);
	debug_console_register_command("bpclear",   CMDFLAG_NONE, 0, 0, 1, execute_bpclear);
	debug_console_register_command("bpdisable", CMDFLAG_NONE, 0, 0, 1, execute_bpdisenable);
	debug_console_register_command("bpenable",  CMDFLAG_NONE, 1, 0, 1, execute_bpdisenable);
	debug_console_register_command("bplist",    CMDFLAG_NONE, 0, 0, 0, execute_bplist);

	debug_console_register_command("wpset",     CMDFLAG_NONE, ADDRESS_SPACE_PROGRAM, 3, 5, execute_wpset);
	debug_console_register_command("wp",        CMDFLAG_NONE, ADDRESS_SPACE_PROGRAM, 3, 5, execute_wpset);
	debug_console_register_command("wpdset",    CMDFLAG_NONE, ADDRESS_SPACE_DATA, 3, 5, execute_wpset);
	debug_console_register_command("wpd",       CMDFLAG_NONE, ADDRESS_SPACE_DATA, 3, 5, execute_wpset);
	debug_console_register_command("wpiset",    CMDFLAG_NONE, ADDRESS_SPACE_IO, 3, 5, execute_wpset);
	debug_console_register_command("wpi",       CMDFLAG_NONE, ADDRESS_SPACE_IO, 3, 5, execute_wpset);
	debug_console_register_command("wpclear",   CMDFLAG_NONE, 0, 0, 1, execute_wpclear);
	debug_console_register_command("wpdisable", CMDFLAG_NONE, 0, 0, 1, execute_wpdisenable);
	debug_console_register_command("wpenable",  CMDFLAG_NONE, 1, 0, 1, execute_wpdisenable);
	debug_console_register_command("wplist",    CMDFLAG_NONE, 0, 0, 0, execute_wplist);

	debug_console_register_command("save",      CMDFLAG_NONE, ADDRESS_SPACE_PROGRAM, 3, 4, execute_save);
	debug_console_register_command("saved",     CMDFLAG_NONE, ADDRESS_SPACE_DATA, 3, 4, execute_save);
	debug_console_register_command("savei",     CMDFLAG_NONE, ADDRESS_SPACE_IO, 3, 4, execute_save);

	debug_console_register_command("dump",      CMDFLAG_NONE, ADDRESS_SPACE_PROGRAM, 3, 6, execute_dump);
	debug_console_register_command("dumpd",     CMDFLAG_NONE, ADDRESS_SPACE_DATA, 3, 6, execute_dump);
	debug_console_register_command("dumpi",     CMDFLAG_NONE, ADDRESS_SPACE_IO, 3, 6, execute_dump);

	debug_console_register_command("f",         CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_PROGRAM, 3, MAX_COMMAND_PARAMS, execute_find);
	debug_console_register_command("find",      CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_PROGRAM, 3, MAX_COMMAND_PARAMS, execute_find);
	debug_console_register_command("fd",        CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_DATA, 3, MAX_COMMAND_PARAMS, execute_find);
	debug_console_register_command("findd",     CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_DATA, 3, MAX_COMMAND_PARAMS, execute_find);
	debug_console_register_command("fi",        CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_IO, 3, MAX_COMMAND_PARAMS, execute_find);
	debug_console_register_command("findi",     CMDFLAG_KEEP_QUOTES, ADDRESS_SPACE_IO, 3, MAX_COMMAND_PARAMS, execute_find);

	debug_console_register_command("dasm",      CMDFLAG_NONE, 0, 3, 5, execute_dasm);

	debug_console_register_command("trace",     CMDFLAG_NONE, 0, 1, 3, execute_trace);
	debug_console_register_command("traceover", CMDFLAG_NONE, 0, 1, 3, execute_traceover);

	debug_console_register_command("snap",      CMDFLAG_NONE, 0, 0, 1, execute_snap);

	debug_console_register_command("source",    CMDFLAG_NONE, 0, 1, 1, execute_source);

	debug_console_register_command("memdump",	CMDFLAG_NONE, 0, 0, 1, execute_memdump);
}


/*-------------------------------------------------
    debug_command_exit - frees the command
    system
-------------------------------------------------*/

void debug_command_exit(void)
{
}



/*###################################################################################################
**  SIMPLE GLOBAL FUNCTIONS
**#################################################################################################*/

/*-------------------------------------------------
    execute_min - return the minimum of two values
-------------------------------------------------*/

static UINT64 execute_min(UINT32 ref, UINT32 params, UINT64 *param)
{
	return (param[0] < param[1]) ? param[0] : param[1];
}


/*-------------------------------------------------
    execute_max - return the maximum of two values
-------------------------------------------------*/

static UINT64 execute_max(UINT32 ref, UINT32 params, UINT64 *param)
{
	return (param[0] > param[1]) ? param[0] : param[1];
}


/*-------------------------------------------------
    execute_if - if (a) return b; else return c;
-------------------------------------------------*/

static UINT64 execute_if(UINT32 ref, UINT32 params, UINT64 *param)
{
	return param[0] ? param[1] : param[2];
}



/*###################################################################################################
**  PARAMETER VALIDATION HELPERS
**#################################################################################################*/

/*-------------------------------------------------
    validate_parameter_number - validates a
    number parameter
-------------------------------------------------*/

static int validate_parameter_number(const char *param, UINT64 *result)
{
	EXPRERR err = debug_expression_evaluate(param, debug_get_cpu_info(cpu_getactivecpu())->symtable, result);
	if (err == EXPRERR_NONE)
		return 1;
	debug_console_printf("Error in expression: %s\n", param);
	debug_console_printf("                     %*s^", EXPRERR_ERROR_OFFSET(err), "");
	debug_console_printf("%s\n", debug_exprerr_to_string(err));
	return 0;
}


/*-------------------------------------------------
    validate_parameter_expression - validates an
    expression parameter
-------------------------------------------------*/

static int validate_parameter_expression(const char *param, struct parsed_expression **result)
{
	EXPRERR err = debug_expression_parse(param, debug_get_cpu_info(cpu_getactivecpu())->symtable, result);
	if (err == EXPRERR_NONE)
		return 1;
	debug_console_printf("Error in expression: %s\n", param);
	debug_console_printf("                     %*s^", EXPRERR_ERROR_OFFSET(err), "");
	debug_console_printf("%s\n", debug_exprerr_to_string(err));
	return 0;
}


/*-------------------------------------------------
    validate_parameter_command - validates a
    command parameter
-------------------------------------------------*/

static int validate_parameter_command(const char *param)
{
	CMDERR err = debug_console_validate_command(param);
	if (err == CMDERR_NONE)
		return 1;
	debug_console_printf("Error in command: %s\n", param);
	debug_console_printf("                  %*s^", CMDERR_ERROR_OFFSET(err), "");
	debug_console_printf("%s\n", debug_cmderr_to_string(err));
	return 0;
}



/*###################################################################################################
**  COMMAND HANDLERS
**#################################################################################################*/

/*-------------------------------------------------
    execute_help - execute the help command
-------------------------------------------------*/

static void execute_help(int ref, int params, const char *param[])
{
	if (params == 0)
		debug_console_write_line(debug_get_help(""));
	else
		debug_console_write_line(debug_get_help(param[0]));
}


/*-------------------------------------------------
    mini_printf - safe printf to a buffer
-------------------------------------------------*/

static int mini_printf(char *buffer, const char *format, int params, UINT64 *param)
{
	const char *f = format;
	char *p = buffer;

	/* parse the string looking for % signs */
	for (;;)
	{
		char c = *f++;
		if (!c) break;

		/* escape sequences */
		if (c == '\\')
		{
			c = *f++;
			if (!c) break;
			switch (c)
			{
				case '\\':	*p++ = c;		break;
				case 'n':	*p++ = '\n';	break;
				default:					break;
			}
			continue;
		}

		/* formatting */
		else if (c == '%')
		{
			int width = 0;
			int zerofill = 0;

			/* parse out the width */
			for (;;)
			{
				c = *f++;
				if (!c || c < '0' || c > '9') break;
				if (c == '0' && width == 0)
					zerofill = 1;
				width = width * 10 + (c - '0');
			}
			if (!c) break;

			/* get the format */
			switch (c)
			{
				case '%':
					*p++ = c;
					break;

				case 'X':
				case 'x':
					if (params == 0)
					{
						debug_console_write_line("Not enough parameters for format!");
						return 0;
					}
					if ((UINT32)(*param >> 32) != 0)
						p += sprintf(p, zerofill ? "%0*X" : "%*X", (width <= 8) ? 1 : width - 8, (UINT32)(*param >> 32));
					else if (width > 8)
						p += sprintf(p, zerofill ? "%0*X" : "%*X", width - 8, 0);
					p += sprintf(p, zerofill ? "%0*X" : "%*X", (width < 8) ? width : 8, (UINT32)*param);
					param++;
					params--;
					break;

				case 'D':
				case 'd':
					if (params == 0)
					{
						debug_console_write_line("Not enough parameters for format!");
						return 0;
					}
					p += sprintf(p, zerofill ? "%0*d" : "%*d", width, (UINT32)*param);
					param++;
					params--;
					break;
			}
		}

		/* normal stuff */
		else
			*p++ = c;
	}

	/* NULL-terminate and exit */
	*p = 0;
	return 1;
}


/*-------------------------------------------------
    execute_printf - execute the printf command
-------------------------------------------------*/

static void execute_printf(int ref, int params, const char *param[])
{
	UINT64 values[MAX_COMMAND_PARAMS];
	char buffer[1024];
	int i;

	/* validate the other parameters */
	for (i = 1; i < params; i++)
		if (!validate_parameter_number(param[i], &values[i]))
			return;

	/* then do a printf */
	if (mini_printf(buffer, param[0], params - 1, &values[1]))
		debug_console_write_line(buffer);
}


/*-------------------------------------------------
    execute_logerror - execute the logerror command
-------------------------------------------------*/

static void execute_logerror(int ref, int params, const char *param[])
{
	UINT64 values[MAX_COMMAND_PARAMS];
	char buffer[1024];
	int i;

	/* validate the other parameters */
	for (i = 1; i < params; i++)
		if (!validate_parameter_number(param[i], &values[i]))
			return;

	/* then do a printf */
	if (mini_printf(buffer, param[0], params - 1, &values[1]))
		logerror("%s", buffer);
}


/*-------------------------------------------------
    execute_tracelog - execute the tracelog command
-------------------------------------------------*/

static void execute_tracelog(int ref, int params, const char *param[])
{
	FILE *file = debug_get_cpu_info(cpu_getactivecpu())->trace.file;
	UINT64 values[MAX_COMMAND_PARAMS];
	char buffer[1024];
	int i;

	/* if no tracefile, skip */
	if (!file)
		return;

	/* validate the other parameters */
	for (i = 1; i < params; i++)
		if (!validate_parameter_number(param[i], &values[i]))
			return;

	/* then do a printf */
	if (mini_printf(buffer, param[0], params - 1, &values[1]))
		fprintf(file, "%s", buffer);
}


/*-------------------------------------------------
    execute_quit - execute the quit command
-------------------------------------------------*/

static void execute_quit(int ref, int params, const char *param[])
{
	int cpunum;

	/* turn off all traces */
	for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		debug_cpu_trace(cpunum, NULL, 0, NULL);

	osd_die("Exited via the debugger\n");
}


/*-------------------------------------------------
    execute_do - execute the do command
-------------------------------------------------*/

static void execute_do(int ref, int params, const char *param[])
{
	UINT64 dummy;
	validate_parameter_number(param[0], &dummy);
}


/*-------------------------------------------------
    execute_step - execute the step command
-------------------------------------------------*/

static void execute_step(int ref, int params, const char *param[])
{
	UINT64 steps = 1;

	/* if we have a parameter, use it instead */
	if (params > 0 && !validate_parameter_number(param[0], &steps))
		return;

	debug_cpu_single_step(steps);
}


/*-------------------------------------------------
    execute_over - execute the over command
-------------------------------------------------*/

static void execute_over(int ref, int params, const char *param[])
{
	UINT64 steps = 1;

	/* if we have a parameter, use it instead */
	if (params > 0 && !validate_parameter_number(param[0], &steps))
		return;

	debug_cpu_single_step_over(steps);
}


/*-------------------------------------------------
    execute_out - execute the out command
-------------------------------------------------*/

static void execute_out(int ref, int params, const char *param[])
{
	debug_cpu_single_step_out();
}


/*-------------------------------------------------
    execute_go - execute the go command
-------------------------------------------------*/

static void execute_go(int ref, int params, const char *param[])
{
	UINT64 addr = ~0;

	/* if we have a parameter, use it instead */
	if (params > 0 && !validate_parameter_number(param[0], &addr))
		return;

	debug_cpu_go(addr);
}


/*-------------------------------------------------
    execute_go_vblank - execute the govblank
    command
-------------------------------------------------*/

static void execute_go_vblank(int ref, int params, const char *param[])
{
	debug_cpu_go_vblank();
}


/*-------------------------------------------------
    execute_go_interrupt - execute the goint command
-------------------------------------------------*/

static void execute_go_interrupt(int ref, int params, const char *param[])
{
	UINT64 irqline = -1;

	/* if we have a parameter, use it instead */
	if (params > 0 && !validate_parameter_number(param[0], &irqline))
		return;

	debug_cpu_go_interrupt(irqline);
}


/*-------------------------------------------------
    execute_next - execute the next command
-------------------------------------------------*/

static void execute_next(int ref, int params, const char *param[])
{
	debug_cpu_next_cpu();
}


/*-------------------------------------------------
    execute_focus - execute the focus command
-------------------------------------------------*/

static void execute_focus(int ref, int params, const char *param[])
{
	UINT64 cpuwhich;
	int cpunum;

	/* validate params */
	if (!validate_parameter_number(param[0], &cpuwhich))
		return;
	if (cpuwhich >= cpu_gettotalcpu())
	{
		debug_console_printf("Invalid CPU number!");
		return;
	}

	/* first clear the ignore flag on the focused CPU */
	debug_cpu_ignore_cpu(cpuwhich, 0);

	/* then loop over CPUs and set the ignore flags on all other CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const struct debug_cpu_info *info = debug_get_cpu_info(cpunum);
		if (info && info->valid && cpunum != cpuwhich)
			debug_cpu_ignore_cpu(cpunum, 1);
	}
	debug_console_printf("Now focused on CPU %d", (int)cpuwhich);
}


/*-------------------------------------------------
    execute_ignore - execute the ignore command
-------------------------------------------------*/

static void execute_ignore(int ref, int params, const char *param[])
{
	UINT64 cpuwhich[MAX_COMMAND_PARAMS];
	int cpunum, paramnum;
	char buffer[100];
	int buflen = 0;

	/* if there are no parameters, dump the ignore list */
	if (params == 0)
	{
		/* loop over all CPUs */
		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *info = debug_get_cpu_info(cpunum);

			/* build up a comma-separated list */
			if (info && info->valid && info->ignoring)
			{
				if (buflen == 0) buflen += sprintf(&buffer[buflen], "Currently ignoring CPU %d", cpunum);
				else buflen += sprintf(&buffer[buflen], ",%d", cpunum);
			}
		}

		/* special message for none */
		if (buflen == 0)
			sprintf(&buffer[buflen], "Not currently ignoring any CPUs");
		debug_console_write_line(buffer);
	}

	/* otherwise clear the ignore flag on all requested CPUs */
	else
	{
		/* validate parameters */
		for (paramnum = 0; paramnum < params; paramnum++)
		{
			if (!validate_parameter_number(param[paramnum], &cpuwhich[paramnum]))
				return;
			if (cpuwhich[paramnum] >= cpu_gettotalcpu())
			{
				debug_console_printf("Invalid CPU number! (%d)", (int)cpuwhich[paramnum]);
				return;
			}
		}

		/* set the ignore flags */
		for (paramnum = 0; paramnum < params; paramnum++)
		{
			/* make sure this isn't the last live CPU */
			for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
			{
				const struct debug_cpu_info *info = debug_get_cpu_info(cpunum);
				if (cpunum != cpuwhich[paramnum] && info && info->valid && !info->ignoring)
					break;
			}
			if (cpunum == MAX_CPU)
			{
				debug_console_printf("Can't ignore all CPUs!\n");
				return;
			}

			debug_cpu_ignore_cpu(cpuwhich[paramnum], 1);
			debug_console_printf("Now ignoring CPU %d", (int)cpuwhich[paramnum]);
		}
	}
}


/*-------------------------------------------------
    execute_observe - execute the observe command
-------------------------------------------------*/

static void execute_observe(int ref, int params, const char *param[])
{
	UINT64 cpuwhich[MAX_COMMAND_PARAMS];
	int cpunum, paramnum;
	char buffer[100];
	int buflen = 0;

	/* if there are no parameters, dump the ignore list */
	if (params == 0)
	{
		/* loop over all CPUs */
		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *info = debug_get_cpu_info(cpunum);

			/* build up a comma-separated list */
			if (info && info->valid && !info->ignoring)
			{
				if (buflen == 0) buflen += sprintf(&buffer[buflen], "Currently observing CPU %d", cpunum);
				else buflen += sprintf(&buffer[buflen], ",%d", cpunum);
			}
		}

		/* special message for none */
		if (buflen == 0)
			buflen += sprintf(&buffer[buflen], "Not currently observing any CPUs");
		debug_console_write_line(buffer);
	}

	/* otherwise set the ignore flag on all requested CPUs */
	else
	{
		/* validate parameters */
		for (paramnum = 0; paramnum < params; paramnum++)
		{
			if (!validate_parameter_number(param[paramnum], &cpuwhich[paramnum]))
				return;
			if (cpuwhich[paramnum] >= cpu_gettotalcpu())
			{
				debug_console_printf("Invalid CPU number! (%d)", (int)cpuwhich[paramnum]);
				return;
			}
		}

		/* clear the ignore flags */
		for (paramnum = 0; paramnum < params; paramnum++)
		{
			debug_cpu_ignore_cpu(cpuwhich[paramnum], 0);
			debug_console_printf("Now observing CPU %d", (int)cpuwhich[paramnum]);
		}
	}
}


/*-------------------------------------------------
    execute_bpset - execute the breakpoint set
    command
-------------------------------------------------*/

static void execute_bpset(int ref, int params, const char *param[])
{
	struct parsed_expression *condition = NULL;
	const char *action = NULL;
	UINT64 address;
	int bpnum;

	/* param 1 is the address */
	if (!validate_parameter_number(param[0], &address))
		return;

	/* param 2 is the condition */
	if (params > 1 && !validate_parameter_expression(param[1], &condition))
		return;

	/* param 3 is the action */
	if (params > 2 && !validate_parameter_command(action = param[2]))
		return;

	/* set the breakpoint */
	bpnum = debug_breakpoint_set(cpu_getactivecpu(), address, condition, action);
	debug_console_printf("Breakpoint %d set\n", bpnum);
}


/*-------------------------------------------------
    execute_bpclear - execute the breakpoint
    clear command
-------------------------------------------------*/

static void execute_bpclear(int ref, int params, const char *param[])
{
	UINT64 bpindex;

	/* if 0 parameters, clear all */
	if (params == 0)
	{
		int cpunum;

		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
			if (cpuinfo->valid)
			{
				struct breakpoint *bp;
				while ((bp = cpuinfo->first_bp) != NULL)
					debug_breakpoint_clear(bp->index);
			}
		}
		debug_console_write_line("Cleared all breakpoints");
	}

	/* otherwise, clear the specific one */
	else if (!validate_parameter_number(param[0], &bpindex))
		return;
	else
	{
		int found = debug_breakpoint_clear(bpindex);
		if (found)
			debug_console_printf("Breakpoint %X cleared", (UINT32)bpindex);
		else
			debug_console_printf("Invalid breakpoint number %X", (UINT32)bpindex);
	}
}


/*-------------------------------------------------
    execute_bpdisenable - execute the breakpoint
    disable/enable commands
-------------------------------------------------*/

static void execute_bpdisenable(int ref, int params, const char *param[])
{
	UINT64 bpindex;

	/* if 0 parameters, clear all */
	if (params == 0)
	{
		int cpunum;

		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
			if (cpuinfo->valid)
			{
				struct breakpoint *bp;
				for (bp = cpuinfo->first_bp; bp; bp = bp->next)
					debug_breakpoint_enable(bp->index, ref);
			}
		}
		if (ref == 0)
			debug_console_write_line("Disabled all breakpoints");
		else
			debug_console_write_line("Enabled all breakpoints");
	}

	/* otherwise, clear the specific one */
	else if (!validate_parameter_number(param[0], &bpindex))
		return;
	else
	{
		int found = debug_breakpoint_enable(bpindex, ref);
		if (found)
			debug_console_printf("Breakpoint %X %s", (UINT32)bpindex, ref ? "enabled" : "disabled");
		else
			debug_console_printf("Invalid breakpoint number %X", (UINT32)bpindex);
	}
}


/*-------------------------------------------------
    execute_bplist - execute the breakpoint list
    command
-------------------------------------------------*/

static void execute_bplist(int ref, int params, const char *param[])
{
	int cpunum, printed = 0;
	char buffer[256];

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);

		if (cpuinfo->valid && cpuinfo->first_bp)
		{
			struct breakpoint *bp;

			debug_console_printf("CPU %d breakpoints:", cpunum);

			/* loop over the breakpoints */
			for (bp = cpuinfo->first_bp; bp; bp = bp->next)
			{
				int buflen;
				buflen = sprintf(buffer, "%c%4X @ %08X", bp->enabled ? ' ' : 'D', bp->index, bp->address);
				if (bp->condition)
					buflen += sprintf(&buffer[buflen], " if %s", debug_expression_original_string(bp->condition));
				if (bp->action)
					buflen += sprintf(&buffer[buflen], " do %s", bp->action);
				debug_console_write_line(buffer);
				printed++;
			}
		}
	}

	if (!printed)
		debug_console_write_line("No breakpoints currently installed");
}


/*-------------------------------------------------
    execute_wpset - execute the watchpoint set
    command
-------------------------------------------------*/

static void execute_wpset(int ref, int params, const char *param[])
{
	struct parsed_expression *condition = NULL;
	const char *action = NULL;
	UINT64 address, length;
	int type = 0;
	int wpnum;

	/* param 1 is the address */
	if (!validate_parameter_number(param[0], &address))
		return;

	/* param 2 is the length */
	if (!validate_parameter_number(param[1], &length))
		return;

	/* param 3 is the type */
	if (!strcmp(param[2], "r"))
		type = WATCHPOINT_READ;
	else if (!strcmp(param[2], "w"))
		type = WATCHPOINT_WRITE;
	else if (!strcmp(param[2], "rw") || !strcmp(param[2], "wr"))
		type = WATCHPOINT_READWRITE;
	else
	{
		debug_console_printf("Invalid watchpoint type: expected r, w, or rw\n");
		return;
	}

	/* param 4 is the condition */
	if (params > 3 && !validate_parameter_expression(param[3], &condition))
		return;

	/* param 5 is the action */
	if (params > 4 && !validate_parameter_command(action = param[4]))
		return;

	/* set the watchpoint */
	wpnum = debug_watchpoint_set(cpu_getactivecpu(), ref, type, address, length, condition, action);
	debug_console_printf("Watchpoint %d set\n", wpnum);
}


/*-------------------------------------------------
    execute_wpclear - execute the watchpoint
    clear command
-------------------------------------------------*/

static void execute_wpclear(int ref, int params, const char *param[])
{
	UINT64 wpindex;

	/* if 0 parameters, clear all */
	if (params == 0)
	{
		int cpunum;

		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
			if (cpuinfo->valid)
			{
				int spacenum;

				for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
				{
					struct watchpoint *wp;
					while ((wp = cpuinfo->space[spacenum].first_wp) != NULL)
						debug_watchpoint_clear(wp->index);
				}
			}
		}
		debug_console_write_line("Cleared all watchpoints");
	}

	/* otherwise, clear the specific one */
	else if (!validate_parameter_number(param[0], &wpindex))
		return;
	else
	{
		int found = debug_watchpoint_clear(wpindex);
		if (found)
			debug_console_printf("Watchpoint %X cleared", (UINT32)wpindex);
		else
			debug_console_printf("Invalid watchpoint number %X", (UINT32)wpindex);
	}
}


/*-------------------------------------------------
    execute_wpdisenable - execute the watchpoint
    disable/enable commands
-------------------------------------------------*/

static void execute_wpdisenable(int ref, int params, const char *param[])
{
	UINT64 wpindex;

	/* if 0 parameters, clear all */
	if (params == 0)
	{
		int cpunum;

		for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		{
			const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);
			if (cpuinfo->valid)
			{
				int spacenum;

				for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
				{
					struct watchpoint *wp;
					for (wp = cpuinfo->space[spacenum].first_wp; wp; wp = wp->next)
						debug_watchpoint_enable(wp->index, ref);
				}
			}
		}
		if (ref == 0)
			debug_console_write_line("Disabled all watchpoints");
		else
			debug_console_write_line("Enabled all watchpoints");
	}

	/* otherwise, clear the specific one */
	else if (!validate_parameter_number(param[0], &wpindex))
		return;
	else
	{
		int found = debug_watchpoint_enable(wpindex, ref);
		if (found)
			debug_console_printf("Watchpoint %X %s", (UINT32)wpindex, ref ? "enabled" : "disabled");
		else
			debug_console_printf("Invalid watchpoint number %X", (UINT32)wpindex);
	}
}


/*-------------------------------------------------
    execute_wplist - execute the watchpoint list
    command
-------------------------------------------------*/

static void execute_wplist(int ref, int params, const char *param[])
{
	static const char *spacenames[ADDRESS_SPACES] = { "program", "data", "I/O" };
	int cpunum, printed = 0;
	char buffer[256];

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
	{
		const struct debug_cpu_info *cpuinfo = debug_get_cpu_info(cpunum);

		if (cpuinfo->valid)
		{
			int spacenum;

			for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			{
				if (cpuinfo->space[spacenum].first_wp)
				{
					static const char *types[] = { "unkn ", "read ", "write", "r/w  " };
					struct watchpoint *wp;

					debug_console_printf("CPU %d %s space watchpoints:", cpunum, spacenames[spacenum]);

					/* loop over the watchpoints */
					for (wp = cpuinfo->space[spacenum].first_wp; wp; wp = wp->next)
					{
						int buflen;
						buflen = sprintf(buffer, "%c%4X @ %08X-%08X %s", wp->enabled ? ' ' : 'D', wp->index, wp->address, wp->address + wp->length - 1, types[wp->type & 3]);
						if (wp->condition)
							buflen += sprintf(&buffer[buflen], " if %s", debug_expression_original_string(wp->condition));
						if (wp->action)
							buflen += sprintf(&buffer[buflen], " do %s", wp->action);
						debug_console_write_line(buffer);
						printed++;
					}
				}
			}
		}
	}

	if (!printed)
		debug_console_write_line("No watchpoints currently installed");
}


/*-------------------------------------------------
    execute_save - execute the save command
-------------------------------------------------*/

static void execute_save(int ref, int params, const char *param[])
{
	UINT64 offset, endoffset, length, cpunum = cpu_getactivecpu();
	const struct debug_cpu_info *info;
	int spacenum = ref;
	FILE *f;
	UINT64 i;

	/* validate parameters */
	if (!validate_parameter_number(param[1], &offset))
		return;
	if (!validate_parameter_number(param[2], &length))
		return;
	if (params > 3 && !validate_parameter_number(param[3], &cpunum))
		return;

	/* determine the addresses to write */
	info = debug_get_cpu_info(cpunum);
	offset = ADDR2BYTE(offset, info, spacenum);
	endoffset = ADDR2BYTE(offset + length - 1, info, spacenum);

	/* open the file */
	f = fopen(param[0], "wb");
	if (!f)
	{
		debug_console_printf("Error opening file '%s'", param[0]);
		return;
	}

	/* now write the data out */
	cpuintrf_push_context(cpunum);
	for (i = offset; i <= endoffset; i++)
	{
		UINT8 byte = debug_read_byte(spacenum, i);
		fwrite(&byte, 1, 1, f);
	}
	cpuintrf_pop_context();

	/* close the file */
	fclose(f);
	debug_console_write_line("Data saved successfully");
}


/*-------------------------------------------------
    execute_dump - execute the dump command
-------------------------------------------------*/

static void execute_dump(int ref, int params, const char *param[])
{
	UINT64 offset, endoffset, length, width = 0, ascii = 1, cpunum = cpu_getactivecpu();
	const struct debug_cpu_info *info;
	int spacenum = ref;
	FILE *f = NULL;
	UINT64 i, j;

	/* validate parameters */
	if (!validate_parameter_number(param[1], &offset))
		return;
	if (!validate_parameter_number(param[2], &length))
		return;
	if (params > 3 && !validate_parameter_number(param[3], &width))
		return;
	if (params > 4 && !validate_parameter_number(param[4], &ascii))
		return;
	if (params > 5 && !validate_parameter_number(param[5], &cpunum))
		return;

	/* further validation */
	if (cpunum >= cpu_gettotalcpu())
	{
		debug_console_printf("Invalid CPU number!");
		return;
	}
	info = debug_get_cpu_info(cpunum);
	if (width == 0)
		width = info->space[spacenum].databytes;
	if (width < ADDR2BYTE(1, info, spacenum))
		width = ADDR2BYTE(1, info, spacenum);
	if (width != 1 && width != 2 && width != 4 && width != 8)
	{
		debug_console_printf("Invalid width! (must be 1,2,4 or 8)");
		return;
	}
	offset = ADDR2BYTE(offset, info, spacenum);
	endoffset = ADDR2BYTE(offset + length - 1, info, spacenum);

	/* open the file */
	f = fopen(param[0], "w");
	if (!f)
	{
		debug_console_printf("Error opening file '%s'", param[0]);
		return;
	}

	/* now write the data out */
	cpuintrf_push_context(cpunum);
	for (i = offset; i <= endoffset; i += 16)
	{
		char output[200];
		int outdex = 0;

		/* print the address */
		outdex += sprintf(&output[outdex], "%0*X: ", info->space[spacenum].addrchars, (UINT32)BYTE2ADDR(i, info, spacenum));

		/* print the bytes */
		switch (width)
		{
			case 1:
				for (j = 0; j < 16; j++)
				{
					UINT8 byte = debug_read_byte(ref, i + j);
					if (i + j <= endoffset)
						outdex += sprintf(&output[outdex], " %02X", byte);
					else
						outdex += sprintf(&output[outdex], "   ");
				}
				break;

			case 2:
				for (j = 0; j < 16; j += 2)
				{
					UINT16 word = debug_read_word(ref, i + j);
					if (i + j <= endoffset)
						outdex += sprintf(&output[outdex], " %04X", word);
					else
						outdex += sprintf(&output[outdex], "     ");
				}
				break;

			case 4:
				for (j = 0; j < 16; j += 4)
				{
					UINT32 dword = debug_read_dword(ref, i + j);
					if (i + j <= endoffset)
						outdex += sprintf(&output[outdex], " %08X", dword);
					else
						outdex += sprintf(&output[outdex], "         ");
				}
				break;

			case 8:
				for (j = 0; j < 16; j += 8)
				{
					UINT64 qword = debug_read_qword(ref, i + j);
					if (i + j <= endoffset)
						outdex += sprintf(&output[outdex], " %08X%08X", (UINT32)(qword >> 32), (UINT32)qword);
					else
						outdex += sprintf(&output[outdex], "                 ");
				}
				break;
		}

		/* print the ASCII */
		if (ascii)
		{
			outdex += sprintf(&output[outdex], "  ");
			for (j = 0; j < 16 && (i + j) <= endoffset; j++)
			{
				UINT8 byte = debug_read_byte(ref, i + j);
				outdex += sprintf(&output[outdex], "%c", (byte >= 32 && byte < 128) ? byte : '.');
			}
		}

		/* output the result */
		fprintf(f, "%s\n", output);
	}
	cpuintrf_pop_context();

	/* close the file */
	fclose(f);
	debug_console_write_line("Data dumped successfully");
}


/*-------------------------------------------------
    execute_find - execute the find command
-------------------------------------------------*/

static void execute_find(int ref, int params, const char *param[])
{
	UINT64 offset, endoffset, length, cpunum = cpu_getactivecpu();
	const struct debug_cpu_info *info;
	UINT64 data_to_find[256];
	UINT8 data_size[256];
	int cur_data_size;
	int data_count = 0;
	int spacenum = ref;
	int found = 0;
	UINT64 i, j;

	/* validate parameters */
	if (!validate_parameter_number(param[0], &offset))
		return;
	if (!validate_parameter_number(param[1], &length))
		return;

	/* further validation */
	if (cpunum >= cpu_gettotalcpu())
	{
		debug_console_printf("Invalid CPU number!");
		return;
	}
	info = debug_get_cpu_info(cpunum);
	offset = ADDR2BYTE(offset, info, spacenum);
	endoffset = ADDR2BYTE(offset + length - 1, info, spacenum);
	cur_data_size = ADDR2BYTE(1, info, spacenum);
	if (cur_data_size == 0)
		cur_data_size = 1;

	/* parse the data parameters */
	for (i = 2; i < params; i++)
	{
		const char *pdata = param[i];

		/* check for a string */
		if (pdata[0] == '"' && pdata[strlen(pdata) - 1] == '"')
		{
			for (j = 1; j < strlen(pdata) - 1; j++)
			{
				data_to_find[data_count] = pdata[j];
				data_size[data_count++] = 1;
			}
		}

		/* otherwise, validate as a number */
		else
		{
			/* check for a 'b','w','d',or 'q' prefix */
			data_size[data_count] = cur_data_size;
			if (tolower(pdata[0]) == 'b' && pdata[1] == '.') { data_size[data_count] = cur_data_size = 1; pdata += 2; }
			if (tolower(pdata[0]) == 'w' && pdata[1] == '.') { data_size[data_count] = cur_data_size = 2; pdata += 2; }
			if (tolower(pdata[0]) == 'd' && pdata[1] == '.') { data_size[data_count] = cur_data_size = 4; pdata += 2; }
			if (tolower(pdata[0]) == 'q' && pdata[1] == '.') { data_size[data_count] = cur_data_size = 8; pdata += 2; }

			/* look for a wildcard */
			if (!strcmp(pdata, "?"))
				data_size[data_count++] |= 0x10;

			/* otherwise, validate as a number */
			else if (!validate_parameter_number(pdata, &data_to_find[data_count++]))
				return;
		}
	}

	/* now search */
	cpuintrf_push_context(cpunum);
	for (i = offset; i <= endoffset; i += data_size[0])
	{
		int suboffset = 0;
		int match = 1;

		/* find the entire string */
		for (j = 0; j < data_count && match; j++)
		{
			switch (data_size[j])
			{
				case 1:	match = ((UINT8)debug_read_byte(spacenum, i + suboffset) == (UINT8)data_to_find[j]);	break;
				case 2:	match = ((UINT16)debug_read_word(spacenum, i + suboffset) == (UINT16)data_to_find[j]);	break;
				case 4:	match = ((UINT32)debug_read_dword(spacenum, i + suboffset) == (UINT32)data_to_find[j]);	break;
				case 8:	match = ((UINT64)debug_read_qword(spacenum, i + suboffset) == (UINT64)data_to_find[j]);	break;
				default:	/* all other cases are wildcards */		break;
			}
			suboffset += data_size[j] & 0x0f;
		}

		/* did we find it? */
		if (match)
		{
			found++;
			debug_console_printf("Found at %08X\n", (UINT32)BYTE2ADDR(i, info, spacenum));
		}
	}
	cpuintrf_pop_context();

	/* print something if not found */
	if (found == 0)
		debug_console_printf("Not found\n");
}


/*-------------------------------------------------
    execute_dasm - execute the dasm command
-------------------------------------------------*/

static void execute_dasm(int ref, int params, const char *param[])
{
	UINT64 offset, length, bytes = 1, cpunum = cpu_getactivecpu();
	const struct debug_cpu_info *info;
	int minbytes, maxbytes, byteswidth;
	FILE *f = NULL;
	int i, j;

	/* validate parameters */
	if (!validate_parameter_number(param[1], &offset))
		return;
	if (!validate_parameter_number(param[2], &length))
		return;
	if (params > 3 && !validate_parameter_number(param[3], &bytes))
		return;
	if (params > 4 && !validate_parameter_number(param[4], &cpunum))
		return;

	/* further validation */
	if (cpunum >= cpu_gettotalcpu())
	{
		debug_console_write_line("Invalid CPU number!");
		return;
	}
	info = debug_get_cpu_info(cpunum);

	/* determine the width of the bytes */
	minbytes = cpunum_min_instruction_bytes(cpunum);
	maxbytes = cpunum_max_instruction_bytes(cpunum);
	byteswidth = 0;
	if (bytes)
	{
		byteswidth = (maxbytes + (minbytes - 1)) / minbytes;
		byteswidth *= (2 * minbytes) + 1;
	}

	/* open the file */
	f = fopen(param[0], "w");
	if (!f)
	{
		debug_console_printf("Error opening file '%s'", param[0]);
		return;
	}

	/* now write the data out */
	cpuintrf_push_context(cpunum);
	for (i = 0; i < length; )
	{
		int pcbyte = ADDR2BYTE(offset + i, info, ADDRESS_SPACE_PROGRAM);
		char output[200], disasm[200];
		UINT64 dummyreadop;
		int outdex = 0;
		int numbytes;

		/* print the address */
		outdex += sprintf(&output[outdex], "%0*X: ", info->space[ADDRESS_SPACE_PROGRAM].addrchars, (UINT32)BYTE2ADDR(pcbyte, info, ADDRESS_SPACE_PROGRAM));

		/* get the disassembly up front, but only if mapped */
		if (memory_get_op_ptr(cpunum, pcbyte) != NULL || (info->readop && (*info->readop)(pcbyte, 1, &dummyreadop)))
		{
			memory_set_opbase(pcbyte);
			i += numbytes = activecpu_dasm(disasm, offset + i) & DASMFLAG_LENGTHMASK;
		}
		else
		{
			sprintf(disasm, "<unmapped>");
			i += numbytes = 1;
		}

		/* print the bytes */
		if (bytes)
		{
			int startdex = outdex;
			numbytes = ADDR2BYTE(numbytes, info, ADDRESS_SPACE_PROGRAM);
			switch (minbytes)
			{
				case 1:
					for (j = 0; j < numbytes; j++)
						outdex += sprintf(&output[outdex], "%02X ", (UINT32)debug_read_opcode(pcbyte + j, 1));
					break;

				case 2:
					for (j = 0; j < numbytes; j += 2)
						outdex += sprintf(&output[outdex], "%04X ", (UINT32)debug_read_opcode(pcbyte + j, 2));
					break;

				case 4:
					for (j = 0; j < numbytes; j += 4)
						outdex += sprintf(&output[outdex], "%08X ", (UINT32)debug_read_opcode(pcbyte + j, 4));
					break;

				case 8:
					for (j = 0; j < numbytes; j += 8)
					{
						UINT64 val = debug_read_opcode(pcbyte + j, 8);
						outdex += sprintf(&output[outdex], "%08X%08X ", (UINT32)(val >> 32), (UINT32)val);
					}
					break;
			}
			if (outdex - startdex < byteswidth)
				outdex += sprintf(&output[outdex], "%*s", byteswidth - (outdex - startdex), "");
			outdex += sprintf(&output[outdex], "  ");
		}

		/* add the disassembly */
		sprintf(&output[outdex], "%s", disasm);

		/* output the result */
		fprintf(f, "%s\n", output);
	}
	cpuintrf_pop_context();

	/* close the file */
	fclose(f);
	debug_console_write_line("Data dumped successfully");
}


/*-------------------------------------------------
    execute_trace_internal - functionality for
    trace over and trace info
-------------------------------------------------*/

static void execute_trace_internal(int ref, int params, const char *param[], int trace_over)
{
	const char *action = NULL, *filename = param[0];
	FILE *f = NULL;
	const char *mode;
	UINT64 cpunum;

	cpunum = cpu_getactivecpu();

	/* validate parameters */
	if (params > 1 && !validate_parameter_number(param[1], &cpunum))
		return;
	if (params > 2 && !validate_parameter_command(action = param[2]))
		return;

	/* further validation */
	if (!stricmp(filename, "off"))
		filename = NULL;
	if (cpunum >= cpu_gettotalcpu())
	{
		debug_console_printf("Invalid CPU number!");
		return;
	}

	/* open the file */
	if (filename)
	{
		mode = "w";

		/* opening for append? */
		if ((filename[0] == '>') && (filename[1] == '>'))
		{
			mode = "a";
			filename += 2;
		}

		f = fopen(filename, mode);
		if (!f)
		{
			debug_console_printf("Error opening file '%s'", param[0]);
			return;
		}
	}

	/* do it */
	debug_cpu_trace(cpunum, f, trace_over, action);
	if (f)
		debug_console_printf("Tracing CPU %d to file %s\n", (int)cpunum, filename);
	else
		debug_console_printf("Stopped tracing on CPU %d\n", (int)cpunum);
}


/*-------------------------------------------------
    execute_trace - execute the trace command
-------------------------------------------------*/

static void execute_trace(int ref, int params, const char *param[])
{
	execute_trace_internal(ref, params, param, 0);
}


/*-------------------------------------------------
    execute_traceover - execute the trace over command
-------------------------------------------------*/

static void execute_traceover(int ref, int params, const char *param[])
{
	execute_trace_internal(ref, params, param, 1);
}


/*-------------------------------------------------
    execute_snap - execute the trace over command
-------------------------------------------------*/

static void execute_snap(int ref, int params, const char *param[])
{
	/* if no params, use the default behavior */
	if (params == 0)
	{
		save_screen_snapshot(artwork_get_ui_bitmap());
		debug_console_printf("Saved snapshot\n");
	}

	/* otherwise, we have to open the file ourselves */
	else
	{
		mame_file *fp = mame_fopen(Machine->gamedrv->name, param[0], FILETYPE_SCREENSHOT, 1);
		if (!fp)
			debug_console_printf("Error creating file '%s'\n", param[0]);
		else
		{
			save_screen_snapshot_as(fp, artwork_get_ui_bitmap());
			mame_fclose(fp);
			debug_console_printf("Saved snapshot as '%s'\n", param[0]);
		}
	}
}


/*-------------------------------------------------
    execute_source - execute the source command
-------------------------------------------------*/

static void execute_source(int ref, int params, const char *param[])
{
	debug_source_script(param[0]);
}


/*-------------------------------------------------
    execute_memdump - execute the memdump command
-------------------------------------------------*/

static void execute_memdump(int ref, int params, const char **param)
{
	FILE *file;
	const char *filename;

	filename = (params == 0) ? "memdump.log" : param[0];

	debug_console_printf("Dumping memory to %s\n", filename);

	file = fopen(filename, "w");
	if (file)
	{
		memory_dump(file);
		fclose(file);
	}
}

