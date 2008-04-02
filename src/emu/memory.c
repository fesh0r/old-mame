/***************************************************************************

    memory.c

    Functions which handle the CPU memory access.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Basic theory of memory handling:

    An address with up to 32 bits is passed to a memory handler. First,
    an address mask is applied to the address, removing unused bits.

    Next, the address is broken into two halves, an upper half and a
    lower half. The number of bits in each half can be controlled via
    the macros in LEVEL1_BITS and LEVEL2_BITS, but they default to the
    upper 18 bits and the lower 14 bits.

    The upper half is then used as an index into a lookup table of bytes.
    If the value pulled from the table is between SUBTABLE_BASE and 255,
    then the lower half of the address is needed to resolve the final
    handler. In this case, the value from the table is combined with the
    lower address bits to form an index into a subtable.

    The final result of the lookup is a value from 0 to SUBTABLE_BASE - 1.
    These values correspond to memory handlers. The lower numbered
    handlers (from 0 through STATIC_COUNT - 1) are fixed handlers and refer
    to either memory banks or other special cases. The remaining handlers
    (from STATIC_COUNT through SUBTABLE_BASE - 1) are dynamically
    allocated to driver-specified handlers.

    Thus, table entries fall into these categories:

        0 .. STATIC_COUNT - 1 = fixed handlers
        STATIC_COUNT .. SUBTABLE_BASE - 1 = driver-specific handlers
        SUBTABLE_BASE .. 255 = need to look up lower bits in subtable

    Caveats:

    * If your driver executes an opcode which crosses a bank-switched
    boundary, it will pull the wrong data out of memory. Although not
    a common case, you may need to revert to memcpy to work around this.
    See machine/tnzs.c for an example.

    To do:

    - Add local banks for RAM/ROM to reduce pressure on banking
    - Always mirror everything out to 32 bits so we don't have to mask the address?
    - Add the ability to start with another memory map and modify it
    - Add fourth memory space for encrypted opcodes
    - Automatically mirror program space into data space if no data space
    - Get rid of opcode/data separation by using address spaces?
    - Add support for internal addressing (maybe just accessors - see TMS3202x)
    - Evaluate min/max opcode ranges and do we include a check in cpu_readop?

****************************************************************************

    Address map fields and restrictions:

    AM_RANGE(start, end)
        Specifies a range of consecutive addresses beginning with 'start' and
        ending with 'end' inclusive. An address hits in this bucket if the
        'address' >= 'start' and 'address' <= 'end'.

    AM_MASK(mask)
        Specifies a mask for the addresses in the current bucket. This mask
        is applied after a positive hit in the bucket specified by AM_RANGE
        or AM_SPACE, and is computed before accessing the RAM or calling
        through to the read/write handler. If you use AM_MIRROR, below, the
        mask is ANDed implicitly with the logical NOT of the mirror. The
        mask specified by this macro is ANDed against any implicit masks.

    AM_MIRROR(mirror)
        Specifies mirror addresses for the given bucket. The current bucket
        is mapped repeatedly according to the mirror mask, once where each
        mirror bit is 0, and once where it is 1. For example, a 'mirror'
        value of 0x14000 would map the bucket at 0x00000, 0x04000, 0x10000,
        and 0x14000.

    AM_READ(read)
        Specifies the read handler for this bucket. All reads will pass
        through the given callback handler. Special static values representing
        RAM, ROM, or BANKs are also allowed here.

    AM_WRITE(write)
        Specifies the write handler for this bucket. All writes will pass
        through the given callback handler. Special static values representing
        RAM, ROM, or BANKs are also allowed here.

    AM_REGION(region, offs)
        Only useful if AM_READ/WRITE point to RAM, ROM, or BANK memory. By
        default, memory is allocated to back each bucket. By specifying
        AM_REGION, you can tell the memory system to point the base of the
        memory backing this bucket to a given memory 'region' at the
        specified 'offs'.

    AM_SHARE(index)
        Similar to AM_REGION, this specifies that the memory backing the
        current bucket is shared with other buckets. The first bucket to
        specify the share 'index' will use its memory as backing for all
        future buckets that specify AM_SHARE with the same 'index'.

    AM_BASE(base)
        Specifies a pointer to a pointer to the base of the memory backing
        the current bucket.

    AM_SIZE(size)
        Specifies a pointer to a size_t variable which will be filled in
        with the size, in bytes, of the current bucket.

***************************************************************************/

#include "driver.h"
#include "profiler.h"
#include "deprecat.h"
#ifdef ENABLE_DEBUGGER
#include "debug/debugcpu.h"
#endif


/***************************************************************************
    DEBUGGING
***************************************************************************/

#define MEM_DUMP		(0)
#define VERBOSE			(0)
#define ALLOW_ONLY_AUTO_MALLOC_BANKS	0

#define VPRINTF(x)	do { if (VERBOSE) mame_printf_debug x; } while (0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* banking constants */
#define MAX_BANKS				(STATIC_BANKMAX - STATIC_BANK1) /* maximum number of banks */
#define MAX_BANK_ENTRIES		256						/* maximum number of possible bank values */
#define MAX_EXPLICIT_BANKS		32						/* maximum number of explicitly-defined banks */

/* address map lookup table definitions */
#define LEVEL1_BITS				18						/* number of address bits in the level 1 table */
#define LEVEL2_BITS				(32 - LEVEL1_BITS)		/* number of address bits in the level 2 table */
#define SUBTABLE_COUNT			64						/* number of slots reserved for subtables */
#define SUBTABLE_BASE			(256 - SUBTABLE_COUNT)	/* first index of a subtable */
#define ENTRY_COUNT				(SUBTABLE_BASE)			/* number of legitimate (non-subtable) entries */
#define SUBTABLE_ALLOC			8						/* number of subtables to allocate at a time */

/* other address map constants */
#define MAX_SHARED_POINTERS		256						/* maximum number of shared pointers in memory maps */
#define MEMORY_BLOCK_CHUNK		65536					/* minimum chunk size of allocated memory blocks */

/* read or write constants */
enum _read_or_write
{
	ROW_READ,
	ROW_WRITE
};
typedef enum _read_or_write read_or_write;


/***************************************************************************
    MACROS
***************************************************************************/

/* table lookup helpers */
#define LEVEL1_INDEX(a)			((a) >> LEVEL2_BITS)
#define LEVEL2_INDEX(e,a)		((1 << LEVEL1_BITS) + (((e) - SUBTABLE_BASE) << LEVEL2_BITS) + ((a) & ((1 << LEVEL2_BITS) - 1)))

/* macros for the profiler */
#define MEMREADSTART()			do { profiler_mark(PROFILER_MEMREAD); } while (0)
#define MEMREADEND(ret)			do { profiler_mark(PROFILER_END); return ret; } while (0)
#define MEMWRITESTART()			do { profiler_mark(PROFILER_MEMWRITE); } while (0)
#define MEMWRITEEND(ret)		do { (ret); profiler_mark(PROFILER_END); return; } while (0)

/* helper macros */
#define HANDLER_IS_RAM(h)		((FPTR)(h) == STATIC_RAM)
#define HANDLER_IS_ROM(h)		((FPTR)(h) == STATIC_ROM)
#define HANDLER_IS_NOP(h)		((FPTR)(h) == STATIC_NOP)
#define HANDLER_IS_BANK(h)		((FPTR)(h) >= STATIC_BANK1 && (FPTR)(h) <= STATIC_BANKMAX)
#define HANDLER_IS_STATIC(h)	((FPTR)(h) < STATIC_COUNT)

#define HANDLER_TO_BANK(h)		((UINT32)(FPTR)(h))
#define BANK_TO_HANDLER(b)		((genf *)(FPTR)(b))

#undef ADDR2BYTE
#undef BYTE2ADDR
#define ADDR2BYTE(s,a)			(((s)->ashift < 0) ? ((a) << -(s)->ashift) : ((a) >> (s)->ashift))
#define ADDR2BYTE_END(s,a)		(((s)->ashift < 0) ? (((a) << -(s)->ashift) | ((1 << -(s)->ashift) - 1)) : ((a) >> (s)->ashift))
#define BYTE2ADDR(s,a)			(((s)->ashift < 0) ? ((a) >> -(s)->ashift) : ((a) << (s)->ashift))

#define SUBTABLE_PTR(tabledata, entry) (&(tabledata)->table[(1 << LEVEL1_BITS) + (((entry) - SUBTABLE_BASE) << LEVEL2_BITS)])

#ifdef ENABLE_DEBUGGER
#define DEBUG_HOOK_READ(spacenum,address,mem_mask) if (debug_hook_read) (*debug_hook_read)(spacenum,address,mem_mask)
#define DEBUG_HOOK_WRITE(spacenum,address,data,mem_mask) if (debug_hook_write) (*debug_hook_write)(spacenum,address,data,mem_mask)
#else
#define DEBUG_HOOK_READ(spacenum,address,mem_mask)
#define DEBUG_HOOK_WRITE(spacenum,address,data,mem_mask)
#endif


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* a memory block is a chunk of RAM associated with a range of memory in a CPU's address space */
typedef struct _memory_block memory_block;
struct _memory_block
{
	memory_block *			next;					/* next memory block in the list */
	UINT8					cpunum;					/* which CPU are we associated with? */
	UINT8					spacenum;				/* which address space are we associated with? */
	UINT8					isallocated;			/* did we allocate this ourselves? */
	offs_t 					bytestart, byteend;		/* byte-normalized start/end for verifying a match */
    UINT8 *					data;					/* pointer to the data for this block */
};

typedef struct _bank_data bank_info;
struct _bank_data
{
	UINT8 					used;					/* is this bank used? */
	UINT8 					dynamic;				/* is this bank allocated dynamically? */
	UINT8 					cpunum;					/* the CPU it is used for */
	UINT8 					spacenum;				/* the address space it is used for */
	UINT8 					read;					/* is this bank used for reads? */
	UINT8 					write;					/* is this bank used for writes? */
	offs_t 					bytestart;				/* byte-adjusted start offset */
	offs_t 					byteend;				/* byte-adjusted end offset */
	UINT16					curentry;				/* current entry */
	void *					entry[MAX_BANK_ENTRIES];/* array of entries for this bank */
	void *					entryd[MAX_BANK_ENTRIES];/* array of decrypted entries for this bank */
};

/* In memory.h: typedef struct _handler_data handler_data */
struct _handler_data
{
	memory_handler			handler;				/* function pointer for handler */
	void *					object;					/* object associated with the handler */
	const char *			name;					/* name of the handler */
	offs_t					bytestart;				/* byte-adjusted start address for handler */
	offs_t					byteend;				/* byte-adjusted end address for handler */
	offs_t					bytemask;				/* byte-adjusted mask against the final address */
};

typedef struct _subtable_data subtable_data;
struct _subtable_data
{
	UINT8					checksum_valid;			/* is the checksum valid */
	UINT32					checksum;				/* checksum over all the bytes */
	UINT32					usecount;				/* number of times this has been used */
};

typedef struct _table_data table_data;
struct _table_data
{
	UINT8 *					table;					/* pointer to base of table */
	UINT8 					subtable_alloc;			/* number of subtables allocated */
	subtable_data			subtable[SUBTABLE_COUNT]; /* info about each subtable */
	handler_data			handlers[ENTRY_COUNT];	/* array of user-installed handlers */
};

typedef struct _addrspace_data addrspace_data;
struct _addrspace_data
{
	UINT8					cpunum;					/* CPU index */
	UINT8					spacenum;				/* address space index */
	INT8					ashift;					/* address shift */
	UINT8					abits;					/* address bits */
	UINT8 					dbits;					/* data bits */
	offs_t					addrmask;				/* global address mask */
	offs_t					bytemask;				/* byte-converted global address mask */
	UINT64					unmap;					/* unmapped value */
	table_data				read;					/* memory read lookup table */
	table_data				write;					/* memory write lookup table */
	const data_accessors *	accessors;				/* pointer to the memory accessors */
	address_map *			map;					/* original memory map */
};

typedef struct _cpu_data cpu_data;
struct _cpu_data
{
	UINT8 *					region;					/* pointer to memory region */
	size_t					regionsize;				/* size of region, in bytes */

	opbase_handler_func 	opbase;					/* opcode base handler */

	void *					op_ram;					/* dynamic RAM base pointer */
	void *					op_rom;					/* dynamic ROM base pointer */
	offs_t					op_mask;				/* dynamic ROM address mask */
	offs_t					op_mem_min;				/* dynamic ROM/RAM min */
	offs_t					op_mem_max;				/* dynamic ROM/RAM max */
	UINT8		 			opcode_entry;			/* opcode base handler */

