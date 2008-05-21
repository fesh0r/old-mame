/***************************************************************************

    drcbex64.c

    64-bit x64 back-end for the universal machine language.

    Copyright Aaron Giles
    Released for general non-commercial use under the MAME license
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Future improvements/changes:

    * Add support for FP registers

    * Optimize to avoid unnecessary reloads

    * Identify common pairs and optimize output

****************************************************************************

    -------------------------
    ABI/conventions (Windows)
    -------------------------

    Registers:
        RAX        - volatile, function return value
        RBX        - non-volatile
        RCX        - volatile, integer function parameter 1
        RDX        - volatile, integer function parameter 2
        RSI        - non-volatile
        RDI        - non-volatile
        RBP        - non-volatile
        R8         - volatile, integer function parameter 3
        R9         - volatile, integer function parameter 4
        R10        - volatile
        R11        - volatile, scratch immediate storage
        R12        - non-volatile
        R13        - non-volatile
        R14        - non-volatile
        R15        - non-volatile

        XMM0       - volatile, FP function parameter 1
        XMM1       - volatile, FP function parameter 2
        XMM2       - volatile, FP function parameter 3
        XMM3       - volatile, FP function parameter 4
        XMM4       - volatile
        XMM5       - volatile
        XMM6       - non-volatile
        XMM7       - non-volatile
        XMM8       - non-volatile
        XMM9       - non-volatile
        XMM10      - non-volatile
        XMM11      - non-volatile
        XMM12      - non-volatile
        XMM13      - non-volatile
        XMM14      - non-volatile
        XMM15      - non-volatile


    -----------------------------
    ABI/conventions (Linux/MacOS)
    -----------------------------

    Registers:
        RAX        - volatile, function return value
        RBX        - non-volatile
        RCX        - volatile, integer function parameter 4
        RDX        - volatile, integer function parameter 3
        RSI        - volatile, integer function parameter 2
        RDI        - volatile, integer function parameter 1
        RBP        - non-volatile
        R8         - volatile, integer function parameter 5
        R9         - volatile, integer function parameter 6
        R10        - volatile
        R11        - volatile, scratch immediate storage
        R12        - non-volatile
        R13        - non-volatile
        R14        - non-volatile
        R15        - non-volatile

        XMM0       - volatile, FP function parameter 1
        XMM1       - volatile, FP function parameter 2
        XMM2       - volatile, FP function parameter 3
        XMM3       - volatile, FP function parameter 4
        XMM4       - volatile
        XMM5       - volatile
        XMM6       - volatile
        XMM7       - volatile
        XMM8       - volatile
        XMM9       - volatile
        XMM10      - volatile
        XMM11      - volatile
        XMM12      - volatile
        XMM13      - volatile
        XMM14      - volatile
        XMM15      - volatile


    ---------------
    Execution model
    ---------------

    Registers (Windows):
        RAX        - scratch register
        RBX        - maps to I0
        RCX        - scratch register
        RDX        - scratch register
        RSI        - maps to I1
        RDI        - maps to I2
        RBP        - pointer to code cache
        R8         - scratch register
        R9         - scratch register
        R10        - scratch register
        R11        - scratch register
        R12        - maps to I3
        R13        - maps to I4
        R14        - maps to I5
        R15        - maps to I6

    Registers (Linux/MacOS):
        RAX        - scratch register
        RBX        - maps to I0
        RCX        - scratch register
        RDX        - scratch register
        RSI        - unused
        RDI        - unused
        RBP        - pointer to code cache
        R8         - scratch register
        R9         - scratch register
        R10        - scratch register
        R11        - scratch register
        R12        - maps to I1
        R13        - maps to I2
        R14        - maps to I3
        R15        - maps to I4

    Entry point:
        Assumes 1 parameter passed, which is the codeptr of the code
        to execute once the environment is set up.

    Exit point:
        Assumes exit value is in RAX.

    Entry stack:
        [rsp]      - return

    Runtime stack:
        [rsp]      - r9 home
        [rsp+8]    - r8 home
        [rsp+16]   - rdx home
        [rsp+24]   - rcx home
        [rsp+40]   - saved r15
        [rsp+48]   - saved r14
        [rsp+56]   - saved r13
        [rsp+64]   - saved r12
        [rsp+72]   - saved ebp
        [rsp+80]   - saved edi
        [rsp+88]   - saved esi
        [rsp+96]   - saved ebx
        [rsp+104]  - ret

***************************************************************************/

