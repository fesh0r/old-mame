/*###################################################################################################
**
**
**      debugcpu.h
**      Debugger CPU/memory interface engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef __DEBUGCPU_H__
#define __DEBUGCPU_H__

#include "debugexp.h"


/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

#define TRACE_LOOPS				64

#define WATCHPOINT_READ			1
#define WATCHPOINT_WRITE		2
#define WATCHPOINT_READWRITE	(WATCHPOINT_READ | WATCHPOINT_WRITE)

enum
{
	EXECUTION_STATE_STOPPED,
	EXECUTION_STATE_RUNNING,
	EXECUTION_STATE_NEXT_CPU,
	EXECUTION_STATE_STEP_INTO,
	EXECUTION_STATE_STEP_OVER,
	EXECUTION_STATE_STEP_OUT
};



/*###################################################################################################
**  MACROS
**#################################################################################################*/

#define ADDR2BYTE(val,info,spc) ((((val) << (info)->space[spc].addr2byte_lshift) >> (info)->space[spc].addr2byte_rshift) & address_space[spc].addrmask)
#define BYTE2ADDR(val,info,spc) ((((val) & address_space[spc].addrmask) << (info)->space[spc].addr2byte_rshift) >> (info)->space[spc].addr2byte_lshift)



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/

struct debug_trace_info
{
	FILE *			file;						/* tracing file for this CPU */
	char *			action;						/* action to perform during a trace */
	offs_t			history[TRACE_LOOPS];		/* history of recent PCs */
	int				loops;						/* number of instructions in a loop */
	int				nextdex;					/* next index */
	offs_t			trace_over_target;			/* target for tracing over
                                                    (0 = not tracing over,
                                                    ~0 = not currently tracing over) */
};


struct debug_space_info
{
	UINT8			databytes;					/* width of the data bus, in bytes */
	UINT8			addr2byte_lshift;			/* left shift to convert CPU address to a byte value */
	UINT8			addr2byte_rshift;			/* right shift to convert CPU address to a byte value */
	UINT8			addrchars;					/* number of characters to use for addresses */
	offs_t			addrmask;					/* address mask */
	struct watchpoint *first_wp;				/* first watchpoint */
};


struct debug_cpu_info
{
	UINT8			valid;						/* are we valid? */
	UINT8			endianness;					/* little or bigendian */
	UINT8			opwidth;					/* width of an opcode */
	UINT8			ignoring;					/* are we ignoring this CPU's execution? */
	offs_t			temp_breakpoint_pc;			/* temporary breakpoint PC */
	int				total_watchpoints;			/* total watchpoints on this CPU */
	struct symbol_table *symtable;				/* symbol table for expression evaluation */
	struct debug_trace_info trace;				/* trace info */
	struct breakpoint *first_bp;				/* first breakpoint */
	struct debug_space_info space[ADDRESS_SPACES];/* per-address space info */
	int 			(*read)(int space, UINT32 offset, int size, UINT64 *value); /* memory read routine */
	int				(*write)(int space, UINT32 offset, int size, UINT64 value); /* memory write routine */
	int				(*readop)(UINT32 offset, int size, UINT64 *value);	/* opcode read routine */
};


struct breakpoint
{
	int				index;						/* user reported index */
	UINT8			enabled;					/* enabled? */
	offs_t			address;					/* execution address */
	struct parsed_expression *condition;		/* condition */
	char *			action;						/* action */
	struct breakpoint *next;					/* next in the list */
};


struct watchpoint
{
	int				index;						/* user reported index */
	UINT8			enabled;					/* enabled? */
	UINT8			type;						/* type (read/write) */
	offs_t			address;					/* start address */
	offs_t			length;						/* length of watch area */
	struct parsed_expression *condition;		/* condition */
	char *			action;						/* action */
	struct watchpoint *next;					/* next in the list */
};



/*###################################################################################################
**  LOCAL VARIABLES
**#################################################################################################*/

extern FILE *debug_source_file;



/*###################################################################################################
**  FUNCTION PROTOTYPES
**#################################################################################################*/

/* initialization */
void				debug_cpu_init(void);
void				debug_cpu_exit(void);

/* utilities */
const struct debug_cpu_info *debug_get_cpu_info(int cpunum);
void				debug_halt_on_next_instruction(void);
void				debug_refresh_display(void);
int					debug_get_execution_state(void);
UINT32				debug_get_execution_counter(void);
void				debug_trace_printf(int cpunum, const char *fmt, ...);
void				debug_source_script(const char *file);

/* CPU execution hooks */
void				debug_vblank_hook(void);
void				debug_interrupt_hook(int cpunum, int irqline);

/* execution control */
void				debug_cpu_single_step(int numsteps);
void				debug_cpu_single_step_over(int numsteps);
void				debug_cpu_single_step_out(void);
void				debug_cpu_go(offs_t targetpc);
void				debug_cpu_go_vblank(void);
void				debug_cpu_go_interrupt(int irqline);
void				debug_cpu_next_cpu(void);
void				debug_cpu_ignore_cpu(int cpunum, int ignore);

/* tracing support */
void				debug_cpu_trace(int cpunum, FILE *file, int trace_over, const char *action);

/* breakpoints */
void				debug_check_breakpoints(int cpunum, offs_t pc);
struct breakpoint *	debug_breakpoint_first(int cpunum);
int					debug_breakpoint_set(int cpunum, offs_t address, struct parsed_expression *condition, const char *action);
int					debug_breakpoint_clear(int bpnum);
int					debug_breakpoint_enable(int bpnum, int enable);

/* watchpoints */
void				debug_check_watchpoints(int cpunum, int spacenum, int type, offs_t address, offs_t size, UINT64 value_to_write);
struct watchpoint *	debug_watchpoint_first(int cpunum, int spacenum);
int					debug_watchpoint_set(int cpunum, int spacenum, int type, offs_t address, offs_t length, struct parsed_expression *condition, const char *action);
int					debug_watchpoint_clear(int wpnum);
int					debug_watchpoint_enable(int wpnum, int enable);
const int *			debug_watchpoint_count_ptr(int cpunum);

/* memory accessors */
data8_t				debug_read_byte(int spacenum, offs_t address);
data16_t			debug_read_word(int spacenum, offs_t address);
data32_t			debug_read_dword(int spacenum, offs_t address);
data64_t			debug_read_qword(int spacenum, offs_t address);
void				debug_write_byte(int spacenum, offs_t address, data8_t data);
void				debug_write_word(int spacenum, offs_t address, data16_t data);
void				debug_write_dword(int spacenum, offs_t address, data32_t data);
void				debug_write_qword(int spacenum, offs_t address, data64_t data);
UINT64				debug_read_opcode(UINT32 offset, int size);
UINT64				debug_read_memory(int space, UINT32 offset, int size);
void				debug_write_memory(int space, UINT32 offset, int size, UINT64 value);

#endif