	UINT8					spacemask;				/* mask of which address spaces are used */
	addrspace_data		 	space[ADDRESS_SPACES];	/* info about each address space */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

UINT8 *						opcode_base;					/* opcode base */
UINT8 *						opcode_arg_base;				/* opcode argument base */
offs_t						opcode_mask;					/* mask to apply to the opcode address */
offs_t						opcode_memory_min;				/* opcode memory minimum */
offs_t						opcode_memory_max;				/* opcode memory maximum */
UINT8		 				opcode_entry;					/* opcode readmem entry */

address_space				active_address_space[ADDRESS_SPACES];/* address space data */

static UINT8 *				bank_ptr[STATIC_COUNT];			/* array of bank pointers */
static UINT8 *				bankd_ptr[STATIC_COUNT];		/* array of decrypted bank pointers */
static void *				shared_ptr[MAX_SHARED_POINTERS];/* array of shared pointers */

static memory_block *		memory_block_list;				/* head of the list of memory blocks */

static int					cur_context;					/* current CPU context */

static opbase_handler_func	opbasefunc;						/* opcode base override */

static UINT8				debugger_access;				/* treat accesses as coming from the debugger */
static UINT8				log_unmap[ADDRESS_SPACES];		/* log unmapped memory accesses */

static cpu_data				cpudata[MAX_CPU];				/* data gathered for each CPU */
static bank_info 			bankdata[STATIC_COUNT];			/* data gathered for each bank */

#ifdef ENABLE_DEBUGGER
static debug_hook_read_func	debug_hook_read;				/* pointer to debugger callback for memory reads */
static debug_hook_write_func	debug_hook_write;				/* pointer to debugger callback for memory writes */
#endif

static const data_accessors memory_accessors[ADDRESS_SPACES][4][2] =
{
	/* program accessors */
	{
		{
			{ memory_set_opbase, program_read_byte_8, NULL, NULL, NULL, NULL, NULL, program_write_byte_8, NULL, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, program_read_byte_8, NULL, NULL, NULL, NULL, NULL, program_write_byte_8, NULL, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, program_read_byte_16le, program_read_word_16le, NULL, NULL, NULL, NULL, program_write_byte_16le, program_write_word_16le, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, program_read_byte_16be, program_read_word_16be, NULL, NULL, NULL, NULL, program_write_byte_16be, program_write_word_16be, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, program_read_byte_32le, program_read_word_32le, program_read_dword_32le, program_read_masked_32le, NULL, NULL, program_write_byte_32le, program_write_word_32le, program_write_dword_32le, program_write_masked_32le, NULL, NULL },
			{ memory_set_opbase, program_read_byte_32be, program_read_word_32be, program_read_dword_32be, program_read_masked_32be, NULL, NULL, program_write_byte_32be, program_write_word_32be, program_write_dword_32be, program_write_masked_32be, NULL, NULL }
		},
		{
			{ memory_set_opbase, program_read_byte_64le, program_read_word_64le, program_read_dword_64le, NULL, program_read_qword_64le, program_read_masked_64le, program_write_byte_64le, program_write_word_64le, program_write_dword_64le, NULL, program_write_qword_64le, program_write_masked_64le },
			{ memory_set_opbase, program_read_byte_64be, program_read_word_64be, program_read_dword_64be, NULL, program_read_qword_64be, program_read_masked_64be, program_write_byte_64be, program_write_word_64be, program_write_dword_64be, NULL, program_write_qword_64be, program_write_masked_64be }
		}
	},

	/* data accessors */
	{
		{
			{ memory_set_opbase, data_read_byte_8, NULL, NULL, NULL, NULL, NULL, data_write_byte_8, NULL, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, data_read_byte_8, NULL, NULL, NULL, NULL, NULL, data_write_byte_8, NULL, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, data_read_byte_16le, data_read_word_16le, NULL, NULL, NULL, NULL, data_write_byte_16le, data_write_word_16le, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, data_read_byte_16be, data_read_word_16be, NULL, NULL, NULL, NULL, data_write_byte_16be, data_write_word_16be, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, data_read_byte_32le, data_read_word_32le, data_read_dword_32le, data_read_masked_32le, NULL, NULL, data_write_byte_32le, data_write_word_32le, data_write_dword_32le, data_write_masked_32le, NULL, NULL },
			{ memory_set_opbase, data_read_byte_32be, data_read_word_32be, data_read_dword_32be, data_read_masked_32be, NULL, NULL, data_write_byte_32be, data_write_word_32be, data_write_dword_32be, data_write_masked_32be, NULL, NULL }
		},
		{
			{ memory_set_opbase, data_read_byte_64le, data_read_word_64le, data_read_dword_64le, NULL, data_read_qword_64le, data_read_masked_64le, data_write_byte_64le, data_write_word_64le, data_write_dword_64le, NULL, data_write_qword_64le, data_write_masked_64le },
			{ memory_set_opbase, data_read_byte_64be, data_read_word_64be, data_read_dword_64be, NULL, data_read_qword_64be, data_read_masked_64be, data_write_byte_64be, data_write_word_64be, data_write_dword_64be, NULL, data_write_qword_64be, data_write_masked_64be }
		}
	},

	/* I/O accessors */
	{
		{
			{ memory_set_opbase, io_read_byte_8, NULL, NULL, NULL, NULL, NULL, io_write_byte_8, NULL, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, io_read_byte_8, NULL, NULL, NULL, NULL, NULL, io_write_byte_8, NULL, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, io_read_byte_16le, io_read_word_16le, NULL, NULL, NULL, NULL, io_write_byte_16le, io_write_word_16le, NULL, NULL, NULL, NULL },
			{ memory_set_opbase, io_read_byte_16be, io_read_word_16be, NULL, NULL, NULL, NULL, io_write_byte_16be, io_write_word_16be, NULL, NULL, NULL, NULL }
		},
		{
			{ memory_set_opbase, io_read_byte_32le, io_read_word_32le, io_read_dword_32le, io_read_masked_32le, NULL, NULL, io_write_byte_32le, io_write_word_32le, io_write_dword_32le, io_write_masked_32le, NULL, NULL },
			{ memory_set_opbase, io_read_byte_32be, io_read_word_32be, io_read_dword_32be, io_read_masked_32be, NULL, NULL, io_write_byte_32be, io_write_word_32be, io_write_dword_32be, io_write_masked_32be, NULL, NULL }
		},
		{
			{ memory_set_opbase, io_read_byte_64le, io_read_word_64le, io_read_dword_64le, NULL, io_read_qword_64le, io_read_masked_64le, io_write_byte_64le, io_write_word_64le, io_write_dword_64le, NULL, io_write_qword_64le, io_write_masked_64le },
			{ memory_set_opbase, io_read_byte_64be, io_read_word_64be, io_read_dword_64be, NULL, io_read_qword_64be, io_read_masked_64be, io_write_byte_64be, io_write_word_64be, io_write_dword_64be, NULL, io_write_qword_64be, io_write_masked_64be }
		}
	},
};

const char *const address_space_names[ADDRESS_SPACES] = { "program", "data", "I/O" };



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void address_map_detokenize(address_map *map, const addrmap_token *tokens);

static void memory_init_cpudata(const machine_config *config);
static void memory_init_preflight(const machine_config *config);
static void memory_init_populate(running_machine *machine);
static void install_mem_handler_private(addrspace_data *space, read_or_write readorwrite, int databits, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, genf *handler, void *object, const char *handler_name);
static void install_mem_handler(addrspace_data *space, read_or_write readorwrite, int databits, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, genf *handler, void *object, const char *handler_name);
static void bank_assign_static(int banknum, int cpunum, int spacenum, read_or_write readorwrite, offs_t bytestart, offs_t byteend);
static genf *bank_assign_dynamic(int cpunum, int spacenum, read_or_write readorwrite, offs_t bytestart, offs_t byteend);
static UINT8 get_handler_index(handler_data *table, void *object, genf *handler, const char *handler_name, offs_t bytestart, offs_t byteend, offs_t bytemask);
static void populate_table_range(addrspace_data *space, read_or_write readorwrite, offs_t bytestart, offs_t byteend, UINT8 handler);
static UINT8 subtable_alloc(table_data *tabledata);
static void subtable_realloc(table_data *tabledata, UINT8 subentry);
static int subtable_merge(table_data *tabledata);
static void subtable_release(table_data *tabledata, UINT8 subentry);
static UINT8 *subtable_open(table_data *tabledata, offs_t l1index);
static void subtable_close(table_data *tabledata, offs_t l1index);
static void memory_init_allocate(const machine_config *config);
static void *allocate_memory_block(int cpunum, int spacenum, offs_t bytestart, offs_t byteend, void *memory);
static void register_for_save(int cpunum, int spacenum, offs_t bytestart, void *base, size_t numbytes);
static address_map_entry *assign_intersecting_blocks(addrspace_data *space, offs_t bytestart, offs_t byteend, UINT8 *base);
static void memory_init_locate(void);
static void *memory_find_base(int cpunum, int spacenum, offs_t byteaddress);
static genf *get_static_handler(int databits, int readorwrite, int spacenum, int which);
static void memory_exit(running_machine *machine);
static void mem_dump(void);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    force_opbase_update - ensure that we update
    the opcode base
-------------------------------------------------*/

INLINE void force_opbase_update(void)
{
	opcode_entry = 0xff;
	memory_set_opbase(activecpu_get_physical_pc_byte());
}


/*-------------------------------------------------
    adjust_addresses - adjust addresses for a
    given address space in a standard fashion
-------------------------------------------------*/

INLINE void adjust_addresses(addrspace_data *space, offs_t *start, offs_t *end, offs_t *mask, offs_t *mirror)
{
	/* adjust start/end/mask values */
	if (*mask == 0)
		*mask = space->addrmask & ~*mirror;
	else
		*mask &= space->addrmask;
	*start &= ~*mirror & space->addrmask;
	*end &= ~*mirror & space->addrmask;

	/* adjust to byte values */
	*start = ADDR2BYTE(space, *start);
	*end = ADDR2BYTE_END(space, *end);
	*mask = ADDR2BYTE(space, *mask);
	*mirror = ADDR2BYTE(space, *mirror);
}



/***************************************************************************
    CORE SYSTEM OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    memory_init - initialize the memory system
-------------------------------------------------*/

void memory_init(running_machine *machine)
{
	int spacenum;

	add_exit_callback(machine, memory_exit);

	/* no current context to start */
	cur_context = -1;
	for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		log_unmap[spacenum] = TRUE;

	/* reset the shared pointers and bank pointers */
	memset(shared_ptr, 0, sizeof(shared_ptr));
	memset(bank_ptr, 0, sizeof(bank_ptr));
	memset(bankd_ptr, 0, sizeof(bankd_ptr));

	/* build up the cpudata array with info about all CPUs and address spaces */
	memory_init_cpudata(machine->config);

	/* preflight the memory handlers and check banks */
	memory_init_preflight(machine->config);

	/* then fill in the tables */
	memory_init_populate(machine);

	/* allocate any necessary memory */
	memory_init_allocate(machine->config);

	/* find all the allocated pointers */
	memory_init_locate();

	/* dump the final memory configuration */
	mem_dump();
}


/*-------------------------------------------------
    memory_exit - free memory
-------------------------------------------------*/

static void memory_exit(running_machine *machine)
{
	int cpunum, spacenum;

	/* free the memory blocks */
	while (memory_block_list != NULL)
	{
		memory_block *block = memory_block_list;
		memory_block_list = block->next;
		free(block);
	}

	/* free all the tables */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
			addrspace_data *space = &cpudata[cpunum].space[spacenum];
			if (space->map != NULL)
				address_map_free(space->map);
			if (space->read.table != NULL)
				free(space->read.table);
			if (space->write.table != NULL)
				free(space->write.table);
		}
}


/*-------------------------------------------------
    memory_set_context - set the memory context
-------------------------------------------------*/

void memory_set_context(int activecpu)
{
	/* remember dynamic RAM/ROM */
	if (cur_context != -1)
	{
		cpudata[cur_context].op_ram = opcode_arg_base;
		cpudata[cur_context].op_rom = opcode_base;
		cpudata[cur_context].op_mask = opcode_mask;
		cpudata[cur_context].op_mem_min = opcode_memory_min;
		cpudata[cur_context].op_mem_max = opcode_memory_max;
		cpudata[cur_context].opcode_entry = opcode_entry;
	}
	cur_context = activecpu;

	opcode_arg_base = cpudata[activecpu].op_ram;
	opcode_base = cpudata[activecpu].op_rom;
	opcode_mask = cpudata[activecpu].op_mask;
	opcode_memory_min = cpudata[activecpu].op_mem_min;
	opcode_memory_max = cpudata[activecpu].op_mem_max;
	opcode_entry = cpudata[activecpu].opcode_entry;

	/* program address space */
	active_address_space[ADDRESS_SPACE_PROGRAM].bytemask = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].bytemask;
	active_address_space[ADDRESS_SPACE_PROGRAM].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].read.table;
	active_address_space[ADDRESS_SPACE_PROGRAM].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].write.table;
	active_address_space[ADDRESS_SPACE_PROGRAM].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].read.handlers;
	active_address_space[ADDRESS_SPACE_PROGRAM].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].write.handlers;
	active_address_space[ADDRESS_SPACE_PROGRAM].accessors = cpudata[activecpu].space[ADDRESS_SPACE_PROGRAM].accessors;

	/* data address space */
	if (cpudata[activecpu].spacemask & (1 << ADDRESS_SPACE_DATA))
	{
		active_address_space[ADDRESS_SPACE_DATA].bytemask = cpudata[activecpu].space[ADDRESS_SPACE_DATA].bytemask;
		active_address_space[ADDRESS_SPACE_DATA].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_DATA].read.table;
		active_address_space[ADDRESS_SPACE_DATA].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_DATA].write.table;
		active_address_space[ADDRESS_SPACE_DATA].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_DATA].read.handlers;
		active_address_space[ADDRESS_SPACE_DATA].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_DATA].write.handlers;
		active_address_space[ADDRESS_SPACE_DATA].accessors = cpudata[activecpu].space[ADDRESS_SPACE_DATA].accessors;
	}

	/* I/O address space */
	if (cpudata[activecpu].spacemask & (1 << ADDRESS_SPACE_IO))
	{
		active_address_space[ADDRESS_SPACE_IO].bytemask = cpudata[activecpu].space[ADDRESS_SPACE_IO].bytemask;
		active_address_space[ADDRESS_SPACE_IO].readlookup = cpudata[activecpu].space[ADDRESS_SPACE_IO].read.table;
		active_address_space[ADDRESS_SPACE_IO].writelookup = cpudata[activecpu].space[ADDRESS_SPACE_IO].write.table;
		active_address_space[ADDRESS_SPACE_IO].readhandlers = cpudata[activecpu].space[ADDRESS_SPACE_IO].read.handlers;
		active_address_space[ADDRESS_SPACE_IO].writehandlers = cpudata[activecpu].space[ADDRESS_SPACE_IO].write.handlers;
		active_address_space[ADDRESS_SPACE_IO].accessors = cpudata[activecpu].space[ADDRESS_SPACE_IO].accessors;
	}

	opbasefunc = cpudata[activecpu].opbase;

#ifdef ENABLE_DEBUGGER
	if (activecpu != -1 && Machine->debug_mode)
		debug_get_memory_hooks(activecpu, &debug_hook_read, &debug_hook_write);
	else
	{
		debug_hook_read = NULL;
		debug_hook_write = NULL;
	}
#endif
}


/*-------------------------------------------------
    memory_get_accessors - get a pointer to the
    set of memory accessor functions based on
    the address space, databus width, and
    endianness
-------------------------------------------------*/

const data_accessors *memory_get_accessors(int spacenum, int databits, int endianness)
{
	int accessorindex = (databits == 8) ? 0 : (databits == 16) ? 1 : (databits == 32) ? 2 : 3;
	return &memory_accessors[spacenum][accessorindex][(endianness == CPU_IS_LE) ? 0 : 1];
}



/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

/*-------------------------------------------------
    address_map_alloc - build and allocate an
    address map for a CPU's address space
-------------------------------------------------*/

address_map *address_map_alloc(const machine_config *config, int cpunum, int spacenum)
{
	int cputype = config->cpu[cpunum].type;
	const addrmap_token *internal_map = (const addrmap_token *)cputype_get_info_ptr(cputype, CPUINFO_PTR_INTERNAL_MEMORY_MAP + spacenum);
	address_map *map;

	map = malloc_or_die(sizeof(*map));
	memset(map, 0, sizeof(*map));

	/* start by constructing the internal CPU map */
	if (internal_map != NULL)
		address_map_detokenize(map, internal_map);

	/* construct the standard map */
	if (config->cpu[cpunum].address_map[spacenum][0] != NULL)
		address_map_detokenize(map, config->cpu[cpunum].address_map[spacenum][0]);
	if (config->cpu[cpunum].address_map[spacenum][1] != NULL)
		address_map_detokenize(map, config->cpu[cpunum].address_map[spacenum][1]);

	return map;
}


/*-------------------------------------------------
    address_map_free - release allocated memory
    for an address map
-------------------------------------------------*/

void address_map_free(address_map *map)
{
	/* free all entries */
	while (map->entrylist != NULL)
	{
		address_map_entry *entry = map->entrylist;
		map->entrylist = entry->next;
		free(entry);
	}

	/* free the map */
	free(map);
}


/*-------------------------------------------------
    memory_get_address_map - return a pointer to
    the constructed address map for a CPU's
    address space
-------------------------------------------------*/

const address_map *memory_get_address_map(int cpunum, int spacenum)
{
	return cpudata[cpunum].space[spacenum].map;
}


/*-------------------------------------------------
    address_map_detokenize - detokenize an array
    of address map tokens
-------------------------------------------------*/

static void address_map_detokenize(address_map *map, const addrmap_token *tokens)
{
	address_map_entry **entryptr;
	address_map_entry *entry;
	UINT8 spacenum, databits;
	UINT32 entrytype;

	/* check the first token */
	TOKEN_GET_UINT32_UNPACK3(tokens, entrytype, 8, spacenum, 8, databits, 8);
	if (entrytype != ADDRMAP_TOKEN_START)
		fatalerror("Address map missing ADDRMAP_TOKEN_START!");
	if (spacenum >= ADDRESS_SPACES)
		fatalerror("Invalid address space %d for memory map!", spacenum);
	if (databits != 8 && databits != 16 && databits != 32 && databits != 64)
		fatalerror("Invalid data bits %d for memory map!", databits);
	if (map->spacenum != 0 && map->spacenum != spacenum)
		fatalerror("Included a mismatched address map (space %d) for an existing map of type %d!\n", spacenum, map->spacenum);
	if (map->databits != 0 && map->databits != databits)
		fatalerror("Included a mismatched address map (databits %d) for an existing map with databits %d!\n", databits, map->databits);

	/* fill in the map values */
	map->spacenum = spacenum;
	map->databits = databits;

	/* find the end of the list */
	for (entryptr = &map->entrylist; *entryptr != NULL; entryptr = &(*entryptr)->next) ;
	entry = NULL;

	/* loop over tokens until we hit the end */
	while (entrytype != ADDRMAP_TOKEN_END)
	{
		const char *tag;

		/* unpack the token from the first entry */
		TOKEN_GET_UINT32_UNPACK1(tokens, entrytype, 8);
		switch (entrytype)
		{
			/* end */
			case ADDRMAP_TOKEN_END:
				break;

			/* including */
			case ADDRMAP_TOKEN_INCLUDE:
				address_map_detokenize(map, TOKEN_GET_PTR(tokens, tokenptr));
				for (entryptr = &map->entrylist; *entryptr != NULL; entryptr = &(*entryptr)->next) ;
				entry = NULL;
				break;

			/* global flags */
			case ADDRMAP_TOKEN_GLOBAL_MASK:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT64_UNPACK2(tokens, entrytype, 8, map->globalmask, 32);
				break;

			case ADDRMAP_TOKEN_UNMAP_VALUE:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT32_UNPACK2(tokens, entrytype, 8, map->unmapval, 1);
				break;

			/* start a new range */
			case ADDRMAP_TOKEN_RANGE:
				entry = *entryptr = malloc_or_die(sizeof(**entryptr));
				entryptr = &entry->next;
				memset(entry, 0, sizeof(*entry));
				TOKEN_GET_UINT64_UNPACK2(tokens, entry->addrstart, 32, entry->addrend, 32);
				break;

			case ADDRMAP_TOKEN_MASK:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT64_UNPACK2(tokens, entrytype, 8, entry->addrmask, 32);
				break;

			case ADDRMAP_TOKEN_MIRROR:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT64_UNPACK2(tokens, entrytype, 8, entry->addrmirror, 32);
				break;

			case ADDRMAP_TOKEN_READ:
				entry->read = TOKEN_GET_PTR(tokens, read);
				entry->read_name = TOKEN_GET_STRING(tokens);
				break;

			case ADDRMAP_TOKEN_WRITE:
				entry->write = TOKEN_GET_PTR(tokens, write);
				entry->write_name = TOKEN_GET_STRING(tokens);
				break;

			case ADDRMAP_TOKEN_DEVICE_READ:
				entry->read = TOKEN_GET_PTR(tokens, read);
				entry->read_name = TOKEN_GET_STRING(tokens);
				entry->read_devtype = TOKEN_GET_PTR(tokens, devtype);
				entry->read_devtag = TOKEN_GET_STRING(tokens);
				break;

			case ADDRMAP_TOKEN_DEVICE_WRITE:
				entry->write = TOKEN_GET_PTR(tokens, write);
				entry->write_name = TOKEN_GET_STRING(tokens);
				entry->write_devtype = TOKEN_GET_PTR(tokens, devtype);
				entry->write_devtag = TOKEN_GET_STRING(tokens);
				break;

			case ADDRMAP_TOKEN_READ_PORT:
				tag = TOKEN_GET_STRING(tokens);
				switch (map->databits)
				{
					case 8:		entry->read.mhandler8  = port_tag_to_handler8(tag);		break;
					case 16:	entry->read.mhandler16 = port_tag_to_handler16(tag);	break;
					case 32:	entry->read.mhandler32 = port_tag_to_handler32(tag);	break;
					case 64:	entry->read.mhandler64 = port_tag_to_handler64(tag);	break;
				}
				if (entry->read.generic == NULL)
					fatalerror("Non-existent port referenced: '%s'\n", tag);
				break;

			case ADDRMAP_TOKEN_REGION:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT64_UNPACK3(tokens, entrytype, 8, entry->region, 24, entry->region_offs, 32);
				break;

			case ADDRMAP_TOKEN_SHARE:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT32_UNPACK2(tokens, entrytype, 8, entry->share, 24);
				break;

			case ADDRMAP_TOKEN_BASEPTR:
				entry->baseptr = (void **)TOKEN_GET_PTR(tokens, voidptr);
				break;

			case ADDRMAP_TOKEN_BASE_MEMBER:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT32_UNPACK2(tokens, entrytype, 8, entry->baseptroffs_plus1, 24);
				entry->baseptroffs_plus1++;
				break;

			case ADDRMAP_TOKEN_SIZEPTR:
				entry->sizeptr = TOKEN_GET_PTR(tokens, sizeptr);
				break;

			case ADDRMAP_TOKEN_SIZE_MEMBER:
				TOKEN_UNGET_UINT32(tokens);
				TOKEN_GET_UINT32_UNPACK2(tokens, entrytype, 8, entry->sizeptroffs_plus1, 24);
				entry->sizeptroffs_plus1++;
				break;

			default:
				fatalerror("Invalid token %d in address map\n", entrytype);
				break;
		}
	}
}