#include "drcuml.h"
#include "drcumld.h"
#include "drcbeut.h"
#include "debugger.h"
#include "x86emit.h"
#include "eminline.h"
#undef REG_SP
#include "x86log.h"
#include <math.h>
#include <stddef.h>



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define LOG_HASHJMPS		(0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PTYPE_M				(1 << DRCUML_PTYPE_MEMORY)
#define PTYPE_I				(1 << DRCUML_PTYPE_IMMEDIATE)
#define PTYPE_R				(1 << DRCUML_PTYPE_INT_REGISTER)
#define PTYPE_F				(1 << DRCUML_PTYPE_FLOAT_REGISTER)
#define PTYPE_MI			(PTYPE_M | PTYPE_I)
#define PTYPE_RI			(PTYPE_R | PTYPE_I)
#define PTYPE_MR			(PTYPE_M | PTYPE_R)
#define PTYPE_MRI			(PTYPE_M | PTYPE_R | PTYPE_I)
#define PTYPE_MF			(PTYPE_M | PTYPE_F)

#ifdef X64_WINDOWS_ABI

#define REG_PARAM1			REG_RCX
#define REG_PARAM2			REG_RDX
#define REG_PARAM3			REG_R8
#define REG_PARAM4			REG_R9

#else

#define REG_PARAM1			REG_RDI
#define REG_PARAM2			REG_RSI
#define REG_PARAM3			REG_RDX
#define REG_PARAM4			REG_RCX

#endif



/***************************************************************************
    MACROS
***************************************************************************/

#define X86_CONDITION(condflags)		(condition_map[condflags - DRCUML_COND_Z])
#define X86_NOT_CONDITION(condflags)	(condition_map[condflags - DRCUML_COND_Z] ^ 1)

#undef MABS
#define MABS(drcbe, ptr)	MBD(REG_RBP, offset_from_rbp(drcbe, (FPTR)(ptr)))



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* entry point */
typedef UINT32 (*x86_entry_point_func)(UINT8 *rbpvalue, x86code *entry);

/* opcode handler */
typedef x86code *(*opcode_generate_func)(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);


/* opcode table entry */
typedef struct _opcode_table_entry opcode_table_entry;
struct _opcode_table_entry
{
	drcuml_opcode			opcode;					/* opcode in question */
	opcode_generate_func	func;					/* function pointer to the work */
};


/* internal backend-specific state */
struct _drcbe_state
{
	drcuml_state *			drcuml;					/* pointer back to our owner */
	drccache *				cache;					/* pointer to the cache */
	drcuml_machine_state 	state;					/* state of the machine */
	drchash_state *			hash;					/* hash table state */
	drcmap_state *			map;					/* code map */
	drclabel_list *			labels;                 /* label list */

	x86_entry_point_func	entry;					/* entry point */
	x86code *				exit;					/* exit point */
	x86code *				nocode;					/* nocode handler */

	x86code *				mame_debug_hook;		/* debugger callback */
	x86code *				debug_log_hashjmp;		/* hashjmp debugging */
	x86code *				drcmap_get_value;		/* map lookup helper */
	data_accessors			accessors[ADDRESS_SPACES];/* memory accessors */

	UINT8					sse41;					/* do we have SSE4.1 support? */
	UINT32					ssemode;				/* saved SSE mode */
	UINT32					ssemodesave;			/* temporary location for saving */
	UINT32					ssecontrol[4];			/* copy of the sse_control array */
	UINT32 *				absmask32;				/* absolute value mask (32-bit) */
	UINT64 *				absmask64;				/* absolute value mask (32-bit) */

	void *					stacksave;				/* saved stack pointer */
	void *					hashstacksave;			/* saved stack pointer for hashjmp */

	UINT8 *					rbpvalue;				/* value of RBP */
	UINT8					flagsmap[0x1000];		/* flags map */
	UINT64					flagsunmap[0x20];		/* flags unmapper */

	x86log_context *		log;					/* logging */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* primary back-end callbacks */
static drcbe_state *drcbex64_alloc(drcuml_state *drcuml, drccache *cache, UINT32 flags, int modes, int addrbits, int ignorebits);
static void drcbex64_free(drcbe_state *drcbe);
static void drcbex64_reset(drcbe_state *drcbe);
static int drcbex64_execute(drcbe_state *drcbe, drcuml_codehandle *entry);
static void drcbex64_generate(drcbe_state *drcbe, drcuml_block *block, const drcuml_instruction *instlist, UINT32 numinst);
static int drcbex64_hash_exists(drcbe_state *drcbe, UINT32 mode, UINT32 pc);
static void drcbex64_get_info(drcbe_state *state, drcbe_info *info);

/* private helper functions */
static void fixup_label(void *parameter, drccodeptr labelcodeptr);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* globally-accessible interface to the backend */
const drcbe_interface drcbe_x64_be_interface =
{
	drcbex64_alloc,
	drcbex64_free,
	drcbex64_reset,
	drcbex64_execute,
	drcbex64_generate,
	drcbex64_hash_exists,
	drcbex64_get_info
};

/* opcode table */
static opcode_generate_func opcode_table[DRCUML_OP_MAX];

/* size-to-mask table */
static const UINT64 size_to_mask[] = { 0, 0xff, 0xffff, 0, 0xffffffff, 0, 0, 0, U64(0xffffffffffffffff) };

/* register mapping tables */
static UINT8 int_register_map[DRCUML_REG_I_END - DRCUML_REG_I0] =
{
#ifdef X64_WINDOWS_ABI
	REG_RBX, REG_RSI, REG_RDI, REG_R12, REG_R13, REG_R14, REG_R15
#else
	REG_RBX, REG_R12, REG_R13, REG_R14, REG_R15
#endif
};

static UINT8 float_register_map[DRCUML_REG_F_END - DRCUML_REG_F0] =
{
	0
};

/* condition mapping table */
static UINT8 condition_map[DRCUML_COND_MAX - DRCUML_COND_Z] =
{
	COND_Z,		/* DRCUML_COND_Z = 0x80,    requires Z */
	COND_NZ,	/* DRCUML_COND_NZ,          requires Z */
	COND_S,		/* DRCUML_COND_S,           requires S */
	COND_NS,	/* DRCUML_COND_NS,          requires S */
	COND_C,		/* DRCUML_COND_C,           requires C */
	COND_NC,	/* DRCUML_COND_NC,          requires C */
	COND_O,		/* DRCUML_COND_V,           requires V */
	COND_NO,	/* DRCUML_COND_NV,          requires V */
	COND_P,		/* DRCUML_COND_U,           requires U */
	COND_NP,	/* DRCUML_COND_NU,          requires U */
	COND_A,		/* DRCUML_COND_A,           requires CZ */
	COND_BE,	/* DRCUML_COND_BE,          requires CZ */
	COND_G,		/* DRCUML_COND_G,           requires SVZ */
	COND_LE,	/* DRCUML_COND_LE,          requires SVZ */
	COND_L,		/* DRCUML_COND_L,           requires SV */
	COND_GE,	/* DRCUML_COND_GE,          requires SV */
};

/* rounding mode mapping table */
static const UINT8 fprnd_map[4] =
{
	FPRND_CHOP,		/* DRCUML_FMOD_TRUNC,       truncate */
	FPRND_NEAR,		/* DRCUML_FMOD_ROUND,       round */
	FPRND_UP,		/* DRCUML_FMOD_CEIL,        round up */
	FPRND_DOWN		/* DRCUML_FMOD_FLOOR        round down */
};



/***************************************************************************
    TABLES
***************************************************************************/

static x86code *op_handle(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_hash(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_label(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_comment(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_mapvar(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);

static x86code *op_debug(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_exit(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_hashjmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_jmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_exh(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_callh(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ret(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_callc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_recover(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);

static x86code *op_setfmod(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_getfmod(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_getexp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_save(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_restore(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);

static x86code *op_load1u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load1s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load2u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load2s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load4u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load4s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_load8u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_store1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_store2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_store4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_store8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read1u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read1s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read2u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read2s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read2m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read4u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read4s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read4m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read8u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_read8m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_write1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_write2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_writ2m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_write4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_writ4m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_write8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_writ8m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_flags(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_mov(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_zext1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_zext2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_zext4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_sext1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_sext2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_sext4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_xtract(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_insert(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_add(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_addc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_sub(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_subc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_cmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_mulu(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_muls(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_divu(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_divs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_and(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_test(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_or(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_xor(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_lzcnt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_shl(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_shr(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_sar(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ror(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_rol(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_rorc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_rolc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);

static x86code *op_fload(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fstore(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fread(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fwrite(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fmov(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi4t(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi4r(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi4f(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi4c(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi8t(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi8r(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi8f(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ftoi8c(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ffrfs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ffrfd(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ffri4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_ffri8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fadd(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fsub(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fcmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fmul(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fdiv(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fneg(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fabs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_fsqrt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_frecip(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);
static x86code *op_frsqrt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst);

static const opcode_table_entry opcode_table_source[] =
{
	/* Compile-time opcodes */
	{ DRCUML_OP_HANDLE,	 op_handle },	/* HANDLE  handle                 */
	{ DRCUML_OP_HASH,    op_hash },		/* HASH    mode,pc                */
	{ DRCUML_OP_LABEL,   op_label },	/* LABEL   imm                    */
	{ DRCUML_OP_COMMENT, op_comment },	/* COMMENT string                 */
	{ DRCUML_OP_MAPVAR,  op_mapvar },	/* MAPVAR  mapvar,value           */

	/* Control Flow Operations */
	{ DRCUML_OP_DEBUG,   op_debug },	/* DEBUG   pc                     */
	{ DRCUML_OP_EXIT,    op_exit },		/* EXIT    src1[,c]               */
	{ DRCUML_OP_HASHJMP, op_hashjmp },	/* HASHJMP mode,pc,handle         */
	{ DRCUML_OP_JMP,     op_jmp },		/* JMP     imm[,c]                */
	{ DRCUML_OP_EXH,     op_exh },		/* EXH     handle,param[,c]       */
	{ DRCUML_OP_CALLH,   op_callh },	/* CALLH   handle[,c]             */
	{ DRCUML_OP_RET,     op_ret },		/* RET     [c]                    */
	{ DRCUML_OP_CALLC,   op_callc },	/* CALLC   func,ptr[,c]           */
	{ DRCUML_OP_RECOVER, op_recover },	/* RECOVER dst,mapvar             */

	/* Internal Register Operations */
	{ DRCUML_OP_SETFMOD, op_setfmod },	/* SETFMOD src                    */
	{ DRCUML_OP_GETFMOD, op_getfmod },	/* GETFMOD dst                    */
	{ DRCUML_OP_GETEXP,  op_getexp },	/* GETEXP  dst,index              */
	{ DRCUML_OP_SAVE,    op_save },		/* SAVE    dst,index              */
	{ DRCUML_OP_RESTORE, op_restore },	/* RESTORE dst,index              */

	/* Integer Operations */
	{ DRCUML_OP_LOAD1U,  op_load1u },	/* LOAD1U  dst,base,index         */
	{ DRCUML_OP_LOAD1S,  op_load1s },	/* LOAD1S  dst,base,index         */
	{ DRCUML_OP_LOAD2U,  op_load2u },	/* LOAD2U  dst,base,index         */
	{ DRCUML_OP_LOAD2S,  op_load2s },	/* LOAD2S  dst,base,index         */
	{ DRCUML_OP_LOAD4U,  op_load4u },	/* LOAD4U  dst,base,index         */
	{ DRCUML_OP_LOAD4S,  op_load4s },	/* LOAD4S  dst,base,index         */
	{ DRCUML_OP_LOAD8U,  op_load8u },	/* LOAD8U  dst,base,index         */
	{ DRCUML_OP_STORE1,  op_store1 },	/* STORE1  base,index,src         */
	{ DRCUML_OP_STORE2,  op_store2 },	/* STORE2  base,index,src         */
	{ DRCUML_OP_STORE4,  op_store4 },	/* STORE4  base,index,src         */
	{ DRCUML_OP_STORE8,  op_store8 },	/* STORE8  base,index,src         */
	{ DRCUML_OP_READ1U,  op_read1u },	/* READ1U  dst,space,src1         */
	{ DRCUML_OP_READ1S,  op_read1s },	/* READ1S  dst,space,src1         */
	{ DRCUML_OP_READ2U,  op_read2u },	/* READ2U  dst,space,src1         */
	{ DRCUML_OP_READ2S,  op_read2s },	/* READ2S  dst,space,src1         */
	{ DRCUML_OP_READ2M,  op_read2m },	/* READ2M  dst,space,src1,mask    */
	{ DRCUML_OP_READ4U,  op_read4u },	/* READ4U  dst,space,src1         */
	{ DRCUML_OP_READ4S,  op_read4s },	/* READ4S  dst,space,src1         */
	{ DRCUML_OP_READ4M,  op_read4m },	/* READ4M  dst,space,src1,mask    */
	{ DRCUML_OP_READ8U,  op_read8u },	/* READ8U  dst,space,src1         */
	{ DRCUML_OP_READ8M,  op_read8m },	/* READ8M  dst,space,src1,mask    */
	{ DRCUML_OP_WRITE1,  op_write1 },	/* WRITE1  space,dst,src1         */
	{ DRCUML_OP_WRITE2,  op_write2 },	/* WRITE2  space,dst,src1         */
	{ DRCUML_OP_WRIT2M,  op_writ2m },	/* WRIT2M  space,dst,src1         */
	{ DRCUML_OP_WRITE4,  op_write4 },	/* WRITE4  space,dst,src1         */
	{ DRCUML_OP_WRIT4M,  op_writ4m },	/* WRIT4M  space,dst,mask,src1    */
	{ DRCUML_OP_WRITE8,  op_write8 },	/* WRITE8  space,dst,src1         */
	{ DRCUML_OP_WRIT8M,  op_writ8m },	/* WRIT8M  space,dst,mask,src1    */
	{ DRCUML_OP_FLAGS,   op_flags },	/* FLAGS   dst,mask,table         */
	{ DRCUML_OP_MOV,     op_mov },		/* MOV     dst,src[,c]            */
	{ DRCUML_OP_ZEXT1,   op_zext1 },	/* ZEXT1   dst,src                */
	{ DRCUML_OP_ZEXT2,   op_zext2 },	/* ZEXT2   dst,src                */
	{ DRCUML_OP_ZEXT4,   op_zext4 },	/* ZEXT4   dst,src                */
	{ DRCUML_OP_SEXT1,   op_sext1 },	/* SEXT1   dst,src                */
	{ DRCUML_OP_SEXT2,   op_sext2 },	/* SEXT2   dst,src                */
	{ DRCUML_OP_SEXT4,   op_sext4 },	/* SEXT4   dst,src                */
	{ DRCUML_OP_XTRACT,  op_xtract },	/* XTRACT  dst,src1,src2,src3     */
	{ DRCUML_OP_INSERT,  op_insert },	/* INSERT  dst,src1,src2,src3     */
	{ DRCUML_OP_ADD,     op_add },		/* ADD     dst,src1,src2[,f]      */
	{ DRCUML_OP_ADDC,    op_addc },		/* ADDC    dst,src1,src2[,f]      */
	{ DRCUML_OP_SUB,     op_sub },		/* SUB     dst,src1,src2[,f]      */
	{ DRCUML_OP_SUBB,    op_subc },		/* SUBB    dst,src1,src2[,f]      */
	{ DRCUML_OP_CMP,     op_cmp },		/* CMP     src1,src2[,f]          */
	{ DRCUML_OP_MULU,    op_mulu },		/* MULU    dst,edst,src1,src2[,f] */
	{ DRCUML_OP_MULS,    op_muls },		/* MULS    dst,edst,src1,src2[,f] */
	{ DRCUML_OP_DIVU,    op_divu },		/* DIVU    dst,edst,src1,src2[,f] */
	{ DRCUML_OP_DIVS,    op_divs },		/* DIVS    dst,edst,src1,src2[,f] */
	{ DRCUML_OP_AND,     op_and },		/* AND     dst,src1,src2[,f]      */
	{ DRCUML_OP_TEST,    op_test },		/* TEST    src1,src2[,f]          */
	{ DRCUML_OP_OR,      op_or },		/* OR      dst,src1,src2[,f]      */
	{ DRCUML_OP_XOR,     op_xor },		/* XOR     dst,src1,src2[,f]      */
	{ DRCUML_OP_LZCNT,   op_lzcnt },	/* LZCNT   dst,src[,f]            */
	{ DRCUML_OP_SHL,     op_shl },		/* SHL     dst,src,count[,f]      */
	{ DRCUML_OP_SHR,     op_shr },		/* SHR     dst,src,count[,f]      */
	{ DRCUML_OP_SAR,     op_sar },		/* SAR     dst,src,count[,f]      */
	{ DRCUML_OP_ROL,     op_rol },		/* ROL     dst,src,count[,f]      */
	{ DRCUML_OP_ROLC,    op_rolc },		/* ROLC    dst,src,count[,f]      */
	{ DRCUML_OP_ROR,     op_ror },		/* ROR     dst,src,count[,f]      */
	{ DRCUML_OP_RORC,    op_rorc },		/* RORC    dst,src,count[,f]      */

	/* Floating Point Operations */
	{ DRCUML_OP_FLOAD,   op_fload },	/* FLOAD   dst,base,index         */
	{ DRCUML_OP_FSTORE,  op_fstore },	/* FSTORE  base,index,src         */
	{ DRCUML_OP_FREAD,   op_fread },	/* FREAD   dst,space,src1         */
	{ DRCUML_OP_FWRITE,  op_fwrite },	/* FWRITE  space,dst,src1         */
	{ DRCUML_OP_FMOV,    op_fmov },		/* FMOV    dst,src1[,c]           */
	{ DRCUML_OP_FTOI4,   op_ftoi4 },	/* FTOI4   dst,src1               */
	{ DRCUML_OP_FTOI4T,  op_ftoi4t },	/* FTOI4T  dst,src1               */
	{ DRCUML_OP_FTOI4R,  op_ftoi4r },	/* FTOI4R  dst,src1               */
	{ DRCUML_OP_FTOI4F,  op_ftoi4f },	/* FTOI4F  dst,src1               */
	{ DRCUML_OP_FTOI4C,  op_ftoi4c },	/* FTOI4C  dst,src1               */
	{ DRCUML_OP_FTOI8,   op_ftoi8 },	/* FTOI8   dst,src1               */
	{ DRCUML_OP_FTOI8T,  op_ftoi8t },	/* FTOI8T  dst,src1               */
	{ DRCUML_OP_FTOI8R,  op_ftoi8r },	/* FTOI8R  dst,src1               */
	{ DRCUML_OP_FTOI8F,  op_ftoi8f },	/* FTOI8F  dst,src1               */
	{ DRCUML_OP_FTOI8C,  op_ftoi8c },	/* FTOI8C  dst,src1               */
	{ DRCUML_OP_FFRFS,   op_ffrfs },	/* FFRFS   dst,src1               */
	{ DRCUML_OP_FFRFD,   op_ffrfd },	/* FFRFD   dst,src1               */
	{ DRCUML_OP_FFRI4,   op_ffri4 },	/* FFRI4   dst,src1               */
	{ DRCUML_OP_FFRI8,   op_ffri8 },	/* FFRI8   dst,src1               */
	{ DRCUML_OP_FADD,    op_fadd },		/* FADD    dst,src1,src2          */
	{ DRCUML_OP_FSUB,    op_fsub },		/* FSUB    dst,src1,src2          */
	{ DRCUML_OP_FCMP,    op_fcmp },		/* FCMP    src1,src2              */
	{ DRCUML_OP_FMUL,    op_fmul },		/* FMUL    dst,src1,src2          */
	{ DRCUML_OP_FDIV,    op_fdiv },		/* FDIV    dst,src1,src2          */
	{ DRCUML_OP_FNEG,    op_fneg },		/* FNEG    dst,src1               */
	{ DRCUML_OP_FABS,    op_fabs },		/* FABS    dst,src1               */
	{ DRCUML_OP_FSQRT,   op_fsqrt },	/* FSQRT   dst,src1               */
	{ DRCUML_OP_FRECIP,  op_frecip },	/* FRECIP  dst,src1               */
	{ DRCUML_OP_FRSQRT,  op_frsqrt }	/* FRSQRT  dst,src1               */
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    param_select_register - select a register
    to use, avoiding conflicts with the optional
    checkparam
-------------------------------------------------*/

INLINE int param_select_register(int defreg, const drcuml_parameter *param, const drcuml_parameter *checkparam)
{
	if ((param->type == DRCUML_PTYPE_INT_REGISTER || param->type == DRCUML_PTYPE_FLOAT_REGISTER) && (checkparam == NULL || param->type != checkparam->type || param->value != checkparam->value))
		return param->value;
	return defreg;
}


/*-------------------------------------------------
    param_select_register2 - select a register
    to use, avoiding conflicts with the optional
    checkparam
-------------------------------------------------*/

INLINE int param_select_register2(int defreg, const drcuml_parameter *param, const drcuml_parameter *checkparam1, const drcuml_parameter *checkparam2)
{
	if (param->type == DRCUML_PTYPE_INT_REGISTER && (param->type != checkparam1->type || param->value != checkparam1->value) && (param->type != checkparam2->type || param->value != checkparam2->value))
		return param->value;
	return defreg;
}


/*-------------------------------------------------
    offset_from_rbp - return the verified offset
    from rbp
-------------------------------------------------*/

INLINE INT32 offset_from_rbp(drcbe_state *drcbe, FPTR ptr)
{
	INT64 delta = (UINT8 *)ptr - drcbe->rbpvalue;
	assert_always((INT32)delta == delta, "offset_from_rbp: delta out of range");
	return (INT32)delta;
}


/*-------------------------------------------------
    short_immediate - true if the given immediate
    fits as a signed 32-bit value
-------------------------------------------------*/

INLINE int short_immediate(INT64 immediate)
{
	return (INT32)immediate == immediate;
}


/*-------------------------------------------------
    get_base_register_and_offset - determine right
    base register and offset to access the given
    target address
-------------------------------------------------*/

INLINE int get_base_register_and_offset(drcbe_state *drcbe, x86code **dst, FPTR target, UINT8 reg, INT32 *offset)
{
	INT64 delta = (UINT8 *)target - drcbe->rbpvalue;
	if (short_immediate(delta))
	{
		*offset = delta;
		return REG_RBP;
	}
	else
	{
		*offset = 0;
		emit_mov_r64_imm(dst, reg, target);												// mov   reg,target
		return reg;
	}
}


/*-------------------------------------------------
    emit_smart_call_r64 - generate a call either
    directly or via a call through pointer
-------------------------------------------------*/

INLINE void emit_smart_call_r64(drcbe_state *drcbe, x86code **dst, x86code *target, UINT8 reg)
{
	INT64 delta = target - (*dst + 5);
	if (short_immediate(delta))
		emit_call(dst, target);															// call  target
	else
	{
		emit_mov_r64_imm(dst, reg, (FPTR)target);										// mov   reg,target
		emit_call_r64(dst, reg);														// call  reg
	}
}


/*-------------------------------------------------
    emit_smart_call_m64 - generate a call either
    directly or via a call through pointer
-------------------------------------------------*/

INLINE void emit_smart_call_m64(drcbe_state *drcbe, x86code **dst, x86code **target)
{
	INT64 delta = *target - (*dst + 5);
	if (short_immediate(delta))
		emit_call(dst, *target);														// call  *target
	else
		emit_call_m64(dst, MABS(drcbe, target));										// call  [target]
}



/***************************************************************************
    BACKEND CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    drcbex64_alloc - allocate back-end-specific
    state
-------------------------------------------------*/

static drcbe_state *drcbex64_alloc(drcuml_state *drcuml, drccache *cache, UINT32 flags, int modes, int addrbits, int ignorebits)
{
	/* SSE control register mapping */
	static const UINT32 sse_control[4] =
	{
		0xffc0, 	/* DRCUML_FMOD_TRUNC */
		0x9fc0, 	/* DRCUML_FMOD_ROUND */
		0xdfc0, 	/* DRCUML_FMOD_CEIL */
		0xbfc0	 	/* DRCUML_FMOD_FLOOR */
	};
	drcbe_state *drcbe;
	int opnum, entry;

	/* allocate space in the cache for our state */
	drcbe = drccache_memory_alloc_near(cache, sizeof(*drcbe));
	if (drcbe == NULL)
		return NULL;
	memset(drcbe, 0, sizeof(*drcbe));

	/* remember our pointers */
	drcbe->drcuml = drcuml;
	drcbe->cache = cache;
	drcbe->rbpvalue = drccache_near(cache) + 0x80;

	/* build up necessary arrays */
	memcpy(drcbe->ssecontrol, sse_control, sizeof(drcbe->ssecontrol));
	drcbe->absmask32 = drccache_memory_alloc_near(cache, 16*2 + 15);
	drcbe->absmask32 = (void *)(((FPTR)drcbe->absmask32 + 15) & ~15);
	drcbe->absmask32[0] = drcbe->absmask32[1] = drcbe->absmask32[2] = drcbe->absmask32[3] = 0x7fffffff;
	drcbe->absmask64 = (UINT64 *)&drcbe->absmask32[4];
	drcbe->absmask64[0] = drcbe->absmask64[1] = U64(0x7fffffffffffffff);

	/* get pointers to C functions we need to call */
#ifdef ENABLE_DEBUGGER
	drcbe->mame_debug_hook = (x86code *)mame_debug_hook;
#endif
#if LOG_HASHJMPS
	drcbe->debug_log_hashjmp = (x86code *)debug_log_hashjmp;
#endif
	drcbe->drcmap_get_value = (x86code *)drcmap_get_value;

	/* allocate hash tables */
	drcbe->hash = drchash_alloc(cache, modes, addrbits, ignorebits);
	if (drcbe->hash == NULL)
		return NULL;

	/* allocate code map */
	drcbe->map = drcmap_alloc(cache, 0);
	if (drcbe->map == NULL)
		return NULL;

	/* allocate a label tracker */
	drcbe->labels = drclabel_list_alloc(cache);
	if (drcbe->labels == NULL)
		return NULL;

	/* build the opcode table (static but it doesn't hurt to regenerate it) */
	for (opnum = 0; opnum < ARRAY_LENGTH(opcode_table_source); opnum++)
		opcode_table[opcode_table_source[opnum].opcode] = opcode_table_source[opnum].func;

	/* build the flags map */
	for (entry = 0; entry < ARRAY_LENGTH(drcbe->flagsmap); entry++)
	{
		UINT8 flags = 0;
		if (entry & 0x001) flags |= DRCUML_FLAG_C;
		if (entry & 0x004) flags |= DRCUML_FLAG_U;
		if (entry & 0x040) flags |= DRCUML_FLAG_Z;
		if (entry & 0x080) flags |= DRCUML_FLAG_S;
		if (entry & 0x800) flags |= DRCUML_FLAG_V;
		drcbe->flagsmap[entry] = flags;
	}
	for (entry = 0; entry < ARRAY_LENGTH(drcbe->flagsunmap); entry++)
	{
		UINT64 flags = 0;
		if (entry & DRCUML_FLAG_C) flags |= 0x001;
		if (entry & DRCUML_FLAG_U) flags |= 0x004;
		if (entry & DRCUML_FLAG_Z) flags |= 0x040;
		if (entry & DRCUML_FLAG_S) flags |= 0x080;
		if (entry & DRCUML_FLAG_V) flags |= 0x800;
		drcbe->flagsunmap[entry] = flags;
	}

	/* create the log */
	if (flags & DRCUML_OPTION_LOG_NATIVE)
		drcbe->log = x86log_create_context("drcbex64.asm");

	return drcbe;
}


/*-------------------------------------------------
    drcbex64_free - free back-end specific state
-------------------------------------------------*/

static void drcbex64_free(drcbe_state *drcbe)
{
	/* free the log context */
	if (drcbe->log != NULL)
		x86log_free_context(drcbe->log);
}


/*-------------------------------------------------
    drcbex64_reset - reset back-end specific state
-------------------------------------------------*/

static void drcbex64_reset(drcbe_state *drcbe)
{
	UINT32 (*cpuid_ecx_stub)(void);
	x86code **dst;
	int spacenum;

	/* output a note to the log */
	if (drcbe->log != NULL)
		x86log_printf(drcbe->log, "\n\n===========\nCACHE RESET\n===========\n\n");

	/* fetch the accessors now that things are ready to go */
	for (spacenum = 0; spacenum < ADDRESS_SPACES; spacenum++)
		if (active_address_space[spacenum].accessors != NULL)
			drcbe->accessors[spacenum] = *active_address_space[spacenum].accessors;

	/* generate a little bit of glue code to set up the environment */
	dst = (x86code **)drccache_begin_codegen(drcbe->cache, 500);
	if (dst == NULL)
		fatalerror("Out of cache space after a reset!");

	/* generate a simple CPUID stub */
	cpuid_ecx_stub = (UINT32 (*)(void))*dst;
	emit_push_r64(dst, REG_RBX);														// push  rbx
	emit_mov_r32_imm(dst, REG_EAX, 1);													// mov   eax,1
	emit_cpuid(dst);																	// cpuid
	emit_mov_r32_r32(dst, REG_EAX, REG_ECX);											// mov   eax,ecx
	emit_pop_r64(dst, REG_RBX);															// pop   rbx
	emit_ret(dst);																		// ret

	/* call it to determine if we have SSE4.1 support */
	drcbe->sse41 = (((*cpuid_ecx_stub)() & 0x80000) != 0);

	/* generate an entry point */
	drcbe->entry = (x86_entry_point_func)*dst;
	emit_push_r64(dst, REG_RBX);														// push  rbx
	emit_push_r64(dst, REG_RSI);														// push  rsi
	emit_push_r64(dst, REG_RDI);														// push  rdi
	emit_push_r64(dst, REG_RBP);														// push  rbp
	emit_push_r64(dst, REG_R12);														// push  r12
	emit_push_r64(dst, REG_R13);														// push  r13
	emit_push_r64(dst, REG_R14);														// push  r14
	emit_push_r64(dst, REG_R15);														// push  r15
	emit_mov_r64_r64(dst, REG_RBP, REG_PARAM1);											// mov   rbp,param1
	emit_sub_r64_imm(dst, REG_RSP, 32);													// sub   rsp,32
	emit_mov_m64_r64(dst, MABS(drcbe, &drcbe->hashstacksave), REG_RSP);					// mov   [hashstacksave],rsp
	emit_sub_r64_imm(dst, REG_RSP, 8);													// sub   rsp,8
	emit_mov_m64_r64(dst, MABS(drcbe, &drcbe->stacksave), REG_RSP);						// mov   [stacksave],rsp
	emit_stmxcsr_m32(dst, MABS(drcbe, &drcbe->ssemode));								// stmxcsr [ssemode]
	emit_jmp_r64(dst, REG_PARAM2);														// jmp   param2
	if (drcbe->log != NULL)
		x86log_disasm_code_range(drcbe->log, "entry_point", (x86code *)drcbe->entry, *dst);

	/* generate an exit point */
	drcbe->exit = *dst;
	emit_ldmxcsr_m32(dst, MABS(drcbe, &drcbe->ssemode));								// ldmxcsr [ssemode]
	emit_mov_r64_m64(dst, REG_RSP, MABS(drcbe, &drcbe->hashstacksave));					// mov   rsp,[hashstacksave]
	emit_add_r64_imm(dst, REG_RSP, 32);													// add   rsp,32
	emit_pop_r64(dst, REG_R15);															// pop   r15
	emit_pop_r64(dst, REG_R14);															// pop   r14
	emit_pop_r64(dst, REG_R13);															// pop   r13
	emit_pop_r64(dst, REG_R12);															// pop   r12
	emit_pop_r64(dst, REG_RBP);															// pop   rbp
	emit_pop_r64(dst, REG_RDI);															// pop   rdi
	emit_pop_r64(dst, REG_RSI);															// pop   rsi
	emit_pop_r64(dst, REG_RBX);															// pop   rbx
	emit_ret(dst);																		// ret
	if (drcbe->log != NULL)
		x86log_disasm_code_range(drcbe->log, "exit_point", drcbe->exit, *dst);

	/* generate a no code point */
	drcbe->nocode = *dst;
	emit_ret(dst);																		// ret
	if (drcbe->log != NULL)
		x86log_disasm_code_range(drcbe->log, "nocode", drcbe->nocode, *dst);

	/* finish up codegen */
	drccache_end_codegen(drcbe->cache);

	/* reset our hash tables */
	drchash_reset(drcbe->hash);
	drchash_set_default_codeptr(drcbe->hash, drcbe->nocode);
}


/*-------------------------------------------------
    drcbex64_execute - execute a block of code
    referenced by the given handle
-------------------------------------------------*/

static int drcbex64_execute(drcbe_state *drcbe, drcuml_codehandle *entry)
{
	/* call our entry point which will jump to the destination */
	return (*drcbe->entry)(drcbe->rbpvalue, (x86code *)drcuml_handle_codeptr(entry));
}


/*-------------------------------------------------
    drcbex64_generate - generate code
-------------------------------------------------*/

static void drcbex64_generate(drcbe_state *drcbe, drcuml_block *block, const drcuml_instruction *instlist, UINT32 numinst)
{
	const char *blockname = NULL;
	char blockbuffer[100];
	drccodeptr *cachetop;
	x86code *base;
	x86code *dst;
	int inum;

	/* tell all of our utility objects that a block is beginning */
	drchash_block_begin(drcbe->hash, block, instlist, numinst);
	drclabel_block_begin(drcbe->labels, block);
	drcmap_block_begin(drcbe->map, block);

	/* begin codegen; fail if we can't */
	cachetop = drccache_begin_codegen(drcbe->cache, numinst * 8 * 4);
	if (cachetop == NULL)
		drcuml_block_abort(block);

	/* compute the base by aligning the cache top to a cache line (assumed to be 64 bytes) */
	base = (x86code *)(((FPTR)*cachetop + 63) & ~63);
	dst = base;

	/* generate code */
	for (inum = 0; inum < numinst; inum++)
	{
		const drcuml_instruction *inst = &instlist[inum];
		assert(inst->opcode < ARRAY_LENGTH(opcode_table));

		/* add a comment */
		if (drcbe->log != NULL)
		{
			char dasm[100];
			drcuml_disasm(inst, dasm);
			x86log_add_comment(drcbe->log, dst, "%s", dasm);
		}

		/* extract a blockname */
		if (blockname == NULL)
		{
			if (inst->opcode == DRCUML_OP_HANDLE)
				blockname = drcuml_handle_name((drcuml_codehandle *)(FPTR)inst->param[0].value);
			else if (inst->opcode == DRCUML_OP_HASH)
			{
				sprintf(blockbuffer, "Code: mode=%d PC=%08X", (UINT32)inst->param[0].value, (offs_t)inst->param[1].value);
				blockname = blockbuffer;
			}
		}

		/* generate code */
		dst = (*opcode_table[inst->opcode])(drcbe, dst, inst);
	}

	/* complete codegen */
	*cachetop = (drccodeptr)dst;
	drccache_end_codegen(drcbe->cache);

	/* log it */
	if (drcbe->log != NULL)
		x86log_disasm_code_range(drcbe->log, (blockname == NULL) ? "Unknown block" : blockname, base, drccache_top(drcbe->cache));

	/* tell all of our utility objects that the block is finished */
	drchash_block_end(drcbe->hash, block);
	drclabel_block_end(drcbe->labels, block);
	drcmap_block_end(drcbe->map, block);
}


/*-------------------------------------------------
    drcbex64_hash_exists - return true if the
    given mode/pc exists in the hash table
-------------------------------------------------*/

static int drcbex64_hash_exists(drcbe_state *drcbe, UINT32 mode, UINT32 pc)
{
	return drchash_code_exists(drcbe->hash, mode, pc);
}


/*-------------------------------------------------
    drcbex64_get_info - return information about
    the back-end implementation
-------------------------------------------------*/

static void drcbex64_get_info(drcbe_state *state, drcbe_info *info)
{
	for (info->direct_iregs = 0; info->direct_iregs < DRCUML_REG_I_END - DRCUML_REG_I0; info->direct_iregs++)
		if (int_register_map[info->direct_iregs] == 0)
			break;
	for (info->direct_fregs = 0; info->direct_fregs < DRCUML_REG_F_END - DRCUML_REG_F0; info->direct_fregs++)
		if (float_register_map[info->direct_fregs] == 0)
			break;
}



/***************************************************************************
    COMPILE HELPERS
***************************************************************************/

/*-------------------------------------------------
    param_normalize - convert a full parameter
    into a reduced set
-------------------------------------------------*/

static void param_normalize(drcbe_state *drcbe, const drcuml_parameter *src, drcuml_parameter *dest, UINT32 allowed)
{
	int regnum;

	switch (src->type)
	{
		/* immediates pass through */
		case DRCUML_PTYPE_IMMEDIATE:
			assert(allowed & PTYPE_I);
			dest->type = DRCUML_PTYPE_IMMEDIATE;
			dest->value = src->value;
			break;

		/* mapvars are converted to immediates with their current value */
		case DRCUML_PTYPE_MAPVAR:
			assert(allowed & PTYPE_I);
			dest->type = DRCUML_PTYPE_IMMEDIATE;
			dest->value = drcmap_get_last_value(drcbe->map, src->value);
			break;

		/* memory passes through */
		case DRCUML_PTYPE_MEMORY:
			assert(allowed & PTYPE_M);
			dest->type = DRCUML_PTYPE_MEMORY;
			dest->value = (FPTR)src->value;
			break;

		/* if a register maps to a register, keep it as a register; otherwise map it to memory */
		case DRCUML_PTYPE_INT_REGISTER:
			assert(allowed & PTYPE_R);
			assert(allowed & PTYPE_M);
			regnum = int_register_map[src->value - DRCUML_REG_I0];
			if (regnum != 0)
			{
				dest->type = DRCUML_PTYPE_INT_REGISTER;
				dest->value = regnum;
			}
			else
			{
				dest->type = DRCUML_PTYPE_MEMORY;
				dest->value = (FPTR)&drcbe->state.r[src->value - DRCUML_REG_I0];
			}
			break;

		/* if a register maps to a register, keep it as a register; otherwise map it to memory */
		case DRCUML_PTYPE_FLOAT_REGISTER:
			assert(allowed & PTYPE_F);
			assert(allowed & PTYPE_M);
			regnum = float_register_map[src->value - DRCUML_REG_F0];
			if (regnum != 0)
			{
				dest->type = DRCUML_PTYPE_FLOAT_REGISTER;
				dest->value = regnum;
			}
			else
			{
				dest->type = DRCUML_PTYPE_MEMORY;
				dest->value = (FPTR)&drcbe->state.f[src->value - DRCUML_REG_F0];
			}
			break;

		/* everything else is unexpected */
		default:
			fatalerror("Unexpected parameter type");
			break;
	}
}


/*-------------------------------------------------
    param_normalize_1 - normalize a single
    parameter instruction
-------------------------------------------------*/

static void param_normalize_1(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0)
{
	assert(inst->numparams == 1);
	param_normalize(drcbe, &inst->param[0], dest0, allowed0);
}


/*-------------------------------------------------
    param_normalize_2 - normalize a 2
    parameter instruction
-------------------------------------------------*/

static void param_normalize_2(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1)
{
	assert(inst->numparams == 2);
	param_normalize(drcbe, &inst->param[0], dest0, allowed0);
	param_normalize(drcbe, &inst->param[1], dest1, allowed1);
}


/*-------------------------------------------------
    param_normalize_2_commutative - normalize a 2
    parameter instruction, shuffling the
    parameters on the assumption that the two
    parameters can be swapped
-------------------------------------------------*/

static void param_normalize_2_commutative(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1)
{
	param_normalize_2(drcbe, inst, dest0, allowed0, dest1, allowed1);

	/* if the inner parameter is a memory operand, push it to the outer */
	if (dest0->type == DRCUML_PTYPE_MEMORY)
	{
		drcuml_parameter temp = *dest0;
		*dest0 = *dest1;
		*dest1 = temp;
	}

	/* if the inner parameter is an immediate, push it to the outer */
	if (dest0->type == DRCUML_PTYPE_IMMEDIATE)
	{
		drcuml_parameter temp = *dest0;
		*dest0 = *dest1;
		*dest1 = temp;
	}
}


/*-------------------------------------------------
    param_normalize_3 - normalize a 3
    parameter instruction
-------------------------------------------------*/

static void param_normalize_3(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1, drcuml_parameter *dest2, UINT32 allowed2)
{
	assert(inst->numparams == 3);
	param_normalize(drcbe, &inst->param[0], dest0, allowed0);
	param_normalize(drcbe, &inst->param[1], dest1, allowed1);
	param_normalize(drcbe, &inst->param[2], dest2, allowed2);
}


/*-------------------------------------------------
    param_normalize_3_commutative - normalize a 3
    parameter instruction, shuffling the
    parameters on the assumption that the last
    2 can be swapped
-------------------------------------------------*/

static void param_normalize_3_commutative(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1, drcuml_parameter *dest2, UINT32 allowed2)
{
	param_normalize_3(drcbe, inst, dest0, allowed0, dest1, allowed1, dest2, allowed2);

	/* if the inner parameter is a memory operand, push it to the outer */
	if (dest1->type == DRCUML_PTYPE_MEMORY)
	{
		drcuml_parameter temp = *dest1;
		*dest1 = *dest2;
		*dest2 = temp;
	}

	/* if the inner parameter is an immediate, push it to the outer */
	if (dest1->type == DRCUML_PTYPE_IMMEDIATE)
	{
		drcuml_parameter temp = *dest1;
		*dest1 = *dest2;
		*dest2 = temp;
	}

	/* if the destination and outer parameters are equal, move the outer to the inner */
	if (dest0->type == dest2->type && dest0->value == dest2->value && dest0->type != DRCUML_PTYPE_IMMEDIATE)
	{
		drcuml_parameter temp = *dest1;
		*dest1 = *dest2;
		*dest2 = temp;
	}
}


/*-------------------------------------------------
    param_normalize_4 - normalize a 4
    parameter instruction
-------------------------------------------------*/

static void param_normalize_4(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1, drcuml_parameter *dest2, UINT32 allowed2, drcuml_parameter *dest3, UINT32 allowed3)
{
	assert(inst->numparams == 4);
	param_normalize(drcbe, &inst->param[0], dest0, allowed0);
	param_normalize(drcbe, &inst->param[1], dest1, allowed1);
	param_normalize(drcbe, &inst->param[2], dest2, allowed2);
	param_normalize(drcbe, &inst->param[3], dest3, allowed3);
}


/*-------------------------------------------------
    param_normalize_4_commutative - normalize a 4
    parameter instruction, shuffling the
    parameters on the assumption that the last
    2 can be swapped
-------------------------------------------------*/

static void param_normalize_4_commutative(drcbe_state *drcbe, const drcuml_instruction *inst, drcuml_parameter *dest0, UINT32 allowed0, drcuml_parameter *dest1, UINT32 allowed1, drcuml_parameter *dest2, UINT32 allowed2, drcuml_parameter *dest3, UINT32 allowed3)
{
	param_normalize_4(drcbe, inst, dest0, allowed0, dest1, allowed1, dest2, allowed2, dest3, allowed3);

	/* if the inner parameter is a memory operand, push it to the outer */
	if (dest2->type == DRCUML_PTYPE_MEMORY)
	{
		drcuml_parameter temp = *dest2;
		*dest2 = *dest3;
		*dest3 = temp;
	}

	/* if the inner parameter is an immediate, push it to the outer */
	if (dest2->type == DRCUML_PTYPE_IMMEDIATE)
	{
		drcuml_parameter temp = *dest2;
		*dest2 = *dest3;
		*dest3 = temp;
	}

	/* if the destination and outer parameters are equal, move the outer to the inner */
	if (dest0->type == dest3->type && dest0->value == dest3->value && dest0->type != DRCUML_PTYPE_IMMEDIATE)
	{
		drcuml_parameter temp = *dest2;
		*dest2 = *dest3;
		*dest3 = temp;
	}
}



/***************************************************************************
    EMITTERS FOR 32-BIT OPERATIONS WITH PARAMETERS
***************************************************************************/

/*-------------------------------------------------
    emit_mov_r32_p32 - move a 32-bit parameter
    into a register
-------------------------------------------------*/

static void emit_mov_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (param->value == 0)
			emit_xor_r32_r32(dst, reg, reg);											// xor   reg,reg
		else
			emit_mov_r32_imm(dst, reg, param->value);									// mov   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_mov_r32_m32(dst, reg, MABS(drcbe, param->value));							// mov   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_mov_r32_r32(dst, reg, param->value);									// mov   reg,param
	}
}


/*-------------------------------------------------
    emit_mov_m32_p32 - move a 32-bit parameter
    into a memory location
-------------------------------------------------*/

static void emit_mov_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
		emit_mov_m32_imm(dst, MEMPARAMS, param->value);									// mov   [mem],param
	else if (param->type == DRCUML_PTYPE_MEMORY)
	{
		emit_mov_r32_m32(dst, REG_EAX, MABS(drcbe, param->value));						// mov   eax,[param]
		emit_mov_m32_r32(dst, MEMPARAMS, REG_EAX);										// mov   [mem],eax
	}
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_mov_m32_r32(dst, MEMPARAMS, param->value);									// mov   [mem],param
}


/*-------------------------------------------------
    emit_mov_p32_r32 - move a register into a
    32-bit parameter
-------------------------------------------------*/

static void emit_mov_p32_r32(drcbe_state *drcbe, x86code **dst, const drcuml_parameter *param, UINT8 reg)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_mov_m32_r32(dst, MABS(drcbe, param->value), reg);							// mov   [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_mov_r32_r32(dst, param->value, reg);									// mov   param,reg
	}
}


/*-------------------------------------------------
    emit_add_r32_p32 - add operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_add_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_add_r32_imm(dst, reg, param->value);									// add   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_add_r32_m32(dst, reg, MABS(drcbe, param->value));							// add   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_add_r32_r32(dst, reg, param->value);										// add   reg,param
}


/*-------------------------------------------------
    emit_add_m32_p32 - add operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_add_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_add_m32_imm(dst, MEMPARAMS, param->value);								// add   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_add_m32_r32(dst, MEMPARAMS, reg);											// add   [dest],reg
	}
}


/*-------------------------------------------------
    emit_adc_r32_p32 - adc operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_adc_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_adc_r32_imm(dst, reg, param->value);									// adc   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_adc_r32_m32(dst, reg, MABS(drcbe, param->value));							// adc   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_adc_r32_r32(dst, reg, param->value);										// adc   reg,param
}


/*-------------------------------------------------
    emit_adc_m32_p32 - adc operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_adc_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_adc_m32_imm(dst, MEMPARAMS, param->value);								// adc   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_adc_m32_r32(dst, MEMPARAMS, reg);											// adc   [dest],reg
	}
}


/*-------------------------------------------------
    emit_sub_r32_p32 - sub operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_sub_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_sub_r32_imm(dst, reg, param->value);									// sub   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_sub_r32_m32(dst, reg, MABS(drcbe, param->value));							// sub   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_sub_r32_r32(dst, reg, param->value);										// sub   reg,param
}


/*-------------------------------------------------
    emit_sub_m32_p32 - sub operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_sub_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_sub_m32_imm(dst, MEMPARAMS, param->value);								// sub   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_sub_m32_r32(dst, MEMPARAMS, reg);											// sub   [dest],reg
	}
}


/*-------------------------------------------------
    emit_sbb_r32_p32 - sbb operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_sbb_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_sbb_r32_imm(dst, reg, param->value);									// sbb   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_sbb_r32_m32(dst, reg, MABS(drcbe, param->value));							// sbb   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_sbb_r32_r32(dst, reg, param->value);										// sbb   reg,param
}


/*-------------------------------------------------
    emit_sbb_m32_p32 - sbb operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_sbb_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
			emit_sbb_m32_imm(dst, MEMPARAMS, param->value);								// sbb   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_sbb_m32_r32(dst, MEMPARAMS, reg);											// sbb   [dest],reg
	}
}


/*-------------------------------------------------
    emit_cmp_r32_p32 - cmp operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_cmp_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
		emit_cmp_r32_imm(dst, reg, param->value);										// cmp   reg,param
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_cmp_r32_m32(dst, reg, MABS(drcbe, param->value));							// cmp   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_cmp_r32_r32(dst, reg, param->value);										// cmp   reg,param
}


/*-------------------------------------------------
    emit_cmp_m32_p32 - cmp operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_cmp_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
		emit_cmp_m32_imm(dst, MEMPARAMS, param->value);									// cmp   [dest],param
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_cmp_m32_r32(dst, MEMPARAMS, reg);											// cmp   [dest],reg
	}
}


/*-------------------------------------------------
    emit_and_r32_p32 - and operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_and_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0xffffffff)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0)
			emit_xor_r32_r32(dst, reg, reg);											// xor   reg,reg
		else
			emit_and_r32_imm(dst, reg, param->value);									// and   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_and_r32_m32(dst, reg, MABS(drcbe, param->value));							// and   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_and_r32_r32(dst, reg, param->value);										// and   reg,param
}


/*-------------------------------------------------
    emit_and_m32_p32 - and operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_and_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0xffffffff)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0)
			emit_mov_m32_imm(dst, MEMPARAMS, 0);										// mov   [dest],0
		else
			emit_and_m32_imm(dst, MEMPARAMS, param->value);								// and   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_and_m32_r32(dst, MEMPARAMS, reg);											// and   [dest],reg
	}
}


/*-------------------------------------------------
    emit_test_r32_p32 - test operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_test_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
		emit_test_r32_imm(dst, reg, param->value);										// test   reg,param
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_test_m32_r32(dst, MABS(drcbe, param->value), reg);							// test   [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_test_r32_r32(dst, reg, param->value);										// test   reg,param
}


/*-------------------------------------------------
    emit_test_m32_p32 - test operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_test_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
		emit_test_m32_imm(dst, MEMPARAMS, param->value);								// test  [dest],param
	else if (param->type == DRCUML_PTYPE_MEMORY)
	{
		emit_mov_r32_p32(drcbe, dst, REG_EAX, param);									// mov   reg,param
		emit_test_m32_r32(dst, MEMPARAMS, REG_EAX);										// test  [dest],reg
	}
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_test_m32_r32(dst, MEMPARAMS, param->value);								// test  [dest],param
}


/*-------------------------------------------------
    emit_or_r32_p32 - or operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_or_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0xffffffff)
			emit_mov_r32_imm(dst, reg, -1);												// mov  reg,-1
		else
			emit_or_r32_imm(dst, reg, param->value);									// or   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_or_r32_m32(dst, reg, MABS(drcbe, param->value));							// or   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_or_r32_r32(dst, reg, param->value);										// or   reg,param
}


/*-------------------------------------------------
    emit_or_m32_p32 - or operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_or_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0xffffffff)
			emit_mov_m32_imm(dst, MEMPARAMS, -1);										// mov   [dest],-1
		else
			emit_or_m32_imm(dst, MEMPARAMS, param->value);								// or   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_or_m32_r32(dst, MEMPARAMS, reg);											// or   [dest],reg
	}
}


/*-------------------------------------------------
    emit_xor_r32_p32 - xor operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_xor_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0xffffffff)
			emit_not_r32(dst, reg);														// not   reg
		else
			emit_xor_r32_imm(dst, reg, param->value);									// xor   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_xor_r32_m32(dst, reg, MABS(drcbe, param->value));							// xor   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_xor_r32_r32(dst, reg, param->value);										// xor   reg,param
}


/*-------------------------------------------------
    emit_xor_m32_p32 - xor operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_xor_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else if (inst->condflags != 0 && (UINT32)param->value == 0xffffffff)
			emit_not_m32(dst, MEMPARAMS);												// not   [dest]
		else
			emit_xor_m32_imm(dst, MEMPARAMS, param->value);								// xor   [dest],param
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r32_p32(drcbe, dst, reg, param);										// mov   reg,param
		emit_xor_m32_r32(dst, MEMPARAMS, reg);											// xor   [dest],reg
	}
}


/*-------------------------------------------------
    emit_shl_r32_p32 - shl operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_shl_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shl_r32_imm(dst, reg, param->value);									// shl   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_shl_r32_cl(dst, reg);														// shl   reg,cl
	}
}


/*-------------------------------------------------
    emit_shl_m32_p32 - shl operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_shl_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shl_m32_imm(dst, MEMPARAMS, param->value);								// shl   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_shl_m32_cl(dst, MEMPARAMS);												// shl   [dest],cl
	}
}


/*-------------------------------------------------
    emit_shr_r32_p32 - shr operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_shr_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shr_r32_imm(dst, reg, param->value);									// shr   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_shr_r32_cl(dst, reg);														// shr   reg,cl
	}
}


/*-------------------------------------------------
    emit_shr_m32_p32 - shr operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_shr_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shr_m32_imm(dst, MEMPARAMS, param->value);								// shr   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_shr_m32_cl(dst, MEMPARAMS);												// shr   [dest],cl
	}
}


/*-------------------------------------------------
    emit_sar_r32_p32 - sar operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_sar_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_sar_r32_imm(dst, reg, param->value);									// sar   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_sar_r32_cl(dst, reg);														// sar   reg,cl
	}
}


/*-------------------------------------------------
    emit_sar_m32_p32 - sar operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_sar_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_sar_m32_imm(dst, MEMPARAMS, param->value);								// sar   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_sar_m32_cl(dst, MEMPARAMS);												// sar   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rol_r32_p32 - rol operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_rol_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rol_r32_imm(dst, reg, param->value);									// rol   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rol_r32_cl(dst, reg);														// rol   reg,cl
	}
}


/*-------------------------------------------------
    emit_rol_m32_p32 - rol operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_rol_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rol_m32_imm(dst, MEMPARAMS, param->value);								// rol   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rol_m32_cl(dst, MEMPARAMS);												// rol   [dest],cl
	}
}


/*-------------------------------------------------
    emit_ror_r32_p32 - ror operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_ror_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_ror_r32_imm(dst, reg, param->value);									// ror   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_ror_r32_cl(dst, reg);														// ror   reg,cl
	}
}


/*-------------------------------------------------
    emit_ror_m32_p32 - ror operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_ror_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_ror_m32_imm(dst, MEMPARAMS, param->value);								// ror   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_ror_m32_cl(dst, MEMPARAMS);												// ror   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rcl_r32_p32 - rcl operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_rcl_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcl_r32_imm(dst, reg, param->value);									// rcl   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rcl_r32_cl(dst, reg);														// rcl   reg,cl
	}
}


/*-------------------------------------------------
    emit_rcl_m32_p32 - rcl operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_rcl_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcl_m32_imm(dst, MEMPARAMS, param->value);								// rcl   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rcl_m32_cl(dst, MEMPARAMS);												// rcl   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rcr_r32_p32 - rcr operation to a 32-bit
    register from a 32-bit parameter
-------------------------------------------------*/

static void emit_rcr_r32_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcr_r32_imm(dst, reg, param->value);									// rcr   reg,param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rcr_r32_cl(dst, reg);														// rcr   reg,cl
	}
}


/*-------------------------------------------------
    emit_rcr_m32_p32 - rcr operation to a 32-bit
    memory location from a 32-bit parameter
-------------------------------------------------*/

static void emit_rcr_m32_p32(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcr_m32_imm(dst, MEMPARAMS, param->value);								// rcr   [dest],param
	}
	else
	{
		emit_mov_r32_p32(drcbe, dst, REG_ECX, param);									// mov   ecx,param
		emit_rcr_m32_cl(dst, MEMPARAMS);												// rcr   [dest],cl
	}
}



/***************************************************************************
    EMITTERS FOR 64-BIT OPERATIONS WITH PARAMETERS
***************************************************************************/

/*-------------------------------------------------
    emit_mov_r64_p64 - move a 64-bit parameter
    into a register
-------------------------------------------------*/

static void emit_mov_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (param->value == 0)
			emit_xor_r32_r32(dst, reg, reg);											// xor   reg,reg
		else
			emit_mov_r64_imm(dst, reg, param->value);									// mov   reg,param
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_mov_r64_m64(dst, reg, MABS(drcbe, param->value));							// mov   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_mov_r64_r64(dst, reg, param->value);									// mov   reg,param
	}
}


/*-------------------------------------------------
    emit_mov_p64_r64 - move a registers into a
    64-bit parameter
-------------------------------------------------*/

static void emit_mov_p64_r64(drcbe_state *drcbe, x86code **dst, const drcuml_parameter *param, UINT8 reg)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_mov_m64_r64(dst, MABS(drcbe, param->value), reg);							// mov   [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_mov_r64_r64(dst, param->value, reg);									// mov   param,reg
	}
}


/*-------------------------------------------------
    emit_add_r64_p64 - add operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_add_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_add_r64_imm(dst, reg, param->value);								// add   reg,param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_add_r64_r64(dst, reg, REG_R11);									// add   reg,r11
			}
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_add_r64_m64(dst, reg, MABS(drcbe, param->value));							// add   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_add_r64_r64(dst, reg, param->value);										// add   reg,param
}


/*-------------------------------------------------
    emit_add_m64_p64 - add operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_add_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_add_m64_imm(dst, MEMPARAMS, param->value);							// add   [mem],param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_add_m64_r64(dst, MEMPARAMS, REG_R11);								// add   [mem],r11
			}
		}
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_add_m64_r64(dst, MEMPARAMS, reg);											// add   [dest],reg
	}
}


/*-------------------------------------------------
    emit_adc_r64_p64 - adc operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_adc_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (short_immediate(param->value))
			emit_adc_r64_imm(dst, reg, param->value);									// adc   reg,param
		else
		{
			emit_mov_r64_imm(dst, REG_R11, param->value);								// mov   r11,param
			emit_adc_r64_r64(dst, reg, REG_R11);										// adc   reg,r11
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_adc_r64_m64(dst, reg, MABS(drcbe, param->value));							// adc   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_adc_r64_r64(dst, reg, param->value);										// adc   reg,param
}


/*-------------------------------------------------
    emit_adc_m64_p64 - adc operation to a 64-bit
    memory locaiton from a 64-bit parameter
-------------------------------------------------*/

static void emit_adc_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE && short_immediate(param->value))
		emit_adc_m64_imm(dst, MEMPARAMS, param->value);									// adc   [mem],param
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_adc_m64_r64(dst, MEMPARAMS, reg);											// adc   [dest],reg
	}
}


/*-------------------------------------------------
    emit_sub_r64_p64 - sub operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_sub_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_sub_r64_imm(dst, reg, param->value);								// sub   reg,param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_sub_r64_r64(dst, reg, REG_R11);									// sub   reg,r11
			}
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_sub_r64_m64(dst, reg, MABS(drcbe, param->value));							// sub   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_sub_r64_r64(dst, reg, param->value);										// sub   reg,param
}


/*-------------------------------------------------
    emit_sub_m64_p64 - sub operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_sub_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_sub_m64_imm(dst, MEMPARAMS, param->value);							// sub   [mem],param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_sub_m64_r64(dst, MEMPARAMS, REG_R11);								// sub   [mem],r11
			}
		}
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_sub_m64_r64(dst, MEMPARAMS, reg);											// sub   [dest],reg
	}
}


/*-------------------------------------------------
    emit_sbb_r64_p64 - sbb operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_sbb_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (short_immediate(param->value))
			emit_sbb_r64_imm(dst, reg, param->value);									// sbb   reg,param
		else
		{
			emit_mov_r64_imm(dst, REG_R11, param->value);								// mov   r11,param
			emit_sbb_r64_r64(dst, reg, REG_R11);										// sbb   reg,r11
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_sbb_r64_m64(dst, reg, MABS(drcbe, param->value));							// sbb   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_sbb_r64_r64(dst, reg, param->value);										// sbb   reg,param
}


/*-------------------------------------------------
    emit_sbb_m64_p64 - sbb operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_sbb_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE && short_immediate(param->value))
		emit_sbb_m64_imm(dst, MEMPARAMS, param->value);									// sbb   [mem],param
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_sbb_m64_r64(dst, MEMPARAMS, reg);											// sbb   [dest],reg
	}
}


/*-------------------------------------------------
    emit_cmp_r64_p64 - cmp operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_cmp_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (short_immediate(param->value))
			emit_cmp_r64_imm(dst, reg, param->value);									// cmp   reg,param
		else
		{
			emit_mov_r64_imm(dst, REG_R11, param->value);								// mov   r11,param
			emit_cmp_r64_r64(dst, reg, REG_R11);										// cmp   reg,r11
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_cmp_r64_m64(dst, reg, MABS(drcbe, param->value));							// cmp   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_cmp_r64_r64(dst, reg, param->value);										// cmp   reg,param
}


/*-------------------------------------------------
    emit_cmp_m64_p64 - cmp operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_cmp_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE && short_immediate(param->value))
		emit_cmp_m64_imm(dst, MEMPARAMS, param->value);									// cmp   [dest],param
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_cmp_m64_r64(dst, MEMPARAMS, reg);											// cmp   [dest],reg
	}
}


/*-------------------------------------------------
    emit_and_r64_p64 - and operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_and_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != U64(0xffffffffffffffff))
		{
			if (short_immediate(param->value))
				emit_and_r64_imm(dst, reg, param->value);								// and   reg,param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_and_r64_r64(dst, reg, REG_R11);									// and   reg,r11
			}
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_and_r64_m64(dst, reg, MABS(drcbe, param->value));							// and   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_and_r64_r64(dst, reg, param->value);										// and   reg,param
}


/*-------------------------------------------------
    emit_and_m64_p64 - and operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_and_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != U64(0xffffffffffffffff))
		{
			if (short_immediate(param->value))
				emit_and_m64_imm(dst, MEMPARAMS, param->value);							// and   [mem],param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_and_m64_r64(dst, MEMPARAMS, REG_R11);								// and   [mem],r11
			}
		}
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_and_m64_r64(dst, MEMPARAMS, reg);											// and   [dest],reg
	}
}


/*-------------------------------------------------
    emit_test_r64_p64 - test operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_test_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (short_immediate(param->value))
			emit_test_r64_imm(dst, reg, param->value);									// test  reg,param
		else
		{
			emit_mov_r64_imm(dst, REG_R11, param->value);								// mov   r11,param
			emit_test_r64_r64(dst, reg, REG_R11);										// test  reg,r11
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_test_m64_r64(dst, MABS(drcbe, param->value), reg);							// test  [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_test_r64_r64(dst, reg, param->value);										// test  reg,param
}


/*-------------------------------------------------
    emit_test_m64_p64 - test operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_test_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE && short_immediate(param->value))
		emit_test_m64_imm(dst, MEMPARAMS, param->value);								// test  [dest],param
	else if (param->type == DRCUML_PTYPE_MEMORY)
	{
		emit_mov_r64_p64(drcbe, dst, REG_EAX, param);									// mov   reg,param
		emit_test_m64_r64(dst, MEMPARAMS, REG_EAX);										// test  [dest],reg
	}
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_test_m64_r64(dst, MEMPARAMS, param->value);								// test  [dest],param
}


/*-------------------------------------------------
    emit_or_r64_p64 - or operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_or_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_or_r64_imm(dst, reg, param->value);								// or    reg,param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_or_r64_r64(dst, reg, REG_R11);										// or    reg,r11
			}
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_or_r64_m64(dst, reg, MABS(drcbe, param->value));							// or    reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_or_r64_r64(dst, reg, param->value);										// or    reg,param
}


/*-------------------------------------------------
    emit_or_m64_p64 - or operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_or_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (short_immediate(param->value))
				emit_or_m64_imm(dst, MEMPARAMS, param->value);							// or    [mem],param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_or_m64_r64(dst, MEMPARAMS, REG_R11);								// or    [mem],r11
			}
		}
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_or_m64_r64(dst, MEMPARAMS, reg);											// or    [dest],reg
	}
}


/*-------------------------------------------------
    emit_xor_r64_p64 - xor operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_xor_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (param->value == U64(0xffffffffffffffff))
				emit_not_r64(dst, reg);													// not   reg
			else if (short_immediate(param->value))
				emit_xor_r64_imm(dst, reg, param->value);								// xor   reg,param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_xor_r64_r64(dst, reg, REG_R11);									// xor   reg,r11
			}
		}
	}
	else if (param->type == DRCUML_PTYPE_MEMORY)
		emit_xor_r64_m64(dst, reg, MABS(drcbe, param->value));							// xor   reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
		emit_xor_r64_r64(dst, reg, param->value);										// xor   reg,param
}


/*-------------------------------------------------
    emit_xor_m64_p64 - xor operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_xor_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags != 0 || param->value != 0)
		{
			if (param->value == U64(0xffffffffffffffff))
				emit_not_m64(dst, MEMPARAMS);											// not   [mem]
			else if (short_immediate(param->value))
				emit_xor_m64_imm(dst, MEMPARAMS, param->value);							// xor   [mem],param
			else
			{
				emit_mov_r64_imm(dst, REG_R11, param->value);							// mov   r11,param
				emit_xor_m64_r64(dst, MEMPARAMS, REG_R11);								// xor   [mem],r11
			}
		}
	}
	else
	{
		int reg = param_select_register(REG_EAX, param, NULL);
		emit_mov_r64_p64(drcbe, dst, reg, param);										// mov   reg,param
		emit_xor_m64_r64(dst, MEMPARAMS, reg);											// xor   [dest],reg
	}
}


/*-------------------------------------------------
    emit_shl_r64_p64 - shl operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_shl_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shl_r64_imm(dst, reg, param->value);									// shl   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_shl_r64_cl(dst, reg);														// shl   reg,cl
	}
}


/*-------------------------------------------------
    emit_shl_m64_p64 - shl operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_shl_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_shl_m64_imm(dst, MEMPARAMS, param->value);								// shl   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_shl_m64_cl(dst, MEMPARAMS);												// shl   [dest],cl
	}
}


/*-------------------------------------------------
    emit_shr_r64_p64 - shr operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_shr_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_shr_r64_imm(dst, reg, param->value);									// shr   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_shr_r64_cl(dst, reg);														// shr   reg,cl
	}
}


/*-------------------------------------------------
    emit_shr_m64_p64 - shr operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_shr_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_shr_m64_imm(dst, MEMPARAMS, param->value);								// shr   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_shr_m64_cl(dst, MEMPARAMS);												// shr   [dest],cl
	}
}


/*-------------------------------------------------
    emit_sar_r64_p64 - sar operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_sar_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_sar_r64_imm(dst, reg, param->value);									// sar   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_sar_r64_cl(dst, reg);														// sar   reg,cl
	}
}


/*-------------------------------------------------
    emit_sar_m64_p64 - sar operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_sar_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_sar_m64_imm(dst, MEMPARAMS, param->value);								// sar   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_sar_m64_cl(dst, MEMPARAMS);												// sar   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rol_r64_p64 - rol operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_rol_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rol_r64_imm(dst, reg, param->value);									// rol   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rol_r64_cl(dst, reg);														// rol   reg,cl
	}
}


/*-------------------------------------------------
    emit_rol_m64_p64 - rol operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_rol_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_rol_m64_imm(dst, MEMPARAMS, param->value);								// rol   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rol_m64_cl(dst, MEMPARAMS);												// rol   [dest],cl
	}
}


/*-------------------------------------------------
    emit_ror_r64_p64 - ror operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_ror_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_ror_r64_imm(dst, reg, param->value);									// ror   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_ror_r64_cl(dst, reg);														// ror   reg,cl
	}
}


/*-------------------------------------------------
    emit_ror_m64_p64 - ror operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_ror_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_ror_m64_imm(dst, MEMPARAMS, param->value);								// ror   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_ror_m64_cl(dst, MEMPARAMS);												// ror   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rcl_r64_p64 - rcl operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_rcl_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcl_r64_imm(dst, reg, param->value);									// rcl   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rcl_r64_cl(dst, reg);														// rcl   reg,cl
	}
}


/*-------------------------------------------------
    emit_rcl_m64_p64 - rcl operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_rcl_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_rcl_m64_imm(dst, MEMPARAMS, param->value);								// rcl   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rcl_m64_cl(dst, MEMPARAMS);												// rcl   [dest],cl
	}
}


/*-------------------------------------------------
    emit_rcr_r64_p64 - rcr operation to a 64-bit
    register from a 64-bit parameter
-------------------------------------------------*/

static void emit_rcr_r64_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT32)param->value == 0)
			/* skip */;
		else
			emit_rcr_r64_imm(dst, reg, param->value);									// rcr   reg,param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rcr_r64_cl(dst, reg);														// rcr   reg,cl
	}
}


/*-------------------------------------------------
    emit_rcr_m64_p64 - rcr operation to a 64-bit
    memory location from a 64-bit parameter
-------------------------------------------------*/

static void emit_rcr_m64_p64(drcbe_state *drcbe, x86code **dst, DECLARE_MEMPARAMS, const drcuml_parameter *param, const drcuml_instruction *inst)
{
	if (param->type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->condflags == 0 && (UINT64)param->value == 0)
			/* skip */;
		else
			emit_rcr_m64_imm(dst, MEMPARAMS, param->value);								// rcr   [dest],param
	}
	else
	{
		emit_mov_r64_p64(drcbe, dst, REG_RCX, param);									// mov   rcx,param
		emit_rcr_m64_cl(dst, MEMPARAMS);												// rcr   [dest],cl
	}
}



/***************************************************************************
    EMITTERS FOR FLOATING POINT OPERATIONS WITH PARAMETERS
***************************************************************************/

/*-------------------------------------------------
    emit_movss_r128_p32 - move a 32-bit parameter
    into a register
-------------------------------------------------*/

static void emit_movss_r128_p32(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_movss_r128_m32(dst, reg, MABS(drcbe, param->value));						// movss reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_movss_r128_r128(dst, reg, param->value);								// movss reg,param
	}
}


/*-------------------------------------------------
    emit_movss_p32_r128 - move a register into a
    32-bit parameter
-------------------------------------------------*/

static void emit_movss_p32_r128(drcbe_state *drcbe, x86code **dst, const drcuml_parameter *param, UINT8 reg)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_movss_m32_r128(dst, MABS(drcbe, param->value), reg);						// movss [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_movss_r128_r128(dst, param->value, reg);								// movss param,reg
	}
}


/*-------------------------------------------------
    emit_movsd_r128_p64 - move a 64-bit parameter
    into a register
-------------------------------------------------*/

static void emit_movsd_r128_p64(drcbe_state *drcbe, x86code **dst, UINT8 reg, const drcuml_parameter *param)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_movsd_r128_m64(dst, reg, MABS(drcbe, param->value));						// movsd reg,[param]
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_movsd_r128_r128(dst, reg, param->value);								// movsd reg,param
	}
}


/*-------------------------------------------------
    emit_movsd_p64_r128 - move a register into a
    64-bit parameter
-------------------------------------------------*/

static void emit_movsd_p64_r128(drcbe_state *drcbe, x86code **dst, const drcuml_parameter *param, UINT8 reg)
{
	assert(param->type != DRCUML_PTYPE_IMMEDIATE);
	if (param->type == DRCUML_PTYPE_MEMORY)
		emit_movsd_m64_r128(dst, MABS(drcbe, param->value), reg);						// movsd [param],reg
	else if (param->type == DRCUML_PTYPE_INT_REGISTER)
	{
		if (reg != param->value)
			emit_movsd_r128_r128(dst, param->value, reg);								// movsd param,reg
	}
}



/***************************************************************************
    HELPERS TO CONVERT OTHER OPERATIONS TO MOVES
***************************************************************************/

/*-------------------------------------------------
    convert_to_mov_imm - convert an instruction
    into a mov immediate
-------------------------------------------------*/

static x86code *convert_to_mov_imm(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst, const drcuml_parameter *dstp, UINT64 imm)
{
	drcuml_instruction temp = *inst;
	temp.numparams = 2;
	temp.param[0] = *dstp;
	temp.param[1].type = DRCUML_PTYPE_IMMEDIATE;
	temp.param[1].value = imm;
	return op_mov(drcbe, dst, &temp);
}


/*-------------------------------------------------
    convert_to_mov_src1 - convert an instruction
    into a mov src1
-------------------------------------------------*/

static x86code *convert_to_mov_src1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst, const drcuml_parameter *dstp, const drcuml_parameter *srcp)
{
	drcuml_instruction temp = *inst;
	temp.numparams = 2;
	temp.param[0] = *dstp;
	temp.param[1] = *srcp;
	return op_mov(drcbe, dst, &temp);
}



/***************************************************************************
    OUT-OF-BAND CODE FIXUP CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    fixup_label - callback to fixup forward-
    referenced labels
-------------------------------------------------*/

static void fixup_label(void *parameter, drccodeptr labelcodeptr)
{
	drccodeptr src = parameter;

	/* find the end of the instruction */
	if (src[0] == 0xe3)
	{
		src += 1 + 1;
		src[-1] = labelcodeptr - src;
	}
	else if (src[0] == 0xe9)
	{
		src += 1 + 4;
		((UINT32 *)src)[-1] = labelcodeptr - src;
	}
	else if (src[0] == 0x0f && (src[1] & 0xf0) == 0x80)
	{
		src += 2 + 4;
		((UINT32 *)src)[-1] = labelcodeptr - src;
	}
	else
		fatalerror("fixup_label called with invalid jmp source!");
}


/*-------------------------------------------------
    fixup_exception - callback to perform cleanup
    and jump to an exception handler
-------------------------------------------------*/

static void fixup_exception(drccodeptr *codeptr, void *param1, void *param2, void *param3)
{
	drcuml_parameter handp, exp;
	drcbe_state *drcbe = param1;
	drccodeptr src = param2;
	const drcuml_instruction *inst = param3;
	drccodeptr dst = *codeptr;
	drccodeptr *targetptr;

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &handp, PTYPE_M, &exp, PTYPE_MRI);

	/* look up the handle target */
	targetptr = drcuml_handle_codeptr_addr((drcuml_codehandle *)handp.value);

	/* first fixup the jump to get us here */
	((UINT32 *)src)[-1] = dst - src;

	/* then store the exception parameter */
	emit_mov_m32_p32(drcbe, &dst, MABS(drcbe, &drcbe->state.exp), &exp);				// mov   [exp],exp

	/* push the original return address on the stack */
	emit_lea_r64_m64(&dst, REG_RAX, MABS(drcbe, src));									// lea   rax,[return]
	emit_push_r64(&dst, REG_RAX);														// push  rax
	if (*targetptr != NULL)
		emit_jmp(&dst, *targetptr);														// jmp   *targetptr
	else
		emit_jmp_m64(&dst, MABS(drcbe, targetptr));										// jmp   [targetptr]

	*codeptr = dst;
}



/***************************************************************************
    DEBUG HELPERS
***************************************************************************/

/*-------------------------------------------------
    debug_log_hashjmp - callback to handle
    logging of hashjmps
-------------------------------------------------*/

#if LOG_HASHJMPS
static void debug_log_hashjmp(int mode, offs_t pc)
{
	printf("mode=%d PC=%08X\n", mode, pc);
}
#endif



/***************************************************************************
    COMPILE-TIME OPCODES
***************************************************************************/

/*-------------------------------------------------
    op_handle - process a HANDLE opcode
-------------------------------------------------*/

static x86code *op_handle(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	assert(inst->numparams == 1);
	assert(inst->param[0].type == DRCUML_PTYPE_MEMORY);

	/* register the current pointer for the handle */
	drcuml_handle_set_codeptr((drcuml_codehandle *)(FPTR)inst->param[0].value, dst);

	/* by default, the handle points to prolog code that moves the stack pointer */
	emit_lea_r64_m64(&dst, REG_RSP, MBD(REG_RSP, -40));									// lea   rsp,[rsp-40]
	return dst;
}


/*-------------------------------------------------
    op_hash - process a HASH opcode
-------------------------------------------------*/

static x86code *op_hash(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	assert(inst->numparams == 2);
	assert(inst->param[0].type == DRCUML_PTYPE_IMMEDIATE);
	assert(inst->param[1].type == DRCUML_PTYPE_IMMEDIATE);

	/* register the current pointer for the mode/PC */
	drchash_set_codeptr(drcbe->hash, inst->param[0].value, inst->param[1].value, dst);
	return dst;
}


/*-------------------------------------------------
    op_label - process a LABEL opcode
-------------------------------------------------*/

static x86code *op_label(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	assert(inst->numparams == 1);
	assert(inst->param[0].type == DRCUML_PTYPE_IMMEDIATE);

	/* register the current pointer for the label */
	drclabel_set_codeptr(drcbe->labels, inst->param[0].value, dst);
	return dst;
}


/*-------------------------------------------------
    op_comment - process a COMMENT opcode
-------------------------------------------------*/

static x86code *op_comment(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	assert(inst->numparams == 1);
	assert(inst->param[0].type == DRCUML_PTYPE_MEMORY);

	/* do nothing */
	return dst;
}


/*-------------------------------------------------
    op_mapvar - process a MAPVAR opcode
-------------------------------------------------*/

static x86code *op_mapvar(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	assert(inst->numparams == 2);
	assert(inst->param[0].type == DRCUML_PTYPE_MAPVAR);
	assert(inst->param[1].type == DRCUML_PTYPE_IMMEDIATE);

	/* set the value of the specified mapvar */
	drcmap_set_value(drcbe->map, dst, inst->param[0].value, inst->param[1].value);
	return dst;
}



/***************************************************************************
    CONTROL FLOW OPCODES
***************************************************************************/

/*-------------------------------------------------
    op_debug - process a DEBUG opcode
-------------------------------------------------*/

static x86code *op_debug(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
#ifdef ENABLE_DEBUGGER
	if (Machine->debug_mode)
	{
		drcuml_parameter pcp;

		/* validate instruction */
		assert(inst->size == 4);
		assert(inst->condflags == DRCUML_COND_ALWAYS);

		/* normalize parameters */
		param_normalize_1(drcbe, inst, &pcp, PTYPE_MRI);

		/* push the parameter */
		emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &pcp);								// mov   param1,pcp
		emit_smart_call_m64(drcbe, &dst, &drcbe->mame_debug_hook);						// call  mame_debug_hook
	}
#endif

	return dst;
}


/*-------------------------------------------------
    op_exit - process an EXIT opcode
-------------------------------------------------*/

static x86code *op_exit(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter retp;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &retp, PTYPE_MRI);

	/* load the parameter into EAX */
	emit_mov_r32_p32(drcbe, &dst, REG_EAX, &retp);										// mov   eax,retp
	if (inst->condflags == DRCUML_COND_ALWAYS)
		emit_jmp(&dst, drcbe->exit);													// jmp   exit
	else
		emit_jcc(&dst, X86_CONDITION(inst->condflags), drcbe->exit);					// jcc   exit

	return dst;
}


/*-------------------------------------------------
    op_hashjmp - process a HASHJMP opcode
-------------------------------------------------*/

static x86code *op_hashjmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter modep, pcp, exp;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &modep, PTYPE_MRI, &pcp, PTYPE_MRI, &exp, PTYPE_M);

#if LOG_HASHJMPS
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &pcp);
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &modep);
	emit_smart_call_m64(drcbe, &dst, &drcbe->debug_log_hashjmp);
#endif

	/* load the stack base one word early so we end up at the right spot after our call below */
	emit_mov_r64_m64(&dst, REG_RSP, MABS(drcbe, &drcbe->hashstacksave));				// mov   rsp,[hashstacksave]

	/* fixed mode cases */
	if (modep.type == DRCUML_PTYPE_IMMEDIATE)
	{
		/* a straight immediate jump is direct, though we need the PC in EAX in case of failure */
		if (pcp.type == DRCUML_PTYPE_IMMEDIATE)
		{
			UINT32 l1val = (pcp.value >> drcbe->hash->l1shift) & drcbe->hash->l1mask;
			UINT32 l2val = (pcp.value >> drcbe->hash->l2shift) & drcbe->hash->l2mask;
			emit_call_m64(&dst, MABS(drcbe, &drcbe->hash->base[modep.value][l1val][l2val]));
																						// call  hash[modep][l1val][l2val]
		}

		/* a fixed mode but variable PC */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, REG_EAX, &pcp);								// mov   eax,pcp
			emit_mov_r32_r32(&dst, REG_EDX, REG_EAX);									// mov   edx,eax
			emit_shr_r32_imm(&dst, REG_EDX, drcbe->hash->l1shift);						// shr   edx,l1shift
			emit_and_r32_imm(&dst, REG_EAX, drcbe->hash->l2mask << drcbe->hash->l2shift);// and  eax,l2mask << l2shift
			emit_mov_r64_m64(&dst, REG_RDX, MBISD(REG_RBP, REG_RDX, 8, offset_from_rbp(drcbe, (FPTR)&drcbe->hash->base[modep.value][0])));
																						// mov   rdx,hash[modep+edx*8]
			emit_call_m64(&dst, MBISD(REG_RDX, REG_RAX, 8 >> drcbe->hash->l2shift, 0));	// call  [rdx+rax*shift]
		}
	}
	else
	{
		/* variable mode */
		int modereg = param_select_register(REG_ECX, &modep, NULL);
		emit_mov_r32_p32(drcbe, &dst, modereg, &modep);									// mov   modereg,modep
		emit_mov_r64_m64(&dst, REG_RCX, MBISD(REG_RBP, modereg, 8, offset_from_rbp(drcbe, (FPTR)&drcbe->hash->base[0])));
																						// mov   rcx,hash[modereg*8]

		/* fixed PC */
		if (pcp.type == DRCUML_PTYPE_IMMEDIATE)
		{
			UINT32 l1val = (pcp.value >> drcbe->hash->l1shift) & drcbe->hash->l1mask;
			UINT32 l2val = (pcp.value >> drcbe->hash->l2shift) & drcbe->hash->l2mask;
			emit_mov_r64_m64(&dst, REG_RDX, MBD(REG_RCX, l1val*8));						// mov   rdx,[rcx+l1val*8]
			emit_call_m64(&dst, MBD(REG_RDX, l2val*8));									// call  [l2val*8]
		}

		/* variable PC */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, REG_EAX, &pcp);								// mov   eax,pcp
			emit_mov_r32_r32(&dst, REG_EDX, REG_EAX);									// mov   edx,eax
			emit_shr_r32_imm(&dst, REG_EDX, drcbe->hash->l1shift);						// shr   edx,l1shift
			emit_mov_r64_m64(&dst, REG_RDX, MBISD(REG_RCX, REG_RDX, 8, 0));				// mov   rdx,[rcx+rdx*8]
			emit_and_r32_imm(&dst, REG_EAX, drcbe->hash->l2mask << drcbe->hash->l2shift);// and  eax,l2mask << l2shift
			emit_call_m64(&dst, MBISD(REG_RDX, REG_RAX, 8 >> drcbe->hash->l2shift, 0));	// call  [rdx+rax*shift]
		}
	}

	/* in all cases, if there is no code, we return here to generate the exception */
	emit_mov_m32_p32(drcbe, &dst, MABS(drcbe, &drcbe->state.exp), &pcp);				// mov   [exp],param
	emit_sub_r64_imm(&dst, REG_RSP, 8);													// sub   rsp,8
	emit_call_m64(&dst, MABS(drcbe, exp.value));										// call  [exp]

	return dst;
}


/*-------------------------------------------------
    op_jmp - process a JMP opcode
-------------------------------------------------*/

static x86code *op_jmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter labelp;
	x86code *jmptarget;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &labelp, PTYPE_I);

	/* look up the jump target and jump there */
	jmptarget = (x86code *)drclabel_get_codeptr(drcbe->labels, labelp.value, fixup_label, dst);
	if (jmptarget == NULL)
		jmptarget = dst + 0x7ffffff0;
	if (inst->condflags == DRCUML_COND_ALWAYS)
		emit_jmp(&dst, jmptarget);														// jmp   target
	else
		emit_jcc(&dst, X86_CONDITION(inst->condflags), jmptarget);						// jcc   target

	return dst;
}


/*-------------------------------------------------
    op_exh - process an EXH opcode
-------------------------------------------------*/

static x86code *op_exh(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter handp, exp;
	drccodeptr *targetptr;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &handp, PTYPE_M, &exp, PTYPE_MRI);

	/* look up the handle target */
	targetptr = drcuml_handle_codeptr_addr((drcuml_codehandle *)handp.value);

	/* perform the exception processing inline if unconditional */
	if (inst->condflags == DRCUML_COND_ALWAYS)
	{
		emit_mov_m32_p32(drcbe, &dst, MABS(drcbe, &drcbe->state.exp), &exp);			// mov   [exp],exp
		if (*targetptr != NULL)
			emit_call(&dst, *targetptr);												// call  *targetptr
		else
			emit_call_m64(&dst, MABS(drcbe, targetptr));								// call  [targetptr]
	}

	/* otherwise, jump to an out-of-band handler */
	else
	{
		emit_jcc(&dst, X86_CONDITION(inst->condflags), dst + 0x7ffffff0);				// jcc   exception
		drccache_request_oob_codegen(drcbe->cache, fixup_exception, drcbe, dst, (void *)inst);
	}
	return dst;
}


/*-------------------------------------------------
    op_callh - process a CALLH opcode
-------------------------------------------------*/

static x86code *op_callh(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter handp;
	drccodeptr *targetptr;
	emit_link skip = { 0 };

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &handp, PTYPE_M);

	/* look up the handle target */
	targetptr = drcuml_handle_codeptr_addr((drcuml_codehandle *)handp.value);

	/* skip if conditional */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		emit_jcc_short_link(&dst, X86_NOT_CONDITION(inst->condflags), &skip);			// jcc   skip

	/* jump through the handle; directly if a normal jump */
	if (*targetptr != NULL)
		emit_call(&dst, *targetptr);													// call  *targetptr
	else
		emit_call_m64(&dst, MABS(drcbe, targetptr));									// call  [targetptr]

	/* resolve the conditional link */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		resolve_link(&dst, &skip);													// skip:
	return dst;
}


/*-------------------------------------------------
    op_ret - process a RET opcode
-------------------------------------------------*/

static x86code *op_ret(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	emit_link skip = { 0 };

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));
	assert(inst->numparams == 0);

	/* skip if conditional */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		emit_jcc_short_link(&dst, X86_NOT_CONDITION(inst->condflags), &skip);			// jcc   skip

	/* return */
	emit_lea_r64_m64(&dst, REG_RSP, MBD(REG_RSP, 40));									// lea   rsp,[rsp+40]
	emit_ret(&dst);																		// ret

	/* resolve the conditional link */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		resolve_link(&dst, &skip);													// skip:
	return dst;
}


/*-------------------------------------------------
    op_callc - process a CALLC opcode
-------------------------------------------------*/

static x86code *op_callc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter funcp, paramp;
	emit_link skip = { 0 };

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &funcp, PTYPE_M, &paramp, PTYPE_M);

	/* skip if conditional */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		emit_jcc_short_link(&dst, X86_NOT_CONDITION(inst->condflags), &skip);			// jcc   skip

	/* perform the call */
	emit_mov_r64_imm(&dst, REG_PARAM1, paramp.value);									// mov   param1,paramp
	emit_smart_call_r64(drcbe, &dst, (x86code *)(FPTR)funcp.value, REG_RAX);			// call  funcp

	/* resolve the conditional link */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		resolve_link(&dst, &skip);													// skip:
	return dst;
}


/*-------------------------------------------------
    op_recover - process a RECOVER opcode
-------------------------------------------------*/

static x86code *op_recover(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, mvparam;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &mvparam, PTYPE_I);

	/* call the recovery code */
	emit_mov_r64_m64(&dst, REG_RAX, MABS(drcbe, &drcbe->stacksave));					// mov   rax,stacksave
	emit_mov_r64_m64(&dst, REG_RAX, MBD(REG_RAX, -8));									// mov   rax,[rax-4]
	emit_mov_r64_imm(&dst, REG_PARAM1, (FPTR)drcbe->map);								// mov   param1,drcbe->map
	emit_lea_r64_m64(&dst, REG_PARAM2, MBD(REG_RAX, -1));								// lea   param2,[rax-1]
	emit_mov_r64_imm(&dst, REG_PARAM3, inst->param[1].value);							// mov   param3,param[1].value
	emit_smart_call_m64(drcbe, &dst, &drcbe->drcmap_get_value);							// call  drcmap_get_value
	emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);										// mov   dstp,eax

	return dst;
}



/***************************************************************************
    INTERNAL REGISTER OPCODES
***************************************************************************/

/*-------------------------------------------------
    op_setfmod - process a SETFMOD opcode
-------------------------------------------------*/

static x86code *op_setfmod(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &srcp, PTYPE_MRI);

	/* immediate case */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		srcp.value &= 3;
		emit_mov_m8_imm(&dst, MABS(drcbe, &drcbe->state.fmod), srcp.value);				// mov   [fmod],srcp
		emit_ldmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssecontrol[srcp.value]));			// ldmxcsr fp_control[srcp]
	}

	/* register/memory case */
	else
	{
		emit_mov_r32_p32(drcbe, &dst, REG_EAX, &srcp);									// mov   eax,srcp
		emit_and_r32_imm(&dst, REG_EAX, 3);												// and   eax,3
		emit_mov_m8_r8(&dst, MABS(drcbe, &drcbe->state.fmod), REG_AL);					// mov   [fmod],al
		emit_ldmxcsr_m32(&dst, MBISD(REG_RBP, REG_RAX, 4, offset_from_rbp(drcbe, (FPTR)&drcbe->ssecontrol[0])));
																						// ldmxcsr fp_control[eax]
	}

	return dst;
}