/***************************************************************************
    OPCODE BASE CONTROL
***************************************************************************/

/*-------------------------------------------------
    memory_set_decrypted_region - registers an
    address range as having a decrypted data
    pointer
-------------------------------------------------*/

void memory_set_decrypted_region(int cpunum, offs_t addrstart, offs_t addrend, void *base)
{
	addrspace_data *space = &cpudata[cpunum].space[ADDRESS_SPACE_PROGRAM];
	offs_t bytestart = ADDR2BYTE(space, addrstart);
	offs_t byteend = ADDR2BYTE_END(space, addrend);
	int banknum, found = FALSE;

	/* loop over banks looking for a match */
	for (banknum = 0; banknum < STATIC_COUNT; banknum++)
	{
		bank_info *bank = &bankdata[banknum];

		/* consider this bank if it is used for reading and matches the CPU/address space */
		if (bank->used && bank->read && bank->cpunum == cpunum && bank->spacenum == ADDRESS_SPACE_PROGRAM)
		{
			/* verify that the region fully covers the decrypted range */
			if (bank->bytestart >= bytestart && bank->byteend <= byteend)
			{
				/* set the decrypted pointer for the corresponding memory bank */
				bankd_ptr[banknum] = (UINT8 *)base + bank->bytestart - bytestart;
				found = TRUE;

				/* if we are executing from here, force an opcode base update */
				if (cpu_getactivecpu() >= 0 && cpunum == cur_context && opcode_entry == banknum)
					force_opbase_update();
			}

			/* fatal error if the decrypted region straddles the bank */
			else if (bank->bytestart < byteend && bank->byteend > bytestart)
				fatalerror("memory_set_decrypted_region found straddled region %08X-%08X for CPU %d", bytestart, byteend, cpunum);
		}
	}

	/* fatal error as well if we didn't find any relevant memory banks */
	if (!found)
		fatalerror("memory_set_decrypted_region unable to find matching region %08X-%08X for CPU %d", bytestart, byteend, cpunum);
}


/*-------------------------------------------------
    memory_set_opbase_handler - register a
    handler for opcode base changes on a given
    CPU
-------------------------------------------------*/

opbase_handler_func memory_set_opbase_handler(int cpunum, opbase_handler_func function)
{
	opbase_handler_func old = cpudata[cpunum].opbase;
	cpudata[cpunum].opbase = function;
	if (cpunum == cpu_getactivecpu())
		opbasefunc = function;
	return old;
}


/*-------------------------------------------------
    memory_set_opbase - called by CPU cores to
    update the opcode base for the given address
-------------------------------------------------*/

void memory_set_opbase(offs_t byteaddress)
{
	const address_space *space = &active_address_space[ADDRESS_SPACE_PROGRAM];
	UINT8 *base = NULL, *based = NULL;
	const handler_data *handlers;
	UINT8 entry;

	/* allow overrides */
	if (opbasefunc != NULL)
	{
		byteaddress = (*opbasefunc)(Machine, byteaddress);
		if (byteaddress == ~0)
			return;
	}

	/* perform the lookup */
	byteaddress &= space->bytemask;
	entry = space->readlookup[LEVEL1_INDEX(byteaddress)];
	if (entry >= SUBTABLE_BASE)
		entry = space->readlookup[LEVEL2_INDEX(entry,byteaddress)];

	/* keep track of current entry */
	opcode_entry = entry;

	/* if we don't map to a bank, see if there are any banks we can map to */
	if (entry < STATIC_BANK1 || entry >= STATIC_RAM)
	{
		/* loop over banks and find a match */
		for (entry = 1; entry < STATIC_COUNT; entry++)
		{
			bank_info *bank = &bankdata[entry];
			if (bank->used && bank->cpunum == cur_context && bank->spacenum == ADDRESS_SPACE_PROGRAM &&
				bank->bytestart < byteaddress && bank->byteend > byteaddress)
				break;
		}

		/* if nothing was found, leave everything alone */
		if (entry == STATIC_COUNT)
		{
			logerror("cpu #%d (PC=%08X): warning - op-code execute on mapped I/O\n",
						cpu_getactivecpu(), activecpu_get_pc());
			return;
		}
	}

	/* if no decrypted opcodes, point to the same base */
	base = bank_ptr[entry];
	based = bankd_ptr[entry];
	if (based == NULL)
		based = base;

	/* compute the adjusted base */
	handlers = &active_address_space[ADDRESS_SPACE_PROGRAM].readhandlers[entry];
	opcode_mask = handlers->bytemask;
	opcode_arg_base = base - (handlers->bytestart & opcode_mask);
	opcode_base = based - (handlers->bytestart & opcode_mask);
	opcode_memory_min = handlers->bytestart;
	opcode_memory_max = handlers->byteend;
}



/***************************************************************************
    OPCODE BASE CONTROL
***************************************************************************/

/*-------------------------------------------------
    memory_get_read_ptr - return a pointer to the
    base of RAM associated with the given CPU
    and offset
-------------------------------------------------*/

void *memory_get_read_ptr(int cpunum, int spacenum, offs_t byteaddress)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	offs_t byteoffset;
	UINT8 entry;

	/* perform the lookup */
	byteaddress &= space->bytemask;
	entry = space->read.table[LEVEL1_INDEX(byteaddress)];
	if (entry >= SUBTABLE_BASE)
		entry = space->read.table[LEVEL2_INDEX(entry, byteaddress)];

	/* 8-bit case: RAM/ROM */
	if (entry >= STATIC_RAM)
		return NULL;
	byteoffset = (byteaddress - space->read.handlers[entry].bytestart) & space->read.handlers[entry].bytemask;
	return &bank_ptr[entry][byteoffset];
}


/*-------------------------------------------------
    memory_get_write_ptr - return a pointer to the
    base of RAM associated with the given CPU
    and offset
-------------------------------------------------*/

void *memory_get_write_ptr(int cpunum, int spacenum, offs_t byteaddress)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	offs_t byteoffset;
	UINT8 entry;

	/* perform the lookup */
	byteaddress &= space->bytemask;
	entry = space->write.table[LEVEL1_INDEX(byteaddress)];
	if (entry >= SUBTABLE_BASE)
		entry = space->write.table[LEVEL2_INDEX(entry, byteaddress)];

	/* 8-bit case: RAM/ROM */
	if (entry >= STATIC_RAM)
		return NULL;
	byteoffset = (byteaddress - space->write.handlers[entry].bytestart) & space->write.handlers[entry].bytemask;
	return &bank_ptr[entry][byteoffset];
}


/*-------------------------------------------------
    memory_get_op_ptr - return a pointer to the
    base of opcode RAM associated with the given
    CPU and offset
-------------------------------------------------*/

void *memory_get_op_ptr(int cpunum, offs_t byteaddress, int arg)
{
	addrspace_data *space = &cpudata[cpunum].space[ADDRESS_SPACE_PROGRAM];
	offs_t byteoffset;
	void *ptr = NULL;
	UINT8 entry;

	/* if there is a custom mapper, use that */
	if (cpudata[cpunum].opbase != NULL)
	{
		/* need to save opcode info */
		UINT8 *saved_opcode_base = opcode_base;
		UINT8 *saved_opcode_arg_base = opcode_arg_base;
		offs_t saved_opcode_mask = opcode_mask;
		offs_t saved_opcode_memory_min = opcode_memory_min;
		offs_t saved_opcode_memory_max = opcode_memory_max;
		UINT8 saved_opcode_entry = opcode_entry;

		/* query the handler */
		offs_t new_byteaddress = (*cpudata[cpunum].opbase)(Machine, byteaddress);

		/* if it returns ~0, we use whatever data the handler set */
		if (new_byteaddress == ~0)
			ptr = arg ? &opcode_arg_base[byteaddress] : &opcode_base[byteaddress];

		/* otherwise, we use the new offset in the generic case below */
		else
			byteaddress = new_byteaddress;

		/* restore opcode info */
		opcode_base = saved_opcode_base;
		opcode_arg_base = saved_opcode_arg_base;
		opcode_mask = saved_opcode_mask;
		opcode_memory_min = saved_opcode_memory_min;
		opcode_memory_max = saved_opcode_memory_max;
		opcode_entry = saved_opcode_entry;

		/* if we got our pointer, we're done */
		if (ptr != NULL)
			return ptr;
	}

	/* perform the lookup */
	byteaddress &= space->bytemask;
	entry = space->read.table[LEVEL1_INDEX(byteaddress)];
	if (entry >= SUBTABLE_BASE)
		entry = space->read.table[LEVEL2_INDEX(entry, byteaddress)];

	/* if a non-RAM area, return NULL */
	if (entry >= STATIC_RAM)
		return NULL;

	/* adjust the offset */
	byteoffset = (byteaddress - space->read.handlers[entry].bytestart) & space->read.handlers[entry].bytemask;
	return (!arg && bankd_ptr[entry]) ? &bankd_ptr[entry][byteoffset] : &bank_ptr[entry][byteoffset];
}


/*-------------------------------------------------
    memory_configure_bank - configure the
    addresses for a bank
-------------------------------------------------*/

void memory_configure_bank(int banknum, int startentry, int numentries, void *base, offs_t stride)
{
	int entrynum;

	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		fatalerror("memory_configure_bank called with invalid bank %d", banknum);
	if (bankdata[banknum].dynamic)
		fatalerror("memory_configure_bank called with dynamic bank %d", banknum);
	if (startentry < 0 || startentry + numentries > MAX_BANK_ENTRIES)
		fatalerror("memory_configure_bank called with out-of-range entries %d-%d", startentry, startentry + numentries - 1);
	if (!base)
		fatalerror("memory_configure_bank called NULL base");

	/* fill in the requested bank entries */
	for (entrynum = startentry; entrynum < startentry + numentries; entrynum++)
		bankdata[banknum].entry[entrynum] = (UINT8 *)base + (entrynum - startentry) * stride;
}



/*-------------------------------------------------
    memory_configure_bank_decrypted - configure
    the decrypted addresses for a bank
-------------------------------------------------*/

void memory_configure_bank_decrypted(int banknum, int startentry, int numentries, void *base, offs_t stride)
{
	int entrynum;

	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		fatalerror("memory_configure_bank called with invalid bank %d", banknum);
	if (bankdata[banknum].dynamic)
		fatalerror("memory_configure_bank called with dynamic bank %d", banknum);
	if (startentry < 0 || startentry + numentries > MAX_BANK_ENTRIES)
		fatalerror("memory_configure_bank called with out-of-range entries %d-%d", startentry, startentry + numentries - 1);
	if (!base)
		fatalerror("memory_configure_bank_decrypted called NULL base");

	/* fill in the requested bank entries */
	for (entrynum = startentry; entrynum < startentry + numentries; entrynum++)
		bankdata[banknum].entryd[entrynum] = (UINT8 *)base + (entrynum - startentry) * stride;
}



/*-------------------------------------------------
    memory_set_bank - set the base of a bank
-------------------------------------------------*/

void memory_set_bank(int banknum, int entrynum)
{
	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		fatalerror("memory_set_bank called with invalid bank %d", banknum);
	if (bankdata[banknum].dynamic)
		fatalerror("memory_set_bank called with dynamic bank %d", banknum);
	if (entrynum < 0 || entrynum > MAX_BANK_ENTRIES)
		fatalerror("memory_set_bank called with out-of-range entry %d", entrynum);
	if (!bankdata[banknum].entry[entrynum])
		fatalerror("memory_set_bank called for bank %d with invalid bank entry %d", banknum, entrynum);

	/* set the base */
	bankdata[banknum].curentry = entrynum;
	bank_ptr[banknum] = bankdata[banknum].entry[entrynum];
	bankd_ptr[banknum] = bankdata[banknum].entryd[entrynum];

	/* if we're executing out of this bank, adjust the opbase pointer */
	if (opcode_entry == banknum && cpu_getactivecpu() >= 0)
		force_opbase_update();
}



/*-------------------------------------------------
    memory_get_bank - return the currently
    selected bank
-------------------------------------------------*/

int memory_get_bank(int banknum)
{
	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		fatalerror("memory_get_bank called with invalid bank %d", banknum);
	if (bankdata[banknum].dynamic)
		fatalerror("memory_get_bank called with dynamic bank %d", banknum);
	return bankdata[banknum].curentry;
}



/*-------------------------------------------------
    memory_set_bankptr - set the base of a bank
-------------------------------------------------*/

void memory_set_bankptr(int banknum, void *base)
{
	/* validation checks */
	if (banknum < STATIC_BANK1 || banknum > MAX_EXPLICIT_BANKS || !bankdata[banknum].used)
		fatalerror("memory_set_bankptr called with invalid bank %d", banknum);
	if (bankdata[banknum].dynamic)
		fatalerror("memory_set_bankptr called with dynamic bank %d", banknum);
	if (base == NULL)
		fatalerror("memory_set_bankptr called NULL base");
	if (ALLOW_ONLY_AUTO_MALLOC_BANKS)
		validate_auto_malloc_memory(base, bankdata[banknum].byteend - bankdata[banknum].bytestart + 1);

	/* set the base */
	bank_ptr[banknum] = base;

	/* if we're executing out of this bank, adjust the opbase pointer */
	if (opcode_entry == banknum && cpu_getactivecpu() >= 0)
		force_opbase_update();
}


/*-------------------------------------------------
    memory_set_debugger_access - set debugger access
-------------------------------------------------*/

void memory_set_debugger_access(int debugger)
{
	debugger_access = debugger;
}


/*-------------------------------------------------
    memory_set_log_unmap - sets whether unmapped
    memory accesses should be logged or not
-------------------------------------------------*/

void memory_set_log_unmap(int spacenum, int log)
{
	log_unmap[spacenum] = log;
}


/*-------------------------------------------------
    memory_get_log_unmap - gets whether unmapped
    memory accesses should be logged or not
-------------------------------------------------*/

int memory_get_log_unmap(int spacenum)
{
	return log_unmap[spacenum];
}


/*-------------------------------------------------
    memory_install_readX_handler - install dynamic
    read handler for X-bit case
-------------------------------------------------*/

void *_memory_install_read_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, FPTR handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	if (handler >= STATIC_COUNT)
		fatalerror("fatal: can only use static banks with memory_install_read_handler()");
	install_mem_handler(space, ROW_READ, space->dbits, addrstart, addrend, addrmask, addrmirror, (genf *)(FPTR)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT8 *_memory_install_read8_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read8_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 8, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT16 *_memory_install_read16_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read16_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 16, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT32 *_memory_install_read32_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read32_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 32, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT64 *_memory_install_read64_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read64_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 64, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}


/*-------------------------------------------------
    memory_install_writeX_handler - install dynamic
    write handler for X-bit case
-------------------------------------------------*/

void *_memory_install_write_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, FPTR handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	if (handler >= STATIC_COUNT)
		fatalerror("fatal: can only use static banks with memory_install_write_handler()");
	install_mem_handler(space, ROW_WRITE, space->dbits, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT8 *_memory_install_write8_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, write8_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_WRITE, 8, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT16 *_memory_install_write16_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, write16_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_WRITE, 16, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT32 *_memory_install_write32_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, write32_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_WRITE, 32, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT64 *_memory_install_write64_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, write64_machine_func handler, const char *handler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_WRITE, 64, addrstart, addrend, addrmask, addrmirror, (genf *)handler, Machine, handler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}


/*-------------------------------------------------
    memory_install_readwriteX_handler - install
    dynamic read and write handlers for X-bit case
-------------------------------------------------*/

void *_memory_install_readwrite_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, FPTR rhandler, FPTR whandler, const char *rhandler_name, const char *whandler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	if (rhandler >= STATIC_COUNT || whandler >= STATIC_COUNT)
		fatalerror("fatal: can only use static banks with memory_install_readwrite_handler()");
	install_mem_handler(space, ROW_READ, space->dbits, addrstart, addrend, addrmask, addrmirror, (genf *)(FPTR)rhandler, Machine, rhandler_name);
	install_mem_handler(space, ROW_WRITE, space->dbits, addrstart, addrend, addrmask, addrmirror, (genf *)(FPTR)whandler, Machine, whandler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT8 *_memory_install_readwrite8_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read8_machine_func rhandler, write8_machine_func whandler, const char *rhandler_name, const char *whandler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 8, addrstart, addrend, addrmask, addrmirror, (genf *)rhandler, Machine, rhandler_name);
	install_mem_handler(space, ROW_WRITE, 8, addrstart, addrend, addrmask, addrmirror, (genf *)whandler, Machine, whandler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT16 *_memory_install_readwrite16_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read16_machine_func rhandler, write16_machine_func whandler, const char *rhandler_name, const char *whandler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 16, addrstart, addrend, addrmask, addrmirror, (genf *)rhandler, Machine, rhandler_name);
	install_mem_handler(space, ROW_WRITE, 16, addrstart, addrend, addrmask, addrmirror, (genf *)whandler, Machine, whandler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT32 *_memory_install_readwrite32_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read32_machine_func rhandler, write32_machine_func whandler, const char *rhandler_name, const char *whandler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 32, addrstart, addrend, addrmask, addrmirror, (genf *)rhandler, Machine, rhandler_name);
	install_mem_handler(space, ROW_WRITE, 32, addrstart, addrend, addrmask, addrmirror, (genf *)whandler, Machine, whandler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}

UINT64 *_memory_install_readwrite64_handler(int cpunum, int spacenum, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, read64_machine_func rhandler, write64_machine_func whandler, const char *rhandler_name, const char *whandler_name)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	install_mem_handler(space, ROW_READ, 64, addrstart, addrend, addrmask, addrmirror, (genf *)rhandler, Machine, rhandler_name);
	install_mem_handler(space, ROW_WRITE, 64, addrstart, addrend, addrmask, addrmirror, (genf *)whandler, Machine, whandler_name);
	mem_dump();
	return memory_find_base(cpunum, spacenum, ADDR2BYTE(space, addrstart));
}


/*-------------------------------------------------
    memory_init_cpudata - initialize the cpudata
    structure for each CPU
-------------------------------------------------*/

static void memory_init_cpudata(const machine_config *config)
{
	int cpunum, spacenum;

	/* zap the cpudata structure */
	memset(&cpudata, 0, sizeof(cpudata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
	{
		cpu_type cputype = config->cpu[cpunum].type;
		cpu_data *cpu = &cpudata[cpunum];

		/* get pointers to the CPU's memory region */
		cpu->region = memory_region(REGION_CPU1 + cpunum);
		cpu->regionsize = memory_region_length(REGION_CPU1 + cpunum);

		/* initialize each address space, and build up a mask of spaces */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		{
			addrspace_data *space = &cpu->space[spacenum];
			int entrynum;

			/* determine the address and data bits */
			space->cpunum = cpunum;
			space->spacenum = spacenum;
			space->ashift = cputype_addrbus_shift(cputype, spacenum);
			space->abits = cputype_addrbus_width(cputype, spacenum);
			space->dbits = cputype_databus_width(cputype, spacenum);
			space->addrmask = 0xffffffffUL >> (32 - space->abits);
			space->bytemask = ADDR2BYTE_END(space, space->addrmask);
			space->accessors = memory_get_accessors(spacenum, space->dbits, cputype_endianness(cputype));
			space->map = NULL;

			/* if there's nothing here, just punt */
			if (space->abits == 0)
				continue;
			cpu->spacemask |= 1 << spacenum;

			/* init the static handlers */
			memset(space->read.handlers, 0, sizeof(space->read.handlers));
			memset(space->write.handlers, 0, sizeof(space->write.handlers));
			for (entrynum = 0; entrynum < ENTRY_COUNT; entrynum++)
			{
				space->read.handlers[entrynum].handler.generic = get_static_handler(space->dbits, 0, spacenum, entrynum);
				space->write.handlers[entrynum].handler.generic = get_static_handler(space->dbits, 1, spacenum, entrynum);
			}

			/* make sure we fix up the mask for the unmap handler */
			space->read.handlers[STATIC_UNMAP].bytemask = ~0;
			space->write.handlers[STATIC_UNMAP].bytemask = ~0;

			/* allocate memory */
			space->read.table = malloc_or_die(1 << LEVEL1_BITS);
			space->write.table = malloc_or_die(1 << LEVEL1_BITS);

			/* initialize everything to unmapped */
			memset(space->read.table, STATIC_UNMAP, 1 << LEVEL1_BITS);
			memset(space->write.table, STATIC_UNMAP, 1 << LEVEL1_BITS);
		}

		/* set the RAM/ROM base */
		cpu->op_ram = cpu->op_rom = memory_region(REGION_CPU1 + cpunum);
		cpu->op_mask = cpu->space[ADDRESS_SPACE_PROGRAM].bytemask;
		cpu->op_mem_min = 0;
		cpu->op_mem_max = memory_region_length(REGION_CPU1 + cpunum);
		cpu->opcode_entry = STATIC_UNMAP;
		cpu->opbase = NULL;
	}
}


/*-------------------------------------------------
    memory_init_preflight - verify the memory structs
    and track which banks are referenced
-------------------------------------------------*/

static void memory_init_preflight(const machine_config *config)
{
	int cpunum;

	/* zap the bank data */
	memset(&bankdata, 0, sizeof(bankdata));

	/* loop over CPUs */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
	{
		cpu_data *cpu = &cpudata[cpunum];
		int spacenum;

		/* loop over valid address spaces */
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpu->spacemask & (1 << spacenum))
			{
				addrspace_data *space = &cpu->space[spacenum];
				address_map_entry *entry;
				int entrynum;

				/* allocate the address map */
				space->map = address_map_alloc(config, cpunum, spacenum);

				/* extract global parameters specified by the map */
				space->unmap = (space->map->unmapval == 0) ? 0 : ~0;
				if (space->map->globalmask != 0)
				{
					space->addrmask = space->map->globalmask;
					space->bytemask = ADDR2BYTE_END(space, space->addrmask);
				}

				/* make a pass over the address map, adjusting for the CPU and getting memory pointers */
				for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
				{
					/* computed adjusted addresses first */
					entry->bytestart = entry->addrstart;
					entry->byteend = entry->addrend;
					entry->bytemirror = entry->addrmirror;
					entry->bytemask = entry->addrmask;
					adjust_addresses(space, &entry->bytestart, &entry->byteend, &entry->bytemask, &entry->bytemirror);

					/* if this is a ROM handler without a specified region, attach it to the implicit region */
					if (spacenum == ADDRESS_SPACE_PROGRAM && HANDLER_IS_ROM(entry->read.generic) && entry->region == 0)
					{
						/* make sure it fits within the memory region before doing so, however */
						if (entry->byteend < cpu->regionsize)
						{
							entry->region = REGION_CPU1 + cpunum;
							entry->region_offs = entry->bytestart;
						}
					}

					/* validate adjusted addresses against implicit regions */
					if (entry->region != 0 && entry->share == 0 && entry->baseptr == NULL)
					{
						UINT8 *base = memory_region(entry->region);
						offs_t length = memory_region_length(entry->region);

						/* validate the region */
						if (base == NULL)
							fatalerror("Error: CPU %d space %d memory map entry %X-%X references non-existant region %d", cpunum, spacenum, entry->addrstart, entry->addrend, entry->region);
						if (entry->region_offs + (entry->byteend - entry->bytestart + 1) > length)
							fatalerror("Error: CPU %d space %d memory map entry %X-%X extends beyond region %d size (%X)", cpunum, spacenum, entry->addrstart, entry->addrend, entry->region, length);
					}

					/* convert any region-relative entries to their memory pointers */
					if (entry->region != 0)
						entry->memory = memory_region(entry->region) + entry->region_offs;

					/* assign static banks for explicitly specified entries */
					if (HANDLER_IS_BANK(entry->read.generic))
						bank_assign_static(HANDLER_TO_BANK(entry->read.generic), cpunum, spacenum, ROW_READ, entry->bytestart, entry->byteend);
					if (HANDLER_IS_BANK(entry->write.generic))
						bank_assign_static(HANDLER_TO_BANK(entry->write.generic), cpunum, spacenum, ROW_WRITE, entry->bytestart, entry->byteend);
				}

				/* now loop over all the handlers and enforce the address mask */
				/* we don't loop over map entries because the mask applies to static handlers as well */
				for (entrynum = 0; entrynum < ENTRY_COUNT; entrynum++)
				{
					space->read.handlers[entrynum].bytemask &= space->bytemask;
					space->write.handlers[entrynum].bytemask &= space->bytemask;
				}
			}
	}
}


/*-------------------------------------------------
    memory_init_populate - populate the memory
    mapping tables with entries
-------------------------------------------------*/

static void memory_init_populate(running_machine *machine)
{
	int cpunum, spacenum;

	/* loop over CPUs and address spaces */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				addrspace_data *space = &cpudata[cpunum].space[spacenum];

				/* install the handlers, using the original, unadjusted memory map */
				if (space->map != NULL)
				{
					const address_map_entry *last_entry = NULL;

					while (last_entry != space->map->entrylist)
					{
						const address_map_entry *entry;

						/* find the entry before the last one we processed */
						for (entry = space->map->entrylist; entry->next != last_entry; entry = entry->next) ;
						last_entry = entry;

						if (entry->read.generic != NULL)
						{
							void *object = machine;
							if (entry->read_devtype != NULL)
							{
								object = (void *)device_list_find_by_tag(machine->config->devicelist, entry->read_devtype, entry->read_devtag);
								if (object == NULL)
									fatalerror("Unidentified object in memory map: type=%s tag=%s\n", devtype_name(entry->read_devtype), entry->read_devtag);
							}
							install_mem_handler_private(space, ROW_READ, space->dbits, entry->addrstart, entry->addrend, entry->addrmask, entry->addrmirror, entry->read.generic, object, entry->read_name);
						}
						if (entry->write.generic != NULL)
						{
							void *object = machine;
							if (entry->write_devtype != NULL)
							{
								object = (void *)device_list_find_by_tag(machine->config->devicelist, entry->write_devtype, entry->write_devtag);
								if (object == NULL)
									fatalerror("Unidentified object in memory map: type=%s tag=%s\n", devtype_name(entry->write_devtype), entry->write_devtag);
							}
							install_mem_handler_private(space, ROW_WRITE, space->dbits, entry->addrstart, entry->addrend, entry->addrmask, entry->addrmirror, entry->write.generic, object, entry->write_name);
						}
					}
				}
			}
}


/*-------------------------------------------------
    install_mem_handler_private - wrapper for
    install_mem_handler which is used at
    initialization time and converts RAM/ROM
    banks to dynamically assigned banks
-------------------------------------------------*/

static void install_mem_handler_private(addrspace_data *space, read_or_write readorwrite, int databits, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, genf *handler, void *object, const char *handler_name)
{
	/* translate ROM to RAM/UNMAP here */
	if (HANDLER_IS_ROM(handler))
		handler = (readorwrite == ROW_WRITE) ? (genf *)STATIC_UNMAP : (genf *)SMH_RAM;

	/* assign banks for RAM/ROM areas */
	if (HANDLER_IS_RAM(handler))
	{
		offs_t bytestart = addrstart;
		offs_t byteend = addrend;
		offs_t bytemask = addrmask;
		offs_t bytemirror = addrmirror;

		/* adjust the incoming addresses (temporarily) */
		adjust_addresses(space, &bytestart, &byteend, &bytemask, &bytemirror);

		/* assign a bank to the adjusted addresses */
		handler = (genf *)bank_assign_dynamic(space->cpunum, space->spacenum, readorwrite, bytestart, byteend);
		if (bank_ptr[HANDLER_TO_BANK(handler)] == NULL)
			bank_ptr[HANDLER_TO_BANK(handler)] = memory_find_base(space->cpunum, space->spacenum, bytestart);
	}

	/* then do a normal installation */
	install_mem_handler(space, readorwrite, databits, addrstart, addrend, addrmask, addrmirror, handler, object, handler_name);
}


/*-------------------------------------------------
    install_mem_handler - installs a handler for
    memory operations
-------------------------------------------------*/

static void install_mem_handler(addrspace_data *space, read_or_write readorwrite, int databits, offs_t addrstart, offs_t addrend, offs_t addrmask, offs_t addrmirror, genf *handler, void *object, const char *handler_name)
{
	offs_t lmirrorbit[LEVEL2_BITS], lmirrorbits, hmirrorbit[32 - LEVEL2_BITS], hmirrorbits, lmirrorcount, hmirrorcount;
	table_data *tabledata = (readorwrite == ROW_WRITE) ? &space->write : &space->read;
	offs_t bytestart, byteend, bytemask, bytemirror;
	UINT8 idx, prev_entry = STATIC_INVALID;
	int cur_index, prev_index = 0;
	int i;

	/* sanity check */
	if (HANDLER_IS_ROM(handler) || HANDLER_IS_RAM(handler))
		fatalerror("fatal: install_mem_handler called with ROM or RAM after initialization");
	if (space->dbits != databits)
		fatalerror("fatal: install_mem_handler called with a %d-bit handler for a %d-bit address space", databits, space->dbits);
	if (addrstart > addrend)
		fatalerror("fatal: install_mem_handler called with start greater than end");

	/* adjust the incoming addresses */
	bytestart = addrstart;
	byteend = addrend;
	bytemirror = addrmirror;
	bytemask = addrmask;
	adjust_addresses(space, &bytestart, &byteend, &bytemask, &bytemirror);

	/* if we're installing a new bank, make sure we mark it */
	if (HANDLER_IS_BANK(handler))
		bank_assign_static(HANDLER_TO_BANK(handler), space->cpunum, space->spacenum, readorwrite, bytestart, byteend);

	/* determine the mirror bits */
	hmirrorbits = lmirrorbits = 0;
	for (i = 0; i < LEVEL2_BITS; i++)
		if (bytemirror & (1 << i))
			lmirrorbit[lmirrorbits++] = 1 << i;
	for (i = LEVEL2_BITS; i < 32; i++)
		if (bytemirror & (1 << i))
			hmirrorbit[hmirrorbits++] = 1 << i;

	/* get the final handler index */
	idx = get_handler_index(tabledata->handlers, object, handler, handler_name, bytestart, byteend, bytemask);

	/* loop over mirrors in the level 2 table */
	for (hmirrorcount = 0; hmirrorcount < (1 << hmirrorbits); hmirrorcount++)
	{
		/* compute the base of this mirror */
		offs_t hmirrorbase = 0;
		for (i = 0; i < hmirrorbits; i++)
			if (hmirrorcount & (1 << i))
				hmirrorbase |= hmirrorbit[i];

		/* if this is not our first time through, and the level 2 entry matches the previous
           level 2 entry, just do a quick map and get out; note that this only works for entries
           which don't span multiple level 1 table entries */
		cur_index = LEVEL1_INDEX(bytestart + hmirrorbase);
		if (cur_index == LEVEL1_INDEX(byteend + hmirrorbase))
		{
			if (hmirrorcount != 0 && prev_entry == tabledata->table[cur_index])
			{
				VPRINTF(("Quick mapping subtable at %08X to match subtable at %08X\n", cur_index << LEVEL2_BITS, prev_index << LEVEL2_BITS));

				/* release the subtable if the old value was a subtable */
				if (tabledata->table[cur_index] >= SUBTABLE_BASE)
					subtable_release(tabledata, tabledata->table[cur_index]);

				/* reallocate the subtable if the new value is a subtable */
				if (tabledata->table[prev_index] >= SUBTABLE_BASE)
					subtable_realloc(tabledata, tabledata->table[prev_index]);

				/* set the new value and short-circuit the mapping step */
				tabledata->table[cur_index] = tabledata->table[prev_index];
				continue;
			}
			prev_index = cur_index;
			prev_entry = tabledata->table[cur_index];
		}

		/* loop over mirrors in the level 1 table */
		for (lmirrorcount = 0; lmirrorcount < (1 << lmirrorbits); lmirrorcount++)
		{
			/* compute the base of this mirror */
			offs_t lmirrorbase = hmirrorbase;
			for (i = 0; i < lmirrorbits; i++)
				if (lmirrorcount & (1 << i))
					lmirrorbase |= lmirrorbit[i];

			/* populate the tables */
			populate_table_range(space, readorwrite, bytestart + lmirrorbase, byteend + lmirrorbase, idx);
		}
	}

	/* if this is being installed to a live CPU, update the context */
	if (space->cpunum == cur_context)
		memory_set_context(cur_context);
}