/*-------------------------------------------------
    op_getfmod - process a GETFMOD opcode
-------------------------------------------------*/

static x86code *op_getfmod(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &dstp, PTYPE_MR);

	/* fetch the current mode and store to the destination */
	if (dstp.type == DRCUML_PTYPE_INT_REGISTER)
		emit_movzx_r32_m8(&dst, dstp.value, MABS(drcbe, &drcbe->state.fmod));					// movzx reg,[fmod]
	else
	{
		emit_movzx_r32_m8(&dst, REG_EAX, MABS(drcbe, &drcbe->state.fmod));						// movzx eax,[fmod]
		emit_mov_m32_r32(&dst, MABS(drcbe, dstp.value), REG_EAX);								// mov   [dstp],eax
	}

	return dst;
}


/*-------------------------------------------------
    op_getexp - process a GETEXP opcode
-------------------------------------------------*/

static x86code *op_getexp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &dstp, PTYPE_MR);

	/* fetch the exception parameter and store to the destination */
	if (dstp.type == DRCUML_PTYPE_INT_REGISTER)
		emit_mov_r32_m32(&dst, dstp.value, MABS(drcbe, &drcbe->state.exp));					// mov   reg,[exp]
	else
	{
		emit_mov_r32_m32(&dst, REG_EAX, MABS(drcbe, &drcbe->state.exp));						// mov   eax,[exp]
		emit_mov_m32_r32(&dst, MABS(drcbe, dstp.value), REG_EAX);								// mov   [dstp],eax
	}

	return dst;
}