/*-------------------------------------------------
    bank_assign_static - assign and tag a static
    bank
-------------------------------------------------*/

static void bank_assign_static(int banknum, int cpunum, int spacenum, read_or_write readorwrite, offs_t bytestart, offs_t byteend)
{
	bank_info *bank = &bankdata[banknum];

	/* if we're not yet used, fill in the data */
	if (!bank->used)
	{
		/* if we're allowed to, wire up state saving for the entry */
		if (state_save_registration_allowed())
			state_save_register_item("memory", banknum, bank->curentry);

		/* fill in information about the bank */
		bank->used = TRUE;
		bank->dynamic = FALSE;
		bank->cpunum = cpunum;
		bank->spacenum = spacenum;
		bank->bytestart = bytestart;
		bank->byteend = byteend;
		bank->curentry = MAX_BANK_ENTRIES;
	}

	/* update the read/write status of the bank */
	if (readorwrite == ROW_READ)
		bank->read = TRUE;
	else
		bank->write = TRUE;
}


/*-------------------------------------------------
    bank_assign_dynamic - finds a free or exact
    matching bank
-------------------------------------------------*/

static genf *bank_assign_dynamic(int cpunum, int spacenum, read_or_write readorwrite, offs_t bytestart, offs_t byteend)
{
	int banknum;

	/* loop over banks, searching for an exact match or an empty */
	for (banknum = MAX_BANKS; banknum >= 1; banknum--)
	{
		bank_info *bank = &bankdata[banknum];
		if (!bank->used || (bank->dynamic && bank->cpunum == cpunum && bank->spacenum == spacenum && bank->bytestart == bytestart))
		{
			bank->used = TRUE;
			bank->dynamic = TRUE;
			bank->cpunum = cpunum;
			bank->spacenum = spacenum;
			bank->bytestart = bytestart;
			bank->byteend = byteend;
			VPRINTF(("Assigned bank %d to %d,%d,%08X\n", banknum, cpunum, spacenum, bytestart));
			return BANK_TO_HANDLER(banknum);
		}
	}

	/* if we got here, we failed */
	fatalerror("cpu #%d: ran out of banks for RAM/ROM regions!", cpunum);
	return NULL;
}


/*-------------------------------------------------
    get_handler_index - finds the index of a
    handler, or allocates a new one as necessary
-------------------------------------------------*/

static UINT8 get_handler_index(handler_data *table, void *object, genf *handler, const char *handler_name, offs_t bytestart, offs_t byteend, offs_t bytemask)
{
	int entry;

	/* all static handlers are hardcoded */
	if (HANDLER_IS_STATIC(handler))
	{
		entry = (FPTR)handler;

		/* if it is a bank, copy in the relevant information */
		if (HANDLER_IS_BANK(handler))
		{
			handler_data *hdata = &table[entry];
			hdata->bytestart = bytestart;
			hdata->byteend = byteend;
			hdata->bytemask = bytemask;
			hdata->name = handler_name;
		}
		return entry;
	}

	/* otherwise, we have to search */
	for (entry = STATIC_COUNT; entry < SUBTABLE_BASE; entry++)
	{
		handler_data *hdata = &table[entry];

		/* if we hit a NULL hdata, then we need to allocate this one as a new one */
		if (hdata->handler.generic == NULL)
		{
			hdata->handler.generic = handler;
			hdata->bytestart = bytestart;
			hdata->byteend = byteend;
			hdata->bytemask = bytemask;
			hdata->name = handler_name;
			hdata->object = object;
			return entry;
		}

		/* if we find a perfect match, return a duplicate entry */
		if (hdata->handler.generic == handler && hdata->bytestart == bytestart && hdata->bytemask == bytemask && hdata->object == object)
			return entry;
	}
	return 0;
}


/*-------------------------------------------------
    populate_table_range - assign a memory handler
    to a range of addresses
-------------------------------------------------*/

static void populate_table_range(addrspace_data *space, read_or_write readorwrite, offs_t bytestart, offs_t byteend, UINT8 handler)
{
	table_data *tabledata = (readorwrite == ROW_WRITE) ? &space->write : &space->read;
	offs_t l2mask = (1 << LEVEL2_BITS) - 1;
	offs_t l1start = bytestart >> LEVEL2_BITS;
	offs_t l2start = bytestart & l2mask;
	offs_t l1stop = byteend >> LEVEL2_BITS;
	offs_t l2stop = byteend & l2mask;
	offs_t l1index;

	/* sanity check */
	if (bytestart > byteend)
		return;

	/* handle the starting edge if it's not on a block boundary */
	if (l2start != 0)
	{
		UINT8 *subtable = subtable_open(tabledata, l1start);

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
		{
			memset(&subtable[l2start], handler, l2stop - l2start + 1);
			subtable_close(tabledata, l1start);
			return;
		}

		/* otherwise, fill until the end */
		memset(&subtable[l2start], handler, (1 << LEVEL2_BITS) - l2start);
		subtable_close(tabledata, l1start);
		if (l1start != (offs_t)~0) l1start++;
	}

	/* handle the trailing edge if it's not on a block boundary */
	if (l2stop != l2mask)
	{
		UINT8 *subtable = subtable_open(tabledata, l1stop);

		/* fill from the beginning */
		memset(&subtable[0], handler, l2stop + 1);
		subtable_close(tabledata, l1stop);

		/* if the start and stop end within the same block, handle that */
		if (l1start == l1stop)
			return;
		if (l1stop != 0) l1stop--;
	}

	/* now fill in the middle tables */
	for (l1index = l1start; l1index <= l1stop; l1index++)
	{
		/* if we have a subtable here, release it */
		if (tabledata->table[l1index] >= SUBTABLE_BASE)
			subtable_release(tabledata, tabledata->table[l1index]);
		tabledata->table[l1index] = handler;
	}
}


/*-------------------------------------------------
    subtable_alloc - allocate a fresh subtable
    and set its usecount to 1
-------------------------------------------------*/

static UINT8 subtable_alloc(table_data *tabledata)
{
	/* loop */
	while (1)
	{
		UINT8 subindex;

		/* find a subtable with a usecount of 0 */
		for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
			if (tabledata->subtable[subindex].usecount == 0)
			{
				/* if this is past our allocation budget, allocate some more */
				if (subindex >= tabledata->subtable_alloc)
				{
					tabledata->subtable_alloc += SUBTABLE_ALLOC;
					tabledata->table = realloc(tabledata->table, (1 << LEVEL1_BITS) + (tabledata->subtable_alloc << LEVEL2_BITS));
					if (!tabledata->table)
						fatalerror("error: ran out of memory allocating memory subtable");
				}

				/* bump the usecount and return */
				tabledata->subtable[subindex].usecount++;
				return subindex + SUBTABLE_BASE;
			}

		/* merge any subtables we can */
		if (!subtable_merge(tabledata))
			fatalerror("Ran out of subtables!");
	}

}


/*-------------------------------------------------
    subtable_realloc - increment the usecount on
    a subtable
-------------------------------------------------*/

static void subtable_realloc(table_data *tabledata, UINT8 subentry)
{
	UINT8 subindex = subentry - SUBTABLE_BASE;

	/* sanity check */
	if (tabledata->subtable[subindex].usecount <= 0)
		fatalerror("Called subtable_realloc on a table with a usecount of 0");

	/* increment the usecount */
	tabledata->subtable[subindex].usecount++;
}


/*-------------------------------------------------
    subtable_merge - merge any duplicate
    subtables
-------------------------------------------------*/

static int subtable_merge(table_data *tabledata)
{
	int merged = 0;
	UINT8 subindex;

	VPRINTF(("Merging subtables....\n"));

	/* okay, we failed; update all the checksums and merge tables */
	for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
		if (!tabledata->subtable[subindex].checksum_valid && tabledata->subtable[subindex].usecount != 0)
		{
			UINT32 *subtable = (UINT32 *)SUBTABLE_PTR(tabledata, subindex + SUBTABLE_BASE);
			UINT32 checksum = 0;
			int l2index;

			/* update the checksum */
			for (l2index = 0; l2index < (1 << LEVEL2_BITS)/4; l2index++)
				checksum += subtable[l2index];
			tabledata->subtable[subindex].checksum = checksum;
			tabledata->subtable[subindex].checksum_valid = 1;
		}

	/* see if there's a matching checksum */
	for (subindex = 0; subindex < SUBTABLE_COUNT; subindex++)
		if (tabledata->subtable[subindex].usecount != 0)
		{
			UINT8 *subtable = SUBTABLE_PTR(tabledata, subindex + SUBTABLE_BASE);
			UINT32 checksum = tabledata->subtable[subindex].checksum;
			UINT8 sumindex;

			for (sumindex = subindex + 1; sumindex < SUBTABLE_COUNT; sumindex++)
				if (tabledata->subtable[sumindex].usecount != 0 &&
					tabledata->subtable[sumindex].checksum == checksum &&
					!memcmp(subtable, SUBTABLE_PTR(tabledata, sumindex + SUBTABLE_BASE), 1 << LEVEL2_BITS))
				{
					int l1index;

					VPRINTF(("Merging subtable %d and %d....\n", subindex, sumindex));

					/* find all the entries in the L1 tables that pointed to the old one, and point them to the merged table */
					for (l1index = 0; l1index <= (0xffffffffUL >> LEVEL2_BITS); l1index++)
						if (tabledata->table[l1index] == sumindex + SUBTABLE_BASE)
						{
							subtable_release(tabledata, sumindex + SUBTABLE_BASE);
							subtable_realloc(tabledata, subindex + SUBTABLE_BASE);
							tabledata->table[l1index] = subindex + SUBTABLE_BASE;
							merged++;
						}
				}
		}

	return merged;
}


/*-------------------------------------------------
    subtable_release - decrement the usecount on
    a subtable and free it if we're done
-------------------------------------------------*/

static void subtable_release(table_data *tabledata, UINT8 subentry)
{
	UINT8 subindex = subentry - SUBTABLE_BASE;

	/* sanity check */
	if (tabledata->subtable[subindex].usecount <= 0)
		fatalerror("Called subtable_release on a table with a usecount of 0");

	/* decrement the usecount and clear the checksum if we're at 0 */
	tabledata->subtable[subindex].usecount--;
	if (tabledata->subtable[subindex].usecount == 0)
		tabledata->subtable[subindex].checksum = 0;
}


/*-------------------------------------------------
    subtable_open - gain access to a subtable for
    modification
-------------------------------------------------*/

static UINT8 *subtable_open(table_data *tabledata, offs_t l1index)
{
	UINT8 subentry = tabledata->table[l1index];

	/* if we don't have a subtable yet, allocate a new one */
	if (subentry < SUBTABLE_BASE)
	{
		UINT8 newentry = subtable_alloc(tabledata);
		memset(SUBTABLE_PTR(tabledata, newentry), subentry, 1 << LEVEL2_BITS);
		tabledata->table[l1index] = newentry;
		tabledata->subtable[newentry - SUBTABLE_BASE].checksum = (subentry + (subentry << 8) + (subentry << 16) + (subentry << 24)) * ((1 << LEVEL2_BITS)/4);
		subentry = newentry;
	}

	/* if we're sharing this subtable, we also need to allocate a fresh copy */
	else if (tabledata->subtable[subentry - SUBTABLE_BASE].usecount > 1)
	{
		UINT8 newentry = subtable_alloc(tabledata);

		/* allocate may cause some additional merging -- look up the subentry again */
		/* when we're done; it should still require a split */
		subentry = tabledata->table[l1index];
		assert(subentry >= SUBTABLE_BASE);
		assert(tabledata->subtable[subentry - SUBTABLE_BASE].usecount > 1);

		memcpy(SUBTABLE_PTR(tabledata, newentry), SUBTABLE_PTR(tabledata, subentry), 1 << LEVEL2_BITS);
		subtable_release(tabledata, subentry);
		tabledata->table[l1index] = newentry;
		tabledata->subtable[newentry - SUBTABLE_BASE].checksum = tabledata->subtable[subentry - SUBTABLE_BASE].checksum;
		subentry = newentry;
	}

	/* mark the table dirty */
	tabledata->subtable[subentry - SUBTABLE_BASE].checksum_valid = 0;

	/* return the pointer to the subtable */
	return SUBTABLE_PTR(tabledata, subentry);
}


/*-------------------------------------------------
    subtable_close - stop access to a subtable
-------------------------------------------------*/

static void subtable_close(table_data *tabledata, offs_t l1index)
{
	/* defer any merging until we run out of tables */
}


/*-------------------------------------------------
    Return whether a given memory map entry implies
    the need of allocating and registering memory
-------------------------------------------------*/

static int amentry_needs_backing_store(int cpunum, int spacenum, const address_map_entry *entry)
{
	FPTR handler;

	if (entry->baseptr != NULL || entry->baseptroffs_plus1 != 0)
		return 1;

	handler = (FPTR)entry->write.generic;
	if (handler < STATIC_COUNT)
	{
		if (handler != STATIC_INVALID &&
			handler != STATIC_ROM &&
			handler != STATIC_NOP &&
			handler != STATIC_UNMAP)
			return 1;
	}

	handler = (FPTR)entry->read.generic;
	if (handler < STATIC_COUNT)
	{
		if (handler != STATIC_INVALID &&
			(handler < STATIC_BANK1 || handler > STATIC_BANK1 + MAX_BANKS - 1) &&
			(handler != STATIC_ROM || spacenum != ADDRESS_SPACE_PROGRAM || entry->addrstart >= memory_region_length(REGION_CPU1 + cpunum)) &&
			handler != STATIC_NOP &&
			handler != STATIC_UNMAP)
			return 1;
	}

	return 0;
}


/*-------------------------------------------------
    memory_init_allocate - allocate memory for
    CPU address spaces
-------------------------------------------------*/