/*-------------------------------------------------
    op_save - process a SAVE opcode
-------------------------------------------------*/

static x86code *op_save(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp;
	int regnum;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &dstp, PTYPE_M);

	/* copy live state to the destination */
	emit_mov_r64_imm(&dst, REG_RCX, dstp.value);										// mov    rcx,dstp

	/* copy flags */
	emit_pushf(&dst);																	// pushf
	emit_pop_r64(&dst, REG_RAX);														// pop    rax
	emit_and_r32_imm(&dst, REG_EAX, 0x8c5);												// and    eax,0x8c5
	emit_mov_r8_m8(&dst, REG_AL, MBISD(REG_RBP, REG_RAX, 1, offset_from_rbp(drcbe, (FPTR)&drcbe->flagsmap[0])));
																						// mov    al,[flags_map]
	emit_mov_m8_r8(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, flags)), REG_AL);	// mov    state->flags,al

	/* copy fmod and exp */
	emit_mov_r8_m8(&dst, REG_AL, MABS(drcbe, &drcbe->state.fmod));						// mov    al,[fmod]
	emit_mov_m8_r8(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, fmod)), REG_AL);	// mov    state->fmod,al
	emit_mov_r32_m32(&dst, REG_EAX, MABS(drcbe, &drcbe->state.exp));					// mov    eax,[exp]
	emit_mov_m32_r32(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, exp)), REG_EAX);	// mov    state->exp,eax

	/* copy integer registers */
	for (regnum = 0; regnum < ARRAY_LENGTH(drcbe->state.r); regnum++)
	{
		if (int_register_map[regnum] != 0)
			emit_mov_m64_r64(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, r[regnum].d)), int_register_map[regnum]);
		else
		{
			emit_mov_r64_m64(&dst, REG_RAX, MABS(drcbe, &drcbe->state.r[regnum].d));
			emit_mov_m64_r64(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, r[regnum].d)), REG_RAX);
		}
	}

	/* copy FP registers */
	for (regnum = 0; regnum < ARRAY_LENGTH(drcbe->state.f); regnum++)
	{
		if (float_register_map[regnum] != 0)
			emit_movsd_m64_r128(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, f[regnum].d)), float_register_map[regnum]);
		else
		{
			emit_mov_r64_m64(&dst, REG_RAX, MABS(drcbe, &drcbe->state.f[regnum].d));
			emit_mov_m64_r64(&dst, MBD(REG_RCX, offsetof(drcuml_machine_state, f[regnum].d)), REG_RAX);
		}
	}

	return dst;
}