static void memory_init_allocate(const machine_config *config)
{
	int cpunum, spacenum;

	/* loop over all CPUs and memory spaces */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				addrspace_data *space = &cpudata[cpunum].space[spacenum];
				address_map_entry *unassigned = NULL;
				address_map_entry *entry;
				memory_block *prev_memblock_head = memory_block_list;
				memory_block *memblock;

				/* make a first pass over the memory map and track blocks with hardcoded pointers */
				/* we do this to make sure they are found by memory_find_base first */
				for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
					if (entry->memory != NULL)
						allocate_memory_block(cpunum, spacenum, entry->bytestart, entry->byteend, entry->memory);

				/* loop over all blocks just allocated and assign pointers from them */
				for (memblock = memory_block_list; memblock != prev_memblock_head; memblock = memblock->next)
					unassigned = assign_intersecting_blocks(space, memblock->bytestart, memblock->byteend, memblock->data);

				/* if we don't have an unassigned pointer yet, try to find one */
				if (unassigned == NULL)
					unassigned = assign_intersecting_blocks(space, ~0, 0, NULL);

				/* loop until we've assigned all memory in this space */
				while (unassigned != NULL)
				{
					offs_t curbytestart, curbyteend;
					int changed;
					void *block;

					/* work in MEMORY_BLOCK_CHUNK-sized chunks */
					offs_t curblockstart = unassigned->bytestart / MEMORY_BLOCK_CHUNK;
					offs_t curblockend = unassigned->byteend / MEMORY_BLOCK_CHUNK;

					/* loop while we keep finding unassigned blocks in neighboring MEMORY_BLOCK_CHUNK chunks */
					do
					{
						changed = FALSE;

						/* scan for unmapped blocks in the adjusted map */
						for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
							if (entry->memory == NULL && entry != unassigned && amentry_needs_backing_store(cpunum, spacenum, entry))
							{
								offs_t blockstart, blockend;

								/* get block start/end blocks for this block */
								blockstart = entry->bytestart / MEMORY_BLOCK_CHUNK;
								blockend = entry->byteend / MEMORY_BLOCK_CHUNK;

								/* if we intersect or are adjacent, adjust the start/end */
								if (blockstart <= curblockend + 1 && blockend >= curblockstart - 1)
								{
									if (blockstart < curblockstart)
										curblockstart = blockstart, changed = TRUE;
									if (blockend > curblockend)
										curblockend = blockend, changed = TRUE;
								}
							}
					} while (changed);

					/* we now have a block to allocate; do it */
					curbytestart = curblockstart * MEMORY_BLOCK_CHUNK;
					curbyteend = curblockend * MEMORY_BLOCK_CHUNK + (MEMORY_BLOCK_CHUNK - 1);
					block = allocate_memory_block(cpunum, spacenum, curbytestart, curbyteend, NULL);

					/* assign memory that intersected the new block */
					unassigned = assign_intersecting_blocks(space, curbytestart, curbyteend, block);
				}
			}
}


/*-------------------------------------------------
    allocate_memory_block - allocate a single
    memory block of data
-------------------------------------------------*/

static void *allocate_memory_block(int cpunum, int spacenum, offs_t bytestart, offs_t byteend, void *memory)
{
	int allocatemem = (memory == NULL);
	memory_block *block;
	size_t bytestoalloc;
	int region;

	VPRINTF(("allocate_memory_block(%d,%d,%08X,%08X,%p)\n", cpunum, spacenum, bytestart, byteend, memory));

	/* determine how much memory to allocate for this */
	bytestoalloc = sizeof(*block);
	if (allocatemem)
		bytestoalloc += byteend - bytestart + 1;

	/* allocate and clear the memory */
	block = malloc_or_die(bytestoalloc);
	memset(block, 0, bytestoalloc);
	if (allocatemem)
		memory = block + 1;

	/* register for saving, but only if we're not part of a memory region */
	for (region = 0; region < MAX_MEMORY_REGIONS; region++)
	{
		UINT8 *region_base = memory_region(region);
		UINT32 region_length = memory_region_length(region);
		if (region_base != NULL && region_length != 0 && (UINT8 *)memory >= region_base && ((UINT8 *)memory + (byteend - bytestart + 1)) < region_base + region_length)
		{
			VPRINTF(("skipping save of this memory block as it is covered by a memory region\n"));
			break;
		}
	}
	if (region == MAX_MEMORY_REGIONS)
		register_for_save(cpunum, spacenum, bytestart, memory, byteend - bytestart + 1);

	/* fill in the tracking block */
	block->cpunum = cpunum;
	block->spacenum = spacenum;
	block->isallocated = allocatemem;
	block->bytestart = bytestart;
	block->byteend = byteend;
	block->data = memory;

	/* attach us to the head of the list */
	block->next = memory_block_list;
	memory_block_list = block;

	return memory;
}


/*-------------------------------------------------
    register_for_save - register a block of
    memory for save states
-------------------------------------------------*/

static void register_for_save(int cpunum, int spacenum, offs_t bytestart, void *base, size_t numbytes)
{
	int bytes_per_element = cpudata[cpunum].space[spacenum].dbits/8;
	char name[256];

	sprintf(name, "%d.%08x-%08x", spacenum, bytestart, (int)(bytestart + numbytes - 1));
	state_save_register_memory("memory", cpunum, name, base, bytes_per_element, (UINT32)numbytes / bytes_per_element);
}


/*-------------------------------------------------
    assign_intersecting_blocks - find all
    intersecting blocks and assign their pointers
-------------------------------------------------*/

static address_map_entry *assign_intersecting_blocks(addrspace_data *space, offs_t bytestart, offs_t byteend, UINT8 *base)
{
	address_map_entry *entry, *unassigned = NULL;

	/* loop over the adjusted map and assign memory to any blocks we can */
	for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
	{
		/* if we haven't assigned this block yet, do it against the last block */
		if (entry->memory == NULL)
		{
			/* inherit shared pointers first */
			if (entry->share != 0 && shared_ptr[entry->share] != NULL)
			{
				entry->memory = shared_ptr[entry->share];
 				VPRINTF(("memory range %08X-%08X -> shared_ptr[%d] [%p]\n", entry->addrstart, entry->addrend, entry->share, entry->memory));
 			}

			/* otherwise, look for a match in this block */
			else if (entry->bytestart >= bytestart && entry->byteend <= byteend)
			{
				entry->memory = base + (entry->bytestart - bytestart);
				VPRINTF(("memory range %08X-%08X -> found in block from %08X-%08X [%p]\n", entry->addrstart, entry->addrend, bytestart, byteend, entry->memory));
			}
		}

		/* if we're the first match on a shared pointer, assign it now */
		if (entry->memory != NULL && entry->share && shared_ptr[entry->share] == NULL)
			shared_ptr[entry->share] = entry->memory;

		/* keep track of the first unassigned entry */
		if (entry->memory == NULL && unassigned == NULL && amentry_needs_backing_store(space->cpunum, space->spacenum, entry))
			unassigned = entry;
	}

	return unassigned;
}


/*-------------------------------------------------
    reattach_banks - reconnect banks after a load
-------------------------------------------------*/

static void reattach_banks(void)
{
	int banknum;

	/* once this is done, find the starting bases for the banks */
	for (banknum = 1; banknum <= MAX_BANKS; banknum++)
		if (bankdata[banknum].used && !bankdata[banknum].dynamic)
		{
			/* if this entry has a changed entry, set the appropriate pointer */
			if (bankdata[banknum].curentry != MAX_BANK_ENTRIES)
				bank_ptr[banknum] = bankdata[banknum].entry[bankdata[banknum].curentry];
		}
}


/*-------------------------------------------------
    memory_init_locate - find all the requested pointers
    into the final allocated memory
-------------------------------------------------*/

static void memory_init_locate(void)
{
	int cpunum, spacenum, banknum;

	/* loop over CPUs and address spaces */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				addrspace_data *space = &cpudata[cpunum].space[spacenum];
				const address_map_entry *entry;

				/* fill in base/size entries, and handle shared memory */
				for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
				{
					/* assign base/size values */
					if (entry->baseptr != NULL)
						*entry->baseptr = entry->memory;
					if (entry->baseptroffs_plus1 != 0)
						*(void **)((UINT8 *)Machine->driver_data + entry->baseptroffs_plus1 - 1) = entry->memory;
					if (entry->sizeptr != NULL)
						*entry->sizeptr = entry->byteend - entry->bytestart + 1;
					if (entry->sizeptroffs_plus1 != 0)
						*(size_t *)((UINT8 *)Machine->driver_data + entry->sizeptroffs_plus1 - 1) = entry->byteend - entry->bytestart + 1;
				}
			}

	/* once this is done, find the starting bases for the banks */
	for (banknum = 1; banknum <= MAX_BANKS; banknum++)
		if (bankdata[banknum].used)
		{
			address_map_entry *entry;

			/* set the initial bank pointer */
			for (entry = cpudata[bankdata[banknum].cpunum].space[bankdata[banknum].spacenum].map->entrylist; entry != NULL; entry = entry->next)
				if (entry->bytestart == bankdata[banknum].bytestart)
				{
					bank_ptr[banknum] = entry->memory;
	 				VPRINTF(("assigned bank %d pointer to memory from range %08X-%08X [%p]\n", banknum, entry->addrstart, entry->addrend, entry->memory));
					break;
				}

			/* if the entry was set ahead of time, override the automatically found pointer */
			if (!bankdata[banknum].dynamic && bankdata[banknum].curentry != MAX_BANK_ENTRIES)
				bank_ptr[banknum] = bankdata[banknum].entry[bankdata[banknum].curentry];
		}

	/* request a callback to fix up the banks when done */
	state_save_register_func_postload(reattach_banks);
}


/*-------------------------------------------------
    memory_find_base - return a pointer to the
    base of RAM associated with the given CPU
    and offset
-------------------------------------------------*/

static void *memory_find_base(int cpunum, int spacenum, offs_t byteaddress)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	address_map_entry *entry;
	memory_block *block;

	VPRINTF(("memory_find_base(%d,%d,%08X) -> ", cpunum, spacenum, byteaddress));

	/* look in the address map first */
	for (entry = space->map->entrylist; entry != NULL; entry = entry->next)
	{
		offs_t maskoffs = byteaddress & entry->bytemask;
		if (maskoffs >= entry->bytestart && maskoffs <= entry->byteend)
		{
			VPRINTF(("found in entry %08X-%08X [%p]\n", entry->addrstart, entry->addrend, (UINT8 *)entry->memory + (maskoffs - entry->bytestart)));
			return (UINT8 *)entry->memory + (maskoffs - entry->bytestart);
		}
	}

	/* if not found there, look in the allocated blocks */
	for (block = memory_block_list; block != NULL; block = block->next)
		if (block->cpunum == cpunum && block->spacenum == spacenum && block->bytestart <= byteaddress && block->byteend > byteaddress)
		{
			VPRINTF(("found in allocated memory block %08X-%08X [%p]\n", block->bytestart, block->byteend, block->data + (byteaddress - block->bytestart)));
			return block->data + byteaddress - block->bytestart;
		}

	VPRINTF(("did not find\n"));
	return NULL;
}


/*-------------------------------------------------
    PERFORM_LOOKUP - common lookup procedure
-------------------------------------------------*/

#define PERFORM_LOOKUP(lookup,handlers,spacenum,extraand)								\
	/* perform lookup */																\
	address &= active_address_space[spacenum].bytemask & extraand;						\
	entry = active_address_space[spacenum].lookup[LEVEL1_INDEX(address)];				\
	if (entry >= SUBTABLE_BASE)															\
		entry = active_address_space[spacenum].lookup[LEVEL2_INDEX(entry,address)];		\
	handler = &active_address_space[spacenum].handlers[entry];							\


/*-------------------------------------------------
    READBYTE - generic byte-sized read handler
-------------------------------------------------*/

#define READBYTE8(name,spacenum)														\
UINT8 name(offs_t original_address)														\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~0);								\
	DEBUG_HOOK_READ(spacenum, address, 0xff);											\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM) 															\
		MEMREADEND(bank_ptr[entry][address]);											\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler8)(handler->object, address));		\
}																						\

#define READBYTE(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)		\
UINT8 name(offs_t original_address)														\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~0);								\
	DEBUG_HOOK_READ(spacenum, address - xormacro(shiftbytes), (masktype)0xff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(bank_ptr[entry][xormacro(address)]);									\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*handler->handler.read.handlertype)(handler->object, address >> (ignorebits), ~((masktype)0xff << shift)) >> shift);\
	}																					\
}																						\

#define READBYTE16BE(name,space)	READBYTE(name,space,BYTE_XOR_BE, mhandler16,1,~address & 1,UINT16)
#define READBYTE16LE(name,space)	READBYTE(name,space,BYTE_XOR_LE, mhandler16,1, address & 1,UINT16)
#define READBYTE32BE(name,space)	READBYTE(name,space,BYTE4_XOR_BE,mhandler32,2,~address & 3,UINT32)
#define READBYTE32LE(name,space)	READBYTE(name,space,BYTE4_XOR_LE,mhandler32,2, address & 3,UINT32)
#define READBYTE64BE(name,space)	READBYTE(name,space,BYTE8_XOR_BE,mhandler64,3,~address & 7,UINT64)
#define READBYTE64LE(name,space)	READBYTE(name,space,BYTE8_XOR_LE,mhandler64,3, address & 7,UINT64)


/*-------------------------------------------------
    READWORD - generic word-sized read handler
    (16-bit, 32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define READWORD16(name,spacenum)														\
UINT16 name(offs_t original_address)													\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~1);								\
	DEBUG_HOOK_READ(spacenum, address, 0xffff);											\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT16 *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler16)(handler->object, address >> 1, 0));\
}																						\

#define READWORD(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)		\
UINT16 name(offs_t original_address)													\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~1);								\
	DEBUG_HOOK_READ(spacenum, address - xormacro(shiftbytes), (masktype)0xffff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT16 *)&bank_ptr[entry][xormacro(address)]);						\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*handler->handler.read.handlertype)(handler->object, address >> (ignorebits), ~((masktype)0xffff << shift)) >> shift);\
	}																					\
}																						\

#define READWORD32BE(name,space)	READWORD(name,space,WORD_XOR_BE, mhandler32,2,~address & 2,UINT32)
#define READWORD32LE(name,space)	READWORD(name,space,WORD_XOR_LE, mhandler32,2, address & 2,UINT32)
#define READWORD64BE(name,space)	READWORD(name,space,WORD2_XOR_BE,mhandler64,3,~address & 6,UINT64)
#define READWORD64LE(name,space)	READWORD(name,space,WORD2_XOR_LE,mhandler64,3, address & 6,UINT64)


/*-------------------------------------------------
    READDWORD - generic dword-sized read handler
    (32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define READDWORD32(name,spacenum)														\
UINT32 name(offs_t original_address)													\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~3);								\
	DEBUG_HOOK_READ(spacenum, address, 0xffffffff);										\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT32 *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler32)(handler->object, address >> 2, 0));\
}																						\

#define READMASKED32(name,spacenum)														\
UINT32 name(offs_t original_address, UINT32 mem_mask)									\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~3);								\
	DEBUG_HOOK_READ(spacenum, address, ~mem_mask);										\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT32 *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler32)(handler->object, address >> 2, mem_mask));\
}																						\

#define READDWORD(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)	\
UINT32 name(offs_t original_address)													\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~3);								\
	DEBUG_HOOK_READ(spacenum, address - xormacro(shiftbytes), (masktype)0xffffffff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT32 *)&bank_ptr[entry][xormacro(address)]);						\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMREADEND((*handler->handler.read.handlertype)(handler->object, address >> (ignorebits), ~((masktype)0xffffffff << shift)) >> shift);\
	}																					\
}																						\

#define READDWORD64BE(name,space)	READDWORD(name,space,DWORD_XOR_BE,mhandler64,3,~address & 4,UINT64)
#define READDWORD64LE(name,space)	READDWORD(name,space,DWORD_XOR_LE,mhandler64,3, address & 4,UINT64)


/*-------------------------------------------------
    READQWORD - generic qword-sized read handler
    (64-bit aligned only!)
-------------------------------------------------*/

#define READQWORD64(name,spacenum)														\
UINT64 name(offs_t original_address)													\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~7);								\
	DEBUG_HOOK_READ(spacenum, address, ~(UINT64)0);										\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT64 *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler64)(handler->object, address >> 3, 0));\
}																						\

#define READMASKED64(name,spacenum)														\
UINT64 name(offs_t original_address, UINT64 mem_mask)									\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMREADSTART();																		\
	PERFORM_LOOKUP(readlookup,readhandlers,spacenum,~7);								\
	DEBUG_HOOK_READ(spacenum, address, ~mem_mask);										\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMREADEND(*(UINT64 *)&bank_ptr[entry][address]);								\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMREADEND((*handler->handler.read.mhandler64)(handler->object, address >> 3, mem_mask));\
}																						\


/*-------------------------------------------------
    WRITEBYTE - generic byte-sized write handler
-------------------------------------------------*/

#define WRITEBYTE8(name,spacenum)														\
void name(offs_t original_address, UINT8 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~0);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, 0xff);									\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(bank_ptr[entry][address] = data);									\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler8)(handler->object, address, data));\
}																						\

#define WRITEBYTE(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)	\
void name(offs_t original_address, UINT8 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~0);								\
	DEBUG_HOOK_WRITE(spacenum, address - xormacro(shiftbytes), (masktype)data << (8 * (shiftbytes)), (masktype)0xff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(bank_ptr[entry][xormacro(address)] = data);							\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*handler->handler.write.handlertype)(handler->object, address >> (ignorebits), (masktype)data << shift, ~((masktype)0xff << shift)));\
	}																					\
}																						\