/*-------------------------------------------------
    op_restore - process a RESTORE opcode
-------------------------------------------------*/

static x86code *op_restore(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp;
	int regnum;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_1(drcbe, inst, &dstp, PTYPE_M);

	/* copy live state from the destination */
	emit_mov_r64_imm(&dst, REG_ECX, dstp.value);										// mov    rcx,dstp

	/* copy integer registers */
	for (regnum = 0; regnum < ARRAY_LENGTH(drcbe->state.r); regnum++)
	{
		if (int_register_map[regnum] != 0)
			emit_mov_r64_m64(&dst, int_register_map[regnum], MBD(REG_RCX, offsetof(drcuml_machine_state, r[regnum].d)));
		else
		{
			emit_mov_r64_m64(&dst, REG_RAX, MBD(REG_RCX, offsetof(drcuml_machine_state, r[regnum].d)));
			emit_mov_m64_r64(&dst, MABS(drcbe, &drcbe->state.r[regnum].d), REG_RAX);
		}
	}

	/* copy FP registers */
	for (regnum = 0; regnum < ARRAY_LENGTH(drcbe->state.f); regnum++)
	{
		if (float_register_map[regnum] != 0)
			emit_movsd_r128_m64(&dst, float_register_map[regnum], MBD(REG_RCX, offsetof(drcuml_machine_state, f[regnum].d)));
		else
		{
			emit_mov_r64_m64(&dst, REG_RAX, MBD(REG_RCX, offsetof(drcuml_machine_state, f[regnum].d)));
			emit_mov_m64_r64(&dst, MABS(drcbe, &drcbe->state.f[regnum].d), REG_RAX);
		}
	}

	/* copy fmod and exp */
	emit_movzx_r32_m8(&dst, REG_EAX, MBD(REG_RCX, offsetof(drcuml_machine_state, fmod)));// movzx eax,state->fmod
	emit_and_r32_imm(&dst, REG_EAX, 3);													// and   eax,3
	emit_mov_m8_r8(&dst, MABS(drcbe, &drcbe->state.fmod), REG_AL);						// mov   [fmod],al
	emit_ldmxcsr_m32(&dst, MBISD(REG_RBP, REG_RAX, 4, offset_from_rbp(drcbe, (FPTR)&drcbe->ssecontrol[0])));
	emit_mov_r32_m32(&dst, REG_EAX, MBD(REG_RCX, offsetof(drcuml_machine_state, exp)));	// mov    eax,state->exp
	emit_mov_m32_r32(&dst, MABS(drcbe, &drcbe->state.exp), REG_EAX);					// mov    [exp],eax

	/* copy flags */
	emit_movzx_r32_m8(&dst, REG_EAX, MBD(REG_RCX, offsetof(drcuml_machine_state, flags)));// movzx eax,state->flags
	emit_push_m64(&dst, MBISD(REG_RBP, REG_RAX, 8, offset_from_rbp(drcbe, (FPTR)&drcbe->flagsunmap[0])));
																						// push   flags_unmap[eax*8]
	emit_popf(&dst);																	// popf

	return dst;
}



/***************************************************************************
    INTEGER OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    op_load1u - process a LOAD1U opcode
-------------------------------------------------*/

static x86code *op_load1u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* immediate index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
		emit_movzx_r32_m8(&dst, dstreg, MBD(basereg, baseoffs + 1*indp.value));			// movzx dstreg,[basep + 1*indp]

	/* other index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
		emit_movzx_r32_m8(&dst, dstreg, MBISD(basereg, indreg, 1, baseoffs));			// movzx dstreg,[basep + 1*indp]
	}

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_load1s - process a LOAD1S opcode
-------------------------------------------------*/

static x86code *op_load1s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* immediate index */
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsx_r32_m8(&dst, dstreg, MBD(basereg, baseoffs + 1*indp.value));		// movsx eax,[basep + 1*indp]

		/* other index */
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
			emit_movsx_r32_m8(&dst, dstreg, MBISD(basereg, indreg, 1, baseoffs));		// movsx eax,[basep + 1*indp]
		}

		/* general case */
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else
	{
		/* immediate index */
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsx_r64_m8(&dst, dstreg, MBD(basereg, baseoffs + 1*indp.value));		// movsx rax,[basep + 1*indp]

		/* other index */
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
			emit_movsx_r64_m8(&dst, dstreg, MBISD(basereg, indreg, 1, baseoffs));		// movsx rax,[basep + 1*indp]
		}

		/* general case */
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_load2u - process a LOAD2U opcode
-------------------------------------------------*/

static x86code *op_load2u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* immediate index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
		emit_movzx_r32_m16(&dst, dstreg, MBD(basereg, baseoffs + 2*indp.value));		// movzx dstreg,[basep + 2*indp]

	/* other index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
		emit_movzx_r32_m16(&dst, dstreg, MBISD(basereg, indreg, 2, baseoffs));			// movzx dstreg,[basep + 2*indp]
	}

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_load2s - process a LOAD2S opcode
-------------------------------------------------*/

static x86code *op_load2s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* immediate index */
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsx_r32_m16(&dst, dstreg, MBD(basereg, baseoffs + 2*indp.value));	// movsx eax,[basep + 2*indp]

		/* other index */
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
			emit_movsx_r32_m16(&dst, dstreg, MBISD(basereg, indreg, 2, baseoffs));		// movsx eax,[basep + 2*indp]
		}

		/* general case */
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else
	{
		/* immediate index */
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsx_r64_m16(&dst, dstreg, MBD(basereg, baseoffs + 2*indp.value));	// movsx rax,[basep + 2*indp]

		/* other index */
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
			emit_movsx_r64_m16(&dst, dstreg, MBISD(basereg, indreg, 2, baseoffs));		// movsx rax,[basep + 2*indp]
		}

		/* general case */
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_load4u - process a LOAD4U opcode
-------------------------------------------------*/

static x86code *op_load4u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* immediate index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
		emit_mov_r32_m32(&dst, dstreg, MBD(basereg, baseoffs + 4*indp.value));			// mov   dstreg,[basep + 4*indp]

	/* other index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
		emit_mov_r32_m32(&dst, dstreg, MBISD(basereg, indreg, 4, baseoffs));			// mov   dstreg,[basep + 4*indp]
	}

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_load4s - process a LOAD4S opcode
-------------------------------------------------*/

static x86code *op_load4s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* immediate index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
		emit_movsxd_r64_m32(&dst, dstreg, MBD(basereg, baseoffs + 4*indp.value));		// mov   dstreg,[basep + 4*indp]

	/* other index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
		emit_movsxd_r64_m32(&dst, dstreg, MISD(indreg, 4, basep.value));				// movsx dstreg,[basep + 4*indp]
	}

	/* general case */
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_load8u - process a LOAD8U opcode
-------------------------------------------------*/

static x86code *op_load8u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* immediate index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
		emit_mov_r64_m64(&dst, dstreg, MBD(basereg, baseoffs + 8*indp.value));			// mov   dstreg,[basep + 8*indp]

	/* other index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);
		emit_mov_r64_m64(&dst, dstreg, MBISD(basereg, indreg, 8, baseoffs));			// mov   dstreg,[basep + 8*indp]
	}

	/* general case */
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,edx:dstreg

	return dst;
}


/*-------------------------------------------------
    op_store1 - process a STORE1 opcode
-------------------------------------------------*/

static x86code *op_store1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp, basep, indp;
	int basereg, srcreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &basep, PTYPE_M, &indp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* pick a source register for the general case */
	srcreg = param_select_register(REG_EAX, &srcp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* degenerate case: constant index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m8_imm(&dst, MBD(basereg, baseoffs + 1*indp.value), srcp.value);	// mov   [basep + 1*indp],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m8_r8(&dst, MBD(basereg, baseoffs + 1*indp.value), srcreg);		// mov   [basep + 1*indp],srcreg
		}
	}

	/* normal case: variable index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);									// mov   indreg,indp
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m8_imm(&dst, MBISD(basereg, indreg, 1, baseoffs), srcp.value);		// mov   [basep + 1*ecx],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m8_r8(&dst, MBISD(basereg, indreg, 1, baseoffs), srcreg);			// mov   [basep + 1*ecx],srcreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_store2 - process a STORE2 opcode
-------------------------------------------------*/

static x86code *op_store2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp, basep, indp;
	int basereg, srcreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &basep, PTYPE_M, &indp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* pick a source register for the general case */
	srcreg = param_select_register(REG_EAX, &srcp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* degenerate case: constant index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m16_imm(&dst, MBD(basereg, baseoffs + 2*indp.value), srcp.value);	// mov   [basep + 2*indp],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m16_r16(&dst, MBD(basereg, baseoffs + 2*indp.value), srcreg);		// mov   [basep + 2*indp],srcreg
		}
	}

	/* normal case: variable index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);									// mov   indreg,indp
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m16_imm(&dst, MBISD(basereg, indreg, 2, baseoffs), srcp.value);	// mov   [basep + 2*ecx],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m16_r16(&dst, MBISD(basereg, indreg, 2, baseoffs), srcreg);		// mov   [basep + 2*ecx],srcreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_store4 - process a STORE4 opcode
-------------------------------------------------*/

static x86code *op_store4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp, basep, indp;
	int basereg, srcreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &basep, PTYPE_M, &indp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* pick a source register for the general case */
	srcreg = param_select_register(REG_EAX, &srcp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* degenerate case: constant index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m32_imm(&dst, MBD(basereg, baseoffs + 4*indp.value), srcp.value);	// mov   [basep + 4*indp],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m32_r32(&dst, MBD(basereg, baseoffs + 4*indp.value), srcreg);		// mov   [basep + 4*indp],srcreg
		}
	}

	/* normal case: variable index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);									// mov   indreg,indp
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m32_imm(&dst, MBISD(basereg, indreg, 4, baseoffs), srcp.value);	// mov   [basep + 4*ecx],srcp
		else
		{
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m32_r32(&dst, MBISD(basereg, indreg, 4, baseoffs), srcreg);		// mov   [basep + 4*ecx],srcreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_store8 - process a STORE8 opcode
-------------------------------------------------*/