#define WRITEBYTE16BE(name,space)	WRITEBYTE(name,space,BYTE_XOR_BE, mhandler16,1,~address & 1,UINT16)
#define WRITEBYTE16LE(name,space)	WRITEBYTE(name,space,BYTE_XOR_LE, mhandler16,1, address & 1,UINT16)
#define WRITEBYTE32BE(name,space)	WRITEBYTE(name,space,BYTE4_XOR_BE,mhandler32,2,~address & 3,UINT32)
#define WRITEBYTE32LE(name,space)	WRITEBYTE(name,space,BYTE4_XOR_LE,mhandler32,2, address & 3,UINT32)
#define WRITEBYTE64BE(name,space)	WRITEBYTE(name,space,BYTE8_XOR_BE,mhandler64,3,~address & 7,UINT64)
#define WRITEBYTE64LE(name,space)	WRITEBYTE(name,space,BYTE8_XOR_LE,mhandler64,3, address & 7,UINT64)


/*-------------------------------------------------
    WRITEWORD - generic word-sized write handler
    (16-bit, 32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define WRITEWORD16(name,spacenum)														\
void name(offs_t original_address, UINT16 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~1);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, 0xffff);									\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(*(UINT16 *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler16)(handler->object, address >> 1, data, 0));\
}																						\

#define WRITEWORD(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)	\
void name(offs_t original_address, UINT16 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~1);								\
	DEBUG_HOOK_WRITE(spacenum, address - xormacro(shiftbytes), (masktype)data << (8 * (shiftbytes)), (masktype)0xffff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(*(UINT16 *)&bank_ptr[entry][xormacro(address)] = data);				\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*handler->handler.write.handlertype)(handler->object, address >> (ignorebits), (masktype)data << shift, ~((masktype)0xffff << shift)));\
	}																					\
}																						\

#define WRITEWORD32BE(name,space)	WRITEWORD(name,space,WORD_XOR_BE, mhandler32,2,~address & 2,UINT32)
#define WRITEWORD32LE(name,space)	WRITEWORD(name,space,WORD_XOR_LE, mhandler32,2, address & 2,UINT32)
#define WRITEWORD64BE(name,space)	WRITEWORD(name,space,WORD2_XOR_BE,mhandler64,3,~address & 6,UINT64)
#define WRITEWORD64LE(name,space)	WRITEWORD(name,space,WORD2_XOR_LE,mhandler64,3, address & 6,UINT64)


/*-------------------------------------------------
    WRITEDWORD - dword-sized write handler
    (32-bit and 64-bit aligned only!)
-------------------------------------------------*/

#define WRITEDWORD32(name,spacenum)														\
void name(offs_t original_address, UINT32 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~3);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, 0xffffffff);								\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(*(UINT32 *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler32)(handler->object, address >> 2, data, 0));\
}																						\

#define WRITEMASKED32(name,spacenum)													\
void name(offs_t original_address, UINT32 data, UINT32 mem_mask)						\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~3);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, ~mem_mask);								\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
	{																					\
		UINT32 *dest = (UINT32 *)&bank_ptr[entry][address];								\
		MEMWRITEEND(*dest = (*dest & mem_mask) | (data & ~mem_mask));					\
	}																					\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler32)(handler->object, address >> 2, data, mem_mask));\
}																						\

#define WRITEDWORD(name,spacenum,xormacro,handlertype,ignorebits,shiftbytes,masktype)	\
void name(offs_t original_address, UINT32 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~3);								\
	DEBUG_HOOK_WRITE(spacenum, address - xormacro(shiftbytes), (masktype)data << (8 * (shiftbytes)), (masktype)0xffffffff << (8 * (shiftbytes)));	\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(*(UINT32 *)&bank_ptr[entry][xormacro(address)] = data);				\
																						\
	/* fall back to the handler */														\
	else																				\
	{																					\
		int shift = 8 * (shiftbytes);													\
		MEMWRITEEND((*handler->handler.write.handlertype)(handler->object, address >> (ignorebits), (masktype)data << shift, ~((masktype)0xffffffff << shift)));\
	}																					\
}																						\

#define WRITEDWORD64BE(name,space)	WRITEDWORD(name,space,DWORD_XOR_BE,mhandler64,3,~address & 4,UINT64)
#define WRITEDWORD64LE(name,space)	WRITEDWORD(name,space,DWORD_XOR_LE,mhandler64,3, address & 4,UINT64)


/*-------------------------------------------------
    WRITEQWORD - qword-sized write handler
    (64-bit aligned only!)
-------------------------------------------------*/

#define WRITEQWORD64(name,spacenum)														\
void name(offs_t original_address, UINT64 data)											\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~7);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, ~(UINT64)0);								\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
		MEMWRITEEND(*(UINT64 *)&bank_ptr[entry][address] = data);						\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler64)(handler->object, address >> 3, data, 0));\
}																						\

#define WRITEMASKED64(name,spacenum)													\
void name(offs_t original_address, UINT64 data, UINT64 mem_mask)						\
{																						\
	offs_t address = original_address;													\
	const handler_data *handler;														\
	UINT32 entry;																		\
	MEMWRITESTART();																	\
	PERFORM_LOOKUP(writelookup,writehandlers,spacenum,~7);								\
	DEBUG_HOOK_WRITE(spacenum, address, data, ~mem_mask);								\
																						\
	/* handle banks inline */															\
	address = (address - handler->bytestart) & handler->bytemask;						\
	if (entry < STATIC_RAM)																\
	{																					\
		UINT64 *dest = (UINT64 *)&bank_ptr[entry][address];								\
		MEMWRITEEND(*dest = (*dest & mem_mask) | (data & ~mem_mask));					\
	}																					\
																						\
	/* fall back to the handler */														\
	else																				\
		MEMWRITEEND((*handler->handler.write.mhandler64)(handler->object, address >> 3, data, mem_mask));\
}																						\


/*-------------------------------------------------
    Program memory handlers
-------------------------------------------------*/

     READBYTE8(program_read_byte_8,      ADDRESS_SPACE_PROGRAM)
    WRITEBYTE8(program_write_byte_8,     ADDRESS_SPACE_PROGRAM)

  READBYTE16BE(program_read_byte_16be,   ADDRESS_SPACE_PROGRAM)
    READWORD16(program_read_word_16be,   ADDRESS_SPACE_PROGRAM)
 WRITEBYTE16BE(program_write_byte_16be,  ADDRESS_SPACE_PROGRAM)
   WRITEWORD16(program_write_word_16be,  ADDRESS_SPACE_PROGRAM)

  READBYTE16LE(program_read_byte_16le,   ADDRESS_SPACE_PROGRAM)
    READWORD16(program_read_word_16le,   ADDRESS_SPACE_PROGRAM)
 WRITEBYTE16LE(program_write_byte_16le,  ADDRESS_SPACE_PROGRAM)
   WRITEWORD16(program_write_word_16le,  ADDRESS_SPACE_PROGRAM)

  READBYTE32BE(program_read_byte_32be,   ADDRESS_SPACE_PROGRAM)
  READWORD32BE(program_read_word_32be,   ADDRESS_SPACE_PROGRAM)
   READDWORD32(program_read_dword_32be,  ADDRESS_SPACE_PROGRAM)
  READMASKED32(program_read_masked_32be, ADDRESS_SPACE_PROGRAM)
 WRITEBYTE32BE(program_write_byte_32be,  ADDRESS_SPACE_PROGRAM)
 WRITEWORD32BE(program_write_word_32be,  ADDRESS_SPACE_PROGRAM)
  WRITEDWORD32(program_write_dword_32be, ADDRESS_SPACE_PROGRAM)
 WRITEMASKED32(program_write_masked_32be,ADDRESS_SPACE_PROGRAM)

  READBYTE32LE(program_read_byte_32le,   ADDRESS_SPACE_PROGRAM)
  READWORD32LE(program_read_word_32le,   ADDRESS_SPACE_PROGRAM)
   READDWORD32(program_read_dword_32le,  ADDRESS_SPACE_PROGRAM)
  READMASKED32(program_read_masked_32le, ADDRESS_SPACE_PROGRAM)
 WRITEBYTE32LE(program_write_byte_32le,  ADDRESS_SPACE_PROGRAM)
 WRITEWORD32LE(program_write_word_32le,  ADDRESS_SPACE_PROGRAM)
  WRITEDWORD32(program_write_dword_32le, ADDRESS_SPACE_PROGRAM)
 WRITEMASKED32(program_write_masked_32le,ADDRESS_SPACE_PROGRAM)

  READBYTE64BE(program_read_byte_64be,   ADDRESS_SPACE_PROGRAM)
  READWORD64BE(program_read_word_64be,   ADDRESS_SPACE_PROGRAM)
 READDWORD64BE(program_read_dword_64be,  ADDRESS_SPACE_PROGRAM)
   READQWORD64(program_read_qword_64be,  ADDRESS_SPACE_PROGRAM)
  READMASKED64(program_read_masked_64be, ADDRESS_SPACE_PROGRAM)
 WRITEBYTE64BE(program_write_byte_64be,  ADDRESS_SPACE_PROGRAM)
 WRITEWORD64BE(program_write_word_64be,  ADDRESS_SPACE_PROGRAM)
WRITEDWORD64BE(program_write_dword_64be, ADDRESS_SPACE_PROGRAM)
  WRITEQWORD64(program_write_qword_64be, ADDRESS_SPACE_PROGRAM)
 WRITEMASKED64(program_write_masked_64be,ADDRESS_SPACE_PROGRAM)

  READBYTE64LE(program_read_byte_64le,   ADDRESS_SPACE_PROGRAM)
  READWORD64LE(program_read_word_64le,   ADDRESS_SPACE_PROGRAM)
 READDWORD64LE(program_read_dword_64le,  ADDRESS_SPACE_PROGRAM)
   READQWORD64(program_read_qword_64le,  ADDRESS_SPACE_PROGRAM)
  READMASKED64(program_read_masked_64le, ADDRESS_SPACE_PROGRAM)
 WRITEBYTE64LE(program_write_byte_64le,  ADDRESS_SPACE_PROGRAM)
 WRITEWORD64LE(program_write_word_64le,  ADDRESS_SPACE_PROGRAM)
WRITEDWORD64LE(program_write_dword_64le, ADDRESS_SPACE_PROGRAM)
  WRITEQWORD64(program_write_qword_64le, ADDRESS_SPACE_PROGRAM)
 WRITEMASKED64(program_write_masked_64le,ADDRESS_SPACE_PROGRAM)


/*-------------------------------------------------
    Data memory handlers
-------------------------------------------------*/

     READBYTE8(data_read_byte_8,      ADDRESS_SPACE_DATA)
    WRITEBYTE8(data_write_byte_8,     ADDRESS_SPACE_DATA)

  READBYTE16BE(data_read_byte_16be,   ADDRESS_SPACE_DATA)
    READWORD16(data_read_word_16be,   ADDRESS_SPACE_DATA)
 WRITEBYTE16BE(data_write_byte_16be,  ADDRESS_SPACE_DATA)
   WRITEWORD16(data_write_word_16be,  ADDRESS_SPACE_DATA)

  READBYTE16LE(data_read_byte_16le,   ADDRESS_SPACE_DATA)
    READWORD16(data_read_word_16le,   ADDRESS_SPACE_DATA)
 WRITEBYTE16LE(data_write_byte_16le,  ADDRESS_SPACE_DATA)
   WRITEWORD16(data_write_word_16le,  ADDRESS_SPACE_DATA)

  READBYTE32BE(data_read_byte_32be,   ADDRESS_SPACE_DATA)
  READWORD32BE(data_read_word_32be,   ADDRESS_SPACE_DATA)
   READDWORD32(data_read_dword_32be,  ADDRESS_SPACE_DATA)
  READMASKED32(data_read_masked_32be, ADDRESS_SPACE_DATA)
 WRITEBYTE32BE(data_write_byte_32be,  ADDRESS_SPACE_DATA)
 WRITEWORD32BE(data_write_word_32be,  ADDRESS_SPACE_DATA)
  WRITEDWORD32(data_write_dword_32be, ADDRESS_SPACE_DATA)
 WRITEMASKED32(data_write_masked_32be,ADDRESS_SPACE_DATA)

  READBYTE32LE(data_read_byte_32le,   ADDRESS_SPACE_DATA)
  READWORD32LE(data_read_word_32le,   ADDRESS_SPACE_DATA)
   READDWORD32(data_read_dword_32le,  ADDRESS_SPACE_DATA)
  READMASKED32(data_read_masked_32le, ADDRESS_SPACE_DATA)
 WRITEBYTE32LE(data_write_byte_32le,  ADDRESS_SPACE_DATA)
 WRITEWORD32LE(data_write_word_32le,  ADDRESS_SPACE_DATA)
  WRITEDWORD32(data_write_dword_32le, ADDRESS_SPACE_DATA)
 WRITEMASKED32(data_write_masked_32le,ADDRESS_SPACE_DATA)

  READBYTE64BE(data_read_byte_64be,   ADDRESS_SPACE_DATA)
  READWORD64BE(data_read_word_64be,   ADDRESS_SPACE_DATA)
 READDWORD64BE(data_read_dword_64be,  ADDRESS_SPACE_DATA)
   READQWORD64(data_read_qword_64be,  ADDRESS_SPACE_DATA)
  READMASKED64(data_read_masked_64be, ADDRESS_SPACE_DATA)
 WRITEBYTE64BE(data_write_byte_64be,  ADDRESS_SPACE_DATA)
 WRITEWORD64BE(data_write_word_64be,  ADDRESS_SPACE_DATA)
WRITEDWORD64BE(data_write_dword_64be, ADDRESS_SPACE_DATA)
  WRITEQWORD64(data_write_qword_64be, ADDRESS_SPACE_DATA)
 WRITEMASKED64(data_write_masked_64be,ADDRESS_SPACE_DATA)

  READBYTE64LE(data_read_byte_64le,   ADDRESS_SPACE_DATA)
  READWORD64LE(data_read_word_64le,   ADDRESS_SPACE_DATA)
 READDWORD64LE(data_read_dword_64le,  ADDRESS_SPACE_DATA)
   READQWORD64(data_read_qword_64le,  ADDRESS_SPACE_DATA)
  READMASKED64(data_read_masked_64le, ADDRESS_SPACE_DATA)
 WRITEBYTE64LE(data_write_byte_64le,  ADDRESS_SPACE_DATA)
 WRITEWORD64LE(data_write_word_64le,  ADDRESS_SPACE_DATA)
WRITEDWORD64LE(data_write_dword_64le, ADDRESS_SPACE_DATA)
  WRITEQWORD64(data_write_qword_64le, ADDRESS_SPACE_DATA)
 WRITEMASKED64(data_write_masked_64le,ADDRESS_SPACE_DATA)


/*-------------------------------------------------
    I/O memory handlers
-------------------------------------------------*/

     READBYTE8(io_read_byte_8,      ADDRESS_SPACE_IO)
    WRITEBYTE8(io_write_byte_8,     ADDRESS_SPACE_IO)

  READBYTE16BE(io_read_byte_16be,   ADDRESS_SPACE_IO)
    READWORD16(io_read_word_16be,   ADDRESS_SPACE_IO)
 WRITEBYTE16BE(io_write_byte_16be,  ADDRESS_SPACE_IO)
   WRITEWORD16(io_write_word_16be,  ADDRESS_SPACE_IO)

  READBYTE16LE(io_read_byte_16le,   ADDRESS_SPACE_IO)
    READWORD16(io_read_word_16le,   ADDRESS_SPACE_IO)
 WRITEBYTE16LE(io_write_byte_16le,  ADDRESS_SPACE_IO)
   WRITEWORD16(io_write_word_16le,  ADDRESS_SPACE_IO)

  READBYTE32BE(io_read_byte_32be,   ADDRESS_SPACE_IO)
  READWORD32BE(io_read_word_32be,   ADDRESS_SPACE_IO)
   READDWORD32(io_read_dword_32be,  ADDRESS_SPACE_IO)
  READMASKED32(io_read_masked_32be, ADDRESS_SPACE_IO)
 WRITEBYTE32BE(io_write_byte_32be,  ADDRESS_SPACE_IO)
 WRITEWORD32BE(io_write_word_32be,  ADDRESS_SPACE_IO)
  WRITEDWORD32(io_write_dword_32be, ADDRESS_SPACE_IO)
 WRITEMASKED32(io_write_masked_32be,ADDRESS_SPACE_IO)

  READBYTE32LE(io_read_byte_32le,   ADDRESS_SPACE_IO)
  READWORD32LE(io_read_word_32le,   ADDRESS_SPACE_IO)
   READDWORD32(io_read_dword_32le,  ADDRESS_SPACE_IO)
  READMASKED32(io_read_masked_32le, ADDRESS_SPACE_IO)
 WRITEBYTE32LE(io_write_byte_32le,  ADDRESS_SPACE_IO)
 WRITEWORD32LE(io_write_word_32le,  ADDRESS_SPACE_IO)
  WRITEDWORD32(io_write_dword_32le, ADDRESS_SPACE_IO)
 WRITEMASKED32(io_write_masked_32le,ADDRESS_SPACE_IO)

  READBYTE64BE(io_read_byte_64be,   ADDRESS_SPACE_IO)
  READWORD64BE(io_read_word_64be,   ADDRESS_SPACE_IO)
 READDWORD64BE(io_read_dword_64be,  ADDRESS_SPACE_IO)
   READQWORD64(io_read_qword_64be,  ADDRESS_SPACE_IO)
  READMASKED64(io_read_masked_64be, ADDRESS_SPACE_IO)
 WRITEBYTE64BE(io_write_byte_64be,  ADDRESS_SPACE_IO)
 WRITEWORD64BE(io_write_word_64be,  ADDRESS_SPACE_IO)