static x86code *op_store8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp, basep, indp;
	int basereg, srcreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &basep, PTYPE_M, &indp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* pick a source register for the general case */
	srcreg = param_select_register(REG_EAX, &srcp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* degenerate case: constant index */
	if (indp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(srcp.value))
			emit_mov_m64_imm(&dst, MBD(basereg, baseoffs + 8*indp.value), srcp.value);	// mov   [basep + 8*indp],srcp
		else
		{
			emit_mov_r64_p64(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m64_r64(&dst, MBD(basereg, baseoffs + 8*indp.value), srcreg);		// mov   [basep + 8*indp],srcreg
		}
	}

	/* normal case: variable index */
	else
	{
		int indreg = param_select_register(REG_ECX, &indp, NULL);
		emit_mov_r32_p32(drcbe, &dst, indreg, &indp);									// mov   indreg,indp
		if (srcp.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(srcp.value))
			emit_mov_m64_imm(&dst, MBISD(basereg, indreg, 8, baseoffs), srcp.value);	// mov   [basep + 8*ecx],srcp
		else
		{
			emit_mov_r64_p64(drcbe, &dst, srcreg, &srcp);								// mov   srcreg,srcp
			emit_mov_m64_r64(&dst, MBISD(basereg, indreg, 8, baseoffs), srcreg);		// mov   [basep + 8*ecx],srcreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_read1u - process a READ1U opcode
-------------------------------------------------*/

static x86code *op_read1u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read byte handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_byte);// call   read_byte
	emit_movzx_r32_r8(&dst, dstreg, REG_AL);											// movzx  dstreg,al

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_read1s - process a READ1S opcode
-------------------------------------------------*/

static x86code *op_read1s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read byte handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_byte);// call   read_byte

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		emit_movsx_r32_r8(&dst, dstreg, REG_AL);										// movsx  dstreg,al
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov    dstp,dstreg
	}

	/* 64-bit form */
	else
	{
		/* general case */
		emit_movsx_r64_r8(&dst, dstreg, REG_AL);										// movsx  dstreg,al
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov    dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_read2u - process a READ2U opcode
-------------------------------------------------*/

static x86code *op_read2u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_word);// call   read_word
	emit_movzx_r32_r16(&dst, dstreg, REG_AX);											// movzx  dstreg,ax

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_read2s - process a READ2S opcode
-------------------------------------------------*/

static x86code *op_read2s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_word);// call   read_word

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		emit_movsx_r32_r16(&dst, dstreg, REG_AX);										// movsx  dstreg,ax
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov    dstp,dstreg
	}

	/* 64-bit form */
	else
	{
		/* general case */
		emit_movsx_r64_r16(&dst, dstreg, REG_AX);										// movsx  dstreg,ax
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov    dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_read2m - process a READ2M opcode
-------------------------------------------------*/

static x86code *op_read2m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp, maskp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read dword masked handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &maskp);									// mov    param1,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_word_masked);// call   read_word_masked
	emit_movzx_r32_r16(&dst, dstreg, REG_AX);											// movzx  dstreg,ax

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_read4u - process a READ4U opcode
-------------------------------------------------*/

static x86code *op_read4u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* set up a call to the read dword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_dword);// call   read_dword

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);									// mov   dstp,eax
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);									// mov   dstp,rax

	return dst;
}


/*-------------------------------------------------
    op_read4s - process a READ4S opcode
-------------------------------------------------*/

static x86code *op_read4s(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* set up a call to the read dword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_dword);// call   read_dword

	/* 64-bit form */
	emit_movsxd_r64_r32(&dst, dstreg, REG_EAX);											// movsxd dstreg,eax
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_read4m - process a READ4M opcode
-------------------------------------------------*/

static x86code *op_read4m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp, maskp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI);

	/* set up a call to the read dword masked handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &maskp);									// mov    param2,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_dword_masked);// call   read_dword_masked

	/* store to appropriate target size */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);									// mov   dstp,eax
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);									// mov   dstp,rax

	return dst;
}


/*-------------------------------------------------
    op_read8u - process a READ8U opcode
-------------------------------------------------*/

static x86code *op_read8u(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* set up a call to the read qword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_qword);// call   read_qword

	/* 64-bit form */
	emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);										// mov   dstp,rax

	return dst;
}


/*-------------------------------------------------
    op_read8m - process a READ8M opcode
-------------------------------------------------*/

static x86code *op_read8m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp, maskp;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI);

	/* set up a call to the read qword masked handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r64_p64(drcbe, &dst, REG_PARAM2, &maskp);									// mov    param2,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_qword_masked);// call   read_qword_masked

	/* 64-bit form */
	emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);										// mov   dstp,rax

	return dst;
}


/*-------------------------------------------------
    op_write1 - process a WRITE1 opcode
-------------------------------------------------*/

static x86code *op_write1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write byte handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_byte);// call   write_byte

	return dst;
}


/*-------------------------------------------------
    op_write2 - process a WRITE2 opcode
-------------------------------------------------*/

static x86code *op_write2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_word);// call   write_word

	return dst;
}


/*-------------------------------------------------
    op_writ2m - process a WRIT2M opcode
-------------------------------------------------*/

static x86code *op_writ2m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, maskp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM3, &maskp);									// mov    param3,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_word_masked);// call   write_word_masked
	return dst;
}


/*-------------------------------------------------
    op_write4 - process a WRITE4 opcode
-------------------------------------------------*/

static x86code *op_write4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write dword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_dword);// call   write_dword

	return dst;
}


/*-------------------------------------------------
    op_writ4m - process a WRIT4M opcode
-------------------------------------------------*/

static x86code *op_writ4m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, maskp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM3, &maskp);									// mov    param3,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_dword_masked);// call   write_dword_masked
	return dst;
}


/*-------------------------------------------------
    op_write8 - process a WRITE8 opcode
-------------------------------------------------*/

static x86code *op_write8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, srcp;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write qword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r64_p64(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_qword);// call   write_qword

	return dst;
}


/*-------------------------------------------------
    op_writ8m - process a WRIT8M opcode
-------------------------------------------------*/

static x86code *op_writ8m(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, maskp, srcp;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &maskp, PTYPE_MRI, &srcp, PTYPE_MRI);

	/* set up a call to the write word handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	emit_mov_r64_p64(drcbe, &dst, REG_PARAM2, &srcp);									// mov    param2,srcp
	emit_mov_r64_p64(drcbe, &dst, REG_PARAM3, &maskp);									// mov    param3,maskp
	emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_qword_masked);// call   write_qword_masked
	return dst;
}


/*-------------------------------------------------
    op_flags - process a FLAGS opcode
-------------------------------------------------*/

static x86code *op_flags(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, maskp, tablep;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &maskp, PTYPE_I, &tablep, PTYPE_M);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_ECX, &dstp, NULL);

	/* translate live flags into UML flags */
	emit_pushf(&dst);																	// pushf
	emit_pop_r64(&dst, REG_RAX);														// pop    rax
	emit_and_r32_imm(&dst, REG_EAX, 0x8c5);												// and    eax,0x8c5
	emit_movzx_r32_m8(&dst, REG_EAX, MBISD(REG_RBP, REG_RAX, 1, offset_from_rbp(drcbe, (FPTR)&drcbe->flagsmap[0])));
																						// movzx  eax,[flags_map]

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* no masking */
		if (maskp.value == 0xffffffff)
		{
			emit_mov_r32_m32(&dst, dstreg, MBISD(REG_RBP, REG_EAX, 4, offset_from_rbp(drcbe, tablep.value)));
																						// mov    dstreg,[eax*4+table]
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov    dstp,dstreg
		}

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &dstp);								// mov    dstreg,dstp
			emit_mov_r32_m32(&dst, REG_EDX, MBISD(REG_RBP, REG_EAX, 4, offset_from_rbp(drcbe, tablep.value)));
																						// mov    edx,[eax*4+table]
			emit_and_r32_imm(&dst, dstreg, ~maskp.value);								// and    dstreg,~mask
			emit_and_r32_imm(&dst, REG_EDX, maskp.value);								// and    edx,mask
			emit_or_r32_r32(&dst, dstreg, REG_EDX);										// or     dstreg,edx
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov    dstp,dstreg
		}
	}

	/* 64-bit form */
	else
	{
		/* no masking */
		if (maskp.value == U64(0xffffffffffffffff))
		{
			emit_mov_r64_m64(&dst, dstreg, MBISD(REG_RBP, REG_EAX, 8, offset_from_rbp(drcbe, tablep.value)));
																						// mov    dstreg,[eax*8+table]
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov    dstp,dstreg
		}

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &dstp);								// mov    dstreg,dstp
			emit_mov_r64_m64(&dst, REG_EDX, MBISD(REG_RBP, REG_EAX, 8, offset_from_rbp(drcbe, tablep.value)));
																						// mov    edx,[eax*84+table]
			if (short_immediate(~maskp.value))
				emit_and_r64_imm(&dst, dstreg, ~maskp.value);							// and    dstreg,~mask
			else
			{
				emit_mov_r64_imm(&dst, REG_R11, ~maskp.value);							// mov    r11,~mask
				emit_and_r64_r64(&dst, dstreg, REG_R11);								// and    dstreg,r11
			}
			if (short_immediate(maskp.value))
				emit_and_r64_imm(&dst, REG_EDX, maskp.value);							// and    edx,mask
			else
			{
				emit_mov_r64_imm(&dst, REG_R11, maskp.value);							// mov    r11,mask
				emit_and_r64_r64(&dst, REG_EDX, REG_R11);								// and    edx,r11
			}
			emit_or_r64_r64(&dst, dstreg, REG_EDX);										// or     dstreg,edx
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov    dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_mov - process a MOV opcode
-------------------------------------------------*/

static x86code *op_mov(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	x86code *savedst = dst;
	emit_link skip = { 0 };
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters, but only if we got here directly */
	/* other opcodes call through here with pre-normalized parameters */
	if (inst->opcode == DRCUML_OP_MOV)
		param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);
	else
	{
		dstp = inst->param[0];
		srcp = inst->param[1];
	}

	/* degenerate case: dest and source are equal */
	if (dstp.type == srcp.type && dstp.value == srcp.value)
		return dst;

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* always start with a jmp */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		emit_jcc_short_link(&dst, X86_NOT_CONDITION(inst->condflags), &skip);			// jcc   skip

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* register to memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_mov_m32_r32(&dst, MABS(drcbe, dstp.value), srcp.value);				// mov   [dstp],srcp

		/* immediate to memory */
		else if (dstp.type == DRCUML_PTYPE_MEMORY && srcp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_mov_m32_imm(&dst, MABS(drcbe, dstp.value), srcp.value);				// mov   [dstp],srcp

		/* conditional memory to register */
		else if (inst->condflags != 0 && dstp.type == DRCUML_PTYPE_INT_REGISTER && srcp.type == DRCUML_PTYPE_MEMORY)
		{
			dst = savedst;
			skip.target = NULL;
			emit_cmovcc_r32_m32(&dst, X86_CONDITION(inst->condflags), dstp.value, MABS(drcbe, srcp.value));
																						// cmovcc dstp,[srcp]
		}

		/* conditional register to register */
		else if (inst->condflags != 0 && dstp.type == DRCUML_PTYPE_INT_REGISTER && srcp.type == DRCUML_PTYPE_INT_REGISTER)
		{
			dst = savedst;
			skip.target = NULL;
			emit_cmovcc_r32_r32(&dst, X86_CONDITION(inst->condflags), dstp.value, srcp.value);
																						// cmovcc dstp,srcp
		}

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &srcp);								// mov   dstreg,srcp
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* register to memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_mov_m64_r64(&dst, MABS(drcbe, dstp.value), srcp.value);				// mov   [dstp],srcp

		/* immediate to memory */
		else if (dstp.type == DRCUML_PTYPE_MEMORY && srcp.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(srcp.value))
			emit_mov_m64_imm(&dst, MABS(drcbe, dstp.value), srcp.value);				// mov   [dstp],srcp

		/* conditional memory to register */
		else if (inst->condflags != 0 && dstp.type == DRCUML_PTYPE_INT_REGISTER && srcp.type == DRCUML_PTYPE_MEMORY)
		{
			dst = savedst;
			skip.target = NULL;
			emit_cmovcc_r64_m64(&dst, X86_CONDITION(inst->condflags), dstp.value, MABS(drcbe, srcp.value));
																						// cmovcc dstp,[srcp]
		}

		/* conditional register to register */
		else if (inst->condflags != 0 && dstp.type == DRCUML_PTYPE_INT_REGISTER && srcp.type == DRCUML_PTYPE_INT_REGISTER)
		{
			dst = savedst;
			skip.target = NULL;
			emit_cmovcc_r64_r64(&dst, X86_CONDITION(inst->condflags), dstp.value, srcp.value);
																						// cmovcc dstp,srcp
		}

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &srcp);								// mov   dstreg,srcp
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* resolve the jump */
	if (skip.target != NULL)
		resolve_link(&dst, &skip);
	return dst;
}


/*-------------------------------------------------
    op_zext1 - process a ZEXT1 opcode
-------------------------------------------------*/

static x86code *op_zext1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT8)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* general case */
	if (srcp.type == DRCUML_PTYPE_MEMORY)
		emit_movzx_r32_m8(&dst, dstreg, MABS(drcbe, srcp.value));						// movzx dstreg,[srcp]
	else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
		emit_movzx_r32_r8(&dst, dstreg, srcp.value);									// movzx dstreg,srcp

	/* 32-bit form */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	/* 64-bit form */
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_zext2 - process a ZEXT2 opcode
-------------------------------------------------*/

static x86code *op_zext2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT16)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* general case */
	if (srcp.type == DRCUML_PTYPE_MEMORY)
		emit_movzx_r32_m16(&dst, dstreg, MABS(drcbe, srcp.value));						// movzx dstreg,[srcp]
	else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
		emit_movzx_r32_r16(&dst, dstreg, srcp.value);									// movzx dstreg,srcp

	/* 32-bit form */
	if (inst->size == 4)
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	/* 64-bit form */
	else if (inst->size == 8)
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_zext4 - process a ZEXT4 opcode
-------------------------------------------------*/

static x86code *op_zext4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT32)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* general case */
	if (srcp.type == DRCUML_PTYPE_MEMORY)
		emit_mov_r32_m32(&dst, dstreg, MABS(drcbe, srcp.value));						// mov   dstreg,[srcp]
	else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
		emit_mov_r32_r32(&dst, dstreg, srcp.value);										// mov   dstreg,srcp
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_sext1 - process a SEXT1 opcode
-------------------------------------------------*/

static x86code *op_sext1(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT8)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* convert 8-bit source registers */
	if (srcp.type == DRCUML_PTYPE_INT_REGISTER && (srcp.value & 4))
	{
		emit_mov_r32_r32(&dst, REG_EAX, srcp.value);									// mov   eax,srcp
		srcp.value = REG_EAX;
	}

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_movsx_r32_m8(&dst, dstreg, MABS(drcbe, srcp.value));					// movsx dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movsx_r32_r8(&dst, dstreg, srcp.value);								// movsx dstreg,srcp
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* general case */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_movsx_r64_m8(&dst, dstreg, MABS(drcbe, srcp.value));					// movsx dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movsx_r64_r8(&dst, dstreg, srcp.value);								// movsx dstreg,srcp
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_sext2 - process a SEXT2 opcode
-------------------------------------------------*/

static x86code *op_sext2(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT16)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_movsx_r32_m16(&dst, dstreg, MABS(drcbe, srcp.value));					// movsx dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movsx_r32_r16(&dst, dstreg, srcp.value);								// movsx dstreg,srcp
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* general case */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_movsx_r64_m16(&dst, dstreg, MABS(drcbe, srcp.value));					// movsx dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movsx_r64_r16(&dst, dstreg, srcp.value);								// movsx dstreg,srcp
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_sext4 - process a SEXT4 opcode
-------------------------------------------------*/

static x86code *op_sext4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)srcp.value);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 64-bit form */
	if (inst->size == 8)
	{
		/* general case */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_movsxd_r64_m32(&dst, dstreg, MABS(drcbe, srcp.value));					// movsxd dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movsxd_r64_r32(&dst, dstreg, srcp.value);								// movsxd dstreg,srcp
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_xtract - process an XTRACT opcode
-------------------------------------------------*/

static x86code *op_xtract(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp, shiftp, maskp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI, &shiftp, PTYPE_MRI, &maskp, PTYPE_MRI);

	/* degenerate cases -- convert to a move or simple shift */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE && shiftp.type == DRCUML_PTYPE_IMMEDIATE && maskp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if (inst->size == 4)
			return convert_to_mov_imm(drcbe, dst, inst, &dstp, ((srcp.value << shiftp.value) | (srcp.value >> (32 - shiftp.value))) & maskp.value);
		else if (inst->size == 8)
			return convert_to_mov_imm(drcbe, dst, inst, &dstp, ((srcp.value << shiftp.value) | (srcp.value >> (64 - shiftp.value))) & maskp.value);
	}
	if (shiftp.type == DRCUML_PTYPE_IMMEDIATE && maskp.type == DRCUML_PTYPE_IMMEDIATE)
	{
		if ((inst->size == 4 && maskp.value == (UINT32)(0xffffffffUL << shiftp.value)) || (inst->size == 8 && maskp.value == U64(0xffffffffffffffff) << shiftp.value))
		{
			drcuml_instruction temp = *inst;
			temp.numparams = 3;
			return op_shl(drcbe, dst, &temp);
		}
		if ((inst->size == 4 && maskp.value == (UINT32)(0xffffffffUL >> (32 - shiftp.value))) || (inst->size == 8 && maskp.value == U64(0xffffffffffffffff) >> (64 - shiftp.value)))
		{
			drcuml_instruction temp = *inst;
			temp.numparams = 3;
			temp.param[2].value = inst->size * 8 - temp.param[2].value;
			return op_shr(drcbe, dst, &temp);
		}
	}

	/* pick a target register for the general case */
	dstreg = param_select_register2(REG_EAX, &dstp, &shiftp, &maskp);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_mov_r32_p32(drcbe, &dst, dstreg, &srcp);									// mov   dstreg,srcp
		emit_rol_r32_p32(drcbe, &dst, dstreg, &shiftp, inst);							// rol   dstreg,shiftp
		emit_and_r32_p32(drcbe, &dst, dstreg, &maskp, inst);							// and   dstreg,maskp
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_mov_r64_p64(drcbe, &dst, dstreg, &srcp);									// mov   dstreg,srcp
		emit_rol_r64_p64(drcbe, &dst, dstreg, &shiftp, inst);							// rol   dstreg,shiftp
		emit_and_r64_p64(drcbe, &dst, dstreg, &maskp, inst);							// and   dstreg,maskp
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_insert - process an INSERT opcode
-------------------------------------------------*/

static x86code *op_insert(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp, shiftp, maskp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI, &shiftp, PTYPE_MRI, &maskp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register2(REG_ECX, &dstp, &shiftp, &maskp);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_mov_r32_p32(drcbe, &dst, REG_EAX, &srcp);									// mov   eax,srcp
		emit_rol_r32_p32(drcbe, &dst, REG_EAX, &shiftp, inst);							// rol   eax,shiftp
		emit_mov_r32_p32(drcbe, &dst, dstreg, &dstp);									// mov   dstreg,dstp
		if (maskp.type == DRCUML_PTYPE_IMMEDIATE)
		{
			emit_and_r32_imm(&dst, REG_EAX, maskp.value);								// and   eax,maskp
			emit_and_r32_imm(&dst, dstreg, ~maskp.value);								// and   dstreg,~maskp
		}
		else
		{
			emit_mov_r32_p32(drcbe, &dst, REG_EDX, &maskp);								// mov   edx,maskp
			emit_and_r32_r32(&dst, REG_EAX, REG_EDX);									// and   eax,edx
			emit_not_r32(&dst, REG_EDX);												// not   edx
			emit_and_r32_r32(&dst, dstreg, REG_EDX);									// and   dstreg,edx
		}
		emit_or_r32_r32(&dst, dstreg, REG_EAX);											// or    dstreg,eax
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_mov_r64_p64(drcbe, &dst, REG_RAX, &srcp);									// mov   rax,srcp
		emit_mov_r64_p64(drcbe, &dst, REG_RDX, &maskp);									// mov   rdx,maskp
		emit_rol_r64_p64(drcbe, &dst, REG_RAX, &shiftp, inst);							// rol   rax,shiftp
		emit_mov_r64_p64(drcbe, &dst, dstreg, &dstp);									// mov   dstreg,dstp
		emit_and_r64_r64(&dst, REG_RAX, REG_RDX);										// and   eax,rdx
		emit_not_r64(&dst, REG_RDX);													// not   rdx
		emit_and_r64_r64(&dst, dstreg, REG_RDX);										// and   dstreg,rdx
		emit_or_r64_r64(&dst, dstreg, REG_RAX);											// or    dstreg,rax
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_add - process a ADD opcode
-------------------------------------------------*/

static x86code *op_add(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_V | DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value + src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && src1p.type == DRCUML_PTYPE_MEMORY && src1p.value == dstp.value)
			emit_add_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// add   [dstp],src2p

		/* reg = reg + imm */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
			emit_lea_r32_m32(&dst, dstp.value, MBD(src1p.value, src2p.value));			// lea   dstp,[src1p+src2p]

		/* reg = reg + reg */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_INT_REGISTER && inst->condflags == 0)
			emit_lea_r32_m32(&dst, dstp.value, MBISD(src1p.value, src2p.value, 1, 0));	// lea   dstp,[src1p+src2p]

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_add_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// add   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && src1p.type == DRCUML_PTYPE_MEMORY && src1p.value == dstp.value)
			emit_add_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// add   [dstp],src2p

		/* reg = reg + imm */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(src2p.value) && inst->condflags == 0)
			emit_lea_r64_m64(&dst, dstp.value, MBD(src1p.value, src2p.value));			// lea   dstp,[src1p+src2p]

		/* reg = reg + reg */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_INT_REGISTER && inst->condflags == 0)
			emit_lea_r64_m64(&dst, dstp.value, MBISD(src1p.value, src2p.value, 1, 0));	// lea   dstp,[src1p+src2p]

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_add_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// add   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_addc - process a ADDC opcode
-------------------------------------------------*/

static x86code *op_addc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_V | DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_adc_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// adc   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_adc_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// adc   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_adc_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// adc   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_adc_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// adc   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_sub - process a SUB opcode
-------------------------------------------------*/

static x86code *op_sub(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_V | DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value - src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && src1p.type == DRCUML_PTYPE_MEMORY && src1p.value == dstp.value)
			emit_sub_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sub   [dstp],src2p

		/* reg = reg - imm */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
			emit_lea_r32_m32(&dst, dstp.value, MBD(src1p.value, -src2p.value));			// lea   dstp,[src1p-src2p]

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sub_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// sub   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (dstp.type == DRCUML_PTYPE_MEMORY && src1p.type == DRCUML_PTYPE_MEMORY && src1p.value == dstp.value)
			emit_sub_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sub   [dstp],src2p

		/* reg = reg - imm */
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER && src1p.type == DRCUML_PTYPE_INT_REGISTER && src2p.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(src2p.type) && inst->condflags == 0)
			emit_lea_r64_m64(&dst, dstp.value, MBD(src1p.value, -src2p.value));			// lea   dstp,[src1p-src2p]

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sub_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// sub   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_subc - process a SUBC opcode
-------------------------------------------------*/

static x86code *op_subc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_V | DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_sbb_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sbb   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sbb_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// sbb   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_sbb_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sbb   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sbb_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// sbb   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_cmp - process a CMP opcode
-------------------------------------------------*/

static x86code *op_cmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter src1p, src2p;
	int src1reg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_V | DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && src1p.value == src2p.value)
	{
		emit_cmp_r32_r32(&dst, REG_EAX, REG_EAX);										// cmp   eax,eax
		return dst;
	}

	/* pick a target register for the general case */
	src1reg = param_select_register(REG_EAX, &src1p, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* memory versus anything */
		if (src1p.type == DRCUML_PTYPE_MEMORY)
			emit_cmp_m32_p32(drcbe, &dst, MABS(drcbe, src1p.value), &src2p, inst);		// cmp   [dstp],src2p

		/* general case */
		else
		{
			if (src1p.type == DRCUML_PTYPE_IMMEDIATE)
				emit_mov_r32_imm(&dst, src1reg, src1p.value);							// mov   src1reg,imm
			emit_cmp_r32_p32(drcbe, &dst, src1reg, &src2p, inst);						// cmp   src1reg,src2p
		}
	}

	/* 64-bit form */
	else
	{
		/* memory versus anything */
		if (src1p.type == DRCUML_PTYPE_MEMORY)
			emit_cmp_m64_p64(drcbe, &dst, MABS(drcbe, src1p.value), &src2p, inst);		// cmp   [dstp],src2p

		/* general case */
		else
		{
			if (src1p.type == DRCUML_PTYPE_IMMEDIATE)
				emit_mov_r64_imm(&dst, src1reg, src1p.value);							// mov   src1reg,imm
			emit_cmp_r64_p64(drcbe, &dst, src1reg, &src2p, inst);						// cmp   src1reg,src2p
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_mulu - process a MULU opcode
-------------------------------------------------*/

static x86code *op_mulu(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, edstp, src1p, src2p;
	int compute_hi;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_4_commutative(drcbe, inst, &dstp, PTYPE_MR, &edstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);
	compute_hi = (dstp.type != edstp.type || dstp.value != edstp.value);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
	{
		if (inst->size == 4)
		{
			if (!compute_hi)
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT32)src1p.value * (UINT32)src2p.value);
			else
			{
				UINT64 result = (UINT64)(UINT32)src1p.value * (UINT64)(UINT32)src2p.value;
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, result & 0xffffffff);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, result >> 32);
			}
		}
	}

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		emit_mov_r32_p32(drcbe, &dst, REG_EAX, &src1p);									// mov   eax,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_mul_m32(&dst, MABS(drcbe, src2p.value));								// mul   [src2p]
		else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
			emit_mul_r32(&dst, src2p.value);											// mul   src2p
		else if (src2p.type == DRCUML_PTYPE_IMMEDIATE)
		{
			emit_mov_r32_imm(&dst, REG_EDX, src2p.value);								// mov   edx,src2p
			emit_mul_r32(&dst, REG_EDX);												// mul   edx
		}
		emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);									// mov   dstp,eax
		if (compute_hi)
			emit_mov_p32_r32(drcbe, &dst, &edstp, REG_EDX);								// mov   edstp,edx
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* general case */
		emit_mov_r64_p64(drcbe, &dst, REG_RAX, &src1p);									// mov   rax,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_mul_m64(&dst, MABS(drcbe, src2p.value));								// mul   [src2p]
		else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
			emit_mul_r64(&dst, src2p.value);											// mul   src2p
		else if (src2p.type == DRCUML_PTYPE_IMMEDIATE)
		{
			emit_mov_r64_imm(&dst, REG_RDX, src2p.value);								// mov   rdx,src2p
			emit_mul_r64(&dst, REG_RDX);												// mul   rdx
		}
		emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);									// mov   dstp,rax
		if (compute_hi)
			emit_mov_p64_r64(drcbe, &dst, &edstp, REG_RDX);								// mov   edstp,rdx
	}
	return dst;
}


/*-------------------------------------------------
    op_muls - process a MULS opcode
-------------------------------------------------*/

static x86code *op_muls(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, edstp, src1p, src2p;
	int compute_hi;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_4_commutative(drcbe, inst, &dstp, PTYPE_MR, &edstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);
	compute_hi = (dstp.type != edstp.type || dstp.value != edstp.value);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
	{
		if (inst->size == 4)
		{
			if (!compute_hi)
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)src1p.value * (INT32)src2p.value);
			else
			{
				UINT64 result = (INT64)(INT32)src1p.value * (INT64)(INT32)src2p.value;
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, result & 0xffffffff);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, result >> 32);
			}
		}
	}

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* 32-bit destination with memory/immediate or register/immediate */
		if (!compute_hi && src1p.type != DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE)
		{
			if (src1p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_r32_m32_imm(&dst, REG_EAX, MABS(drcbe, src1p.value), src2p.value);	// imul  eax,[src1p],src2p
			else if (src1p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r32_r32_imm(&dst, REG_EAX, src1p.value, src2p.value);			// imul  eax,src1p,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);								// mov   dstp,eax
		}

		/* 32-bit destination, general case */
		else if (!compute_hi)
		{
			emit_mov_r32_p32(drcbe, &dst, REG_EAX, &src1p);								// mov   eax,src1p
			if (src2p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_r32_m32(&dst, REG_EAX, MABS(drcbe, src2p.value));					// imul  eax,[src2p]
			else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r32_r32(&dst, REG_EAX, src2p.value);							// imul  eax,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);								// mov   dstp,eax
		}

		/* 64-bit destination, general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, REG_EAX, &src1p);								// mov   eax,src1p
			if (src2p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_m32(&dst, MABS(drcbe, src2p.value));									// imul  [src2p]
			else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r32(&dst, src2p.value);										// imul  src2p
			else if (src2p.type == DRCUML_PTYPE_IMMEDIATE)
			{
				emit_mov_r32_imm(&dst, REG_EDX, src2p.value);							// mov   edx,src2p
				emit_imul_r32(&dst, REG_EDX);											// imul  edx
			}
			emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);								// mov   dstp,eax
			emit_mov_p32_r32(drcbe, &dst, &edstp, REG_EDX);								// mov   edstp,edx
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* 64-bit destination with memory/immediate or register/immediate */
		if (!compute_hi && src1p.type != DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && short_immediate(src2p.type))
		{
			if (src1p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_r64_m64_imm(&dst, REG_RAX, MABS(drcbe, src1p.value), src2p.value);// imul  rax,[src1p],src2p
			else if (src1p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r64_r64_imm(&dst, REG_RAX, src1p.value, src2p.value);			// imul  rax,src1p,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);								// mov   dstp,rax
		}

		/* 64-bit destination, general case */
		else if (!compute_hi)
		{
			emit_mov_r64_p64(drcbe, &dst, REG_RAX, &src1p);								// mov   rax,src1p
			if (src2p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_r64_m64(&dst, REG_RAX, MABS(drcbe, src2p.value));				// imul  rax,[src2p]
			else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r64_r64(&dst, REG_RAX, src2p.value);							// imul  rax,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);								// mov   dstp,rax
		}

		/* 128-bit destination, general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, REG_RAX, &src1p);								// mov   rax,src1p
			if (src2p.type == DRCUML_PTYPE_MEMORY)
				emit_imul_m64(&dst, MABS(drcbe, src2p.value));							// imul  [src2p]
			else if (src2p.type == DRCUML_PTYPE_INT_REGISTER)
				emit_imul_r64(&dst, src2p.value);										// imul  src2p
			else if (src2p.type == DRCUML_PTYPE_IMMEDIATE)
			{
				emit_mov_r64_imm(&dst, REG_RDX, src2p.value);							// mov   rdx,src2p
				emit_imul_r64(&dst, REG_RDX);											// imul  rdx
			}
			emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);								// mov   dstp,rax
			emit_mov_p64_r64(drcbe, &dst, &edstp, REG_RDX);								// mov   edstp,rdx
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_divu - process a DIVU opcode
-------------------------------------------------*/

static x86code *op_divu(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, edstp, src1p, src2p;
	int compute_rem;
	emit_link skip;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &edstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);
	compute_rem = (dstp.type != edstp.type || dstp.value != edstp.value);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
	{
		if (inst->size == 4)
		{
			if ((UINT32)src2p.value == 0)
				return dst;
			if (!compute_rem)
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT32)src1p.value / (UINT32)src2p.value);
			else
			{
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT32)src1p.value / (UINT32)src2p.value);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT32)src1p.value % (UINT32)src2p.value);
			}
		}
		else if (inst->size == 8)
		{
			UINT64 reslo, reshi;
			if (src2p.value == 0)
				return dst;
			if (!compute_rem)
			{
				reslo = (UINT64)src1p.value / (UINT64)src2p.value;
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, reslo);
			}
			else
			{
				reslo = (UINT64)src1p.value / (UINT64)src2p.value;
				reshi = (UINT64)src1p.value % (UINT64)src2p.value;
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, reslo);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, reshi);
			}
		}
	}

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		emit_mov_r32_p32(drcbe, &dst, REG_ECX, &src2p);									// mov   ecx,src2p
		emit_jecxz_link(&dst, &skip);													// jecxz skip
		emit_mov_r32_p32(drcbe, &dst, REG_EAX, &src1p);									// mov   eax,src1p
		emit_xor_r32_r32(&dst, REG_EDX, REG_EDX);										// xor   edx,edx
		emit_div_r32(&dst, REG_ECX);													// div   ecx
		emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);									// mov   dstp,eax
		if (compute_rem)
			emit_mov_p32_r32(drcbe, &dst, &edstp, REG_EDX);								// mov   edstp,edx
		resolve_link(&dst, &skip);													// skip:
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* general case */
		emit_mov_r64_p64(drcbe, &dst, REG_RCX, &src2p);									// mov   rcx,src2p
		emit_jrcxz_link(&dst, &skip);													// jrcxz skip
		emit_mov_r64_p64(drcbe, &dst, REG_RAX, &src1p);									// mov   rax,src1p
		emit_xor_r32_r32(&dst, REG_EDX, REG_EDX);										// xor   edx,edx
		emit_div_r64(&dst, REG_RCX);													// div   rcx
		emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);									// mov   dstp,rax
		if (compute_rem)
			emit_mov_p64_r64(drcbe, &dst, &edstp, REG_RDX);								// mov   edstp,rdx
		resolve_link(&dst, &skip);													// skip:
	}
	return dst;
}


/*-------------------------------------------------
    op_divs - process a DIVS opcode
-------------------------------------------------*/

static x86code *op_divs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, edstp, src1p, src2p;
	int compute_rem;
	emit_link skip;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_4(drcbe, inst, &dstp, PTYPE_MR, &edstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);
	compute_rem = (dstp.type != edstp.type || dstp.value != edstp.value);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
	{
		if (inst->size == 4)
		{
			if ((INT32)src2p.value == 0)
				return dst;
			if (!compute_rem)
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)src1p.value / (INT32)src2p.value);
			else
			{
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)src1p.value / (INT32)src2p.value);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)src1p.value % (INT32)src2p.value);
			}
		}
		else if (inst->size == 8)
		{
			UINT64 reslo, reshi;
			if (src2p.value == 0)
				return dst;
			if (!compute_rem)
			{
				reslo = (INT64)src1p.value / (INT64)src2p.value;
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, reslo);
			}
			else
			{
				reslo = (INT64)src1p.value / (INT64)src2p.value;
				reshi = (INT64)src1p.value % (INT64)src2p.value;
				dst = convert_to_mov_imm(drcbe, dst, inst, &dstp, reslo);
				return convert_to_mov_imm(drcbe, dst, inst, &dstp, reshi);
			}
		}
	}

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* general case */
		emit_mov_r32_p32(drcbe, &dst, REG_ECX, &src2p);									// mov   ecx,src2p
		emit_jecxz_link(&dst, &skip);													// jecxz skip
		emit_mov_r32_p32(drcbe, &dst, REG_EAX, &src1p);									// mov   eax,src1p
		emit_cdq(&dst);																	// cdq
		emit_idiv_r32(&dst, REG_ECX);													// idiv  ecx
		emit_mov_p32_r32(drcbe, &dst, &dstp, REG_EAX);									// mov   dstp,eax
		if (compute_rem)
			emit_mov_p32_r32(drcbe, &dst, &edstp, REG_EDX);								// mov   edstp,edx
		resolve_link(&dst, &skip);													// skip:
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* general case */
		emit_mov_r64_p64(drcbe, &dst, REG_RCX, &src2p);									// mov   rcx,src2p
		emit_jrcxz_link(&dst, &skip);													// jrcxz skip
		emit_mov_r64_p64(drcbe, &dst, REG_RAX, &src1p);									// mov   rax,src1p
		emit_cqo(&dst);																	// cqo
		emit_idiv_r64(&dst, REG_RCX);													// idiv  rcx
		emit_mov_p64_r64(drcbe, &dst, &dstp, REG_RAX);									// mov   dstp,rax
		if (compute_rem)
			emit_mov_p64_r64(drcbe, &dst, &edstp, REG_RDX);								// mov   edstp,rdx
		resolve_link(&dst, &skip);													// skip:
	}
	return dst;
}


/*-------------------------------------------------
    op_and - process a AND opcode
-------------------------------------------------*/

static x86code *op_and(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value & src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && (src2p.value & size_to_mask[inst->size]) == size_to_mask[inst->size] && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_and_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// and   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_and_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// and   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_and_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// and   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_and_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// and   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_test - process a TEST opcode
-------------------------------------------------*/

static x86code *op_test(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter src1p, src2p;
	int src1reg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_2_commutative(drcbe, inst, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* pick a target register for the general case */
	src1reg = param_select_register(REG_EAX, &src1p, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* src1p in memory */
		if (src1p.type == DRCUML_PTYPE_MEMORY)
			emit_test_m32_p32(drcbe, &dst, MABS(drcbe, src1p.value), &src2p, inst);		// test  [src1p],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, src1reg, &src1p);								// mov   src1reg,src1p
			emit_test_r32_p32(drcbe, &dst, src1reg, &src2p, inst);						// test  src1reg,src2p
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* src1p in memory */
		if (src1p.type == DRCUML_PTYPE_MEMORY)
			emit_test_m64_p64(drcbe, &dst, MABS(drcbe, src1p.value), &src2p, inst);		// test  [src1p],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, src1reg, &src1p);								// mov   src1reg,src1p
			emit_test_r64_p64(drcbe, &dst, src1reg, &src2p, inst);						// test  src1reg,src2p
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_or - process a OR opcode
-------------------------------------------------*/

static x86code *op_or(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value | src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_or_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// or    [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_or_r32_p32(drcbe, &dst, dstreg, &src2p, inst);							// or    dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_or_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// or    [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_or_r64_p64(drcbe, &dst, dstreg, &src2p, inst);							// or    dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_xor - process a XOR opcode
-------------------------------------------------*/

static x86code *op_xor(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value & src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_xor_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// xor   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_xor_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// xor   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_xor_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// xor   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_xor_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// xor   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_lzcnt - process a LZCNT opcode
-------------------------------------------------*/

static x86code *op_lzcnt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == 0);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (srcp.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, count_leading_zeros(srcp.value));

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &srcp);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_mov_r32_p32(drcbe, &dst, dstreg, &srcp);									// mov   dstreg,src1p
		emit_mov_r32_imm(&dst, REG_ECX, 32);											// mov   ecx,32
		emit_bsr_r32_r32(&dst, dstreg, dstreg);											// bsr   dstreg,dstreg
		emit_cmovcc_r32_r32(&dst, COND_Z, dstreg, REG_ECX);								// cmovz dstreg,ecx
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_mov_r64_p64(drcbe, &dst, dstreg, &srcp);									// mov   dstreg,src1p
		emit_mov_r64_imm(&dst, REG_RCX, 64);											// mov   rcx,64
		emit_bsr_r64_r64(&dst, dstreg, dstreg);											// bsr   dstreg,dstreg
		emit_cmovcc_r64_r64(&dst, COND_Z, dstreg, REG_RCX);								// cmovz dstreg,rcx
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_shl - process a SHL opcode
-------------------------------------------------*/

static x86code *op_shl(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, src1p.value << src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_shl_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// shl   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_shl_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// shl   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_shl_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// shl   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_shl_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// shl   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_shr - process a SHR opcode
-------------------------------------------------*/

static x86code *op_shr(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT64)src1p.value >> src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_shr_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// shr   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_shr_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// shr   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_shr_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// shr   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_shr_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// shr   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_sar - process a SAR opcode
-------------------------------------------------*/

static x86code *op_sar(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
	{
		if (inst->size == 4)
			return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT32)src1p.value >> src2p.value);
		else if (inst->size == 8)
			return convert_to_mov_imm(drcbe, dst, inst, &dstp, (INT64)src1p.value >> src2p.value);
	}
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_sar_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sar   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sar_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// sar   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_sar_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// sar   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_sar_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// sar   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_rol - process a rol opcode
-------------------------------------------------*/

static x86code *op_rol(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT64)src1p.value >> src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rol_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rol   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rol_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// rol   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rol_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rol   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rol_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// rol   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_ror - process a ROR opcode
-------------------------------------------------*/

static x86code *op_ror(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT64)src1p.value >> src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_ror_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// ror   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_ror_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// ror   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_ror_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// ror   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_ror_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// ror   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_rolc - process a ROLC opcode
-------------------------------------------------*/

static x86code *op_rolc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT64)src1p.value >> src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rcl_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rcl   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rcl_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// rcl   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rcl_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rcl   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rcl_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// rcl   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_rorc - process a RORC opcode
-------------------------------------------------*/

static x86code *op_rorc(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_Z | DRCUML_FLAG_S)) == 0);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MR, &src1p, PTYPE_MRI, &src2p, PTYPE_MRI);

	/* degenerate cases -- convert to a move */
	if (src1p.type == DRCUML_PTYPE_IMMEDIATE && src2p.type == DRCUML_PTYPE_IMMEDIATE && inst->condflags == 0)
		return convert_to_mov_imm(drcbe, dst, inst, &dstp, (UINT64)src1p.value >> src2p.value);
	if (src2p.type == DRCUML_PTYPE_IMMEDIATE && src2p.value == 0 && inst->condflags == 0)
		return convert_to_mov_src1(drcbe, dst, inst, &dstp, &src1p);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rcr_m32_p32(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rcr   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r32_p32(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rcr_r32_p32(drcbe, &dst, dstreg, &src2p, inst);						// rcr   dstreg,src2p
			emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		/* dstp == src1p in memory */
		if (src1p.type == dstp.type && src1p.value == dstp.value && dstp.type == DRCUML_PTYPE_MEMORY)
			emit_rcr_m64_p64(drcbe, &dst, MABS(drcbe, dstp.value), &src2p, inst);		// rcr   [dstp],src2p

		/* general case */
		else
		{
			emit_mov_r64_p64(drcbe, &dst, dstreg, &src1p);								// mov   dstreg,src1p
			emit_rcr_r64_p64(drcbe, &dst, dstreg, &src2p, inst);						// rcr   dstreg,src2p
			emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);								// mov   dstp,dstreg
		}
	}
	return dst;
}



/***************************************************************************
    FLOATING POINT OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    op_fload - process a FLOAD opcode
-------------------------------------------------*/

static x86code *op_fload(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, basep, indp;
	int basereg, dstreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MF, &basep, PTYPE_M, &indp, PTYPE_MRI);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movss_r128_m32(&dst, dstreg, MBD(basereg, baseoffs + 4*indp.value));	// movss  dstreg,[basep + 4*indp]
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);								// mov    indreg,indp
			emit_movss_r128_m32(&dst, dstreg, MBISD(basereg, indreg, 4, baseoffs));		// movss  dstreg,[basep + 4*indp]
		}
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss  dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsd_r128_m64(&dst, dstreg, MBD(basereg, baseoffs + 8*indp.value));	// movsd  dstreg,[basep + 8*indp]
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);								// mov    indreg,indp
			emit_movsd_r128_m64(&dst, dstreg, MBISD(basereg, indreg, 8, baseoffs));		// movsd  dstreg,[basep + 8*indp]
		}
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd  dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fstore - process a FSTORE opcode
-------------------------------------------------*/

static x86code *op_fstore(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter srcp, basep, indp;
	int basereg, srcreg;
	INT32 baseoffs;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &basep, PTYPE_M, &indp, PTYPE_MRI, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	srcreg = param_select_register(REG_XMM0, &srcp, NULL);

	/* determine the pointer base */
	basereg = get_base_register_and_offset(drcbe, &dst, basep.value, REG_RDX, &baseoffs);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, srcreg, &srcp);								// movss  srcreg,srcp
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movss_m32_r128(&dst, MBD(basereg, baseoffs + 4*indp.value), srcreg);	// movss  [basep + 4*indp],srcreg
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);								// mov    indreg,indp
			emit_movss_m32_r128(&dst, MBISD(basereg, indreg, 4, baseoffs), srcreg);		// movss  [basep + 4*indp],srcreg
		}
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, srcreg, &srcp);								// movsd  srcreg,srcp
		if (indp.type == DRCUML_PTYPE_IMMEDIATE)
			emit_movsd_m64_r128(&dst, MBD(basereg, baseoffs + 8*indp.value), srcreg);	// movsd  [basep + 8*indp],srcreg
		else
		{
			int indreg = param_select_register(REG_ECX, &indp, NULL);
			emit_mov_r32_p32(drcbe, &dst, indreg, &indp);								// mov    indreg,indp
			emit_movsd_m64_r128(&dst, MBISD(basereg, indreg, 8, baseoffs), srcreg);		// movsd  [basep + 8*indp],srcreg
		}
	}
	return dst;
}