WRITEDWORD64BE(io_write_dword_64be, ADDRESS_SPACE_IO)
  WRITEQWORD64(io_write_qword_64be, ADDRESS_SPACE_IO)
 WRITEMASKED64(io_write_masked_64be,ADDRESS_SPACE_IO)

  READBYTE64LE(io_read_byte_64le,   ADDRESS_SPACE_IO)
  READWORD64LE(io_read_word_64le,   ADDRESS_SPACE_IO)
 READDWORD64LE(io_read_dword_64le,  ADDRESS_SPACE_IO)
   READQWORD64(io_read_qword_64le,  ADDRESS_SPACE_IO)
  READMASKED64(io_read_masked_64le, ADDRESS_SPACE_IO)
 WRITEBYTE64LE(io_write_byte_64le,  ADDRESS_SPACE_IO)
 WRITEWORD64LE(io_write_word_64le,  ADDRESS_SPACE_IO)
WRITEDWORD64LE(io_write_dword_64le, ADDRESS_SPACE_IO)
  WRITEQWORD64(io_write_qword_64le, ADDRESS_SPACE_IO)
 WRITEMASKED64(io_write_masked_64le,ADDRESS_SPACE_IO)


/*-------------------------------------------------
    safe opcode reading
-------------------------------------------------*/

UINT8 cpu_readop_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop_unsafe(byteaddress);
}

UINT16 cpu_readop16_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop16_unsafe(byteaddress);
}

UINT32 cpu_readop32_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop32_unsafe(byteaddress);
}

UINT64 cpu_readop64_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop64_unsafe(byteaddress);
}

UINT8 cpu_readop_arg_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop_arg_unsafe(byteaddress);
}

UINT16 cpu_readop_arg16_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop_arg16_unsafe(byteaddress);
}

UINT32 cpu_readop_arg32_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop_arg32_unsafe(byteaddress);
}

UINT64 cpu_readop_arg64_safe(offs_t byteaddress)
{
	activecpu_set_opbase(byteaddress);
	return cpu_readop_arg64_unsafe(byteaddress);
}


/*-------------------------------------------------
    unmapped memory handlers
-------------------------------------------------*/

static READ8_HANDLER( mrh8_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ16_HANDLER( mrh16_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ32_HANDLER( mrh32_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}
static READ64_HANDLER( mrh64_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap;
}

static WRITE8_HANDLER( mwh8_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_program )
{
	if (log_unmap[ADDRESS_SPACE_PROGRAM] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped program memory qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}

static READ8_HANDLER( mrh8_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ16_HANDLER( mrh16_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ32_HANDLER( mrh32_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}
static READ64_HANDLER( mrh64_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap;
}

static WRITE8_HANDLER( mwh8_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_data )
{
	if (log_unmap[ADDRESS_SPACE_DATA] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped data memory qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}

static READ8_HANDLER( mrh8_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O byte read from %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset));
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ16_HANDLER( mrh16_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O word read from %08X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*2), mem_mask ^ 0xffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ32_HANDLER( mrh32_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O dword read from %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*4), mem_mask ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}
static READ64_HANDLER( mrh64_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O qword read from %08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*8), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
	return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap;
}

static WRITE8_HANDLER( mwh8_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O byte write to %08X = %02X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset), data);
}
static WRITE16_HANDLER( mwh16_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O word write to %08X = %04X & %04X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*2), data, mem_mask ^ 0xffff);
}
static WRITE32_HANDLER( mwh32_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O dword write to %08X = %08X & %08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*4), data, mem_mask ^ 0xffffffff);
}
static WRITE64_HANDLER( mwh64_unmap_io )
{
	if (log_unmap[ADDRESS_SPACE_IO] && !debugger_access) logerror("cpu #%d (PC=%08X): unmapped I/O qword write to %08X = %08X%08X & %08X%08X\n", cpu_getactivecpu(), activecpu_get_pc(), BYTE2ADDR(&cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO], offset*8), (int)(data >> 32), (int)(data & 0xffffffff), (int)(mem_mask >> 32) ^ 0xffffffff, (int)(mem_mask & 0xffffffff) ^ 0xffffffff);
}


/*-------------------------------------------------
    no-op memory handlers
-------------------------------------------------*/

static READ8_HANDLER( mrh8_nop_program )   { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ16_HANDLER( mrh16_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ32_HANDLER( mrh32_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }
static READ64_HANDLER( mrh64_nop_program ) { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_PROGRAM].unmap; }

static READ8_HANDLER( mrh8_nop_data )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ16_HANDLER( mrh16_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ32_HANDLER( mrh32_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }
static READ64_HANDLER( mrh64_nop_data )    { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_DATA].unmap; }

static READ8_HANDLER( mrh8_nop_io )        { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ16_HANDLER( mrh16_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ32_HANDLER( mrh32_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }
static READ64_HANDLER( mrh64_nop_io )      { return cpudata[cpu_getactivecpu()].space[ADDRESS_SPACE_IO].unmap; }

static WRITE8_HANDLER( mwh8_nop )          {  }
static WRITE16_HANDLER( mwh16_nop )        {  }
static WRITE32_HANDLER( mwh32_nop )        {  }
static WRITE64_HANDLER( mwh64_nop )        {  }


/*-------------------------------------------------
    get_static_handler - returns points to static
    memory handlers
-------------------------------------------------*/

static genf *get_static_handler(int databits, int readorwrite, int spacenum, int which)
{
	static const struct
	{
		UINT8		databits;
		UINT8		handlernum;
		UINT8		spacenum;
		genf *		read;
		genf *		write;
	} static_handler_list[] =
	{
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (genf *)mrh8_unmap_program, (genf *)mwh8_unmap_program },
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (genf *)mrh8_unmap_data,    (genf *)mwh8_unmap_data },
		{  8, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (genf *)mrh8_unmap_io,      (genf *)mwh8_unmap_io },
		{  8, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (genf *)mrh8_nop_program,   (genf *)mwh8_nop },
		{  8, STATIC_NOP,    ADDRESS_SPACE_DATA,    (genf *)mrh8_nop_data,      (genf *)mwh8_nop },
		{  8, STATIC_NOP,    ADDRESS_SPACE_IO,      (genf *)mrh8_nop_io,        (genf *)mwh8_nop },

		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (genf *)mrh16_unmap_program,(genf *)mwh16_unmap_program },
		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (genf *)mrh16_unmap_data,   (genf *)mwh16_unmap_data },
		{ 16, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (genf *)mrh16_unmap_io,     (genf *)mwh16_unmap_io },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (genf *)mrh16_nop_program,  (genf *)mwh16_nop },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_DATA,    (genf *)mrh16_nop_data,     (genf *)mwh16_nop },
		{ 16, STATIC_NOP,    ADDRESS_SPACE_IO,      (genf *)mrh16_nop_io,       (genf *)mwh16_nop },

		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (genf *)mrh32_unmap_program,(genf *)mwh32_unmap_program },
		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (genf *)mrh32_unmap_data,   (genf *)mwh32_unmap_data },
		{ 32, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (genf *)mrh32_unmap_io,     (genf *)mwh32_unmap_io },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (genf *)mrh32_nop_program,  (genf *)mwh32_nop },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_DATA,    (genf *)mrh32_nop_data,     (genf *)mwh32_nop },
		{ 32, STATIC_NOP,    ADDRESS_SPACE_IO,      (genf *)mrh32_nop_io,       (genf *)mwh32_nop },

		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_PROGRAM, (genf *)mrh64_unmap_program,(genf *)mwh64_unmap_program },
		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_DATA,    (genf *)mrh64_unmap_data,   (genf *)mwh64_unmap_data },
		{ 64, STATIC_UNMAP,  ADDRESS_SPACE_IO,      (genf *)mrh64_unmap_io,     (genf *)mwh64_unmap_io },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_PROGRAM, (genf *)mrh64_nop_program,  (genf *)mwh64_nop },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_DATA,    (genf *)mrh64_nop_data,     (genf *)mwh64_nop },
		{ 64, STATIC_NOP,    ADDRESS_SPACE_IO,      (genf *)mrh64_nop_io,       (genf *)mwh64_nop },
	};
	int tablenum;

	for (tablenum = 0; tablenum < sizeof(static_handler_list) / sizeof(static_handler_list[0]); tablenum++)
		if (static_handler_list[tablenum].databits == databits && static_handler_list[tablenum].handlernum == which)
			if (static_handler_list[tablenum].spacenum == 0xff || static_handler_list[tablenum].spacenum == spacenum)
				return readorwrite ? static_handler_list[tablenum].write : static_handler_list[tablenum].read;

	return NULL;
}


/*-------------------------------------------------
    debugging
-------------------------------------------------*/

static const char *handler_to_string(const table_data *table, UINT8 entry)
{
	static const char *const strings[] =
	{
		"invalid",		"bank 1",		"bank 2",		"bank 3",
		"bank 4",		"bank 5",		"bank 6",		"bank 7",
		"bank 8",		"bank 9",		"bank 10",		"bank 11",
		"bank 12",		"bank 13",		"bank 14",		"bank 15",
		"bank 16",		"bank 17",		"bank 18",		"bank 19",
		"bank 20",		"bank 21",		"bank 22",		"bank 23",
		"bank 24",		"bank 25",		"bank 26",		"bank 27",
		"bank 28",		"bank 29",		"bank 30",		"bank 31",
		"bank 32",		"ram[33]",		"ram[34]",		"ram[35]",
		"ram[36]",		"ram[37]",		"ram[38]",		"ram[39]",
		"ram[40]",		"ram[41]",		"ram[42]",		"ram[43]",
		"ram[44]",		"ram[45]",		"ram[46]",		"ram[47]",
		"ram[48]",		"ram[49]",		"ram[50]",		"ram[51]",
		"ram[52]",		"ram[53]",		"ram[54]",		"ram[55]",
		"ram[56]",		"ram[57]",		"ram[58]",		"ram[59]",
		"ram[60]",		"ram[61]",		"ram[62]",		"ram[63]",
		"ram[64]",		"ram[65]",		"ram[66]",		"ram[67]",
		"ram[68]",		"rom",			"nop",			"unmapped"
	};

	/* constant strings for lower entries */
	if (entry < STATIC_COUNT)
		return strings[entry];
	else
		return table->handlers[entry].name ? table->handlers[entry].name : "???";
}

static void dump_map(FILE *file, const addrspace_data *space, const table_data *table)
{
	int l1count = 1 << LEVEL1_BITS;
	int l2count = 1 << LEVEL2_BITS;
	UINT8 lastentry = STATIC_UNMAP;
	int entrymatches = 0;
	int i, j;

	/* dump generic information */
	fprintf(file, "  Address bits = %d\n", space->abits);
	fprintf(file, "     Data bits = %d\n", space->dbits);
	fprintf(file, "       L1 bits = %d\n", LEVEL1_BITS);
	fprintf(file, "       L2 bits = %d\n", LEVEL2_BITS);
	fprintf(file, "  Address mask = %X\n", space->bytemask);
	fprintf(file, "\n");

	/* loop over level 1 entries */
	for (i = 0; i < l1count; i++)
	{
		UINT8 entry = table->table[i];

		/* if this entry matches the previous one, just count it */
		if (entry < SUBTABLE_BASE && entry == lastentry)
		{
			entrymatches++;
			continue;
		}

		/* otherwise, print accumulated info */
		if (lastentry < SUBTABLE_BASE && lastentry != STATIC_UNMAP)
			fprintf(file, "%08X-%08X    = %02X: %s [offset=%08X]\n",
							(i - entrymatches) << LEVEL2_BITS,
							(i << LEVEL2_BITS) - 1,
							lastentry,
							handler_to_string(table, lastentry),
							table->handlers[lastentry].bytestart);

		/* start counting with this entry */
		lastentry = entry;
		entrymatches = 1;

		/* if we're a subtable, we need to drill down */
		if (entry >= SUBTABLE_BASE)
		{
			UINT8 lastentry2 = STATIC_UNMAP;
			int entry2matches = 0;

			/* loop over level 2 entries */
			entry -= SUBTABLE_BASE;
			for (j = 0; j < l2count; j++)
			{
				UINT8 entry2 = table->table[(1 << LEVEL1_BITS) + (entry << LEVEL2_BITS) + j];

				/* if this entry matches the previous one, just count it */
				if (entry2 < SUBTABLE_BASE && entry2 == lastentry2)
				{
					entry2matches++;
					continue;
				}

				/* otherwise, print accumulated info */
				if (lastentry2 < SUBTABLE_BASE && lastentry2 != STATIC_UNMAP)
					fprintf(file, "%08X-%08X    = %02X: %s [offset=%08X]\n",
									((i << LEVEL2_BITS) | (j - entry2matches)),
									((i << LEVEL2_BITS) | (j - 1)),
									lastentry2,
									handler_to_string(table, lastentry2),
									table->handlers[lastentry2].bytestart);

				/* start counting with this entry */
				lastentry2 = entry2;
				entry2matches = 1;
			}

			/* flush the last entry */
			if (lastentry2 < SUBTABLE_BASE && lastentry2 != STATIC_UNMAP)
				fprintf(file, "%08X-%08X    = %02X: %s [offset=%08X]\n",
								((i << LEVEL2_BITS) | (j - entry2matches)),
								((i << LEVEL2_BITS) | (j - 1)),
								lastentry2,
								handler_to_string(table, lastentry2),
								table->handlers[lastentry2].bytestart);
		}
	}

	/* flush the last entry */
	if (lastentry < SUBTABLE_BASE && lastentry != STATIC_UNMAP)
		fprintf(file, "%08X-%08X    = %02X: %s [offset=%08X]\n",
						(i - entrymatches) << LEVEL2_BITS,
						(i << LEVEL2_BITS) - 1,
						lastentry,
						handler_to_string(table, lastentry),
						table->handlers[lastentry].bytestart);
}

void memory_dump(FILE *file)
{
	int cpunum, spacenum;

	/* skip if we can't open the file */
	if (!file)
		return;

	/* loop over CPUs */
	for (cpunum = 0; cpunum < ARRAY_LENGTH(cpudata); cpunum++)
		for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
			if (cpudata[cpunum].spacemask & (1 << spacenum))
			{
				fprintf(file, "\n\n"
				              "=========================================\n"
				              "CPU %d address space %d read handler dump\n"
				              "=========================================\n", cpunum, spacenum);
				dump_map(file, &cpudata[cpunum].space[spacenum], &cpudata[cpunum].space[spacenum].read);

				fprintf(file, "\n\n"
				              "==========================================\n"
				              "CPU %d address space %d write handler dump\n"
				              "==========================================\n", cpunum, spacenum);
				dump_map(file, &cpudata[cpunum].space[spacenum], &cpudata[cpunum].space[spacenum].write);
			}
}


/*-------------------------------------------------
    memory_get_handler_string - return a string
    describing the handler at a particular offset
-------------------------------------------------*/

const char *memory_get_handler_string(int read0_or_write1, int cpunum, int spacenum, offs_t byteaddress)
{
	addrspace_data *space = &cpudata[cpunum].space[spacenum];
	const table_data *table = read0_or_write1 ? &space->write : &space->read;
	UINT8 entry;

	/* perform the lookup */
	byteaddress &= space->bytemask;
	entry = table->table[LEVEL1_INDEX(byteaddress)];
	if (entry >= SUBTABLE_BASE)
		entry = table->table[LEVEL2_INDEX(entry, byteaddress)];

	/* 8-bit case: RAM/ROM */
	return handler_to_string(table, entry);
}


static void mem_dump(void)
{
	FILE *file;

	if (MEM_DUMP)
	{
		file = fopen("memdump.log", "w");
		if (file)
		{
			memory_dump(file);
			fclose(file);
		}
	}
}