/*-------------------------------------------------
    op_fread - process a FREAD opcode
-------------------------------------------------*/

static x86code *op_fread(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, spacep, addrp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MF, &spacep, PTYPE_I, &addrp, PTYPE_MRI);

	/* set up a call to the read dword/qword handler */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp
	if (inst->size == 4)
		emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_dword);// call   read_dword
	else if (inst->size == 8)
		emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].read_qword);// call   read_qword

	/* store result */
	if (inst->size == 4)
	{
		if (dstp.type == DRCUML_PTYPE_MEMORY)
			emit_mov_m32_r32(&dst, MABS(drcbe, dstp.value), REG_EAX);					// mov   [dstp],eax
		else if (dstp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_movd_r128_r32(&dst, dstp.value, REG_EAX);								// movd  dstp,eax
	}
	else if (inst->size == 8)
	{
		if (dstp.type == DRCUML_PTYPE_MEMORY)
			emit_mov_m64_r64(&dst, MABS(drcbe, dstp.value), REG_RAX);					// mov   [dstp],rax
		else if (dstp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_movq_r128_r64(&dst, dstp.value, REG_RAX);								// movq  dstp,rax
	}
	return dst;
}


/*-------------------------------------------------
    op_fwrite - process a FWRITE opcode
-------------------------------------------------*/

static x86code *op_fwrite(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter spacep, addrp, srcp;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &spacep, PTYPE_I, &addrp, PTYPE_MRI, &srcp, PTYPE_MF);

	/* general case */
	emit_mov_r32_p32(drcbe, &dst, REG_PARAM1, &addrp);									// mov    param1,addrp

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_mov_r32_m32(&dst, REG_PARAM2, MABS(drcbe, srcp.value));				// mov    param2,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_movd_r32_r128(&dst, REG_PARAM2, srcp.value);							// movd   param2,srcp
		emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_dword);// call   write_dword
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_mov_r64_m64(&dst, REG_PARAM2, MABS(drcbe, srcp.value));				// mov    param2,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_movq_r64_r128(&dst, REG_PARAM2, srcp.value);							// movq   param2,srcp
		emit_smart_call_m64(drcbe, &dst, (x86code **)&drcbe->accessors[spacep.value].write_qword);// call   write_qword
	}

	return dst;
}


/*-------------------------------------------------
    op_fmov - process a FMOV opcode
-------------------------------------------------*/

static x86code *op_fmov(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	emit_link skip = { 0 };
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS || (inst->condflags >= COND_Z && inst->condflags < DRCUML_COND_MAX));

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* always start with a jmp */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		emit_jcc_short_link(&dst, X86_NOT_CONDITION(inst->condflags), &skip);			// jcc   skip

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &srcp);								// movss dstreg,srcp
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &srcp);								// movsd dstreg,srcp
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}

	/* resolve the jump */
	if (inst->condflags != DRCUML_COND_ALWAYS)
		resolve_link(&dst, &skip);													// skip:
	return dst;
}


/*-------------------------------------------------
    op_ftoi4 - process a FTOI4 opcode
-------------------------------------------------*/

static x86code *op_ftoi4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtss2si_r32_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtss2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtss2si_r32_r128(&dst, dstreg, srcp.value);							// cvtss2si dstreg,srcp
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsd2si_r32_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsd2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtsd2si_r32_r128(&dst, dstreg, srcp.value);							// cvtsd2si dstreg,srcp
	}

	/* general case */
	emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg
	return dst;
}


/*-------------------------------------------------
    op_ftoi4t - process a FTOI4T opcode
-------------------------------------------------*/

static x86code *op_ftoi4t(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvttss2si_r32_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvttss2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvttss2si_r32_r128(&dst, dstreg, srcp.value);							// cvttss2si dstreg,srcp
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvttsd2si_r32_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvttsd2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvttsd2si_r32_r128(&dst, dstreg, srcp.value);							// cvttsd2si dstreg,srcp
	}

	/* general case */
	emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg
	return dst;
}


/*-------------------------------------------------
    op_ftoi4x - process a FTOI4R/F/C opcode
-------------------------------------------------*/

static x86code *op_ftoi4x(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst, int mode)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* non-SSE4.1 case */
	if (!drcbe->sse41 || inst->size == 8)
	{
		/* save and set the control word */
		emit_stmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssemodesave));						// stmxcsr [ssemodesave]
		emit_ldmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssecontrol[mode]));					// ldmxcsr fpcontrol[mode]

		/* 32-bit form */
		if (inst->size == 4)
		{
			if (srcp.type == DRCUML_PTYPE_MEMORY)
				emit_cvtss2si_r32_m32(&dst, dstreg, MABS(drcbe, srcp.value));			// cvtss2si dstreg,[srcp]
			else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
				emit_cvtss2si_r32_r128(&dst, dstreg, srcp.value);						// cvtss2si dstreg,srcp
		}

		/* 64-bit form */
		else if (inst->size == 8)
		{
			if (srcp.type == DRCUML_PTYPE_MEMORY)
				emit_cvtsd2si_r32_m64(&dst, dstreg, MABS(drcbe, srcp.value));			// cvtsd2si dstreg,[srcp]
			else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
				emit_cvtsd2si_r32_r128(&dst, dstreg, srcp.value);						// cvtsd2si dstreg,srcp
		}

		/* restore control word and proceed */
		emit_mov_p32_r32(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
		emit_ldmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssemodesave));						// ldmxcsr [ssemodesave]
	}

	/* SSE4.1 case */
	else
	{
		/* 32-bit form */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_roundss_r128_m32_imm(&dst, REG_XMM0, MABS(drcbe, srcp.value), fprnd_map[mode]);
																						// roundss  xmm0,[srcp],mode
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_roundss_r128_r128_imm(&dst, REG_XMM0, srcp.value, fprnd_map[mode]);	// roundss  xmm0,srcp,mode

		/* store to the destination */
		if (dstp.type == DRCUML_PTYPE_MEMORY)
			emit_movd_m32_r128(&dst, MABS(drcbe, dstp.value), REG_XMM0);				// movd     [dstp],xmm0
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movd_r32_r128(&dst, dstp.value, REG_XMM0);								// movd     dstp,xmm0
	}
	return dst;

}

static x86code *op_ftoi4r(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi4x(drcbe, dst, inst, DRCUML_FMOD_ROUND);
}

static x86code *op_ftoi4f(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi4x(drcbe, dst, inst, DRCUML_FMOD_FLOOR);
}

static x86code *op_ftoi4c(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi4x(drcbe, dst, inst, DRCUML_FMOD_CEIL);
}


/*-------------------------------------------------
    op_ftoi8 - process a FTOI8 opcode
-------------------------------------------------*/

static x86code *op_ftoi8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtss2si_r64_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtss2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtss2si_r64_r128(&dst, dstreg, srcp.value);							// cvtss2si dstreg,srcp
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsd2si_r64_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsd2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtsd2si_r64_r128(&dst, dstreg, srcp.value);							// cvtsd2si dstreg,srcp
	}

	/* general case */
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg
	return dst;
}


/*-------------------------------------------------
    op_ftoi8t - process a FTOI8T opcode
-------------------------------------------------*/

static x86code *op_ftoi8t(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvttss2si_r64_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvttss2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvttss2si_r64_r128(&dst, dstreg, srcp.value);							// cvttss2si dstreg,srcp
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvttsd2si_r64_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvttsd2si dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvttsd2si_r64_r128(&dst, dstreg, srcp.value);							// cvttsd2si dstreg,srcp
	}

	/* general case */
	emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);										// mov   dstp,dstreg
	return dst;
}


/*-------------------------------------------------
    op_ftoi8x - process a FTOI8R/F/C opcode
-------------------------------------------------*/

static x86code *op_ftoi8x(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst, int mode)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MR, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_EAX, &dstp, NULL);

	/* non-SSE4.1 case */
	if (!drcbe->sse41 || inst->size == 4)
	{
		/* save and set the control word */
		emit_stmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssemodesave));						// stmxcsr [ssemodesave]
		emit_ldmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssecontrol[mode]));					// ldmxcsr fpcontrol[mode]

		/* 32-bit form */
		if (inst->size == 4)
		{
			if (srcp.type == DRCUML_PTYPE_MEMORY)
				emit_cvtss2si_r64_m32(&dst, dstreg, MABS(drcbe, srcp.value));			// cvtss2si dstreg,[srcp]
			else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
				emit_cvtss2si_r64_r128(&dst, dstreg, srcp.value);						// cvtss2si dstreg,srcp
		}

		/* 64-bit form */
		else if (inst->size == 8)
		{
			if (srcp.type == DRCUML_PTYPE_MEMORY)
				emit_cvtsd2si_r64_m64(&dst, dstreg, MABS(drcbe, srcp.value));			// cvtsd2si dstreg,[srcp]
			else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
				emit_cvtsd2si_r64_r128(&dst, dstreg, srcp.value);						// cvtsd2si dstreg,srcp
		}

		/* restore control word and proceed */
		emit_mov_p64_r64(drcbe, &dst, &dstp, dstreg);									// mov   dstp,dstreg
		emit_ldmxcsr_m32(&dst, MABS(drcbe, &drcbe->ssemodesave));						// ldmxcsr [ssemodesave]
	}

	/* SSE4.1 case */
	else
	{
		/* 64-bit form */
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_roundsd_r128_m64_imm(&dst, REG_XMM0, MABS(drcbe, srcp.value), fprnd_map[mode]);
																						// roundsd  xmm0,[srcp],mode
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_roundsd_r128_r128_imm(&dst, REG_XMM0, srcp.value, fprnd_map[mode]);	// roundsd  xmm0,srcp,mode

		/* store to the destination */
		if (dstp.type == DRCUML_PTYPE_MEMORY)
			emit_movq_m64_r128(&dst, MABS(drcbe, dstp.value), REG_XMM0);				// movq     [dstp],xmm0
		else if (dstp.type == DRCUML_PTYPE_INT_REGISTER)
			emit_movq_r64_r128(&dst, dstp.value, REG_XMM0);								// movq     dstp,xmm0
	}
	return dst;

}

static x86code *op_ftoi8r(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi8x(drcbe, dst, inst, DRCUML_FMOD_ROUND);
}

static x86code *op_ftoi8f(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi8x(drcbe, dst, inst, DRCUML_FMOD_FLOOR);
}

static x86code *op_ftoi8c(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	return op_ftoi8x(drcbe, dst, inst, DRCUML_FMOD_CEIL);
}


/*-------------------------------------------------
    op_ffrfs - process a FFRFS opcode
-------------------------------------------------*/

static x86code *op_ffrfs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* general case */
	if (srcp.type == DRCUML_PTYPE_MEMORY)
		emit_cvtss2sd_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));					// cvtss2sd dstreg,[srcp]
	else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
		emit_cvtss2sd_r128_r128(&dst, dstreg, srcp.value);								// cvtss2sd dstreg,srcp
	emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);									// movsd    dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_ffrfd - process a FFRFD opcode
-------------------------------------------------*/

static x86code *op_ffrfd(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* general case */
	if (srcp.type == DRCUML_PTYPE_MEMORY)
		emit_cvtsd2ss_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));					// cvtsd2ss dstreg,[srcp]
	else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
		emit_cvtsd2ss_r128_r128(&dst, dstreg, srcp.value);								// cvtsd2ss dstreg,srcp
	emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);									// movss    dstp,dstreg

	return dst;
}


/*-------------------------------------------------
    op_ffri4 - process a FFRI4 opcode
-------------------------------------------------*/

static x86code *op_ffri4(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsi2ss_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsi2ss dstreg,[srcp]
		else
		{
			int srcreg = param_select_register(REG_EAX, &srcp, NULL);
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov      srcreg,srcp
			emit_cvtsi2ss_r128_r32(&dst, dstreg, srcreg);								// cvtsi2ss dstreg,srcreg
		}
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss    dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsi2sd_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsi2sd dstreg,[srcp]
		else
		{
			int srcreg = param_select_register(REG_EAX, &srcp, NULL);
			emit_mov_r32_p32(drcbe, &dst, srcreg, &srcp);								// mov      srcreg,srcp
			emit_cvtsi2sd_r128_r32(&dst, dstreg, srcreg);								// cvtsi2sd dstreg,srcreg
		}
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd    dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_ffri8 - process a FFRI8 opcode
-------------------------------------------------*/

static x86code *op_ffri8(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsi2ss_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsi2ss dstreg,[srcp]
		else
		{
			int srcreg = param_select_register(REG_EAX, &srcp, NULL);
			emit_mov_r64_p64(drcbe, &dst, srcreg, &srcp);								// mov      srcreg,srcp
			emit_cvtsi2ss_r128_r64(&dst, dstreg, srcreg);								// cvtsi2ss dstreg,srcreg
		}
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss    dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsi2sd_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsi2sd dstreg,[srcp]
		else
		{
			int srcreg = param_select_register(REG_EAX, &srcp, NULL);
			emit_mov_r64_p64(drcbe, &dst, srcreg, &srcp);								// mov      srcreg,srcp
			emit_cvtsi2sd_r128_r64(&dst, dstreg, srcreg);								// cvtsi2sd dstreg,srcreg
		}
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd    dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fadd - process a FADD opcode
-------------------------------------------------*/

static x86code *op_fadd(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MF, &src1p, PTYPE_MF, &src2p, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &src1p);								// movss dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_addss_r128_m32(&dst, dstreg, MABS(drcbe, src2p.value));				// addss dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_addss_r128_r128(&dst, dstreg, src2p.value);							// addss dstreg,src2p
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &src1p);								// movsd dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_addsd_r128_m64(&dst, dstreg, MABS(drcbe, src2p.value));				// addsd dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_addsd_r128_r128(&dst, dstreg, src2p.value);							// addsd dstreg,src2p
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fsub - process a FSUB opcode
-------------------------------------------------*/

static x86code *op_fsub(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MF, &src1p, PTYPE_MF, &src2p, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &src1p);								// movss dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_subss_r128_m32(&dst, dstreg, MABS(drcbe, src2p.value));				// subss dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_subss_r128_r128(&dst, dstreg, src2p.value);							// subss dstreg,src2p
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &src1p);								// movsd dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_subsd_r128_m64(&dst, dstreg, MABS(drcbe, src2p.value));				// subsd dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_subsd_r128_r128(&dst, dstreg, src2p.value);							// subsd dstreg,src2p
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fcmp - process a FCMP opcode
-------------------------------------------------*/

static x86code *op_fcmp(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter src1p, src2p;
	int src1reg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert((inst->condflags & ~(DRCUML_FLAG_C | DRCUML_FLAG_Z | DRCUML_FLAG_U)) == 0);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &src1p, PTYPE_MF, &src2p, PTYPE_MF);

	/* pick a target register for the general case */
	src1reg = param_select_register(REG_XMM0, &src1p, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, src1reg, &src1p);								// movss src1reg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_comiss_r128_m32(&dst, src1reg, MABS(drcbe, src2p.value));				// comiss src1reg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_comiss_r128_r128(&dst, src1reg, src2p.value);							// comiss src1reg,src2p
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, src1reg, &src1p);								// movsd src1reg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_comisd_r128_m64(&dst, src1reg, MABS(drcbe, src2p.value));				// comisd src1reg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_comisd_r128_r128(&dst, src1reg, src2p.value);							// comisd src1reg,src2p
	}
	return dst;
}


/*-------------------------------------------------
    op_fmul - process a FMUL opcode
-------------------------------------------------*/

static x86code *op_fmul(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3_commutative(drcbe, inst, &dstp, PTYPE_MF, &src1p, PTYPE_MF, &src2p, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &src1p);								// movss dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_mulss_r128_m32(&dst, dstreg, MABS(drcbe, src2p.value));				// mulss dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_mulss_r128_r128(&dst, dstreg, src2p.value);							// mulss dstreg,src2p
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &src1p);								// movsd dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_mulsd_r128_m64(&dst, dstreg, MABS(drcbe, src2p.value));				// mulsd dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_mulsd_r128_r128(&dst, dstreg, src2p.value);							// mulsd dstreg,src2p
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fdiv - process a FDIV opcode
-------------------------------------------------*/

static x86code *op_fdiv(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, src1p, src2p;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_3(drcbe, inst, &dstp, PTYPE_MF, &src1p, PTYPE_MF, &src2p, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &src2p);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &src1p);								// movss dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_divss_r128_m32(&dst, dstreg, MABS(drcbe, src2p.value));				// divss dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_divss_r128_r128(&dst, dstreg, src2p.value);							// divss dstreg,src2p
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &src1p);								// movsd dstreg,src1p
		if (src2p.type == DRCUML_PTYPE_MEMORY)
			emit_divsd_r128_m64(&dst, dstreg, MABS(drcbe, src2p.value));				// divsd dstreg,[src2p]
		else if (src2p.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_divsd_r128_r128(&dst, dstreg, src2p.value);							// divsd dstreg,src2p
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fneg - process a FNEG opcode
-------------------------------------------------*/

static x86code *op_fneg(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &srcp);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_xorps_r128_r128(&dst, dstreg, dstreg);										// xorps dstreg,dstreg
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_subss_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));					// subss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_subss_r128_r128(&dst, dstreg, srcp.value);								// subss dstreg,srcp
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_xorpd_r128_r128(&dst, dstreg, dstreg);										// xorpd dstreg,dstreg
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_subsd_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));					// subsd dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_subsd_r128_r128(&dst, dstreg, srcp.value);								// subsd dstreg,srcp
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fabs - process a FABS opcode
-------------------------------------------------*/

static x86code *op_fabs(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, &srcp);

	/* 32-bit form */
	if (inst->size == 4)
	{
		emit_movss_r128_p32(drcbe, &dst, dstreg, &srcp);								// movss dstreg,srcp
		emit_andps_r128_m128(&dst, dstreg, MABS(drcbe, drcbe->absmask32));				// andps dstreg,[absmask32]
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		emit_movsd_r128_p64(drcbe, &dst, dstreg, &srcp);								// movsd dstreg,srcp
		emit_andpd_r128_m128(&dst, dstreg, MABS(drcbe, drcbe->absmask64));				// andpd dstreg,[absmask64]
		emit_movsd_p64_r128(drcbe, &dst, &srcp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_fsqrt - process a FSQRT opcode
-------------------------------------------------*/

static x86code *op_fsqrt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_sqrtss_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// sqrtss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_sqrtss_r128_r128(&dst, dstreg, srcp.value);							// sqrtss dstreg,srcp
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_sqrtsd_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// sqrtsd dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_sqrtsd_r128_r128(&dst, dstreg, srcp.value);							// sqrtsd dstreg,srcp
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_frecip - process a FRECIP opcode
-------------------------------------------------*/

static x86code *op_frecip(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_rcpss_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));					// rcpss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_rcpss_r128_r128(&dst, dstreg, srcp.value);								// rcpss dstreg,srcp
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsd2ss_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsd2ss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtsd2ss_r128_r128(&dst, dstreg, srcp.value);							// cvtsd2ss dstreg,srcp
		emit_rcpss_r128_r128(&dst, dstreg, dstreg);										// rcpss dstreg,dstreg
		emit_cvtss2sd_r128_r128(&dst, dstreg, dstreg);									// cvtss2sd dstreg,dstreg
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}


/*-------------------------------------------------
    op_frsqrt - process a FRSQRT opcode
-------------------------------------------------*/

static x86code *op_frsqrt(drcbe_state *drcbe, x86code *dst, const drcuml_instruction *inst)
{
	drcuml_parameter dstp, srcp;
	int dstreg;

	/* validate instruction */
	assert(inst->size == 4 || inst->size == 8);
	assert(inst->condflags == DRCUML_COND_ALWAYS);

	/* normalize parameters */
	param_normalize_2(drcbe, inst, &dstp, PTYPE_MF, &srcp, PTYPE_MF);

	/* pick a target register for the general case */
	dstreg = param_select_register(REG_XMM0, &dstp, NULL);

	/* 32-bit form */
	if (inst->size == 4)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_rsqrtss_r128_m32(&dst, dstreg, MABS(drcbe, srcp.value));				// rsqrtss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_rsqrtss_r128_r128(&dst, dstreg, srcp.value);							// rsqrtss dstreg,srcp
		emit_movss_p32_r128(drcbe, &dst, &dstp, dstreg);								// movss dstp,dstreg
	}

	/* 64-bit form */
	else if (inst->size == 8)
	{
		if (srcp.type == DRCUML_PTYPE_MEMORY)
			emit_cvtsd2ss_r128_m64(&dst, dstreg, MABS(drcbe, srcp.value));				// cvtsd2ss dstreg,[srcp]
		else if (srcp.type == DRCUML_PTYPE_FLOAT_REGISTER)
			emit_cvtsd2ss_r128_r128(&dst, dstreg, srcp.value);							// cvtsd2ss dstreg,srcp
		emit_rsqrtss_r128_r128(&dst, dstreg, dstreg);									// rsqrtss dstreg,dstreg
		emit_cvtss2sd_r128_r128(&dst, dstreg, dstreg);									// cvtss2sd dstreg,dstreg
		emit_movsd_p64_r128(drcbe, &dst, &dstp, dstreg);								// movsd dstp,dstreg
	}
	return dst;
}
