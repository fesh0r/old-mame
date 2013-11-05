/***************************************************************************

    rspdrc.c

    Universal machine language-based Nintendo/SGI RSP emulator.
    Written by Harmony of the MESS team.
    SIMD versions of vector multiplication opcodes provided by Marathon Man
      of the CEN64 team.

    Copyright the MESS team.
    Released for general non-commercial use under the MAME license
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Future improvements/changes:

    * Confer with Aaron Giles about adding a memory hash-based caching
      system and static recompilation for maximum overhead minimization

***************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "rsp.h"
#include "rspdiv.h"
#include "rspfe.h"
#include "cpu/drcfe.h"
#include "cpu/drcuml.h"
#include "cpu/drcumlsh.h"

using namespace uml;

CPU_DISASSEMBLE( rsp );

extern offs_t rsp_dasm_one(char *buffer, offs_t pc, UINT32 op);

/***************************************************************************
    DEBUGGING
***************************************************************************/

#define LOG_UML                         (0)
#define LOG_NATIVE                      (0)

#define SINGLE_INSTRUCTION_MODE         (0)


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* map variables */
#define MAPVAR_PC                       M0
#define MAPVAR_CYCLES                   M1

/* size of the execution code cache */
#define CACHE_SIZE                      (32 * 1024 * 1024)

/* compilation boundaries -- how far back/forward does the analysis extend? */
#define COMPILE_BACKWARDS_BYTES         128
#define COMPILE_FORWARDS_BYTES          512
#define COMPILE_MAX_INSTRUCTIONS        ((COMPILE_BACKWARDS_BYTES/4) + (COMPILE_FORWARDS_BYTES/4))
#define COMPILE_MAX_SEQUENCE            64

/* exit codes */
#define EXECUTE_OUT_OF_CYCLES           0
#define EXECUTE_MISSING_CODE            1
#define EXECUTE_UNMAPPED_CODE           2
#define EXECUTE_RESET_CACHE             3



/***************************************************************************
    MACROS
***************************************************************************/

#define R32(reg)                rsp->impstate->regmap[reg]

/***************************************************************************
    STRUCTURES & TYPEDEFS
***************************************************************************/

/* fast RAM info */
struct fast_ram_info
{
	offs_t              start;                      /* start of the RAM block */
	offs_t              end;                        /* end of the RAM block */
	UINT8               readonly;                   /* TRUE if read-only */
	void *              base;                       /* base in memory where the RAM lives */
};


/* internal compiler state */
struct compiler_state
{
	UINT32              cycles;                     /* accumulated cycles */
	UINT8               checkints;                  /* need to check interrupts before next instruction */
	UINT8               checksoftints;              /* need to check software interrupts before next instruction */
	code_label  labelnum;                   /* index for local labels */
};

struct rspimp_state
{
	/* core state */
	drc_cache *         cache;                      /* pointer to the DRC code cache */
	drcuml_state *      drcuml;                     /* DRC UML generator state */
	rsp_frontend *      drcfe;                      /* pointer to the DRC front-end state */
	UINT32              drcoptions;                 /* configurable DRC options */

	/* internal stuff */
	UINT8               cache_dirty;                /* true if we need to flush the cache */
	UINT32              jmpdest;                    /* destination jump target */

	/* parameters for subroutines */
	UINT64              numcycles;                  /* return value from gettotalcycles */
	const char *        format;                     /* format string for print_debug */
	UINT32              arg0;                       /* print_debug argument 1 */
	UINT32              arg1;                       /* print_debug argument 2 */
	UINT32              arg2;                       /* print_debug argument 3 */
	UINT32              arg3;                       /* print_debug argument 4 */
	UINT32              vres[8];                    /* used for temporary vector results */

	/* register mappings */
	parameter   regmap[34];                 /* parameter to register mappings for all 32 integer registers */

	/* subroutines */
	code_handle *   entry;                      /* entry point */
	code_handle *   nocode;                     /* nocode exception handler */
	code_handle *   out_of_cycles;              /* out of cycles exception handler */
	code_handle *   read8;                      /* read byte */
	code_handle *   write8;                     /* write byte */
	code_handle *   read16;                     /* read half */
	code_handle *   write16;                    /* write half */
	code_handle *   read32;                     /* read word */
	code_handle *   write32;                    /* write word */
};

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void code_flush_cache(rsp_state *rsp);
static void code_compile_block(rsp_state *rsp, offs_t pc);

static void cfunc_unimplemented(void *param);
static void cfunc_set_cop0_reg(void *param);
static void cfunc_get_cop0_reg(void *param);
static void cfunc_mfc2(void *param);
static void cfunc_cfc2(void *param);
static void cfunc_mtc2(void *param);
static void cfunc_ctc2(void *param);
//static void cfunc_swc2(void *param);
//static void cfunc_lwc2(void *param);
static void cfunc_sp_set_status_cb(void *param);

static void cfunc_rsp_lbv(void *param);
static void cfunc_rsp_lsv(void *param);
static void cfunc_rsp_llv(void *param);
static void cfunc_rsp_ldv(void *param);
static void cfunc_rsp_lqv(void *param);
static void cfunc_rsp_lrv(void *param);
static void cfunc_rsp_lpv(void *param);
static void cfunc_rsp_luv(void *param);
static void cfunc_rsp_lhv(void *param);
static void cfunc_rsp_lfv(void *param);
static void cfunc_rsp_lwv(void *param);
static void cfunc_rsp_ltv(void *param);

static void cfunc_rsp_sbv(void *param);
static void cfunc_rsp_ssv(void *param);
static void cfunc_rsp_slv(void *param);
static void cfunc_rsp_sdv(void *param);
static void cfunc_rsp_sqv(void *param);
static void cfunc_rsp_srv(void *param);
static void cfunc_rsp_spv(void *param);
static void cfunc_rsp_suv(void *param);
static void cfunc_rsp_shv(void *param);
static void cfunc_rsp_sfv(void *param);
static void cfunc_rsp_swv(void *param);
static void cfunc_rsp_stv(void *param);

static void static_generate_entry_point(rsp_state *rsp);
static void static_generate_nocode_handler(rsp_state *rsp);
static void static_generate_out_of_cycles(rsp_state *rsp);
static void static_generate_memory_accessor(rsp_state *rsp, int size, int iswrite, const char *name, code_handle *&handleptr);

static int generate_lwc2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_swc2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static void generate_update_cycles(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, parameter param, int allow_exception);
static void generate_checksum_block(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *seqhead, const opcode_desc *seqlast);
static void generate_sequence_instruction(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static void generate_delay_slot_and_branch(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 linkreg);
static int generate_opcode(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_special(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_regimm(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_cop0(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);
static int generate_cop2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc);

static void log_add_disasm_comment(rsp_state *rsp, drcuml_block *block, UINT32 pc, UINT32 op);

/***************************************************************************
    HELPFUL DEFINES
***************************************************************************/

#define VDREG                       ((op >> 6) & 0x1f)
#define VS1REG                      ((op >> 11) & 0x1f)
#define VS2REG                      ((op >> 16) & 0x1f)
#define EL                          ((op >> 21) & 0xf)

#define SIMD_EXTRACT16(reg, value, element) \
	switch((element) & 7) \
	{ \
		case 0: value = _mm_extract_epi16(reg, 0); break; \
		case 1: value = _mm_extract_epi16(reg, 1); break; \
		case 2: value = _mm_extract_epi16(reg, 2); break; \
		case 3: value = _mm_extract_epi16(reg, 3); break; \
		case 4: value = _mm_extract_epi16(reg, 4); break; \
		case 5: value = _mm_extract_epi16(reg, 5); break; \
		case 6: value = _mm_extract_epi16(reg, 6); break; \
		case 7: value = _mm_extract_epi16(reg, 7); break; \
	}


#define SIMD_INSERT16(reg, value, element) \
	switch((element) & 7) \
	{ \
		case 0: reg = _mm_insert_epi16(reg, value, 0); break; \
		case 1: reg = _mm_insert_epi16(reg, value, 1); break; \
		case 2: reg = _mm_insert_epi16(reg, value, 2); break; \
		case 3: reg = _mm_insert_epi16(reg, value, 3); break; \
		case 4: reg = _mm_insert_epi16(reg, value, 4); break; \
		case 5: reg = _mm_insert_epi16(reg, value, 5); break; \
		case 6: reg = _mm_insert_epi16(reg, value, 6); break; \
		case 7: reg = _mm_insert_epi16(reg, value, 7); break; \
	}


#define SIMD_EXTRACT16C(reg, value, element) value = _mm_extract_epi16(reg, element);
#define SIMD_INSERT16C(reg, value, element) reg = _mm_insert_epi16(reg, value, element);

#define VREG_B(reg, offset)         rsp->v[(reg)].b[(offset)^1]
#define W_VREG_S(reg, offset)       rsp->v[(reg)].s[(offset)]
#define VREG_S(reg, offset)         (INT16)rsp->v[(reg)].s[(offset)]

#define VEC_EL_2(x,z)               (vector_elements_2[(x)][(z)])

#define ACCUM(x)        rsp->accum[x].q
#if USE_SIMD
INLINE UINT16 ACCUM_H(const rsp_state *rsp, int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->accum_h, out, x);
	return out;
}

INLINE UINT16 ACCUM_M(const rsp_state *rsp, int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->accum_m, out, x);
	return out;
}

INLINE UINT16 ACCUM_L(const rsp_state *rsp, int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->accum_l, out, x);
	return out;
}

#define SET_ACCUM_H(v, x)       SIMD_INSERT16(rsp->accum_h, v, x);
#define SET_ACCUM_M(v, x)       SIMD_INSERT16(rsp->accum_m, v, x);
#define SET_ACCUM_L(v, x)       SIMD_INSERT16(rsp->accum_l, v, x);

#define SCALAR_GET_VS1(out, i)  SIMD_EXTRACT16(rsp->xv[VS1REG], out, i)
#define SCALAR_GET_VS2(out, i)  SIMD_EXTRACT16(rsp->xv[VS2REG], out, VEC_EL_2(EL, i))

#else

#define ACCUM_H(v, x)           (UINT16)rsp->accum[x].w[3]
#define ACCUM_M(v, x)           (UINT16)rsp->accum[x].w[2]
#define ACCUM_L(v, x)           (UINT16)rsp->accum[x].w[1]

#define SET_ACCUM_H(v, x)       rsp->accum[x].w[3] = v;
#define SET_ACCUM_M(v, x)       rsp->accum[x].w[2] = v;
#define SET_ACCUM_L(v, x)       rsp->accum[x].w[1] = v;

#define SCALAR_GET_VS1(out, i)  out = VREG_S(VS1REG, i)
#define SCALAR_GET_VS2(out, i)  out = VREG_S(VS2REG, VEC_EL_2(EL, i))

#endif // USE_SIMD

#define CARRY       0
#define COMPARE     1
#define CLIP1       2
#define ZERO        3
#define CLIP2       4

#if USE_SIMD
INLINE UINT16 CARRY_FLAG(rsp_state *rsp, const int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->xvflag[CARRY], out, x);
	return out;
}

INLINE UINT16 COMPARE_FLAG(rsp_state *rsp, const int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->xvflag[COMPARE], out, x);
	return out;
}

INLINE UINT16 CLIP1_FLAG(rsp_state *rsp, const int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->xvflag[CLIP1], out, x);
	return out;
}

INLINE UINT16 ZERO_FLAG(rsp_state *rsp, const int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->xvflag[ZERO], out, x);
	return out;
}

INLINE UINT16 CLIP2_FLAG(rsp_state *rsp, const int x)
{
	UINT16 out;
	SIMD_EXTRACT16(rsp->xvflag[CLIP2], out, x);
	return out;
}

#define CLEAR_CARRY_FLAGS()     { rsp->xvflag[CARRY] = _mm_setzero_si128(); }
#define CLEAR_COMPARE_FLAGS()   { rsp->xvflag[COMPARE] = _mm_setzero_si128(); }
#define CLEAR_CLIP1_FLAGS()     { rsp->xvflag[CLIP1] = _mm_setzero_si128(); }
#define CLEAR_ZERO_FLAGS()      { rsp->xvflag[ZERO] = _mm_setzero_si128(); }
#define CLEAR_CLIP2_FLAGS()     { rsp->xvflag[CLIP2] = _mm_setzero_si128(); }

#define SET_CARRY_FLAG(x)       { SIMD_INSERT16(rsp->xvflag[CARRY], 0xffff, x); }
#define SET_COMPARE_FLAG(x)     { SIMD_INSERT16(rsp->xvflag[COMPARE], 0xffff, x); }
#define SET_CLIP1_FLAG(x)       { SIMD_INSERT16(rsp->xvflag[CLIP1], 0xffff, x); }
#define SET_ZERO_FLAG(x)        { SIMD_INSERT16(rsp->xvflag[ZERO], 0xffff, x); }
#define SET_CLIP2_FLAG(x)       { SIMD_INSERT16(rsp->xvflag[CLIP2], 0xffff, x); }

#define CLEAR_CARRY_FLAG(x)     { SIMD_INSERT16(rsp->xvflag[CARRY], 0, x); }
#define CLEAR_COMPARE_FLAG(x)   { SIMD_INSERT16(rsp->xvflag[COMPARE], 0, x); }
#define CLEAR_CLIP1_FLAG(x)     { SIMD_INSERT16(rsp->xvflag[CLIP1], 0, x); }
#define CLEAR_ZERO_FLAG(x)      { SIMD_INSERT16(rsp->xvflag[ZERO], 0, x); }
#define CLEAR_CLIP2_FLAG(x)     { SIMD_INSERT16(rsp->xvflag[CLIP2], 0, x); }

#else
#define CARRY_FLAG(rsp, x)          (rsp->vflag[CARRY][x & 7] != 0 ? 0xffff : 0)
#define COMPARE_FLAG(rsp, x)        (rsp->vflag[COMPARE][x & 7] != 0 ? 0xffff : 0)
#define CLIP1_FLAG(rsp, x)          (rsp->vflag[CLIP1][x & 7] != 0 ? 0xffff : 0)
#define ZERO_FLAG(rsp, x)           (rsp->vflag[ZERO][x & 7] != 0 ? 0xffff : 0)
#define CLIP2_FLAG(rsp, x)          (rsp->vflag[CLIP2][x & 7] != 0 ? 0xffff : 0)

#define CLEAR_CARRY_FLAGS()         { memset(rsp->vflag[0], 0, 16); }
#define CLEAR_COMPARE_FLAGS()       { memset(rsp->vflag[1], 0, 16); }
#define CLEAR_CLIP1_FLAGS()         { memset(rsp->vflag[2], 0, 16); }
#define CLEAR_ZERO_FLAGS()          { memset(rsp->vflag[3], 0, 16); }
#define CLEAR_CLIP2_FLAGS()         { memset(rsp->vflag[4], 0, 16); }

#define SET_CARRY_FLAG(x)           { rsp->vflag[0][x & 7] = 0xffff; }
#define SET_COMPARE_FLAG(x)         { rsp->vflag[1][x & 7] = 0xffff; }
#define SET_CLIP1_FLAG(x)           { rsp->vflag[2][x & 7] = 0xffff; }
#define SET_ZERO_FLAG(x)            { rsp->vflag[3][x & 7] = 0xffff; }
#define SET_CLIP2_FLAG(x)           { rsp->vflag[4][x & 7] = 0xffff; }

#define CLEAR_CARRY_FLAG(x)         { rsp->vflag[0][x & 7] = 0; }
#define CLEAR_COMPARE_FLAG(x)       { rsp->vflag[1][x & 7] = 0; }
#define CLEAR_CLIP1_FLAG(x)         { rsp->vflag[2][x & 7] = 0; }
#define CLEAR_ZERO_FLAG(x)          { rsp->vflag[3][x & 7] = 0; }
#define CLEAR_CLIP2_FLAG(x)         { rsp->vflag[4][x & 7] = 0; }
#endif

INLINE rsp_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == RSP_DRC);
	return *(rsp_state **)downcast<legacy_cpu_device *>(device)->token();
}

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    epc - compute the exception PC from a
    descriptor
-------------------------------------------------*/

INLINE UINT32 epc(const opcode_desc *desc)
{
	return ((desc->flags & OPFLAG_IN_DELAY_SLOT) ? (desc->pc - 3) : desc->pc) | 0x1000;
}


/*-------------------------------------------------
    alloc_handle - allocate a handle if not
    already allocated
-------------------------------------------------*/

INLINE void alloc_handle(drcuml_state *drcuml, code_handle **handleptr, const char *name)
{
	if (*handleptr == NULL)
		*handleptr = drcuml->handle_alloc(name);
}


/*-------------------------------------------------
    load_fast_iregs - load any fast integer
    registers
-------------------------------------------------*/

INLINE void load_fast_iregs(rsp_state *rsp, drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(rsp->impstate->regmap); regnum++)
		if (rsp->impstate->regmap[regnum].is_int_register())
			UML_MOV(block, ireg(rsp->impstate->regmap[regnum].ireg() - REG_I0), mem(&rsp->r[regnum]));
}


/*-------------------------------------------------
    save_fast_iregs - save any fast integer
    registers
-------------------------------------------------*/

INLINE void save_fast_iregs(rsp_state *rsp, drcuml_block *block)
{
	int regnum;

	for (regnum = 0; regnum < ARRAY_LENGTH(rsp->impstate->regmap); regnum++)
		if (rsp->impstate->regmap[regnum].is_int_register())
			UML_MOV(block, mem(&rsp->r[regnum]), ireg(rsp->impstate->regmap[regnum].ireg() - REG_I0));
}

/***************************************************************************
    CORE CALLBACKS
***************************************************************************/

void rspdrc_add_imem(device_t *device, UINT32 *base)
{
	if (!device->machine().options().drc()) return;
	rsp_state *rsp = get_safe_token(device);
	rsp->imem32 = base;
	rsp->imem16 = (UINT16*)base;
	rsp->imem8 = (UINT8*)base;
}

void rspdrc_add_dmem(device_t *device, UINT32 *base)
{
	if (!device->machine().options().drc()) return;
	rsp_state *rsp = get_safe_token(device);
	rsp->dmem32 = base;
	rsp->dmem16 = (UINT16*)base;
	rsp->dmem8 = (UINT8*)base;
}

INLINE UINT8 READ8(rsp_state *rsp, UINT32 address)
{
	UINT8 ret = rsp->dmem8[BYTE4_XOR_BE(address & 0xfff)];
	return ret;
}

static void cfunc_read8(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	rsp->impstate->arg0 = READ8(rsp, rsp->impstate->arg0);
}

INLINE UINT16 READ16(rsp_state *rsp, UINT32 address)
{
	UINT16 ret;
	address &= 0xfff;
	ret = rsp->dmem8[BYTE4_XOR_BE(address)] << 8;
	ret |= rsp->dmem8[BYTE4_XOR_BE(address + 1)];
	return ret;
}

static void cfunc_read16(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	rsp->impstate->arg0 = READ16(rsp, rsp->impstate->arg0);
}

INLINE UINT32 READ32(rsp_state *rsp, UINT32 address)
{
	UINT32 ret;
	address &= 0xfff;
	ret = rsp->dmem8[BYTE4_XOR_BE(address)] << 24;
	ret |= rsp->dmem8[BYTE4_XOR_BE(address + 1)] << 16;
	ret |= rsp->dmem8[BYTE4_XOR_BE(address + 2)] << 8;
	ret |= rsp->dmem8[BYTE4_XOR_BE(address + 3)];
	return ret;
}

static void cfunc_read32(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	rsp->impstate->arg0 = READ32(rsp, rsp->impstate->arg0);
}

INLINE void WRITE8(rsp_state *rsp, UINT32 address, UINT8 data)
{
	address &= 0xfff;
	rsp->dmem8[BYTE4_XOR_BE(address)] = data;
}

static void cfunc_write8(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	WRITE8(rsp, rsp->impstate->arg0, (UINT8)rsp->impstate->arg1);
}

INLINE void WRITE16(rsp_state *rsp, UINT32 address, UINT16 data)
{
	address &= 0xfff;
	rsp->dmem8[BYTE4_XOR_BE(address)] = data >> 8;
	rsp->dmem8[BYTE4_XOR_BE(address + 1)] = data & 0xff;
}

static void cfunc_write16(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	WRITE16(rsp, rsp->impstate->arg0, (UINT16)rsp->impstate->arg1);
}

INLINE void WRITE32(rsp_state *rsp, UINT32 address, UINT32 data)
{
	address &= 0xfff;
	rsp->dmem8[BYTE4_XOR_BE(address)] = data >> 24;
	rsp->dmem8[BYTE4_XOR_BE(address + 1)] = (data >> 16) & 0xff;
	rsp->dmem8[BYTE4_XOR_BE(address + 2)] = (data >> 8) & 0xff;
	rsp->dmem8[BYTE4_XOR_BE(address + 3)] = data & 0xff;
}

static void cfunc_write32(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	WRITE32(rsp, rsp->impstate->arg0, rsp->impstate->arg1);
}

/*****************************************************************************/

/*-------------------------------------------------
    rspdrc_set_options - configure DRC options
-------------------------------------------------*/

void rspdrc_set_options(device_t *device, UINT32 options)
{
	if (!device->machine().options().drc()) return;
	rsp_state *rsp = get_safe_token(device);
	rsp->impstate->drcoptions = options;
}


/*-------------------------------------------------
    cfunc_printf_debug - generic printf for
    debugging
-------------------------------------------------*/

#ifdef UNUSED_CODE
static void cfunc_printf_debug(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	switch(rsp->impstate->arg2)
	{
		case 0: // WRITE8
			printf("%04x:%02x\n", rsp->impstate->arg0 & 0xffff, (UINT8)rsp->impstate->arg1);
			break;
		case 1: // WRITE16
			printf("%04x:%04x\n", rsp->impstate->arg0 & 0xffff, (UINT16)rsp->impstate->arg1);
			break;
		case 2: // WRITE32
			printf("%04x:%08x\n", rsp->impstate->arg0 & 0xffff, rsp->impstate->arg1);
			break;
		case 3: // READ8
			printf("%04xr%02x\n", rsp->impstate->arg0 & 0xffff, (UINT8)rsp->impstate->arg1);
			break;
		case 4: // READ16
			printf("%04xr%04x\n", rsp->impstate->arg0 & 0xffff, (UINT16)rsp->impstate->arg1);
			break;
		case 5: // READ32
			printf("%04xr%08x\n", rsp->impstate->arg0 & 0xffff, rsp->impstate->arg1);
			break;
		case 6: // Checksum
			printf("Sum: %08x\n", rsp->impstate->arg0);
			break;
		case 7: // Checksum
			printf("Correct Sum: %08x\n", rsp->impstate->arg0);
			break;
		default: // ???
			printf("%08x %08x\n", rsp->impstate->arg0 & 0xffff, rsp->impstate->arg1);
			break;
	}
}
#endif

static void cfunc_get_cop0_reg(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int reg = rsp->impstate->arg0;
	int dest = rsp->impstate->arg1;

	if (reg >= 0 && reg < 8)
	{
		if(dest)
		{
			rsp->r[dest] = (rsp->sp_reg_r_func)(reg, 0x00000000);
		}
	}
	else if (reg >= 8 && reg < 16)
	{
		if(dest)
		{
			rsp->r[dest] = (rsp->dp_reg_r_func)(reg - 8, 0x00000000);
		}
	}
	else
	{
		fatalerror("RSP: cfunc_get_cop0_reg: %d\n", reg);
	}
}

static void cfunc_set_cop0_reg(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int reg = rsp->impstate->arg0;
	UINT32 data = rsp->impstate->arg1;

	if (reg >= 0 && reg < 8)
	{
		(rsp->sp_reg_w_func)(reg, data, 0x00000000);
	}
	else if (reg >= 8 && reg < 16)
	{
		(rsp->dp_reg_w_func)(reg - 8, data, 0x00000000);
	}
	else
	{
		fatalerror("RSP: set_cop0_reg: %d, %08X\n", reg, data);
	}
}

static void cfunc_unimplemented_opcode(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;
	if ((rsp->device->machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		char string[200];
		rsp_dasm_one(string, rsp->ppc, op);
		mame_printf_debug("%08X: %s\n", rsp->ppc, string);
	}

	fatalerror("RSP: unknown opcode %02X (%08X) at %08X\n", op >> 26, op, rsp->ppc);
}

static void unimplemented_opcode(rsp_state *rsp, UINT32 op)
{
	if ((rsp->device->machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		char string[200];
		rsp_dasm_one(string, rsp->ppc, op);
		mame_printf_debug("%08X: %s\n", rsp->ppc, string);
	}

	fatalerror("RSP: unknown opcode %02X (%08X) at %08X\n", op >> 26, op, rsp->ppc);
}

/*****************************************************************************/

/* Legacy.  Going forward, this will be transitioned into unrolled opcode decodes. */
static const int vector_elements_2[16][8] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     // none
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     // ???
	{ 0, 0, 2, 2, 4, 4, 6, 6 },     // 0q
	{ 1, 1, 3, 3, 5, 5, 7, 7 },     // 1q
	{ 0, 0, 0, 0, 4, 4, 4, 4 },     // 0h
	{ 1, 1, 1, 1, 5, 5, 5, 5 },     // 1h
	{ 2, 2, 2, 2, 6, 6, 6, 6 },     // 2h
	{ 3, 3, 3, 3, 7, 7, 7, 7 },     // 3h
	{ 0, 0, 0, 0, 0, 0, 0, 0 },     // 0
	{ 1, 1, 1, 1, 1, 1, 1, 1 },     // 1
	{ 2, 2, 2, 2, 2, 2, 2, 2 },     // 2
	{ 3, 3, 3, 3, 3, 3, 3, 3 },     // 3
	{ 4, 4, 4, 4, 4, 4, 4, 4 },     // 4
	{ 5, 5, 5, 5, 5, 5, 5, 5 },     // 5
	{ 6, 6, 6, 6, 6, 6, 6, 6 },     // 6
	{ 7, 7, 7, 7, 7, 7, 7, 7 },     // 7
};

#if USE_SIMD
static __m128i vec_himask;
static __m128i vec_lomask;
static __m128i vec_hibit;
static __m128i vec_lobit;
static __m128i vec_n32768;
static __m128i vec_32767;
static __m128i vec_flagmask;
static __m128i vec_shiftmask2;
static __m128i vec_shiftmask4;
static __m128i vec_flag_reverse;
static __m128i vec_neg1;
static __m128i vec_shuf[16];
static __m128i vec_shuf_inverse[16];
#endif

static void rspcom_init(rsp_state *rsp, legacy_cpu_device *device, device_irq_acknowledge_callback irqcallback)
{
	int regIdx = 0;
	int accumIdx;

	memset(rsp, 0, sizeof(*rsp));

	const rsp_config *config = (const rsp_config *)device->static_config();
	// resolve callbacks
	rsp->dp_reg_r_func.resolve(config->dp_reg_r_cb, *device);
	rsp->dp_reg_w_func.resolve(config->dp_reg_w_cb, *device);
	rsp->sp_reg_r_func.resolve(config->sp_reg_r_cb, *device);
	rsp->sp_reg_w_func.resolve(config->sp_reg_w_cb, *device);
	rsp->sp_set_status_func.resolve(config->sp_set_status_cb, *device);

	rsp->irq_callback = irqcallback;
	rsp->device = device;
	rsp->program = &device->space(AS_PROGRAM);
	rsp->direct = &rsp->program->direct();

#if 1
	// Inaccurate.  RSP registers power on to a random state...
	for(regIdx = 0; regIdx < 32; regIdx++ )
	{
		rsp->r[regIdx] = 0;
		rsp->v[regIdx].d[0] = 0;
		rsp->v[regIdx].d[1] = 0;
	}
	CLEAR_CARRY_FLAGS();
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP1_FLAGS();
	CLEAR_ZERO_FLAGS();
	CLEAR_CLIP2_FLAGS();
	rsp->reciprocal_res = 0;
	rsp->reciprocal_high = 0;
#endif

	// ...except for the accumulators.
	for(accumIdx = 0; accumIdx < 8; accumIdx++ )
	{
		rsp->accum[accumIdx].q = 0;
	}

	rsp->sr = RSP_STATUS_HALT;
	rsp->step_count = 0;

#if USE_SIMD
	vec_shuf_inverse[ 0] = _mm_set_epi16(0x0f0e, 0x0d0c, 0x0b0a, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100); // none
	vec_shuf_inverse[ 1] = _mm_set_epi16(0x0f0e, 0x0d0c, 0x0b0a, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100); // ???
	vec_shuf_inverse[ 2] = _mm_set_epi16(0x0d0c, 0x0d0c, 0x0908, 0x0908, 0x0504, 0x0504, 0x0100, 0x0100); // 0q
	vec_shuf_inverse[ 3] = _mm_set_epi16(0x0f0e, 0x0f0e, 0x0b0a, 0x0b0a, 0x0706, 0x0706, 0x0302, 0x0302); // 1q
	vec_shuf_inverse[ 4] = _mm_set_epi16(0x0908, 0x0908, 0x0908, 0x0908, 0x0100, 0x0100, 0x0100, 0x0100); // 0h
	vec_shuf_inverse[ 5] = _mm_set_epi16(0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0302, 0x0302, 0x0302, 0x0302); // 1h
	vec_shuf_inverse[ 6] = _mm_set_epi16(0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0504, 0x0504, 0x0504, 0x0504); // 2h
	vec_shuf_inverse[ 7] = _mm_set_epi16(0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0706, 0x0706, 0x0706, 0x0706); // 3h
	vec_shuf_inverse[ 8] = _mm_set_epi16(0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100); // 0
	vec_shuf_inverse[ 9] = _mm_set_epi16(0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302); // 1
	vec_shuf_inverse[10] = _mm_set_epi16(0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504); // 2
	vec_shuf_inverse[11] = _mm_set_epi16(0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706); // 3
	vec_shuf_inverse[12] = _mm_set_epi16(0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908); // 4
	vec_shuf_inverse[13] = _mm_set_epi16(0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a); // 5
	vec_shuf_inverse[14] = _mm_set_epi16(0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c); // 6
	vec_shuf_inverse[15] = _mm_set_epi16(0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e); // 7

	vec_shuf[ 0] = _mm_set_epi16(0x0100, 0x0302, 0x0504, 0x0706, 0x0908, 0x0b0a, 0x0d0c, 0x0f0e); // none
	vec_shuf[ 1] = _mm_set_epi16(0x0100, 0x0302, 0x0504, 0x0706, 0x0908, 0x0b0a, 0x0d0c, 0x0f0e); // ???
	vec_shuf[ 2] = _mm_set_epi16(0x0302, 0x0302, 0x0706, 0x0706, 0x0b0a, 0x0b0a, 0x0f0e, 0x0f0e); // 0q
	vec_shuf[ 3] = _mm_set_epi16(0x0100, 0x0100, 0x0504, 0x0706, 0x0908, 0x0908, 0x0d0c, 0x0d0c); // 1q
	vec_shuf[ 4] = _mm_set_epi16(0x0706, 0x0706, 0x0706, 0x0706, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e); // 0q
	vec_shuf[ 5] = _mm_set_epi16(0x0504, 0x0504, 0x0504, 0x0504, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c); // 1q
	vec_shuf[ 6] = _mm_set_epi16(0x0302, 0x0302, 0x0302, 0x0302, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a); // 2q
	vec_shuf[ 7] = _mm_set_epi16(0x0100, 0x0100, 0x0100, 0x0100, 0x0908, 0x0908, 0x0908, 0x0908); // 3q
	vec_shuf[ 8] = _mm_set_epi16(0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e, 0x0f0e); // 0
	vec_shuf[ 9] = _mm_set_epi16(0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c, 0x0d0c); // 1
	vec_shuf[10] = _mm_set_epi16(0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a, 0x0b0a); // 2
	vec_shuf[11] = _mm_set_epi16(0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908, 0x0908); // 3
	vec_shuf[12] = _mm_set_epi16(0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706, 0x0706); // 4
	vec_shuf[13] = _mm_set_epi16(0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504, 0x0504); // 5
	vec_shuf[14] = _mm_set_epi16(0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302, 0x0302); // 6
	vec_shuf[15] = _mm_set_epi16(0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100); // 7
	rsp->accum_h = _mm_setzero_si128();
	rsp->accum_m = _mm_setzero_si128();
	rsp->accum_l = _mm_setzero_si128();
	vec_neg1 = _mm_set_epi64x(0xffffffffffffffffL, 0xffffffffffffffffL);
	vec_himask = _mm_set_epi64x(0xffff0000ffff0000L, 0xffff0000ffff0000L);
	vec_lomask = _mm_set_epi64x(0x0000ffff0000ffffL, 0x0000ffff0000ffffL);
	vec_hibit = _mm_set_epi64x(0x0001000000010000L, 0x0001000000010000L);
	vec_lobit = _mm_set_epi64x(0x0000000100000001L, 0x0000000100000001L);
	vec_32767 = _mm_set_epi64x(0x7fff7fff7fff7fffL, 0x7fff7fff7fff7fffL);
	vec_n32768 = _mm_set_epi64x(0x8000800080008000L, 0x8000800080008000L);
	vec_flagmask = _mm_set_epi64x(0x0001000100010001L, 0x0001000100010001L);
	vec_shiftmask2 = _mm_set_epi64x(0x0000000300000003L, 0x0000000300000003L);
	vec_shiftmask4 = _mm_set_epi64x(0x000000000000000fL, 0x000000000000000fL);
	vec_flag_reverse = _mm_set_epi16(0x0100, 0x0302, 0x0504, 0x0706, 0x0908, 0x0b0a, 0x0d0c, 0x0f0e);
#endif
}

static CPU_INIT( rsp )
{
	rsp_state *rsp;
	drc_cache *cache;
	UINT32 flags = 0;
	int regnum;
	//int elnum;

	/* allocate enough space for the cache and the core */
	cache = auto_alloc(device->machine(), drc_cache(CACHE_SIZE + sizeof(*rsp)));

	/* allocate the core memory */
	*(rsp_state **)device->token() = rsp = (rsp_state *)cache->alloc_near(sizeof(*rsp));
	memset(rsp, 0, sizeof(*rsp));

	rspcom_init(rsp, device, irqcallback);

	/* allocate the implementation-specific state from the full cache */
	rsp->impstate = (rspimp_state *)cache->alloc_near(sizeof(*rsp->impstate));
	memset(rsp->impstate, 0, sizeof(*rsp->impstate));
	rsp->impstate->cache = cache;

	/* initialize the UML generator */
	if (LOG_UML)
	{
		flags |= DRCUML_OPTION_LOG_UML;
	}
	if (LOG_NATIVE)
	{
		flags |= DRCUML_OPTION_LOG_NATIVE;
	}
	rsp->impstate->drcuml = auto_alloc(device->machine(), drcuml_state(*device, *cache, flags, 8, 32, 2));

	/* add symbols for our stuff */
	rsp->impstate->drcuml->symbol_add(&rsp->pc, sizeof(rsp->pc), "pc");
	rsp->impstate->drcuml->symbol_add(&rsp->icount, sizeof(rsp->icount), "icount");
	for (regnum = 0; regnum < 32; regnum++)
	{
		char buf[10];
		sprintf(buf, "r%d", regnum);
		rsp->impstate->drcuml->symbol_add(&rsp->r[regnum], sizeof(rsp->r[regnum]), buf);
	}
	rsp->impstate->drcuml->symbol_add(&rsp->impstate->arg0, sizeof(rsp->impstate->arg0), "arg0");
	rsp->impstate->drcuml->symbol_add(&rsp->impstate->arg1, sizeof(rsp->impstate->arg1), "arg1");
	rsp->impstate->drcuml->symbol_add(&rsp->impstate->arg2, sizeof(rsp->impstate->arg2), "arg2");
	rsp->impstate->drcuml->symbol_add(&rsp->impstate->arg3, sizeof(rsp->impstate->arg3), "arg3");
	rsp->impstate->drcuml->symbol_add(&rsp->impstate->numcycles, sizeof(rsp->impstate->numcycles), "numcycles");

	/* initialize the front-end helper */
	rsp->impstate->drcfe = auto_alloc(device->machine(), rsp_frontend(*rsp, COMPILE_BACKWARDS_BYTES, COMPILE_FORWARDS_BYTES, SINGLE_INSTRUCTION_MODE ? 1 : COMPILE_MAX_SEQUENCE));

	/* compute the register parameters */
	for (regnum = 0; regnum < 32; regnum++)
		rsp->impstate->regmap[regnum] = (regnum == 0) ? parameter(0) : parameter::make_memory(&rsp->r[regnum]);

	/*
	drcbe_info beinfo;
	rsp->impstate->drcuml->get_backend_info(beinfo);
	if (beinfo.direct_iregs > 2)
	{
	    rsp->impstate->regmap[30] = I2;
	}
	if (beinfo.direct_iregs > 3)
	{
	    rsp->impstate->regmap[31] = I3;
	}
	if (beinfo.direct_iregs > 4)
	{
	    rsp->impstate->regmap[2] = I4;
	}
	if (beinfo.direct_iregs > 5)
	{
	    rsp->impstate->regmap[3] = I5;
	}
	if (beinfo.direct_iregs > 6)
	{
	    rsp->impstate->regmap[4] = I6;
	}
	*/

	/* mark the cache dirty so it is updated on next execute */
	rsp->impstate->cache_dirty = TRUE;
}

static CPU_EXIT( rsp )
{
	rsp_state *rsp = get_safe_token(device);

	/* clean up the DRC */
	auto_free(device->machine(), rsp->impstate->drcfe);
	auto_free(device->machine(), rsp->impstate->drcuml);
	auto_free(device->machine(), rsp->impstate->cache);
}


static CPU_RESET( rsp )
{
	rsp_state *rsp = get_safe_token(device);
	rsp->nextpc = ~0;
}

static void cfunc_rsp_lbv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;

	UINT32 ea = 0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00000 | IIII | Offset |
	// --------------------------------------------------
	//
	// Load 1 byte to vector byte index

	ea = (base) ? rsp->r[base] + offset : offset;

#if USE_SIMD
	UINT16 element;
	SIMD_EXTRACT16(rsp->xv[dest], element, (index >> 1));
	element &= 0xff00 >> ((1-(index & 1)) * 8);
	element |= READ8(rsp, ea) << ((1-(index & 1)) * 8);
	SIMD_INSERT16(rsp->xv[dest], element, (index >> 1));
#else
	VREG_B(dest, index) = READ8(rsp, ea);
#endif
}

static void cfunc_rsp_lsv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xe;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00001 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads 2 bytes starting from vector byte index

	UINT32 ea = (base) ? rsp->r[base] + (offset * 2) : (offset * 2);
	int end = index + 2;
	for (int i = index; i < end; i++)
	{
#if USE_SIMD
		UINT16 element;
		SIMD_EXTRACT16(rsp->xv[dest], element, (i >> 1));
		element &= 0xff00 >> ((1 - (i & 1)) * 8);
		element |= READ8(rsp, ea) << ((1 - (i & 1)) * 8);
		SIMD_INSERT16(rsp->xv[dest], element, (i >> 1));
#else
		VREG_B(dest, i) = READ8(rsp, ea);
#endif
		ea++;
	}
}

static void cfunc_rsp_llv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	UINT32 ea = 0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xc;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00010 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads 4 bytes starting from vector byte index

	ea = (base) ? rsp->r[base] + (offset * 4) : (offset * 4);

	int end = index + 4;

	for (int i = index; i < end; i++)
	{
#if USE_SIMD
		UINT16 element;
		SIMD_EXTRACT16(rsp->xv[dest], element, (i >> 1));
		element &= 0xff00 >> ((1 - (i & 1)) * 8);
		element |= READ8(rsp, ea) << ((1 - (i & 1)) * 8);
		SIMD_INSERT16(rsp->xv[dest], element, (i >> 1));
#else
		VREG_B(dest, i) = READ8(rsp, ea);
#endif
		ea++;
	}
}

static void cfunc_rsp_ldv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	UINT32 ea = 0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0x8;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00011 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads 8 bytes starting from vector byte index

	ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);

	int end = index + 8;

	for (int i = index; i < end; i++)
	{
#if USE_SIMD
		UINT16 element;
		SIMD_EXTRACT16(rsp->xv[dest], element, (i >> 1));
		element &= 0xff00 >> ((1 - (i & 1)) * 8);
		element |= READ8(rsp, ea) << ((1 - (i & 1)) * 8);
		SIMD_INSERT16(rsp->xv[dest], element, (i >> 1));
#else
		VREG_B(dest, i) = READ8(rsp, ea);
#endif
		ea++;
	}
}

static void cfunc_rsp_lqv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	//int index = 0; // Just a test, it goes right back the way it was if something breaks //(op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00100 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads up to 16 bytes starting from vector byte index

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	int end = 16 - (ea & 0xf);
	if (end > 16) end = 16;

	for (int i = 0; i < end; i++)
	{
#if USE_SIMD
		UINT16 element;
		SIMD_EXTRACT16(rsp->xv[dest], element, (i >> 1));
		element &= 0xff00 >> ((1 - (i & 1)) * 8);
		element |= READ8(rsp, ea) << ((1 - (i & 1)) * 8);
		SIMD_INSERT16(rsp->xv[dest], element, (i >> 1));
#else
		VREG_B(dest, i) = READ8(rsp, ea);
#endif
		ea++;
	}
}

static void cfunc_rsp_lrv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00101 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores up to 16 bytes starting from right side until 16-byte boundary

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	index = 16 - ((ea & 0xf) - index);
	ea &= ~0xf;

	for (int i = index; i < 16; i++)
	{
#if USE_SIMD
		UINT16 element;
		SIMD_EXTRACT16(rsp->xv[dest], element, (i >> 1));
		element &= 0xff00 >> ((1-(i & 1)) * 8);
		element |= READ8(rsp, ea) << ((1-(i & 1)) * 8);
		SIMD_INSERT16(rsp->xv[dest], element, (i >> 1));
#else
		VREG_B(dest, i) = READ8(rsp, ea);
#endif
		ea++;
	}
}

static void cfunc_rsp_lpv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00110 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads a byte as the upper 8 bits of each element

	UINT32 ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);

	for (int i = 0; i < 8; i++)
	{
#if USE_SIMD
		SIMD_INSERT16(rsp->xv[dest], READ8(rsp, ea + (((16-index) + i) & 0xf)) << 8, i);
#else
		W_VREG_S(dest, i) = READ8(rsp, ea + (((16-index) + i) & 0xf)) << 8;
#endif
	}
}

static void cfunc_rsp_luv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 00111 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads a byte as the bits 14-7 of each element

	UINT32 ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);

	for (int i = 0; i < 8; i++)
	{
#if USE_SIMD
		SIMD_INSERT16(rsp->xv[dest], READ8(rsp, ea + (((16-index) + i) & 0xf)) << 7, i);
#else
		W_VREG_S(dest, i) = READ8(rsp, ea + (((16-index) + i) & 0xf)) << 7;
#endif
	}
}

static void cfunc_rsp_lhv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 01000 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads a byte as the bits 14-7 of each element, with 2-byte stride

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	for (int i = 0; i < 8; i++)
	{
#if USE_SIMD
		SIMD_INSERT16(rsp->xv[dest], READ8(rsp, ea + (((16-index) + (i<<1)) & 0xf)) << 7, i);
#else
		W_VREG_S(dest, i) = READ8(rsp, ea + (((16-index) + (i<<1)) & 0xf)) << 7;
#endif
	}
}

static void cfunc_rsp_lfv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 01001 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads a byte as the bits 14-7 of upper or lower quad, with 4-byte stride

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	// not sure what happens if 16-byte boundary is crossed...

	int end = (index >> 1) + 4;

	for (int i = index >> 1; i < end; i++)
	{
#if USE_SIMD
		SIMD_INSERT16(rsp->xv[dest], READ8(rsp, ea) << 7, i);
#else
		W_VREG_S(dest, i) = READ8(rsp, ea) << 7;
#endif
		ea += 4;
	}
}

static void cfunc_rsp_lwv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 01010 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads the full 128-bit vector starting from vector byte index and wrapping to index 0
	// after byte index 15

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	int end = (16 - index) + 16;

#if USE_SIMD
	UINT8 val[16];
#endif
	for (int i = (16 - index); i < end; i++)
	{
#if USE_SIMD
		val[i & 0xf] = READ8(rsp, ea);
#else
		VREG_B(dest, i & 0xf) = READ8(rsp, ea);
#endif
		ea += 4;
	}

#if USE_SIMD
	rsp->xv[dest] = _mm_set_epi8(val[15], val[14], val[13], val[12], val[11], val[10], val[ 9], val[ 8],
									val[ 7], val[ 6], val[ 5], val[ 4], val[ 3], val[ 2], val[ 1], val[ 0]);
#endif
}

static void cfunc_rsp_ltv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);

	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 110010 | BBBBB | TTTTT | 01011 | IIII | Offset |
	// --------------------------------------------------
	//
	// Loads one element to maximum of 8 vectors, while incrementing element index

	// FIXME: has a small problem with odd indices

	int vs = dest;
	int ve = dest + 8;
	if (ve > 32)
	{
		ve = 32;
	}

	int element = 7 - (index >> 1);

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	ea = ((ea + 8) & ~0xf) + (index & 1);
	for (int i = vs; i < ve; i++)
	{
		element = ((8 - (index >> 1) + (i - vs)) << 1);
#if USE_SIMD
		UINT16 value = (READ8(rsp, ea) << 8) | READ8(rsp, ea + 1);
		SIMD_INSERT16(rsp->xv[i], value, (element >> 1));
#else
		VREG_B(i, (element & 0xf)) = READ8(rsp, ea);
		VREG_B(i, ((element + 1) & 0xf)) = READ8(rsp, ea + 1);
#endif

		ea += 2;
	}
}

static int generate_lwc2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	//int loopdest;
	UINT32 op = desc->opptr.l[0];
	//int dest = (op >> 16) & 0x1f;
	//int base = (op >> 21) & 0x1f;
	//int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	//int skip;
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}

	switch ((op >> 11) & 0x1f)
	{
		case 0x00:      /* LBV */
			//UML_ADD(block, I0, R32(RSREG), offset);
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lbv, rsp);
			return TRUE;
		case 0x01:      /* LSV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lsv, rsp);
			return TRUE;
		case 0x02:      /* LLV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_llv, rsp);
			return TRUE;
		case 0x03:      /* LDV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_ldv, rsp);
			return TRUE;
		case 0x04:      /* LQV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lqv, rsp);
			return TRUE;
		case 0x05:      /* LRV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lrv, rsp);
			return TRUE;
		case 0x06:      /* LPV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lpv, rsp);
			return TRUE;
		case 0x07:      /* LUV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_luv, rsp);
			return TRUE;
		case 0x08:      /* LHV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lhv, rsp);
			return TRUE;
		case 0x09:      /* LFV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lfv, rsp);
			return TRUE;
		case 0x0a:      /* LWV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_lwv, rsp);
			return TRUE;
		case 0x0b:      /* LTV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_ltv, rsp);
			return TRUE;

		default:
			return FALSE;
	}
}

static void cfunc_rsp_sbv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}

	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00000 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores 1 byte from vector byte index

	UINT32 ea = (base) ? rsp->r[base] + offset : offset;
#if USE_SIMD
	UINT16 value;
	SIMD_EXTRACT16(rsp->xv[dest], value, (index >> 1));
	value >>= (1-(index & 1)) * 8;
	WRITE8(rsp, ea, (UINT8)value);
#else
	WRITE8(rsp, ea, VREG_B(dest, index));
#endif
}

static void cfunc_rsp_ssv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}

	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00001 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores 2 bytes starting from vector byte index

	UINT32 ea = (base) ? rsp->r[base] + (offset * 2) : (offset * 2);

#if USE_SIMD
	UINT16 value;
	SIMD_EXTRACT16(rsp->xv[dest], value, (index >> 1));
	WRITE8(rsp, ea, (UINT8)(value >> 8));
	WRITE8(rsp, ea+1, (UINT8)(value & 0x00ff));
#else
	int end = index + 2;
	for (int i = index; i < end; i++)
	{
		WRITE8(rsp, ea, VREG_B(dest, i));
		ea++;
	}
#endif
}

static void cfunc_rsp_slv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00010 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores 4 bytes starting from vector byte index

	UINT32 ea = (base) ? rsp->r[base] + (offset * 4) : (offset * 4);

#if USE_SIMD
	UINT16 value0, value1;
	index >>= 1;
	SIMD_EXTRACT16(rsp->xv[dest], value0, index);
	SIMD_EXTRACT16(rsp->xv[dest], value1, index+1);
	WRITE8(rsp, ea, (UINT8)(value0 >> 8));
	WRITE8(rsp, ea+1, (UINT8)(value0 & 0x00ff));
	WRITE8(rsp, ea+2, (UINT8)(value1 >> 8));
	WRITE8(rsp, ea+3, (UINT8)(value1 & 0x00ff));
#else
	int end = index + 4;
	for (int i = index; i < end; i++)
	{
		WRITE8(rsp, ea, VREG_B(dest, i));
		ea++;
	}
#endif
}

static void cfunc_rsp_sdv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0x8;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00011 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores 8 bytes starting from vector byte index
	UINT32 ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);

#if USE_SIMD
	UINT16 value0, value1, value2, value3;
	index >>= 1;
	SIMD_EXTRACT16(rsp->xv[dest], value0, index);
	SIMD_EXTRACT16(rsp->xv[dest], value1, index+1);
	SIMD_EXTRACT16(rsp->xv[dest], value2, index+2);
	SIMD_EXTRACT16(rsp->xv[dest], value3, index+3);
	WRITE8(rsp, ea, (UINT8)(value0 >> 8));
	WRITE8(rsp, ea+1, (UINT8)(value0 & 0x00ff));
	WRITE8(rsp, ea+2, (UINT8)(value1 >> 8));
	WRITE8(rsp, ea+3, (UINT8)(value1 & 0x00ff));
	WRITE8(rsp, ea+4, (UINT8)(value2 >> 8));
	WRITE8(rsp, ea+5, (UINT8)(value2 & 0x00ff));
	WRITE8(rsp, ea+6, (UINT8)(value3 >> 8));
	WRITE8(rsp, ea+7, (UINT8)(value3 & 0x00ff));
#else
	int end = index + 8;
	for (int i = index; i < end; i++)
	{
		WRITE8(rsp, ea, VREG_B(dest, i));
		ea++;
	}
#endif
}

static void cfunc_rsp_sqv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00100 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores up to 16 bytes starting from vector byte index until 16-byte boundary

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	int end = index + (16 - (ea & 0xf));
	for (int i=index; i < end; i++)
	{
#if USE_SIMD
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, (i >> 1));
		value >>= (1-(i & 1)) * 8;
		WRITE8(rsp, ea, (UINT8)value);
#else
		WRITE8(rsp, ea, VREG_B(dest, i & 0xf));
#endif
		ea++;
	}
}

static void cfunc_rsp_srv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00101 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores up to 16 bytes starting from right side until 16-byte boundary

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);

	int end = index + (ea & 0xf);
	int o = (16 - (ea & 0xf)) & 0xf;
	ea &= ~0xf;

	for (int i = index; i < end; i++)
	{
#if USE_SIMD
		UINT32 bi = (i + o) & 0xf;
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, (bi >> 1));
		value >>= (1-(bi & 1)) * 8;
		WRITE8(rsp, ea, (UINT8)value);
#else
		WRITE8(rsp, ea, VREG_B(dest, ((i + o) & 0xf)));
#endif
		ea++;
	}
}

static void cfunc_rsp_spv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00110 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores upper 8 bits of each element

	UINT32 ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);
	int end = index + 8;
	for (int i=index; i < end; i++)
	{
		if ((i & 0xf) < 8)
		{
#if USE_SIMD
			UINT16 value;
			SIMD_EXTRACT16(rsp->xv[dest], value, i);
			WRITE8(rsp, ea, (UINT8)(value >> 8));
#else
			WRITE8(rsp, ea, VREG_B(dest, (i & 0xf) << 1));
#endif
		}
		else
		{
#if USE_SIMD
			UINT16 value;
			SIMD_EXTRACT16(rsp->xv[dest], value, i);
			value >>= 7;
			WRITE8(rsp, ea, (UINT8)value);
#else
			WRITE8(rsp, ea, VREG_S(dest, (i & 0x7)) >> 7);
#endif
		}
		ea++;
	}
}

static void cfunc_rsp_suv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 00111 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores bits 14-7 of each element

	UINT32 ea = (base) ? rsp->r[base] + (offset * 8) : (offset * 8);
	int end = index + 8;
	for (int i=index; i < end; i++)
	{
		if ((i & 0xf) < 8)
		{
#if USE_SIMD
			UINT16 value;
			SIMD_EXTRACT16(rsp->xv[dest], value, i);
			value >>= 7;
			WRITE8(rsp, ea, (UINT8)value);
#else
			WRITE8(rsp, ea, VREG_S(dest, (i & 0x7)) >> 7);
#endif
		}
		else
		{
#if USE_SIMD
			UINT16 value;
			SIMD_EXTRACT16(rsp->xv[dest], value, i);
			WRITE8(rsp, ea, (UINT8)value >> 8);
#else
			WRITE8(rsp, ea, VREG_B(dest, ((i & 0x7) << 1)));
#endif
		}
		ea++;
	}
}

static void cfunc_rsp_shv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 01000 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores bits 14-7 of each element, with 2-byte stride

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	for (int i=0; i < 8; i++)
	{
		int element = index + (i << 1);
#if USE_SIMD
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, element >> 1);
		WRITE8(rsp, ea, (value >> 7) & 0x00ff);
#else
		UINT8 d = (VREG_B(dest, (element & 0xf)) << 1) |
					(VREG_B(dest, ((element + 1) & 0xf)) >> 7);
		WRITE8(rsp, ea, d);
#endif
		ea += 2;
	}
}

static void cfunc_rsp_sfv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 01001 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores bits 14-7 of upper or lower quad, with 4-byte stride

	if (index & 0x7)    printf("RSP: SFV: index = %d at %08X\n", index, rsp->ppc);

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	int eaoffset = ea & 0xf;
	ea &= ~0xf;

	int end = (index >> 1) + 4;

	for (int i = index>>1; i < end; i++)
	{
#if USE_SIMD
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, i);
		WRITE8(rsp, ea + (eaoffset & 0xf), (value >> 7) & 0x00ff);
#else
		WRITE8(rsp, ea + (eaoffset & 0xf), VREG_S(dest, i) >> 7);
#endif
		eaoffset += 4;
	}
}

static void cfunc_rsp_swv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 01010 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores the full 128-bit vector starting from vector byte index and wrapping to index 0
	// after byte index 15

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	int eaoffset = ea & 0xf;
	ea &= ~0xf;

	int end = index + 16;
	for (int i = index; i < end; i++)
	{
#if USE_SIMD
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, i >> 1);
		WRITE8(rsp, ea + (eaoffset & 0xf), (value >> ((1-(i & 1)) * 8)) & 0xff);
#else
		WRITE8(rsp, ea + (eaoffset & 0xf), VREG_B(dest, i & 0xf));
#endif
		eaoffset++;
	}
}

static void cfunc_rsp_stv(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int dest = (op >> 16) & 0x1f;
	int base = (op >> 21) & 0x1f;
	int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);

	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}
	// 31       25      20      15      10     6        0
	// --------------------------------------------------
	// | 111010 | BBBBB | TTTTT | 01011 | IIII | Offset |
	// --------------------------------------------------
	//
	// Stores one element from maximum of 8 vectors, while incrementing element index

	int vs = dest;
	int ve = dest + 8;
	if (ve > 32)
	{
		ve = 32;
	}

	int element = 8 - (index >> 1);

	UINT32 ea = (base) ? rsp->r[base] + (offset * 16) : (offset * 16);
	int eaoffset = (ea & 0xf) + (element * 2);
	ea &= ~0xf;

	for (int i = vs; i < ve; i++)
	{
#if USE_SIMD
		UINT16 value;
		SIMD_EXTRACT16(rsp->xv[dest], value, element);
		WRITE16(rsp, ea + (eaoffset & 0xf), value);
#else
		WRITE16(rsp, ea + (eaoffset & 0xf), VREG_S(i, element & 0x7));
#endif
		eaoffset += 2;
		element++;
	}
}

static int generate_swc2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
//  int loopdest;
	UINT32 op = desc->opptr.l[0];
	//int dest = (op >> 16) & 0x1f;
	//int base = (op >> 21) & 0x1f;
	//int index = (op >> 7) & 0xf;
	int offset = (op & 0x7f);
	//int skip;
	if (offset & 0x40)
	{
		offset |= 0xffffffc0;
	}

	switch ((op >> 11) & 0x1f)
	{
		case 0x00:      /* SBV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_sbv, rsp);
			return TRUE;
		case 0x01:      /* SSV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_ssv, rsp);
			return TRUE;
		case 0x02:      /* SLV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_slv, rsp);
			return TRUE;
		case 0x03:      /* SDV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_sdv, rsp);
			return TRUE;
		case 0x04:      /* SQV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_sqv, rsp);
			return TRUE;
		case 0x05:      /* SRV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_srv, rsp);
			return TRUE;
		case 0x06:      /* SPV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_spv, rsp);
			return TRUE;
		case 0x07:      /* SUV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_suv, rsp);
			return TRUE;
		case 0x08:      /* SHV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_shv, rsp);
			return TRUE;
		case 0x09:      /* SFV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_sfv, rsp);
			return TRUE;
		case 0x0a:      /* SWV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_swv, rsp);
			return TRUE;
		case 0x0b:      /* STV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_stv, rsp);
			return TRUE;

		default:
			unimplemented_opcode(rsp, op);
			return FALSE;
	}

	return TRUE;
}

INLINE UINT16 SATURATE_ACCUM(rsp_state *rsp, int accum, int slice, UINT16 negative, UINT16 positive)
{
	if ((INT16)ACCUM_H(rsp, accum) < 0)
	{
		if ((UINT16)(ACCUM_H(rsp, accum)) != 0xffff)
		{
			return negative;
		}
		else
		{
			if ((INT16)ACCUM_M(rsp, accum) >= 0)
			{
				return negative;
			}
			else
			{
				if (slice == 0)
				{
					return ACCUM_L(rsp, accum);
				}
				else if (slice == 1)
				{
					return ACCUM_M(rsp, accum);
				}
			}
		}
	}
	else
	{
		if ((UINT16)(ACCUM_H(rsp, accum)) != 0)
		{
			return positive;
		}
		else
		{
			if ((INT16)ACCUM_M(rsp, accum) < 0)
			{
				return positive;
			}
			else
			{
				if (slice == 0)
				{
					return ACCUM_L(rsp, accum);
				}
				else
				{
					return ACCUM_M(rsp, accum);
				}
			}
		}
	}

	return 0;
}

#if USE_SIMD
__m128i SATURATE_ACCUM1(__m128i accum_h, __m128i accum_m, UINT16 negative, UINT16 positive)
{
	__m128i vnegative = _mm_set_epi16(negative, negative, negative, negative, negative, negative, negative, negative);
	__m128i vpositive = _mm_set_epi16(positive, positive, positive, positive, positive, positive, positive, positive);

	// conditional masks
	__m128i accum_hlz = _mm_cmplt_epi16(accum_h, _mm_setzero_si128());
	__m128i accum_hgz = _mm_cmpgt_epi16(accum_h, _mm_setzero_si128());
	__m128i accum_hz = _mm_cmpeq_epi16(accum_h, _mm_setzero_si128());
	__m128i accum_hn1 = _mm_cmpeq_epi16(accum_h, vec_neg1);
	__m128i accum_hnn1 = _mm_xor_si128(accum_hn1, vec_neg1);

	__m128i accum_mlz = _mm_cmplt_epi16(accum_m, _mm_setzero_si128());
	__m128i accum_mgz = _mm_cmpgt_epi16(accum_m, _mm_setzero_si128());
	__m128i accum_mz = _mm_cmpeq_epi16(accum_m, _mm_setzero_si128());
	__m128i accum_mgez = _mm_or_si128(accum_mz, accum_mgz);

	// Return negative if H<0 && (H!=0xffff || M >= 0)
	// Return positive if H>0 || (H==0 && M<0)
	// Return medium slice if H==0xffff && M<0
	// Return medium slice if H==0 && M>=0

	__m128i negative_mask = _mm_and_si128(accum_hlz, _mm_or_si128(accum_hnn1, accum_mgez));
	__m128i positive_mask = _mm_or_si128(accum_hgz, _mm_and_si128(accum_hz, accum_mlz));
	__m128i accumm_mask = _mm_or_si128(_mm_and_si128(accum_hz, accum_mgez), _mm_and_si128(accum_hn1, accum_mlz));

	__m128i output = _mm_and_si128(accum_m, accumm_mask);
	output = _mm_or_si128(output, _mm_and_si128(vnegative, negative_mask));
	output = _mm_or_si128(output, _mm_and_si128(vpositive, positive_mask));
	return output;
}
#endif

INLINE UINT16 SATURATE_ACCUM1(rsp_state *rsp, int accum, UINT16 negative, UINT16 positive)
{
	// Return negative if H<0 && (H!=0xffff || M >= 0)
	// Return positive if H>0 || (H==0 && M<0)
	// Return medium slice if H==0xffff && M<0
	// Return medium slice if H==0 && M>=0
	if ((INT16)ACCUM_H(rsp, accum) < 0)
	{
		if ((UINT16)(ACCUM_H(rsp, accum)) != 0xffff)
		{
			return negative;
		}
		else
		{
			if ((INT16)ACCUM_M(rsp, accum) >= 0)
			{
				return negative;
			}
			else
			{
				return ACCUM_M(rsp, accum);
			}
		}
	}
	else
	{
		if ((UINT16)(ACCUM_H(rsp, accum)) != 0)
		{
			return positive;
		}
		else
		{
			if ((INT16)ACCUM_M(rsp, accum) < 0)
			{
				return positive;
			}
			else
			{
				return ACCUM_M(rsp, accum);
			}
		}
	}

	return 0;
}

INLINE UINT16 C_SATURATE_ACCUM1(UINT16 *h, UINT16 *m, int accum, UINT16 negative, UINT16 positive)
{
	// Return negative if H<0 && (H!=0xffff || M >= 0)
	// Return positive if H>0 || (H==0 && M<0)
	// Return medium slice if H==0xffff && M<0
	// Return medium slice if H==0 && M>=0
	if ((INT16)h[accum] < 0)
	{
		if ((UINT16)h[accum] != 0xffff)
		{
			return negative;
		}
		else
		{
			if ((INT16)m[accum] >= 0)
			{
				return negative;
			}
			else
			{
				return m[accum];
			}
		}
	}
	else
	{
		if ((UINT16)h[accum] != 0)
		{
			return positive;
		}
		else
		{
			if ((INT16)m[accum] < 0)
			{
				return positive;
			}
			else
			{
				return m[accum];
			}
		}
	}

	return 0;
}

#if USE_SIMD
#define WRITEBACK_RESULT() { \
		SIMD_INSERT16(rsp->xv[VDREG], vres[0], 0); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[1], 1); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[2], 2); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[3], 3); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[4], 4); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[5], 5); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[6], 6); \
		SIMD_INSERT16(rsp->xv[VDREG], vres[7], 7); \
}
#else
#define WRITEBACK_RESULT() { \
		W_VREG_S(VDREG, 0) = vres[0];   \
		W_VREG_S(VDREG, 1) = vres[1];   \
		W_VREG_S(VDREG, 2) = vres[2];   \
		W_VREG_S(VDREG, 3) = vres[3];   \
		W_VREG_S(VDREG, 4) = vres[4];   \
		W_VREG_S(VDREG, 5) = vres[5];   \
		W_VREG_S(VDREG, 6) = vres[6];   \
		W_VREG_S(VDREG, 7) = vres[7];   \
}
#endif

#if USE_SIMD
/* ============================================================================
* RSPPackLo32to16: Pack LSBs of 32-bit vectors to 16-bits without saturation.
* TODO: 5 SSE2 operations is kind of expensive just to truncate values?
* ========================================================================= */
INLINE __m128i RSPPackLo32to16(__m128i vectorLow, __m128i vectorHigh)
{
	vectorLow = _mm_slli_epi32(vectorLow, 16);
	vectorHigh = _mm_slli_epi32(vectorHigh, 16);
	vectorLow = _mm_srai_epi32(vectorLow, 16);
	vectorHigh = _mm_srai_epi32(vectorHigh, 16);
	return _mm_packs_epi32(vectorLow, vectorHigh);
}

/* ============================================================================
* RSPPackHi32to16: Pack MSBs of 32-bit vectors to 16-bits without saturation.
* ========================================================================= */
INLINE __m128i RSPPackHi32to16(__m128i vectorLow, __m128i vectorHigh)
{
	vectorLow = _mm_srai_epi32(vectorLow, 16);
	vectorHigh = _mm_srai_epi32(vectorHigh, 16);
	return _mm_packs_epi32(vectorLow, vectorHigh);
}

/* ============================================================================
* RSPSignExtend16to32: Sign-extend 16-bit slices to 32-bit slices.
* ========================================================================= */
INLINE void RSPSignExtend16to32(__m128i source, __m128i *vectorLow, __m128i *vectorHigh)
{
	__m128i vMask = _mm_srai_epi16(source, 15);
	*vectorHigh = _mm_unpackhi_epi16(source, vMask);
	*vectorLow = _mm_unpacklo_epi16(source, vMask);
}

/* ============================================================================
* RSPZeroExtend16to32: Zero-extend 16-bit slices to 32-bit slices.
* ========================================================================= */
INLINE void RSPZeroExtend16to32(__m128i source, __m128i *vectorLow, __m128i *vectorHigh)
{
	*vectorHigh = _mm_unpackhi_epi16(source, _mm_setzero_si128());
	*vectorLow = _mm_unpacklo_epi16(source, _mm_setzero_si128());
}

/* ============================================================================
* _mm_mullo_epi32: SSE2 lacks _mm_mullo_epi32, define it manually.
* TODO/WARNING/DISCLAIMER: Assumes one argument is positive.
* ========================================================================= */
INLINE __m128i _mm_mullo_epi32(__m128i a, __m128i b)
{
	__m128i a4 = _mm_srli_si128(a, 4);
	__m128i b4 = _mm_srli_si128(b, 4);
	__m128i ba = _mm_mul_epu32(b, a);
	__m128i b4a4 = _mm_mul_epu32(b4, a4);

	__m128i mask = _mm_setr_epi32(~0, 0, ~0, 0);
	__m128i baMask = _mm_and_si128(ba, mask);
	__m128i b4a4Mask = _mm_and_si128(b4a4, mask);
	__m128i b4a4MaskShift = _mm_slli_si128(b4a4Mask, 4);

	return _mm_or_si128(baMask, b4a4MaskShift);
}

/* ============================================================================
* RSPClampLowToVal: Clamps the low word of the accumulator.
* ========================================================================= */
INLINE __m128i RSPClampLowToVal(__m128i vaccLow, __m128i vaccMid, __m128i vaccHigh)
{
	__m128i setMask = _mm_cmpeq_epi16(_mm_setzero_si128(), _mm_setzero_si128());
	__m128i negCheck, useValMask, negVal, posVal;

	/* Compute some common values ahead of time. */
	negCheck = _mm_cmplt_epi16(vaccHigh, _mm_setzero_si128());

	/* If accmulator < 0, clamp to val if val != TMin. */
	useValMask = _mm_and_si128(vaccHigh, _mm_srai_epi16(vaccMid, 15));
	useValMask = _mm_cmpeq_epi16(useValMask, setMask);
	negVal = _mm_and_si128(useValMask, vaccLow);

	/* Otherwise, clamp to ~0 if any high bits are set. */
	useValMask = _mm_or_si128(vaccHigh, _mm_srai_epi16(vaccMid, 15));
	useValMask = _mm_cmpeq_epi16(useValMask, _mm_setzero_si128());
	posVal = _mm_and_si128(useValMask, vaccLow);

	negVal = _mm_and_si128(negCheck, negVal);
	posVal = _mm_andnot_si128(negCheck, posVal);
	return _mm_or_si128(negVal, posVal);
}
#endif
INLINE void cfunc_rsp_vmulf(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;
	//int i;
	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 000000 |
	// ------------------------------------------------------
	//
	// Multiplies signed integer by signed integer * 2

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		if (s1 == -32768 && s2 == -32768)
		{
			// overflow
			SET_ACCUM_H(0, i);
			SET_ACCUM_M(-32768, i);
			SET_ACCUM_L(-32768, i);
			vres[i] = 0x7fff;
		}
		else
		{
			INT64 r =  s1 * s2 * 2;
			r += 0x8000;    // rounding ?
			SET_ACCUM_H((r < 0) ? 0xffff : 0, i);
			SET_ACCUM_M((INT16)(r >> 16), i);
			SET_ACCUM_L((UINT16)(r), i);
			vres[i] = ACCUM_M(rsp, i);
		}
	}
	WRITEBACK_RESULT();
}

INLINE void cfunc_rsp_vmulu(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;
	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 000001 |
	// ------------------------------------------------------
	//

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT64 r = s1 * s2 * 2;
		r += 0x8000;    // rounding ?

		SET_ACCUM_H((UINT16)(r >> 32), i);
		SET_ACCUM_M((UINT16)(r >> 16), i);
		SET_ACCUM_L((UINT16)(r), i);

		if (r < 0)
		{
			vres[i] = 0;
		}
		else if (((INT16)(ACCUM_H(rsp, i)) ^ (INT16)(ACCUM_M(rsp, i))) < 0)
		{
			vres[i] = -1;
		}
		else
		{
			vres[i] = ACCUM_M(rsp, i);
		}
	}
	WRITEBACK_RESULT();
}

INLINE void cfunc_rsp_vmudl(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 001101 |
	// ------------------------------------------------------
	//
	// Multiplies signed integer by unsigned fraction
	// The result is added into accumulator
	// The middle slice of accumulator is stored into destination element

#if USE_SIMD

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Unpack to obtain for 32-bit precision. */
	__m128i unpackLo = _mm_mullo_epi16(vsReg, vtReg);
	__m128i unpackHi = _mm_mulhi_epu16(vsReg, vtReg);
	__m128i loProduct = _mm_unpacklo_epi16(unpackLo, unpackHi);
	__m128i hiProduct = _mm_unpackhi_epi16(unpackLo, unpackHi);

	rsp->xv[VDREG] = rsp->accum_l = RSPPackHi32to16(loProduct, hiProduct);

	rsp->accum_m = _mm_setzero_si128();
	rsp->accum_h = _mm_setzero_si128();

#else

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		UINT32 s1 = (UINT32)(UINT16)w1;
		UINT32 s2 = (UINT32)(UINT16)w2;

		UINT32 r = s1 * s2;

		SET_ACCUM_H(0, i);
		SET_ACCUM_M(0, i);
		SET_ACCUM_L((UINT16)(r >> 16), i);

		vres[i] = ACCUM_L(rsp, i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmudm(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 000101 |
	// ------------------------------------------------------
	//
	// Multiplies signed integer by unsigned fraction
	// The result is stored into accumulator
	// The middle slice of accumulator is stored into destination element

#if USE_SIMD

	__m128i vsRegLo, vsRegHi, vtRegLo, vtRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Unpack to obtain for 32-bit precision. */
	RSPSignExtend16to32(vsReg, &vsRegLo, &vsRegHi);
	RSPZeroExtend16to32(vtReg, &vtRegLo, &vtRegHi);

	/* Begin accumulating the products. */
	__m128i loProduct = _mm_mullo_epi32(vsRegLo, vtRegLo);
	__m128i hiProduct = _mm_mullo_epi32(vsRegHi, vtRegHi);
	rsp->accum_l = RSPPackLo32to16(loProduct, hiProduct);
	rsp->accum_m = rsp->xv[VDREG] = RSPPackHi32to16(loProduct, hiProduct);

	loProduct = _mm_cmplt_epi32(loProduct, _mm_setzero_si128());
	hiProduct = _mm_cmplt_epi32(hiProduct, _mm_setzero_si128());
	rsp->accum_h = _mm_packs_epi32(loProduct, hiProduct);

#else

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (UINT16)w2;

		INT32 r =  s1 * s2;

		SET_ACCUM_H((r < 0) ? 0xffff : 0, i);      // sign-extend to 48-bit
		SET_ACCUM_M((INT16)(r >> 16), i);
		SET_ACCUM_L((UINT16)r, i);

		vres[i] = ACCUM_M(rsp, i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmudn(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 000110 |
	// ------------------------------------------------------
	//
	// Multiplies unsigned fraction by signed integer
	// The result is stored into accumulator
	// The low slice of accumulator is stored into destination element

#if USE_SIMD

	__m128i vsRegLo, vsRegHi, vtRegLo, vtRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Unpack to obtain for 32-bit precision. */
	RSPZeroExtend16to32(vsReg, &vsRegLo, &vsRegHi);
	RSPSignExtend16to32(vtReg, &vtRegLo, &vtRegHi);

	/* Begin accumulating the products. */
	__m128i loProduct = _mm_mullo_epi32(vsRegLo, vtRegLo);
	__m128i hiProduct = _mm_mullo_epi32(vsRegHi, vtRegHi);
	rsp->xv[VDREG] = rsp->accum_l = RSPPackLo32to16(loProduct, hiProduct);
	rsp->accum_m = RSPPackHi32to16(loProduct, hiProduct);
	rsp->accum_h = _mm_cmplt_epi16(rsp->accum_m, _mm_setzero_si128());

#else

	INT16 vres[8] = { 0 };
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (UINT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT32 r = s1 * s2;

		SET_ACCUM_H((r < 0) ? 0xffff : 0, i);      // sign-extend to 48-bit
		SET_ACCUM_M((INT16)(r >> 16), i);
		SET_ACCUM_L((UINT16)(r), i);

		vres[i] = (UINT16)(r);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmudh(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 000111 |
	// ------------------------------------------------------
	//
	// Multiplies signed integer by signed integer
	// The result is stored into highest 32 bits of accumulator, the low slice is zero
	// The highest 32 bits of accumulator is saturated into destination element

#if USE_SIMD

	__m128i vaccLow, vaccHigh;
	__m128i unpackLo, unpackHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Multiply the sources, accumulate the product. */
	unpackLo = _mm_mullo_epi16(vsReg, vtReg);
	unpackHi = _mm_mulhi_epi16(vsReg, vtReg);
	vaccHigh = _mm_unpackhi_epi16(unpackLo, unpackHi);
	vaccLow = _mm_unpacklo_epi16(unpackLo, unpackHi);

	/* Pack the accumulator and result back up. */
	rsp->xv[VDREG] = _mm_packs_epi32(vaccLow, vaccHigh);
	rsp->accum_l = _mm_setzero_si128();
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);

#else

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT32 r = s1 * s2;

		SET_ACCUM_H((INT16)(r >> 16), i);
		SET_ACCUM_M((UINT16)(r), i);
		SET_ACCUM_L(0, i);

		if (r < -32768) r = -32768;
		if (r >  32767) r = 32767;
		vres[i] = (INT16)(r);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmacf(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

#if USE_SIMD

	__m128i loProduct, hiProduct, unpackLo, unpackHi;
	__m128i vaccHigh;
	__m128i vdReg, vdRegLo, vdRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	__m128i vaccLow = rsp->accum_l;

	/* Unpack to obtain for 32-bit precision. */
	RSPZeroExtend16to32(vaccLow, &vaccLow, &vaccHigh);

	/* Begin accumulating the products. */
	unpackLo = _mm_mullo_epi16(vsReg, vtReg);
	unpackHi = _mm_mulhi_epi16(vsReg, vtReg);
	loProduct = _mm_unpacklo_epi16(unpackLo, unpackHi);
	hiProduct = _mm_unpackhi_epi16(unpackLo, unpackHi);
	loProduct = _mm_slli_epi32(loProduct, 1);
	hiProduct = _mm_slli_epi32(hiProduct, 1);

	vdRegLo = _mm_srli_epi32(loProduct, 16);
	vdRegHi = _mm_srli_epi32(hiProduct, 16);
	vdRegLo = _mm_slli_epi32(vdRegLo, 16);
	vdRegHi = _mm_slli_epi32(vdRegHi, 16);
	vdRegLo = _mm_xor_si128(vdRegLo, loProduct);
	vdRegHi = _mm_xor_si128(vdRegHi, hiProduct);

	vaccLow = _mm_add_epi32(vaccLow, vdRegLo);
	vaccHigh = _mm_add_epi32(vaccHigh, vdRegHi);

	rsp->accum_l = vdReg = RSPPackLo32to16(vaccLow, vaccHigh);

	/* Multiply the MSB of sources, accumulate the product. */
	vdRegLo = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	vdRegHi = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	loProduct = _mm_srai_epi32(loProduct, 16);
	hiProduct = _mm_srai_epi32(hiProduct, 16);
	vaccLow = _mm_srai_epi32(vaccLow, 16);
	vaccHigh = _mm_srai_epi32(vaccHigh, 16);

	vaccLow = _mm_add_epi32(loProduct, vaccLow);
	vaccHigh = _mm_add_epi32(hiProduct, vaccHigh);
	vaccLow = _mm_add_epi32(vdRegLo, vaccLow);
	vaccHigh = _mm_add_epi32(vdRegHi, vaccHigh);

	/* Clamp the accumulator and write it all out. */
	rsp->xv[VDREG] = _mm_packs_epi32(vaccLow, vaccHigh);
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);

#else

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT32 r = s1 * s2;

		UINT64 q = ACCUM(i) & 0x000000000000ffffL;
		q |= (((UINT64)(UINT16)ACCUM_L(rsp, i)) << 16);
		q |= (((UINT64)(UINT16)ACCUM_M(rsp, i)) << 32);
		q |= (((UINT64)(UINT16)ACCUM_H(rsp, i)) << 48);

		q += (INT64)(r) << 17;
		ACCUM(i) = q & 0x000000000000ffffL;

		SET_ACCUM_L((UINT16)(q >> 16), i);
		SET_ACCUM_M((UINT16)(q >> 32), i);
		SET_ACCUM_H((UINT16)(q >> 48), i);

		vres[i] = SATURATE_ACCUM(rsp, i, 1, 0x8000, 0x7fff);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmacu(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 001001 |
	// ------------------------------------------------------
	//

#if USE_SIMD

	__m128i loProduct, hiProduct, unpackLo, unpackHi;
	__m128i vaccHigh;
	__m128i vdReg, vdRegLo, vdRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	__m128i vaccLow = rsp->accum_l;

	/* Unpack to obtain for 32-bit precision. */
	RSPZeroExtend16to32(vaccLow, &vaccLow, &vaccHigh);

	/* Begin accumulating the products. */
	unpackLo = _mm_mullo_epi16(vsReg, vtReg);
	unpackHi = _mm_mulhi_epi16(vsReg, vtReg);
	loProduct = _mm_unpacklo_epi16(unpackLo, unpackHi);
	hiProduct = _mm_unpackhi_epi16(unpackLo, unpackHi);
	loProduct = _mm_slli_epi32(loProduct, 1);
	hiProduct = _mm_slli_epi32(hiProduct, 1);

	vdRegLo = _mm_srli_epi32(loProduct, 16);
	vdRegHi = _mm_srli_epi32(hiProduct, 16);
	vdRegLo = _mm_slli_epi32(vdRegLo, 16);
	vdRegHi = _mm_slli_epi32(vdRegHi, 16);
	vdRegLo = _mm_xor_si128(vdRegLo, loProduct);
	vdRegHi = _mm_xor_si128(vdRegHi, hiProduct);

	vaccLow = _mm_add_epi32(vaccLow, vdRegLo);
	vaccHigh = _mm_add_epi32(vaccHigh, vdRegHi);

	rsp->accum_l = vdReg = RSPPackLo32to16(vaccLow, vaccHigh);

	/* Multiply the MSB of sources, accumulate the product. */
	vdRegLo = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	vdRegHi = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	loProduct = _mm_srai_epi32(loProduct, 16);
	hiProduct = _mm_srai_epi32(hiProduct, 16);
	vaccLow = _mm_srai_epi32(vaccLow, 16);
	vaccHigh = _mm_srai_epi32(vaccHigh, 16);

	vaccLow = _mm_add_epi32(loProduct, vaccLow);
	vaccHigh = _mm_add_epi32(hiProduct, vaccHigh);
	vaccLow = _mm_add_epi32(vdRegLo, vaccLow);
	vaccHigh = _mm_add_epi32(vdRegHi, vaccHigh);

	/* Clamp the accumulator and write it all out. */
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);
#else

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT32 r1 = s1 * s2;
		UINT32 r2 = (UINT16)ACCUM_L(rsp, i) + ((UINT16)(r1) * 2);
		UINT32 r3 = (UINT16)ACCUM_M(rsp, i) + (UINT16)((r1 >> 16) * 2) + (UINT16)(r2 >> 16);

		SET_ACCUM_L((UINT16)(r2), i);
		SET_ACCUM_M((UINT16)(r3), i);
		SET_ACCUM_H(ACCUM_H(rsp, i) + (UINT16)(r3 >> 16) + (UINT16)(r1 >> 31), i);

		//res = SATURATE_ACCUM(i, 1, 0x0000, 0xffff);
		if ((INT16)ACCUM_H(rsp, i) < 0)
		{
			vres[i] = 0;
		}
		else
		{
			if (ACCUM_H(rsp, i) != 0)
			{
				vres[i] = (INT16)0xffff;
			}
			else
			{
				if ((INT16)ACCUM_M(rsp, i) < 0)
				{
					vres[i] = (INT16)0xffff;
				}
				else
				{
					vres[i] = ACCUM_M(rsp, i);
				}
			}
		}
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmadl(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 001100 |
	// ------------------------------------------------------
	//
	// Multiplies unsigned fraction by unsigned fraction
	// Adds the higher 16 bits of the 32-bit result to accumulator
	// The low slice of accumulator is stored into destination element

#if USE_SIMD

	__m128i vaccHigh;
	__m128i unpackHi, loProduct, hiProduct;
	__m128i vdReg, vdRegLo, vdRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	__m128i vaccLow = rsp->accum_l;

	/* Unpack to obtain for 32-bit precision. */
	RSPZeroExtend16to32(vaccLow, &vaccLow, &vaccHigh);

	/* Begin accumulating the products. */
	unpackHi = _mm_mulhi_epu16(vsReg, vtReg);
	loProduct = _mm_unpacklo_epi16(unpackHi, _mm_setzero_si128());
	hiProduct = _mm_unpackhi_epi16(unpackHi, _mm_setzero_si128());

	vaccLow = _mm_add_epi32(vaccLow, loProduct);
	vaccHigh = _mm_add_epi32(vaccHigh, hiProduct);
	rsp->accum_l = vdReg = RSPPackLo32to16(vaccLow, vaccHigh);

	/* Finish accumulating whatever is left. */
	vdRegLo = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	vdRegHi = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	vaccLow = _mm_srai_epi32(vaccLow, 16);
	vaccHigh = _mm_srai_epi32(vaccHigh, 16);
	vaccLow = _mm_add_epi32(vdRegLo, vaccLow);
	vaccHigh = _mm_add_epi32(vdRegHi, vaccHigh);

	/* Clamp the accumulator and write it all out. */
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);
	rsp->xv[VDREG] = RSPClampLowToVal(vdReg, rsp->accum_m, rsp->accum_h);

#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		UINT32 s1 = w1;
		UINT32 s2 = w2;

		UINT32 r1 = s1 * s2;
		UINT32 r2 = (UINT16)ACCUM_L(rsp, i) + (r1 >> 16);
		UINT32 r3 = (UINT16)ACCUM_M(rsp, i) + (r2 >> 16);

		SET_ACCUM_L((UINT16)r2, i);
		SET_ACCUM_M((UINT16)r3, i);
		SET_ACCUM_H(ACCUM_H(rsp, i) + (INT16)(r3 >> 16), i);

		vres[i] = SATURATE_ACCUM(rsp, i, 0, 0x0000, 0xffff);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmadm(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

#if USE_SIMD
	__m128i vaccLow, vaccHigh, loProduct, hiProduct;
	__m128i vsRegLo, vsRegHi, vtRegLo, vtRegHi, vdRegLo, vdRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Unpack to obtain for 32-bit precision. */
	RSPSignExtend16to32(vsReg, &vsRegLo, &vsRegHi);
	RSPZeroExtend16to32(vtReg, &vtRegLo, &vtRegHi);
	RSPZeroExtend16to32(rsp->accum_l, &vaccLow, &vaccHigh);

	/* Begin accumulating the products. */
	loProduct = _mm_mullo_epi32(vsRegLo, vtRegLo);
	hiProduct = _mm_mullo_epi32(vsRegHi, vtRegHi);

	vdRegLo = _mm_srli_epi32(loProduct, 16);
	vdRegHi = _mm_srli_epi32(hiProduct, 16);
	vdRegLo = _mm_slli_epi32(vdRegLo, 16);
	vdRegHi = _mm_slli_epi32(vdRegHi, 16);
	vdRegLo = _mm_xor_si128(vdRegLo, loProduct);
	vdRegHi = _mm_xor_si128(vdRegHi, hiProduct);
	vaccLow = _mm_add_epi32(vaccLow, vdRegLo);
	vaccHigh = _mm_add_epi32(vaccHigh, vdRegHi);

	rsp->accum_l = rsp->xv[VDREG] = RSPPackLo32to16(vaccLow, vaccHigh);

	/* Multiply the MSB of sources, accumulate the product. */
	vdRegLo = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	vdRegHi = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	loProduct = _mm_srai_epi32(loProduct, 16);
	hiProduct = _mm_srai_epi32(hiProduct, 16);
	vaccLow = _mm_srai_epi32(vaccLow, 16);
	vaccHigh = _mm_srai_epi32(vaccHigh, 16);

	vaccLow = _mm_add_epi32(loProduct, vaccLow);
	vaccHigh = _mm_add_epi32(hiProduct, vaccHigh);
	vaccLow = _mm_add_epi32(vdRegLo, vaccLow);
	vaccHigh = _mm_add_epi32(vdRegHi, vaccHigh);

	/* Clamp the accumulator and write it all out. */
	rsp->xv[VDREG] = _mm_packs_epi32(vaccLow, vaccHigh);
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);

#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		UINT32 s1 = (INT32)(INT16)w1;
		UINT32 s2 = (UINT16)w2;

		UINT32 r1 = s1 * s2;
		UINT32 r2 = (UINT16)ACCUM_L(rsp, i) + (UINT16)(r1);
		UINT32 r3 = (UINT16)ACCUM_M(rsp, i) + (r1 >> 16) + (r2 >> 16);

		SET_ACCUM_L((UINT16)r2, i);
		SET_ACCUM_M((UINT16)r3, i);
		SET_ACCUM_H((UINT16)ACCUM_H(rsp, i) + (UINT16)(r3 >> 16), i);
		if ((INT32)(r1) < 0)
		{
			SET_ACCUM_H((UINT16)ACCUM_H(rsp, i) - 1, i);
		}

		vres[i] = SATURATE_ACCUM(rsp, i, 1, 0x8000, 0x7fff);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmadn(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

#if USE_SIMD
	__m128i vaccLow, vaccHigh, loProduct, hiProduct;
	__m128i vsRegLo, vsRegHi, vtRegLo, vtRegHi, vdRegLo, vdRegHi;

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	vaccLow = rsp->accum_l;

	RSPZeroExtend16to32(vsReg, &vsRegLo, &vsRegHi);
	RSPSignExtend16to32(vtReg, &vtRegLo, &vtRegHi);
	RSPZeroExtend16to32(vaccLow, &vaccLow, &vaccHigh);

	/* Begin accumulating the products. */
	loProduct = _mm_mullo_epi32(vsRegLo, vtRegLo);
	hiProduct = _mm_mullo_epi32(vsRegHi, vtRegHi);

	vdRegLo = _mm_srli_epi32(loProduct, 16);
	vdRegHi = _mm_srli_epi32(hiProduct, 16);
	vdRegLo = _mm_slli_epi32(vdRegLo, 16);
	vdRegHi = _mm_slli_epi32(vdRegHi, 16);
	vdRegLo = _mm_xor_si128(vdRegLo, loProduct);
	vdRegHi = _mm_xor_si128(vdRegHi, hiProduct);

	vaccLow = _mm_add_epi32(vaccLow, vdRegLo);
	vaccHigh = _mm_add_epi32(vaccHigh, vdRegHi);

	rsp->accum_l = RSPPackLo32to16(vaccLow, vaccHigh);

	/* Multiply the MSB of sources, accumulate the product. */
	vdRegLo = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	vdRegHi = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	loProduct = _mm_srai_epi32(loProduct, 16);
	hiProduct = _mm_srai_epi32(hiProduct, 16);
	vaccLow = _mm_srai_epi32(vaccLow, 16);
	vaccHigh = _mm_srai_epi32(vaccHigh, 16);

	vaccLow = _mm_add_epi32(loProduct, vaccLow);
	vaccHigh = _mm_add_epi32(hiProduct, vaccHigh);
	vaccLow = _mm_add_epi32(vdRegLo, vaccLow);
	vaccHigh = _mm_add_epi32(vdRegHi, vaccHigh);

	/* Clamp the accumulator and write it all out. */
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);
	rsp->xv[VDREG] = RSPClampLowToVal(rsp->accum_l, rsp->accum_m, rsp->accum_h);
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (UINT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		UINT64 q = (UINT64)ACCUM(i) & 0x000000000000ffffL;
		q |= (((UINT64)ACCUM_L(rsp, i)) << 16);
		q |= (((UINT64)ACCUM_M(rsp, i)) << 32);
		q |= (((UINT64)ACCUM_H(rsp, i)) << 48);
		q += (INT64)(s1*s2) << 16;

		ACCUM(i) = q & 0x000000000000ffffL;
		SET_ACCUM_L((UINT16)(q >> 16), i);
		SET_ACCUM_M((UINT16)(q >> 32), i);
		SET_ACCUM_H((UINT16)(q >> 48), i);

		vres[i] = SATURATE_ACCUM(rsp, i, 0, 0x0000, 0xffff);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmadh(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 001111 |
	// ------------------------------------------------------
	//
	// Multiplies signed integer by signed integer
	// The result is added into highest 32 bits of accumulator, the low slice is zero
	// The highest 32 bits of accumulator is saturated into destination element

#if USE_SIMD

	__m128i vsReg = rsp->xv[VS1REG];
	__m128i vtReg = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	/* Unpack to obtain for 32-bit precision. */
	__m128i vaccLow = _mm_unpacklo_epi16(rsp->accum_m, rsp->accum_h);
	__m128i vaccHigh = _mm_unpackhi_epi16(rsp->accum_m, rsp->accum_h);

	/* Multiply the sources, accumulate the product. */
	__m128i unpackLo = _mm_mullo_epi16(vsReg, vtReg);
	__m128i unpackHi = _mm_mulhi_epi16(vsReg, vtReg);
	__m128i loProduct = _mm_unpacklo_epi16(unpackLo, unpackHi);
	__m128i hiProduct = _mm_unpackhi_epi16(unpackLo, unpackHi);
	vaccLow = _mm_add_epi32(vaccLow, loProduct);
	vaccHigh = _mm_add_epi32(vaccHigh, hiProduct);

	/* Pack the accumulator and result back up. */
	rsp->xv[VDREG] = _mm_packs_epi32(vaccLow, vaccHigh);
	rsp->accum_m = RSPPackLo32to16(vaccLow, vaccHigh);
	rsp->accum_h = RSPPackHi32to16(vaccLow, vaccHigh);

#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;

		INT32 accum = (UINT32)(UINT16)ACCUM_M(rsp, i);
		accum |= ((UINT32)((UINT16)ACCUM_H(rsp, i))) << 16;
		accum += s1*s2;

		SET_ACCUM_H((UINT16)(accum >> 16), i);
		SET_ACCUM_M((UINT16)accum, i);

		vres[i] = SATURATE_ACCUM1(rsp, i, 0x8000, 0x7fff);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vadd(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 010000 |
	// ------------------------------------------------------
	//
	// Adds two vector registers and carry flag, the result is saturated to 32767

#if USE_SIMD

	__m128i shuffled = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i carry = _mm_and_si128(rsp->xvflag[CARRY], vec_flagmask);
	rsp->accum_l = _mm_add_epi16(_mm_add_epi16(rsp->xv[VS1REG], shuffled), carry);

	__m128i addvec = _mm_adds_epi16(rsp->xv[VS1REG], shuffled);

	carry = _mm_and_si128(carry, _mm_xor_si128(_mm_cmpeq_epi16(addvec, vec_32767), vec_neg1));
	carry = _mm_and_si128(carry, _mm_xor_si128(_mm_cmpeq_epi16(addvec, vec_n32768), vec_neg1));

	rsp->xv[VDREG] = _mm_add_epi16(addvec, carry);

	rsp->xvflag[ZERO] = _mm_setzero_si128();
	rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	INT16 vres[8] = { 0 };
	for (int i = 0; i < 8; i++)
	{
		INT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;
		INT32 r = s1 + s2 + (((CARRY_FLAG(rsp, i)) != 0) ? 1 : 0);

		SET_ACCUM_L((INT16)(r), i);

		if (r > 32767) r = 32767;
		if (r < -32768) r = -32768;
		vres[i] = (INT16)(r);
	}
	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vsub(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 010001 |
	// ------------------------------------------------------
	//
	// Subtracts two vector registers and carry flag, the result is saturated to -32768

	// TODO: check VS2REG == VDREG

#if USE_SIMD
	__m128i shuffled = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i carry = _mm_and_si128(rsp->xvflag[CARRY], vec_flagmask);
	__m128i unsat = _mm_sub_epi16(_mm_sub_epi16(rsp->xv[VS1REG], shuffled), carry);

	__m128i subvec = _mm_subs_epi16(rsp->xv[VS1REG], shuffled);

	carry = _mm_and_si128(carry, _mm_xor_si128(_mm_cmpeq_epi16(subvec, vec_n32768), vec_neg1));

	rsp->xv[VDREG] = _mm_sub_epi16(subvec, carry);

	rsp->accum_l = unsat;

	rsp->xvflag[ZERO] = _mm_setzero_si128();
	rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (INT32)(INT16)w1;
		INT32 s2 = (INT32)(INT16)w2;
		INT32 r = s1 - s2 - (((CARRY_FLAG(rsp, i)) != 0) ? 1 : 0);

		SET_ACCUM_L((INT16)(r), i);

		if (r > 32767) r = 32767;
		if (r < -32768) r = -32768;

		vres[i] = (INT16)(r);
	}
	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vabs(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 010011 |
	// ------------------------------------------------------
	//
	// Changes the sign of source register 2 if source register 1 is negative and stores
	// the result to destination register

#if USE_SIMD
	__m128i shuf2 = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i negs2 = _mm_sub_epi16(_mm_setzero_si128(), shuf2);
	__m128i s2_n32768 = _mm_cmpeq_epi16(shuf2, vec_n32768);
	__m128i s1_lz = _mm_cmplt_epi16(rsp->xv[VS1REG], _mm_setzero_si128());

	__m128i result_gz = _mm_and_si128(shuf2, _mm_cmpgt_epi16(rsp->xv[VS1REG], _mm_setzero_si128()));
	__m128i result_n32768 = _mm_and_si128(s1_lz, _mm_and_si128(vec_32767, s2_n32768));
	__m128i result_negs2 = _mm_and_si128(s1_lz, _mm_and_si128(negs2, _mm_xor_si128(s2_n32768, vec_neg1)));
	rsp->xv[VDREG] = rsp->accum_l = _mm_or_si128(result_gz, _mm_or_si128(result_n32768, result_negs2));
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if (s1 < 0)
		{
			if (s2 == -32768)
			{
				vres[i] = 32767;
			}
			else
			{
				vres[i] = -s2;
			}
		}
		else if (s1 > 0)
		{
			vres[i] = s2;
		}
		else
		{
			vres[i] = 0;
		}

		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vaddc(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 010100 |
	// ------------------------------------------------------
	//
	// Adds two vector registers, the carry out is stored into carry register

	// TODO: check VS2REG = VDREG

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();

#if USE_SIMD
	__m128i shuf2 = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i vec7531 = _mm_and_si128(rsp->xv[VS1REG], vec_lomask);
	__m128i vec6420 = _mm_srli_epi32(rsp->xv[VS1REG], 16);
	__m128i shuf7531 = _mm_and_si128(shuf2, vec_lomask);
	__m128i shuf6420 = _mm_srli_epi32(shuf2, 16);
	__m128i sum7531 = _mm_add_epi32(vec7531, shuf7531);
	__m128i sum6420 = _mm_add_epi32(vec6420, shuf6420);

	__m128i over7531 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum7531, _mm_setzero_si128()), vec_neg1), vec_himask);
	__m128i over6420 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum6420, _mm_setzero_si128()), vec_neg1), vec_himask);

	rsp->xvflag[CARRY] = _mm_or_si128(over6420, _mm_srli_epi32(over7531, 16));
	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(_mm_slli_epi32(sum6420, 16), sum7531);
#else
	INT16 vres[8] = { 0 };
	for (int i = 0; i < 8; i++)
	{
		INT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (UINT32)(UINT16)w1;
		INT32 s2 = (UINT32)(UINT16)w2;
		INT32 r = s1 + s2;

		vres[i] = (INT16)r;
		SET_ACCUM_L((INT16)r, i);

		if (r & 0xffff0000)
		{
			SET_CARRY_FLAG(i);
		}
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vsubc(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 010101 |
	// ------------------------------------------------------
	//
	// Subtracts two vector registers, the carry out is stored into carry register

	// TODO: check VS2REG = VDREG

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();

#if USE_SIMD
	__m128i shuf2 = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i vec7531 = _mm_and_si128(rsp->xv[VS1REG], vec_lomask);
	__m128i vec6420 = _mm_srli_epi32(rsp->xv[VS1REG], 16);
	__m128i shuf7531 = _mm_and_si128(shuf2, vec_lomask);
	__m128i shuf6420 = _mm_srli_epi32(shuf2, 16);
	__m128i sum7531 = _mm_sub_epi32(vec7531, shuf7531);
	__m128i sum6420 = _mm_sub_epi32(vec6420, shuf6420);

	__m128i over7531 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum7531, _mm_setzero_si128()), vec_neg1), vec_himask);
	__m128i over6420 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum6420, _mm_setzero_si128()), vec_neg1), vec_himask);
	sum7531 = _mm_and_si128(sum7531, vec_lomask);
	sum6420 = _mm_and_si128(sum6420, vec_lomask);
	__m128i zero7531 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum7531, _mm_setzero_si128()), vec_neg1), vec_lomask);
	__m128i zero6420 = _mm_and_si128(_mm_xor_si128(_mm_cmpeq_epi16(sum6420, _mm_setzero_si128()), vec_neg1), vec_lomask);

	rsp->xvflag[CARRY] = _mm_or_si128(over6420, _mm_srli_epi32(over7531, 16));
	rsp->xvflag[ZERO] = _mm_or_si128(zero6420, _mm_srli_epi32(zero7531, 16));

	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(_mm_slli_epi32(sum6420, 16), sum7531);
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 w1, w2;
		SCALAR_GET_VS1(w1, i);
		SCALAR_GET_VS2(w2, i);
		INT32 s1 = (UINT32)(UINT16)w1;
		INT32 s2 = (UINT32)(UINT16)w2;
		INT32 r = s1 - s2;

		vres[i] = (INT16)(r);
		SET_ACCUM_L((UINT16)r, i);

		if ((UINT16)(r) != 0)
		{
			SET_ZERO_FLAG(i);
		}
		if (r & 0xffff0000)
		{
			SET_CARRY_FLAG(i);
		}
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vsaw(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 011101 |
	// ------------------------------------------------------
	//
	// Stores high, middle or low slice of accumulator to destination vector

	switch (EL)
	{
		case 0x08:      // VSAWH
		{
#if USE_SIMD
			rsp->xv[VDREG] = rsp->accum_h;
#else
			for (int i = 0; i < 8; i++)
			{
				W_VREG_S(VDREG, i) = ACCUM_H(rsp, i);
			}
#endif
			break;
		}
		case 0x09:      // VSAWM
		{
#if USE_SIMD
			rsp->xv[VDREG] = rsp->accum_m;
#else
			for (int i = 0; i < 8; i++)
			{
				W_VREG_S(VDREG, i) = ACCUM_M(rsp, i);
			}
#endif
			break;
		}
		case 0x0a:      // VSAWL
		{
#if USE_SIMD
			rsp->xv[VDREG] = rsp->accum_l;
#else
			for (int i = 0; i < 8; i++)
			{
				W_VREG_S(VDREG, i) = ACCUM_L(rsp, i);
			}
#endif
			break;
		}
		default:    fatalerror("RSP: VSAW: el = %d\n", EL);
	}
}

INLINE void cfunc_rsp_vlt(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	//int i;
	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100000 |
	// ------------------------------------------------------
	//
	// Sets compare flags if elements in VS1 are less than VS2
	// Moves the element in VS2 to destination vector

#if USE_SIMD
	rsp->xvflag[COMPARE] = rsp->xvflag[CLIP2] = _mm_setzero_si128();

	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i zc_mask = _mm_and_si128(rsp->xvflag[ZERO], rsp->xvflag[CARRY]);
	__m128i lt_mask = _mm_cmplt_epi16(rsp->xv[VS1REG], shuf);
	__m128i eq_mask = _mm_and_si128(_mm_cmpeq_epi16(rsp->xv[VS1REG], shuf), zc_mask);

	rsp->xvflag[COMPARE] = _mm_or_si128(lt_mask, eq_mask);

	__m128i result = _mm_and_si128(rsp->xv[VS1REG], rsp->xvflag[COMPARE]);
	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(result, _mm_and_si128(shuf, _mm_xor_si128(rsp->xvflag[COMPARE], vec_neg1)));

	rsp->xvflag[ZERO] = rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP2_FLAGS();

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if (s1 < s2)
		{
			SET_COMPARE_FLAG(i);
		}
		else if (s1 == s2)
		{
			if (ZERO_FLAG(rsp, i) != 0 && CARRY_FLAG(rsp, i) != 0)
			{
				SET_COMPARE_FLAG(i);
			}
		}

		if (COMPARE_FLAG(rsp, i) != 0)
		{
			vres[i] = s1;
		}
		else
		{
			vres[i] = s2;
		}

		SET_ACCUM_L(vres[i], i);
	}

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_veq(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100001 |
	// ------------------------------------------------------
	//
	// Sets compare flags if elements in VS1 are equal with VS2
	// Moves the element in VS2 to destination vector

#if USE_SIMD
	rsp->xvflag[COMPARE] = rsp->xvflag[CLIP2] = _mm_setzero_si128();

	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i zero_mask = _mm_cmpeq_epi16(rsp->xvflag[ZERO], _mm_setzero_si128());
	__m128i eq_mask = _mm_cmpeq_epi16(rsp->xv[VS1REG], shuf);

	rsp->xvflag[COMPARE] = _mm_and_si128(zero_mask, eq_mask);

	__m128i result = _mm_and_si128(rsp->xv[VS1REG], rsp->xvflag[COMPARE]);
	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(result, _mm_and_si128(shuf, _mm_xor_si128(rsp->xvflag[COMPARE], vec_neg1)));

	rsp->xvflag[ZERO] = rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP2_FLAGS();

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if ((s1 == s2) && ZERO_FLAG(rsp, i) == 0)
		{
			SET_COMPARE_FLAG(i);
			vres[i] = s1;
		}
		else
		{
			vres[i] = s2;
		}

		SET_ACCUM_L(vres[i], i);
	}

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vne(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100010 |
	// ------------------------------------------------------
	//
	// Sets compare flags if elements in VS1 are not equal with VS2
	// Moves the element in VS2 to destination vector

#if USE_SIMD
	rsp->xvflag[COMPARE] = rsp->xvflag[CLIP2] = _mm_setzero_si128();

	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i neq_mask = _mm_xor_si128(_mm_cmpeq_epi16(rsp->xv[VS1REG], shuf), vec_neg1);

	rsp->xvflag[COMPARE] = _mm_or_si128(rsp->xvflag[ZERO], neq_mask);

	__m128i result = _mm_and_si128(rsp->xv[VS1REG], rsp->xvflag[COMPARE]);
	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(result, _mm_and_si128(shuf, _mm_xor_si128(rsp->xvflag[COMPARE], vec_neg1)));

	rsp->xvflag[ZERO] = rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP2_FLAGS();

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if (s1 != s2 || ZERO_FLAG(rsp, i) != 0)
		{
			SET_COMPARE_FLAG(i);
			vres[i] = s1;
		}
		else
		{
			vres[i] = s2;
		}

		SET_ACCUM_L(vres[i], i);
	}

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vge(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	//int i;
	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100011 |
	// ------------------------------------------------------
	//
	// Sets compare flags if elements in VS1 are greater or equal with VS2
	// Moves the element in VS2 to destination vector

#if USE_SIMD
	rsp->xvflag[COMPARE] = rsp->xvflag[CLIP2] = _mm_setzero_si128();

	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i flag_mask = _mm_cmpeq_epi16(_mm_or_si128(rsp->xvflag[ZERO], rsp->xvflag[CARRY]), _mm_setzero_si128());
	__m128i eq_mask = _mm_and_si128(_mm_cmpeq_epi16(rsp->xv[VS1REG], shuf), flag_mask);
	__m128i gt_mask = _mm_cmpgt_epi16(rsp->xv[VS1REG], shuf);
	rsp->xvflag[COMPARE] = _mm_or_si128(eq_mask, gt_mask);

	__m128i result = _mm_and_si128(rsp->xv[VS1REG], rsp->xvflag[COMPARE]);
	rsp->accum_l = rsp->xv[VDREG] = _mm_or_si128(result, _mm_and_si128(shuf, _mm_xor_si128(rsp->xvflag[COMPARE], vec_neg1)));

	rsp->xvflag[ZERO] = rsp->xvflag[CARRY] = _mm_setzero_si128();
#else
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP2_FLAGS();

	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		if ((s1 == s2 && (ZERO_FLAG(rsp, i) == 0 || CARRY_FLAG(rsp, i) == 0)) || s1 > s2)
		{
			SET_COMPARE_FLAG(i);
			vres[i] = s1;
		}
		else
		{
			vres[i] = s2;
		}

		SET_ACCUM_L(vres[i], i);
	}

	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vcl(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;
	INT16 vres[8];

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100100 |
	// ------------------------------------------------------
	//
	// Vector clip low

#if 0//USE_SIMD
	__m128i flag0_07 = _mm_set_epi16(CARRY_FLAG(0), CARRY_FLAG(1), CARRY_FLAG(2), CARRY_FLAG(3),
										CARRY_FLAG(4), CARRY_FLAG(5), CARRY_FLAG(6), CARRY_FLAG(7));
	__m128i flag0_815 = _mm_set_epi16(ZERO_FLAG(0), ZERO_FLAG(1), ZERO_FLAG(2), ZERO_FLAG(3),
										ZERO_FLAG(4), ZERO_FLAG(5), ZERO_FLAG(6), ZERO_FLAG(7));
	__m128i flag1_07 = _mm_set_epi16(COMPARE_FLAG(0), COMPARE_FLAG(1), COMPARE_FLAG(2), COMPARE_FLAG(3),
										COMPARE_FLAG(4), COMPARE_FLAG(5), COMPARE_FLAG(6), COMPARE_FLAG(7));
	__m128i flag1_815 = _mm_set_epi16((rsp->flag[1] >>  8) & 1, (rsp->flag[1] >>  9) & 1, (rsp->flag[1] >> 10) & 1, (rsp->flag[1] >> 11) & 1,
										(rsp->flag[1] >> 12) & 1, (rsp->flag[1] >> 13) & 1, (rsp->flag[1] >> 14) & 1, (rsp->flag[1] >> 15) & 1);
	__m128i flag2_07 = _mm_set_epi16(rsp->flag[2][0], rsp->flag[2][1], rsp->flag[2][2], rsp->flag[2][3],
										rsp->flag[2][4], rsp->flag[2][5], rsp->flag[2][6], rsp->flag[2][7]);
	__m128i n0_07 = _mm_xor_si128(flag0_07, vec_neg1);
	__m128i n0_815 = _mm_xor_si128(flag0_815, vec_neg1);
	__m128i n1_07 = _mm_xor_si128(flag1_07, vec_neg1);
	__m128i n1_815 = _mm_xor_si128(flag1_815, vec_neg1);
	__m128i n2_07 = _mm_xor_si128(flag2_07, vec_neg1);

	__m128i shuf2 = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i vec7531 = _mm_and_si128(rsp->xv[VS1REG], vec_lomask);
	__m128i vec6420 = _mm_srli_epi32(rsp->xv[VS1REG], 16);
	__m128i shuf7531 = _mm_and_si128(shuf2, vec_lomask);
	__m128i shuf6420 = _mm_srli_epi32(shuf2, 16);
	__m128i sub7531 = _mm_sub_epi32(vec7531, shuf7531);
	__m128i sub6420 = _mm_sub_epi32(vec6420, shuf6420);
	__m128i subh7531 = _mm_and_si128(sub7531, vec_himask);
	__m128i subh6420 = _mm_and_si128(sub6420, vec_himask);
	__m128i sub_gez = _mm_or_si128(_mm_slli_epi32(_mm_cmpeq_epi16(subh6420, _mm_setzero_si128()), 16), _mm_cmpeq_epi16(subh7531, _mm_setzero_si128()));
	__m128i sub_lz  = _mm_xor_si128(sub_gez, vec_neg1);

	__m128i sum7531 = _mm_add_epi32(vec7531, shuf7531);
	__m128i sum6420 = _mm_add_epi32(vec6420, shuf6420);
	__m128i suml7531 = _mm_and_si128(sum7531, vec_lomask);
	__m128i suml6420 = _mm_and_si128(sum6420, vec_lomask);
	__m128i sumh7531 = _mm_and_si128(sum7531, vec_himask);
	__m128i sumh6420 = _mm_and_si128(sum6420, vec_himask);
	__m128i suml_z = _mm_or_si128(_mm_slli_epi32(_mm_cmpeq_epi16(suml6420, _mm_setzero_si128()), 16), _mm_cmpeq_epi16(suml7531, _mm_setzero_si128()));
	__m128i sumh_1 = _mm_or_si128(_mm_slli_epi32(_mm_cmpeq_epi16(sumh6420, vec_hibit), 16), _mm_cmpeq_epi16(sumh7531, vec_hibit));
	__m128i sumh_z = _mm_or_si128(_mm_slli_epi32(_mm_cmpeq_epi16(sumh6420, _mm_setzero_si128()), 16), _mm_cmpeq_epi16(sumh7531, _mm_setzero_si128()));
	__m128i sum_z = _mm_and_si128(suml_z, sumh_z);
	__m128i sum_nz = _mm_xor_si128(sum_z, vec_neg1);
	__m128i sum_le0x10000 = _mm_or_si128(_mm_and_si128(suml_z, sumh_1), sumh_z);
	__m128i sum_g0x10000 = _mm_xor_si128(sum_le0x10000, vec_neg1);

	__m128i  f0a_and_nf0b = _mm_and_si128(flag0_07, n0_815);
	__m128i nf0a_and_nf0b = _mm_and_si128(   n0_07, n0_815);

	// accum set to -s2 if  flag0[0-7] &&  flag0[8-15] &&  flag1[0-7]
	// accum set to -s2 if  flag0[0-7] && !flag0[8-15] &&  flag2[0-7] && (s1 + s2) >  0x10000
	// accum set to -s2 if  flag0[0-7] && !flag0[8-15] && !flag2[0-7] && (s1 + s2) == 0
	__m128i accum_ns2 = _mm_and_si128(_mm_and_si128(flag0_07, flag0_815), flag1_07);
	accum_ns2 = _mm_or_si128(accum_ns2, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, flag2_07), sum_g0x10000));
	accum_ns2 = _mm_or_si128(accum_ns2, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, n2_07), sum_z));

	// accum set to  s2 if !flag0[0-7] &&  flag0[8-15] &&  flag1[8-15]
	// accum set to  s2 if !flag0[0-7] && !flag0[8-15] && (s1 - s2) >= 0
	__m128i accum_s2 = _mm_and_si128(n0_07, _mm_and_si128(flag0_815, flag1_815));
	accum_s2 = _mm_or_si128(accum_s2, _mm_and_si128(_mm_and_si128(n0_07, n0_815), sub_gez));

	// flag1[8-15]  set if !flag0[0-7] && !flag0[8-15] && (s1 - s2) >= 0
	__m128i new_f1b_s = _mm_and_si128(_mm_and_si128(nf0a_and_nf0b, sub_gez), vec_flagmask);
	UINT16 flag1_set = 0;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 0) << 8;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 1) << 9;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 2) << 10;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 3) << 11;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 4) << 12;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 5) << 13;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 6) << 14;
	flag1_set |= _mm_extract_epi16(new_f1b_s, 7) << 15;

	// flag1[8-15]unset if !flag0[0-7] && !flag0[8-15] && (s1 - s2) < 0
	__m128i new_f1b_u = _mm_xor_si128(vec_neg1, _mm_and_si128(nf0a_and_nf0b, sub_lz));
	new_f1b_u = _mm_and_si128(new_f1b_u, vec_flagmask);
	UINT16 flag1_unset = 0;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 0) << 8;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 1) << 9;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 2) << 10;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 3) << 11;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 4) << 12;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 5) << 13;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 6) << 14;
	flag1_unset |= _mm_extract_epi16(new_f1b_u, 7) << 15;

	// flag1[0-7]   set if  flag0[0-7] && !flag0[8-15] &&  flag2[0-7] && (s1 + s2) <= 0x10000
	// flag1[0-7]   set if  flag0[0-7] && !flag0[8-15] && !flag2[0-7] && (s1 + s2) == 0
	__m128i new_f1a_s = _mm_and_si128(_mm_and_si128(f0a_and_nf0b, flag2_07), sum_le0x10000);
	new_f1a_s = _mm_or_si128(new_f1a_u, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, n2_07), sum_z));
	new_f1a_s = _mm_and_si128(new_f1a_s, vec_flagmask);
	flag1_set |= _mm_extract_epi16(new_f1a_s, 0) << 0;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 1) << 1;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 2) << 2;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 3) << 3;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 4) << 4;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 5) << 5;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 6) << 6;
	flag1_set |= _mm_extract_epi16(new_f1a_s, 7) << 7;

	// flag1[0-7] unset if  flag0[0-7] && !flag0[8-15] &&  flag2[0-7] && (s1 + s2) >  0x10000
	// flag1[0-7] unset if  flag0[0-7] && !flag0[8-15] && !flag2[0-7] && (s1 + s2) != 0
	__m128i new_f1a_u = _mm_and_si128(_mm_and_si128(f0a_and_nf0b, flag2_07), sum_g0x10000);
	new_f1a_u = _mm_or_si128(new_f1a_u, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, n2_07), sum_nz));
	new_f1a_u = _mm_and_si128(new_f1a_u, vec_flagmask);
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 0) << 0;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 1) << 1;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 2) << 2;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 3) << 3;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 4) << 4;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 5) << 5;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 6) << 6;
	flag1_unset |= _mm_extract_epi16(new_f1a_u, 7) << 7;

	rsp->flag[1] &= ~flag1_unset;
	rsp->flag[1] |= flag1_set;

	// accum set to  s1 if  flag0[0-7] &&  flag0[8-15] && !flag1[0-7]
	// accum set to  s1 if  flag0[0-7] && !flag0[8-15] &&  flag2[0-7] && (s1 + s2) <= 0x10000
	// accum set to  s1 if  flag0[0-7] && !flag0[8-15] && !flag2[0-7] && (s1 + s2) != 0
	// accum set to  s1 if !flag0[0-7] &&  flag0[8-15] && !flag1[8-15]
	// accum set to  s1 if !flag0[0-7] && !flag0[8-15] && (s1 - s2) < 0
	__m128i accum_s1 = _mm_and_si128(flag0_07, _mm_and_si128(flag0_815, n1_07));
	accum_s1 = _mm_or_si128(accum_s1, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, flag2_07), sum_le0x10000));
	accum_s1 = _mm_or_si128(accum_s1, _mm_and_si128(_mm_and_si128(f0a_and_nf0b, n2_07), sum_nz));
	accum_s1 = _mm_or_si128(accum_s1, _mm_and_si128(_mm_and_si128(n0_07, flag0_815), n1_815));
	accum_s1 = _mm_or_si128(accum_s1, _mm_and_si128(nf0a_and_nf0b, sub_lz));
	//__m128i zms2 = _mm_sub_epi16(_mm_setzero_si128(), shuf2);

	/*
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i s1_xor_s2 = _mm_xor_si128(rsp->xv[VS1REG], shuf);
	__m128i s1_plus_s2 = _mm_add_epi16(rsp->xv[VS1REG], shuf);
	__m128i s1_sub_s2 = _mm_sub_epi16(rsp->xv[VS1REG], shuf);
	__m128i s2_neg = _mm_xor_si128(shuf, vec_neg1);

	__m128i s2_lz = _mm_cmplt_epi16(shuf, _mm_setzero_si128());
	__m128i s1s2_xor_lz = _mm_cmplt_epi16(s1_xor_s2, _mm_setzero_si128());
	__m128i s1s2_xor_gez = _mm_xor_si128(s1s2_xor_lz, vec_neg1);
	__m128i s1s2_plus_nz = _mm_xor_si128(_mm_cmpeq_epi16(s1_plus_s2, _mm_setzero_si128()), vec_neg1);
	__m128i s1s2_plus_gz = _mm_cmpgt_epi16(s1_plus_s2, _mm_setzero_si128());
	__m128i s1s2_plus_lez = _mm_xor_si128(s1s2_plus_gz, vec_neg1);
	__m128i s1s2_plus_n1 = _mm_cmpeq_epi16(s1_plus_s2, vec_neg1);
	__m128i s1s2_sub_nz = _mm_xor_si128(_mm_cmpeq_epi16(s1_sub_s2, _mm_setzero_si128()), vec_neg1);
	__m128i s1s2_sub_lz = _mm_cmplt_epi16(s1_sub_s2, _mm_setzero_si128());
	__m128i s1s2_sub_gez = _mm_xor_si128(s1s2_sub_lz, vec_neg1);
	__m128i s1_nens2 = _mm_xor_si128(_mm_cmpeq_epi16(rsp->xv[VS1REG], s2_neg), vec_neg1);

	__m128i ext_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_lz, s1s2_plus_n1), vec_flagmask);
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 0) << 0;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 1) << 1;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 2) << 2;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 3) << 3;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 4) << 4;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 5) << 5;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 6) << 6;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 7) << 7;

	__m128i carry_mask = _mm_and_si128(s1s2_xor_lz, vec_flagmask);
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 0) << 0;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 1) << 1;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 2) << 2;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 3) << 3;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 4) << 4;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 5) << 5;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 6) << 6;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 7) << 7;

	__m128i z0_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_nz), s1_nens2);
	__m128i z1_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_lz, s1s2_plus_nz), s1_nens2);
	__m128i z_mask = _mm_and_si128(_mm_or_si128(z0_mask, z1_mask), vec_flagmask);
	z_mask = _mm_and_si128(_mm_or_si128(z_mask, _mm_srli_epi32(z_mask, 15)), vec_shiftmask2);
	z_mask = _mm_and_si128(_mm_or_si128(z_mask, _mm_srli_epi64(z_mask, 30)), vec_shiftmask4);
	z_mask = _mm_or_si128(z_mask, _mm_srli_si128(z_mask, 7));
	z_mask = _mm_or_si128(z_mask, _mm_srli_epi16(z_mask, 4));
	rsp->flag[0] |= (_mm_extract_epi16(z_mask, 0) << 8) & 0x00ff00;

	__m128i f0_mask = _mm_and_si128(_mm_or_si128(_mm_and_si128(s1s2_xor_gez, s2_lz),         _mm_and_si128(s1s2_xor_lz, s1s2_plus_lez)), vec_flagmask);
	__m128i f8_mask = _mm_and_si128(_mm_or_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_gez),  _mm_and_si128(s1s2_xor_lz, s2_lz)), vec_flagmask);
	f0_mask = _mm_and_si128(f0_mask, vec_flagmask);
	f8_mask = _mm_and_si128(f8_mask, vec_flagmask);
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 0) << 0;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 1) << 1;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 2) << 2;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 3) << 3;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 4) << 4;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 5) << 5;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 6) << 6;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 7) << 7;

	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 0) << 8;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 1) << 9;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 2) << 10;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 3) << 11;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 4) << 12;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 5) << 13;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 6) << 14;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 7) << 15;*/
#else
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if (CARRY_FLAG(rsp, i) != 0)
		{
			if (ZERO_FLAG(rsp, i) != 0)
			{
				if (COMPARE_FLAG(rsp, i) != 0)
				{
					SET_ACCUM_L(-(UINT16)s2, i);
				}
				else
				{
					SET_ACCUM_L(s1, i);
				}
			}
			else//ZERO_FLAG(rsp, i)==0
			{
				if (CLIP1_FLAG(rsp, i) != 0)
				{
					if (((UINT32)(UINT16)(s1) + (UINT32)(UINT16)(s2)) > 0x10000)
					{//proper fix for Harvest Moon 64, r4
						SET_ACCUM_L(s1, i);
						CLEAR_COMPARE_FLAG(i);
					}
					else
					{
						SET_ACCUM_L(-((UINT16)s2), i);
						SET_COMPARE_FLAG(i);
					}
				}
				else
				{
					if (((UINT32)(UINT16)(s1) + (UINT32)(UINT16)(s2)) != 0)
					{
						SET_ACCUM_L(s1, i);
						CLEAR_COMPARE_FLAG(i);
					}
					else
					{
						SET_ACCUM_L(-((UINT16)s2), i);
						SET_COMPARE_FLAG(i);
					}
				}
			}
		}
		else//CARRY_FLAG(rsp, i)==0
		{
			if (ZERO_FLAG(rsp, i) != 0)
			{
				if (CLIP2_FLAG(rsp, i) != 0)
				{
					SET_ACCUM_L(s2, i);
				}
				else
				{
					SET_ACCUM_L(s1, i);
				}
			}
			else
			{
				if (((INT32)(UINT16)s1 - (INT32)(UINT16)s2) >= 0)
				{
					SET_ACCUM_L(s2, i);
					SET_CLIP2_FLAG(i);
				}
				else
				{
					SET_ACCUM_L(s1, i);
					CLEAR_CLIP2_FLAG(i);
				}
			}
		}
		vres[i] = ACCUM_L(rsp, i);
	}
	CLEAR_ZERO_FLAGS();
	CLEAR_CARRY_FLAGS();
	CLEAR_CLIP1_FLAGS();
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vch(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100101 |
	// ------------------------------------------------------
	//
	// Vector clip high

	CLEAR_CARRY_FLAGS();
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP1_FLAGS();
	CLEAR_ZERO_FLAGS();
	CLEAR_CLIP2_FLAGS();

#if 0//USE_SIMD
	// Compare flag
	// flag[1] bit [0- 7] set if (s1 ^ s2) < 0 && (s1 + s2) <= 0)
	// flag[1] bit [0- 7] set if (s1 ^ s2) >= 0 && (s2 < 0)

	// flag[1] bit [8-15] set if (s1 ^ s2) < 0 && (s2 < 0)
	// flag[1] bit [8-15] set if (s1 ^ s2) >= 0 && (s1 - s2) >= 0

	// Carry flag
	// flag[0] bit [0- 7] set if (s1 ^ s2) < 0

	// Zero flag
	// flag[0] bit [8-15] set if (s1 ^ s2) < 0  && (s1 + s2) != 0 && (s1 != ~s2)
	// flag[0] bit [8-15] set if (s1 ^ s2) >= 0 && (s1 - s2) != 0 && (s1 != ~s2)

	// flag[2] bit [0- 7] set if (s1 ^ s2) < 0 && (s1 + s2) == -1

	// accum set to -s2 if (s1 ^ s2) < 0 && (s1 + s2) <= 0)
	// accum set to -s2 if (s1 ^ s2) >= 0 && (s1 - s2) >= 0

	// accum set to s1 if (s1 ^ s2) < 0 && (s1 + s2) > 0)
	// accum set to s1 if (s1 ^ s2) >= 0 && (s1 - s2) < 0

	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i s1_xor_s2 = _mm_xor_si128(rsp->xv[VS1REG], shuf);
	__m128i s1_plus_s2 = _mm_add_epi16(rsp->xv[VS1REG], shuf);
	__m128i s1_sub_s2 = _mm_sub_epi16(rsp->xv[VS1REG], shuf);
	__m128i s2_neg = _mm_xor_si128(shuf, vec_neg1);

	__m128i s2_lz = _mm_cmplt_epi16(shuf, _mm_setzero_si128());
	__m128i s1s2_xor_lz = _mm_cmplt_epi16(s1_xor_s2, _mm_setzero_si128());
	__m128i s1s2_xor_gez = _mm_xor_si128(s1s2_xor_lz, vec_neg1);
	__m128i s1s2_plus_nz = _mm_xor_si128(_mm_cmpeq_epi16(s1_plus_s2, _mm_setzero_si128()), vec_neg1);
	__m128i s1s2_plus_gz = _mm_cmpgt_epi16(s1_plus_s2, _mm_setzero_si128());
	__m128i s1s2_plus_lez = _mm_xor_si128(s1s2_plus_gz, vec_neg1);
	__m128i s1s2_plus_n1 = _mm_cmpeq_epi16(s1_plus_s2, vec_neg1);
	__m128i s1s2_sub_nz = _mm_xor_si128(_mm_cmpeq_epi16(s1_sub_s2, _mm_setzero_si128()), vec_neg1);
	__m128i s1s2_sub_lz = _mm_cmplt_epi16(s1_sub_s2, _mm_setzero_si128());
	__m128i s1s2_sub_gez = _mm_xor_si128(s1s2_sub_lz, vec_neg1);
	__m128i s1_nens2 = _mm_xor_si128(_mm_cmpeq_epi16(rsp->xv[VS1REG], s2_neg), vec_neg1);

	__m128i ext_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_lz, s1s2_plus_n1), vec_flagmask);
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 0) << 0;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 1) << 1;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 2) << 2;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 3) << 3;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 4) << 4;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 5) << 5;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 6) << 6;
	rsp->flag[2] |= _mm_extract_epi16(ext_mask, 7) << 7;

	__m128i carry_mask = _mm_and_si128(s1s2_xor_lz, vec_flagmask);
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 0) << 0;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 1) << 1;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 2) << 2;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 3) << 3;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 4) << 4;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 5) << 5;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 6) << 6;
	rsp->flag[0] |= _mm_extract_epi16(carry_mask, 7) << 7;

	__m128i z0_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_nz), s1_nens2);
	__m128i z1_mask = _mm_and_si128(_mm_and_si128(s1s2_xor_lz, s1s2_plus_nz), s1_nens2);
	__m128i z_mask = _mm_and_si128(_mm_or_si128(z0_mask, z1_mask), vec_flagmask);
	z_mask = _mm_and_si128(_mm_or_si128(z_mask, _mm_srli_epi32(z_mask, 15)), vec_shiftmask2);
	z_mask = _mm_and_si128(_mm_or_si128(z_mask, _mm_srli_epi64(z_mask, 30)), vec_shiftmask4);
	z_mask = _mm_or_si128(z_mask, _mm_srli_si128(z_mask, 7));
	z_mask = _mm_or_si128(z_mask, _mm_srli_epi16(z_mask, 4));
	rsp->flag[0] |= (_mm_extract_epi16(z_mask, 0) << 8) & 0x00ff00;

	__m128i f0_mask = _mm_and_si128(_mm_or_si128(_mm_and_si128(s1s2_xor_gez, s2_lz),         _mm_and_si128(s1s2_xor_lz, s1s2_plus_lez)), vec_flagmask);
	__m128i f8_mask = _mm_and_si128(_mm_or_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_gez),  _mm_and_si128(s1s2_xor_lz, s2_lz)), vec_flagmask);
	f0_mask = _mm_and_si128(f0_mask, vec_flagmask);
	f8_mask = _mm_and_si128(f8_mask, vec_flagmask);
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 0) << 0;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 1) << 1;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 2) << 2;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 3) << 3;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 4) << 4;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 5) << 5;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 6) << 6;
	rsp->flag[1] |= _mm_extract_epi16(f0_mask, 7) << 7;

	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 0) << 8;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 1) << 9;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 2) << 10;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 3) << 11;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 4) << 12;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 5) << 13;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 6) << 14;
	rsp->flag[1] |= _mm_extract_epi16(f8_mask, 7) << 15;
#else

	INT16 vres[8];
	UINT32 vce = 0;
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if ((s1 ^ s2) < 0)
		{
			vce = (s1 + s2 == -1);
			SET_CARRY_FLAG(i);
			if (s2 < 0)
			{
				SET_CLIP2_FLAG(i);
			}

			if ((s1 + s2) <= 0)
			{
				SET_COMPARE_FLAG(i);
				vres[i] = -((UINT16)s2);
			}
			else
			{
				vres[i] = s1;
			}

			if ((s1 + s2) != 0 && s1 != ~s2)
			{
				SET_ZERO_FLAG(i);
			}
		}//sign
		else
		{
			vce = 0;
			if (s2 < 0)
			{
				SET_COMPARE_FLAG(i);
			}
			if ((s1 - s2) >= 0)
			{
				SET_CLIP2_FLAG(i);
				vres[i] = s2;
			}
			else
			{
				vres[i] = s1;
			}

			if ((s1 - s2) != 0 && s1 != ~s2)
			{
				SET_ZERO_FLAG(i);
			}
		}
		if (vce)
		{
			SET_CLIP1_FLAG(i);
		}
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vcr(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100110 |
	// ------------------------------------------------------
	//
	// Vector clip reverse

	CLEAR_CARRY_FLAGS();
	CLEAR_COMPARE_FLAGS();
	CLEAR_CLIP1_FLAGS();
	CLEAR_ZERO_FLAGS();
	CLEAR_CLIP2_FLAGS();

#if 0//USE_SIMD
	// flag[1] bit [0- 7] set if (s1 ^ s2) < 0 && (s1 + s2) <= 0)
	// flag[1] bit [0- 7] set if (s1 ^ s2) >= 0 && (s2 < 0)

	// flag[1] bit [8-15] set if (s1 ^ s2) < 0 && (s2 < 0)
	// flag[1] bit [8-15] set if (s1 ^ s2) >= 0 && (s1 - s2) >= 0

	// accum set to ~s2 if (s1 ^ s2) < 0 && (s1 + s2) <= 0)
	// accum set to ~s2 if (s1 ^ s2) >= 0 && (s1 - s2) >= 0

	// accum set to s1 if (s1 ^ s2) < 0 && (s1 + s2) > 0)
	// accum set to s1 if (s1 ^ s2) >= 0 && (s1 - s2) < 0
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i s1_xor_s2 = _mm_xor_si128(rsp->xv[VS1REG], shuf);
	__m128i s1_plus_s2 = _mm_add_epi16(rsp->xv[VS1REG], shuf);
	__m128i s1_sub_s2 = _mm_sub_epi16(rsp->xv[VS1REG], shuf);
	__m128i s2_neg = _mm_xor_si128(shuf, vec_neg1);

	__m128i s2_lz = _mm_cmplt_epi16(shuf, _mm_setzero_si128());
	__m128i s1s2_xor_lz = _mm_cmplt_epi16(s1_xor_s2, _mm_setzero_si128());
	__m128i s1s2_xor_gez = _mm_xor_si128(s1s2_xor_lz, vec_neg1);
	__m128i s1s2_plus_gz = _mm_cmpgt_epi16(s1_plus_s2, _mm_setzero_si128());
	__m128i s1s2_plus_lez = _mm_xor_si128(s1s2_plus_gz, vec_neg1);
	__m128i s1s2_sub_lz = _mm_cmplt_epi16(s1_sub_s2, _mm_setzero_si128());
	__m128i s1s2_sub_gez = _mm_xor_si128(s1s2_sub_lz, vec_neg1);

	__m128i s1_mask = _mm_or_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_lz),   _mm_and_si128(s1s2_xor_lz, s1s2_plus_gz));
	__m128i s2_mask = _mm_or_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_gez),  _mm_and_si128(s1s2_xor_lz, s1s2_plus_lez));
	rsp->accum_l = _mm_or_si128(_mm_and_si128(rsp->xv[VS1REG], s1_mask), _mm_and_si128(s2_neg, s2_mask));
	rsp->xv[VDREG] = rsp->accum_l;

	rsp->xvflag[COMPARE] = _mm_or_si128(_mm_and_si128(s1s2_xor_gez, s2_lz),         _mm_and_si128(s1s2_xor_lz, s1s2_plus_lez));
	rsp->xvflag[CLIP2] = _mm_or_si128(_mm_and_si128(s1s2_xor_gez, s1s2_sub_gez),  _mm_and_si128(s1s2_xor_lz, s2_lz));
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);

		if ((INT16)(s1 ^ s2) < 0)
		{
			if (s2 < 0)
			{
				SET_CLIP2_FLAG(i);
			}
			if ((s1 + s2) <= 0)
			{
				SET_ACCUM_L(~((UINT16)s2), i);
				SET_COMPARE_FLAG(i);
			}
			else
			{
				SET_ACCUM_L(s1, i);
			}
		}
		else
		{
			if (s2 < 0)
			{
				SET_COMPARE_FLAG(i);
			}
			if ((s1 - s2) >= 0)
			{
				SET_ACCUM_L(s2, i);
				SET_CLIP2_FLAG(i);
			}
			else
			{
				SET_ACCUM_L(s1, i);
			}
		}

		vres[i] = ACCUM_L(rsp, i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vmrg(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 100111 |
	// ------------------------------------------------------
	//
	// Merges two vectors according to compare flags

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	__m128i s2mask = _mm_cmpeq_epi16(rsp->xvflag[COMPARE], _mm_setzero_si128());
	__m128i s1mask = _mm_xor_si128(s2mask, vec_neg1);
	__m128i result = _mm_and_si128(rsp->xv[VS1REG], s1mask);
	rsp->xv[VDREG] = _mm_or_si128(result, _mm_and_si128(shuf, s2mask));
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		INT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		if (COMPARE_FLAG(rsp, i) != 0)
		{
			vres[i] = s1;
		}
		else
		{
			vres[i] = s2;
		}

		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vand(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101000 |
	// ------------------------------------------------------
	//
	// Bitwise AND of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_and_si128(rsp->xv[VS1REG], shuf);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = s1 & s2;
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vnand(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101001 |
	// ------------------------------------------------------
	//
	// Bitwise NOT AND of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_xor_si128(_mm_and_si128(rsp->xv[VS1REG], shuf), vec_neg1);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = ~((s1 & s2));
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vor(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101010 |
	// ------------------------------------------------------
	//
	// Bitwise OR of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_or_si128(rsp->xv[VS1REG], shuf);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = s1 | s2;
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vnor(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101011 |
	// ------------------------------------------------------
	//
	// Bitwise NOT OR of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_xor_si128(_mm_or_si128(rsp->xv[VS1REG], shuf), vec_neg1);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = ~(s1 | s2);
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vxor(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101100 |
	// ------------------------------------------------------
	//
	// Bitwise XOR of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_xor_si128(rsp->xv[VS1REG], shuf);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = s1 ^ s2;
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vnxor(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | TTTTT | DDDDD | 101101 |
	// ------------------------------------------------------
	//
	// Bitwise NOT XOR of two vector registers

#if USE_SIMD
	__m128i shuf = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
	rsp->xv[VDREG] = _mm_xor_si128(_mm_xor_si128(rsp->xv[VS1REG], shuf), vec_neg1);
	rsp->accum_l = rsp->xv[VDREG];
#else
	INT16 vres[8];
	for (int i = 0; i < 8; i++)
	{
		UINT16 s1, s2;
		SCALAR_GET_VS1(s1, i);
		SCALAR_GET_VS2(s2, i);
		vres[i] = ~(s1 ^ s2);
		SET_ACCUM_L(vres[i], i);
	}
	WRITEBACK_RESULT();
#endif
}

INLINE void cfunc_rsp_vrcp(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110000 |
	// ------------------------------------------------------
	//
	// Calculates reciprocal

	INT32 shifter = 0;
#if USE_SIMD
	UINT16 urec;
	INT32 rec;
	SIMD_EXTRACT16(rsp->xv[VS2REG], urec, EL);
	rec = (INT16)urec;
#else
	INT32 rec = (INT16)(VREG_S(VS2REG, EL & 7));
#endif
	INT32 datainput = (rec < 0) ? (-rec) : rec;
	if (datainput)
	{
		for (int i = 0; i < 32; i++)
		{
			if (datainput & (1 << ((~i) & 0x1f)))
			{
				shifter = i;
				break;
			}
		}
	}
	else
	{
		shifter = 0x10;
	}

	INT32 address = ((datainput << shifter) & 0x7fc00000) >> 22;
	INT32 fetchval = rsp_divtable[address];
	INT32 temp = (0x40000000 | (fetchval << 14)) >> ((~shifter) & 0x1f);
	if (rec < 0)
	{
		temp = ~temp;
	}
	if (!rec)
	{
		temp = 0x7fffffff;
	}
	else if (rec == 0xffff8000)
	{
		temp = 0xffff0000;
	}
	rec = temp;

	rsp->reciprocal_res = rec;
	rsp->dp_allowed = 0;

#if USE_SIMD
	SIMD_INSERT16(rsp->xv[VDREG], (UINT16)rec, VS1REG);
	rsp->accum_l = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
#else
	W_VREG_S(VDREG, VS1REG & 7) = (UINT16)rec;
	for (int i = 0; i < 8; i++)
	{
		SET_ACCUM_L(VREG_S(VS2REG, VEC_EL_2(EL, i)), i);
	}
#endif
}

INLINE void cfunc_rsp_vrcpl(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110001 |
	// ------------------------------------------------------
	//
	// Calculates reciprocal low part

	INT32 shifter = 0;

#if USE_SIMD
	UINT16 urec;
	SIMD_EXTRACT16(rsp->xv[VS2REG], urec, EL);
	INT32 rec = (urec | rsp->reciprocal_high);
#else
	INT32 rec = ((UINT16)(VREG_S(VS2REG, EL & 7)) | rsp->reciprocal_high);
#endif

	INT32 datainput = rec;

	if (rec < 0)
	{
		if (rsp->dp_allowed)
		{
			if (rec < -32768)
			{
				datainput = ~datainput;
			}
			else
			{
				datainput = -datainput;
			}
		}
		else
		{
			datainput = -datainput;
		}
	}


	if (datainput)
	{
		for (int i = 0; i < 32; i++)
		{
			if (datainput & (1 << ((~i) & 0x1f)))
			{
				shifter = i;
				break;
			}
		}
	}
	else
	{
		if (rsp->dp_allowed)
		{
			shifter = 0;
		}
		else
		{
			shifter = 0x10;
		}
	}

	INT32 address = ((datainput << shifter) & 0x7fc00000) >> 22;
	INT32 fetchval = rsp_divtable[address];
	INT32 temp = (0x40000000 | (fetchval << 14)) >> ((~shifter) & 0x1f);
	if (rec < 0)
	{
		temp = ~temp;
	}
	if (!rec)
	{
		temp = 0x7fffffff;
	}
	else if (rec == 0xffff8000)
	{
		temp = 0xffff0000;
	}
	rec = temp;

	rsp->reciprocal_res = rec;
	rsp->dp_allowed = 0;

#if USE_SIMD
	SIMD_INSERT16(rsp->xv[VDREG], (UINT16)rec, VS1REG);
#else
	W_VREG_S(VDREG, VS1REG & 7) = (UINT16)rec;
#endif

	for (int i = 0; i < 8; i++)
	{
#if USE_SIMD
		INT16 val;
		SIMD_EXTRACT16(rsp->xv[VS2REG], val, VEC_EL_2(EL, i));
#else
		INT16 val = VREG_S(VS2REG, VEC_EL_2(EL, i));
#endif
		SET_ACCUM_L(val, i);
	}
}

INLINE void cfunc_rsp_vrcph(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110010 |
	// ------------------------------------------------------
	//
	// Calculates reciprocal high part

#if USE_SIMD
	UINT16 rcph;
	SIMD_EXTRACT16(rsp->xv[VS2REG], rcph, EL);
	rsp->reciprocal_high = rcph << 16;
	rsp->dp_allowed = 1;

	rsp->accum_l = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	SIMD_INSERT16(rsp->xv[VDREG], (INT16)(rsp->reciprocal_res >> 16), VS1REG);
#else
	rsp->reciprocal_high = (VREG_S(VS2REG, EL & 7)) << 16;
	rsp->dp_allowed = 1;

	for (int i = 0; i < 8; i++)
	{
		SET_ACCUM_L(VREG_S(VS2REG, VEC_EL_2(EL, i)), i);
	}

	W_VREG_S(VDREG, VS1REG & 7) = (INT16)(rsp->reciprocal_res >> 16);
#endif
}

INLINE void cfunc_rsp_vmov(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110011 |
	// ------------------------------------------------------
	//
	// Moves element from vector to destination vector

#if USE_SIMD
	INT16 val;
	SIMD_EXTRACT16(rsp->xv[VS2REG], val, EL);
	SIMD_INSERT16(rsp->xv[VDREG], val, VS1REG);
	rsp->accum_l = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
#else
	W_VREG_S(VDREG, VS1REG & 7) = VREG_S(VS2REG, EL & 7);
	for (int i = 0; i < 8; i++)
	{
		SET_ACCUM_L(VREG_S(VS2REG, VEC_EL_2(EL, i)), i);
	}
#endif
}

INLINE void cfunc_rsp_vrsql(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110101 |
	// ------------------------------------------------------
	//
	// Calculates reciprocal square-root low part

	INT32 shifter = 0;
#if USE_SIMD
	UINT16 val;
	SIMD_EXTRACT16(rsp->xv[VS2REG], val, EL);
	INT32 rec = rsp->reciprocal_high | val;
#else
	INT32 rec = rsp->reciprocal_high | (UINT16)VREG_S(VS2REG, EL & 7);
#endif
	INT32 datainput = rec;

	if (rec < 0)
	{
		if (rsp->dp_allowed)
		{
			if (rec < -32768)
			{
				datainput = ~datainput;
			}
			else
			{
				datainput = -datainput;
			}
		}
		else
		{
			datainput = -datainput;
		}
	}

	if (datainput)
	{
		for (int i = 0; i < 32; i++)
		{
			if (datainput & (1 << ((~i) & 0x1f)))
			{
				shifter = i;
				break;
			}
		}
	}
	else
	{
		if (rsp->dp_allowed)
		{
			shifter = 0;
		}
		else
		{
			shifter = 0x10;
		}
	}

	INT32 address = ((datainput << shifter) & 0x7fc00000) >> 22;
	address = ((address | 0x200) & 0x3fe) | (shifter & 1);

	INT32 fetchval = rsp_divtable[address];
	INT32 temp = (0x40000000 | (fetchval << 14)) >> (((~shifter) & 0x1f) >> 1);
	if (rec < 0)
	{
		temp = ~temp;
	}
	if (!rec)
	{
		temp = 0x7fffffff;
	}
	else if (rec == 0xffff8000)
	{
		temp = 0xffff0000;
	}
	rec = temp;

	rsp->reciprocal_res = rec;
	rsp->dp_allowed = 0;

#if USE_SIMD
	SIMD_INSERT16(rsp->xv[VDREG], (UINT16)rec, VS1REG);
	rsp->accum_l = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);
#else
	W_VREG_S(VDREG, VS1REG & 7) = (UINT16)(rec & 0xffff);
	for (int i = 0; i < 8; i++)
	{
		SET_ACCUM_L(VREG_S(VS2REG, VEC_EL_2(EL, i)), i);
	}
#endif
}

INLINE void cfunc_rsp_vrsqh(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	int op = rsp->impstate->arg0;

	// 31       25  24     20      15      10      5        0
	// ------------------------------------------------------
	// | 010010 | 1 | EEEE | SSSSS | ?FFFF | DDDDD | 110110 |
	// ------------------------------------------------------
	//
	// Calculates reciprocal square-root high part

#if USE_SIMD
	UINT16 val;
	SIMD_EXTRACT16(rsp->xv[VS2REG], val, EL);
	rsp->reciprocal_high = val << 16;
	rsp->dp_allowed = 1;

	rsp->accum_l = _mm_shuffle_epi8(rsp->xv[VS2REG], vec_shuf_inverse[EL]);

	SIMD_INSERT16(rsp->xv[VDREG], (INT16)(rsp->reciprocal_res >> 16), VS1REG); // store high part
#else
	rsp->reciprocal_high = (VREG_S(VS2REG, EL & 7)) << 16;
	rsp->dp_allowed = 1;

	for (int i = 0; i < 8; i++)
	{
		SET_ACCUM_L(VREG_S(VS2REG, VEC_EL_2(EL, i)), i);
	}

	W_VREG_S(VDREG, VS1REG & 7) = (INT16)(rsp->reciprocal_res >> 16);  // store high part
#endif
}

static void cfunc_sp_set_status_cb(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	(rsp->sp_set_status_func)(0, rsp->impstate->arg0);
}

static CPU_EXECUTE( rsp )
{
	rsp_state *rsp = get_safe_token(device);
	drcuml_state *drcuml = rsp->impstate->drcuml;
	int execute_result;

	/* reset the cache if dirty */
	if (rsp->impstate->cache_dirty)
		code_flush_cache(rsp);
	rsp->impstate->cache_dirty = FALSE;

	/* execute */
	do
	{
		if( rsp->sr & ( RSP_STATUS_HALT | RSP_STATUS_BROKE ) )
		{
			rsp->icount = MIN(rsp->icount, 0);
			break;
		}

		/* run as much as we can */
		execute_result = drcuml->execute(*rsp->impstate->entry);

		/* if we need to recompile, do it */
		if (execute_result == EXECUTE_MISSING_CODE)
		{
			code_compile_block(rsp, rsp->pc);
		}
		else if (execute_result == EXECUTE_UNMAPPED_CODE)
		{
			fatalerror("Attempted to execute unmapped code at PC=%08X\n", rsp->pc);
		}
		else if (execute_result == EXECUTE_RESET_CACHE)
		{
			code_flush_cache(rsp);
		}
	} while (execute_result != EXECUTE_OUT_OF_CYCLES);
}

/***************************************************************************
    CACHE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    rspdrc_flush_drc_cache - outward-facing
    accessor to code_flush_cache
-------------------------------------------------*/

void rspdrc_flush_drc_cache(device_t *device)
{
	if (!device->machine().options().drc()) return;
	rsp_state *rsp = get_safe_token(device);
	rsp->impstate->cache_dirty = TRUE;
}

/*-------------------------------------------------
    code_flush_cache - flush the cache and
    regenerate static code
-------------------------------------------------*/

static void code_flush_cache(rsp_state *rsp)
{
	/* empty the transient cache contents */
	rsp->impstate->drcuml->reset();

	try
	{
		/* generate the entry point and out-of-cycles handlers */
		static_generate_entry_point(rsp);
		static_generate_nocode_handler(rsp);
		static_generate_out_of_cycles(rsp);

		/* add subroutines for memory accesses */
		static_generate_memory_accessor(rsp, 1, FALSE, "read8",       rsp->impstate->read8);
		static_generate_memory_accessor(rsp, 1, TRUE,  "write8",      rsp->impstate->write8);
		static_generate_memory_accessor(rsp, 2, FALSE, "read16",      rsp->impstate->read16);
		static_generate_memory_accessor(rsp, 2, TRUE,  "write16",     rsp->impstate->write16);
		static_generate_memory_accessor(rsp, 4, FALSE, "read32",      rsp->impstate->read32);
		static_generate_memory_accessor(rsp, 4, TRUE,  "write32",     rsp->impstate->write32);
	}
	catch (drcuml_block::abort_compilation &)
	{
		fatalerror("Unable to generate static RSP code\n");
	}
}


/*-------------------------------------------------
    code_compile_block - compile a block of the
    given mode at the specified pc
-------------------------------------------------*/

static void code_compile_block(rsp_state *rsp, offs_t pc)
{
	drcuml_state *drcuml = rsp->impstate->drcuml;
	compiler_state compiler = { 0 };
	const opcode_desc *seqhead, *seqlast;
	const opcode_desc *desclist;
	int override = FALSE;
	drcuml_block *block;

	g_profiler.start(PROFILER_DRC_COMPILE);

	/* get a description of this sequence */
	desclist = rsp->impstate->drcfe->describe_code(pc);

	bool succeeded = false;
	while (!succeeded)
	{
		try
		{
			/* start the block */
			block = drcuml->begin_block(4096);

			/* loop until we get through all instruction sequences */
			for (seqhead = desclist; seqhead != NULL; seqhead = seqlast->next())
			{
				const opcode_desc *curdesc;
				UINT32 nextpc;

				/* add a code log entry */
				if (LOG_UML)
					block->append_comment("-------------------------");                 // comment

				/* determine the last instruction in this sequence */
				for (seqlast = seqhead; seqlast != NULL; seqlast = seqlast->next())
					if (seqlast->flags & OPFLAG_END_SEQUENCE)
						break;
				assert(seqlast != NULL);

				/* if we don't have a hash for this mode/pc, or if we are overriding all, add one */
				if (override || !drcuml->hash_exists(0, seqhead->pc))
					UML_HASH(block, 0, seqhead->pc);                                        // hash    mode,pc

				/* if we already have a hash, and this is the first sequence, assume that we */
				/* are recompiling due to being out of sync and allow future overrides */
				else if (seqhead == desclist)
				{
					override = TRUE;
					UML_HASH(block, 0, seqhead->pc);                                        // hash    mode,pc
				}

				/* otherwise, redispatch to that fixed PC and skip the rest of the processing */
				else
				{
					UML_LABEL(block, seqhead->pc | 0x80000000);                             // label   seqhead->pc
					UML_HASHJMP(block, 0, seqhead->pc, *rsp->impstate->nocode);
																							// hashjmp <0>,seqhead->pc,nocode
					continue;
				}

				/* validate this code block if we're not pointing into ROM */
				if (rsp->program->get_write_ptr(seqhead->physpc) != NULL)
					generate_checksum_block(rsp, block, &compiler, seqhead, seqlast);

				/* label this instruction, if it may be jumped to locally */
				if (seqhead->flags & OPFLAG_IS_BRANCH_TARGET)
					UML_LABEL(block, seqhead->pc | 0x80000000);                             // label   seqhead->pc

				/* iterate over instructions in the sequence and compile them */
				for (curdesc = seqhead; curdesc != seqlast->next(); curdesc = curdesc->next())
					generate_sequence_instruction(rsp, block, &compiler, curdesc);

				/* if we need to return to the start, do it */
				if (seqlast->flags & OPFLAG_RETURN_TO_START)
					nextpc = pc;

				/* otherwise we just go to the next instruction */
				else
					nextpc = seqlast->pc + (seqlast->skipslots + 1) * 4;

				/* count off cycles and go there */
				generate_update_cycles(rsp, block, &compiler, nextpc, TRUE);            // <subtract cycles>

				/* if the last instruction can change modes, use a variable mode; otherwise, assume the same mode */
				if (seqlast->next() == NULL || seqlast->next()->pc != nextpc)
					UML_HASHJMP(block, 0, nextpc, *rsp->impstate->nocode);          // hashjmp <mode>,nextpc,nocode
			}

			/* end the sequence */
			block->end();
			g_profiler.stop();
			succeeded = true;
		}
		catch (drcuml_block::abort_compilation &)
		{
			code_flush_cache(rsp);
		}
	}
}

/***************************************************************************
    C FUNCTION CALLBACKS
***************************************************************************/

/*-------------------------------------------------
    cfunc_unimplemented - handler for
    unimplemented opcdes
-------------------------------------------------*/

static void cfunc_unimplemented(void *param)
{
	rsp_state *rsp = (rsp_state *)param;
	UINT32 opcode = rsp->impstate->arg0;
	fatalerror("PC=%08X: Unimplemented op %08X (%02X,%02X)\n", rsp->pc, opcode, opcode >> 26, opcode & 0x3f);
}


/*-------------------------------------------------
    cfunc_fatalerror - a generic fatalerror call
-------------------------------------------------*/

#ifdef UNUSED_CODE
static void cfunc_fatalerror(void *param)
{
	fatalerror("fatalerror\n");
}
#endif


/***************************************************************************
    STATIC CODEGEN
***************************************************************************/

/*-------------------------------------------------
    ferate_entry_point - generate a
    static entry point
-------------------------------------------------*/

static void static_generate_entry_point(rsp_state *rsp)
{
	drcuml_state *drcuml = rsp->impstate->drcuml;
	drcuml_block *block;

	/* begin generating */
	block = drcuml->begin_block(20);

	/* forward references */
	alloc_handle(drcuml, &rsp->impstate->nocode, "nocode");

	alloc_handle(drcuml, &rsp->impstate->entry, "entry");
	UML_HANDLE(block, *rsp->impstate->entry);                                       // handle  entry

	/* load fast integer registers */
	load_fast_iregs(rsp, block);

	/* generate a hash jump via the current mode and PC */
	UML_HASHJMP(block, 0, mem(&rsp->pc), *rsp->impstate->nocode);                   // hashjmp <mode>,<pc>,nocode
	block->end();
}


/*-------------------------------------------------
    static_generate_nocode_handler - generate an
    exception handler for "out of code"
-------------------------------------------------*/

static void static_generate_nocode_handler(rsp_state *rsp)
{
	drcuml_state *drcuml = rsp->impstate->drcuml;
	drcuml_block *block;

	/* begin generating */
	block = drcuml->begin_block(10);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(drcuml, &rsp->impstate->nocode, "nocode");
	UML_HANDLE(block, *rsp->impstate->nocode);                                      // handle  nocode
	UML_GETEXP(block, I0);                                                      // getexp  i0
	UML_MOV(block, mem(&rsp->pc), I0);                                          // mov     [pc],i0
	save_fast_iregs(rsp, block);
	UML_EXIT(block, EXECUTE_MISSING_CODE);                                      // exit    EXECUTE_MISSING_CODE

	block->end();
}


/*-------------------------------------------------
    static_generate_out_of_cycles - generate an
    out of cycles exception handler
-------------------------------------------------*/

static void static_generate_out_of_cycles(rsp_state *rsp)
{
	drcuml_state *drcuml = rsp->impstate->drcuml;
	drcuml_block *block;

	/* begin generating */
	block = drcuml->begin_block(10);

	/* generate a hash jump via the current mode and PC */
	alloc_handle(drcuml, &rsp->impstate->out_of_cycles, "out_of_cycles");
	UML_HANDLE(block, *rsp->impstate->out_of_cycles);                               // handle  out_of_cycles
	UML_GETEXP(block, I0);                                                      // getexp  i0
	UML_MOV(block, mem(&rsp->pc), I0);                                          // mov     <pc>,i0
	save_fast_iregs(rsp, block);
	UML_EXIT(block, EXECUTE_OUT_OF_CYCLES);                                 // exit    EXECUTE_OUT_OF_CYCLES

	block->end();
}

/*------------------------------------------------------------------
    static_generate_memory_accessor
------------------------------------------------------------------*/

static void static_generate_memory_accessor(rsp_state *rsp, int size, int iswrite, const char *name, code_handle *&handleptr)
{
	/* on entry, address is in I0; data for writes is in I1 */
	/* on exit, read result is in I0 */
	/* routine trashes I0-I1 */
	drcuml_state *drcuml = rsp->impstate->drcuml;
	drcuml_block *block;

	/* begin generating */
	block = drcuml->begin_block(1024);

	/* add a global entry for this */
	alloc_handle(drcuml, &handleptr, name);
	UML_HANDLE(block, *handleptr);                                                  // handle  *handleptr

	// write:
	if (iswrite)
	{
		if (size == 1)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);              // mov     [arg0],i0 ; address
			UML_MOV(block, mem(&rsp->impstate->arg1), I1);              // mov     [arg1],i1 ; data
			UML_CALLC(block, cfunc_write8, rsp);                            // callc   cfunc_write8
		}
		else if (size == 2)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);              // mov     [arg0],i0 ; address
			UML_MOV(block, mem(&rsp->impstate->arg1), I1);              // mov     [arg1],i1 ; data
			UML_CALLC(block, cfunc_write16, rsp);                           // callc   cfunc_write16
		}
		else if (size == 4)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);              // mov     [arg0],i0 ; address
			UML_MOV(block, mem(&rsp->impstate->arg1), I1);              // mov     [arg1],i1 ; data
			UML_CALLC(block, cfunc_write32, rsp);                           // callc   cfunc_write32
		}
	}
	else
	{
		if (size == 1)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);          // mov     [arg0],i0 ; address
			UML_CALLC(block, cfunc_read8, rsp);                         // callc   cfunc_printf_debug
			UML_MOV(block, I0, mem(&rsp->impstate->arg0));          // mov     i0,[arg0],i0 ; result
		}
		else if (size == 2)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);          // mov     [arg0],i0 ; address
			UML_CALLC(block, cfunc_read16, rsp);                        // callc   cfunc_read16
			UML_MOV(block, I0, mem(&rsp->impstate->arg0));          // mov     i0,[arg0],i0 ; result
		}
		else if (size == 4)
		{
			UML_MOV(block, mem(&rsp->impstate->arg0), I0);          // mov     [arg0],i0 ; address
			UML_CALLC(block, cfunc_read32, rsp);                        // callc   cfunc_read32
			UML_MOV(block, I0, mem(&rsp->impstate->arg0));          // mov     i0,[arg0],i0 ; result
		}
	}
	UML_RET(block);

	block->end();
}



/***************************************************************************
    CODE GENERATION
***************************************************************************/

/*-------------------------------------------------
    generate_update_cycles - generate code to
    subtract cycles from the icount and generate
    an exception if out
-------------------------------------------------*/
static void generate_update_cycles(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, parameter param, int allow_exception)
{
	/* account for cycles */
	if (compiler->cycles > 0)
	{
		UML_SUB(block, mem(&rsp->icount), mem(&rsp->icount), MAPVAR_CYCLES);        // sub     icount,icount,cycles
		UML_MAPVAR(block, MAPVAR_CYCLES, 0);                                        // mapvar  cycles,0
		UML_EXHc(block, COND_S, *rsp->impstate->out_of_cycles, param);
	}
	compiler->cycles = 0;
}

/*-------------------------------------------------
    generate_checksum_block - generate code to
    validate a sequence of opcodes
-------------------------------------------------*/

static void generate_checksum_block(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *seqhead, const opcode_desc *seqlast)
{
	const opcode_desc *curdesc;
	if (LOG_UML)
	{
		block->append_comment("[Validation for %08X]", seqhead->pc | 0x1000);       // comment
	}
	/* loose verify or single instruction: just compare and fail */
	if (!(rsp->impstate->drcoptions & RSPDRC_STRICT_VERIFY) || seqhead->next() == NULL)
	{
		if (!(seqhead->flags & OPFLAG_VIRTUAL_NOOP))
		{
			UINT32 sum = seqhead->opptr.l[0];
			void *base = rsp->direct->read_decrypted_ptr(seqhead->physpc | 0x1000);
			UML_LOAD(block, I0, base, 0, SIZE_DWORD, SCALE_x4);                         // load    i0,base,0,dword

			if (seqhead->delay.first() != NULL && seqhead->physpc != seqhead->delay.first()->physpc)
			{
				base = rsp->direct->read_decrypted_ptr(seqhead->delay.first()->physpc | 0x1000);
				UML_LOAD(block, I1, base, 0, SIZE_DWORD, SCALE_x4);                 // load    i1,base,dword
				UML_ADD(block, I0, I0, I1);                     // add     i0,i0,i1

				sum += seqhead->delay.first()->opptr.l[0];
			}

			UML_CMP(block, I0, sum);                                    // cmp     i0,opptr[0]
			UML_EXHc(block, COND_NE, *rsp->impstate->nocode, epc(seqhead));     // exne    nocode,seqhead->pc
		}
	}

	/* full verification; sum up everything */
	else
	{
		UINT32 sum = 0;
		void *base = rsp->direct->read_decrypted_ptr(seqhead->physpc | 0x1000);
		UML_LOAD(block, I0, base, 0, SIZE_DWORD, SCALE_x4);                             // load    i0,base,0,dword
		sum += seqhead->opptr.l[0];
		for (curdesc = seqhead->next(); curdesc != seqlast->next(); curdesc = curdesc->next())
			if (!(curdesc->flags & OPFLAG_VIRTUAL_NOOP))
			{
				base = rsp->direct->read_decrypted_ptr(curdesc->physpc | 0x1000);
				UML_LOAD(block, I1, base, 0, SIZE_DWORD, SCALE_x4);                     // load    i1,base,dword
				UML_ADD(block, I0, I0, I1);                         // add     i0,i0,i1
				sum += curdesc->opptr.l[0];

				if (curdesc->delay.first() != NULL && (curdesc == seqlast || (curdesc->next() != NULL && curdesc->next()->physpc != curdesc->delay.first()->physpc)))
				{
					base = rsp->direct->read_decrypted_ptr(curdesc->delay.first()->physpc | 0x1000);
					UML_LOAD(block, I1, base, 0, SIZE_DWORD, SCALE_x4);                 // load    i1,base,dword
					UML_ADD(block, I0, I0, I1);                     // add     i0,i0,i1

					sum += curdesc->delay.first()->opptr.l[0];
				}
			}
		UML_CMP(block, I0, sum);                                            // cmp     i0,sum
		UML_EXHc(block, COND_NE, *rsp->impstate->nocode, epc(seqhead));         // exne    nocode,seqhead->pc
	}
}


/*-------------------------------------------------
    generate_sequence_instruction - generate code
    for a single instruction in a sequence
-------------------------------------------------*/

static void generate_sequence_instruction(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	offs_t expc;

	/* add an entry for the log */
	if (LOG_UML && !(desc->flags & OPFLAG_VIRTUAL_NOOP))
		log_add_disasm_comment(rsp, block, desc->pc, desc->opptr.l[0]);

	/* set the PC map variable */
	expc = (desc->flags & OPFLAG_IN_DELAY_SLOT) ? desc->pc - 3 : desc->pc;
	UML_MAPVAR(block, MAPVAR_PC, expc);                                             // mapvar  PC,expc

	/* accumulate total cycles */
	compiler->cycles += desc->cycles;

	/* update the icount map variable */
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);                             // mapvar  CYCLES,compiler->cycles

	/* if we are debugging, call the debugger */
	if ((rsp->device->machine().debug_flags & DEBUG_FLAG_ENABLED) != 0)
	{
		UML_MOV(block, mem(&rsp->pc), desc->pc);                                // mov     [pc],desc->pc
		save_fast_iregs(rsp, block);
		UML_DEBUG(block, desc->pc);                                         // debug   desc->pc
	}

	/* if we hit an unmapped address, fatal error */
#if 0
	if (desc->flags & OPFLAG_COMPILER_UNMAPPED)
	{
		UML_MOV(block, mem(&rsp->pc), desc->pc);                               // mov     [pc],desc->pc
		save_fast_iregs(rsp, block);
		UML_EXIT(block, EXECUTE_UNMAPPED_CODE);                             // exit EXECUTE_UNMAPPED_CODE
	}
#endif

	/* otherwise, unless this is a virtual no-op, it's a regular instruction */
	/*else*/ if (!(desc->flags & OPFLAG_VIRTUAL_NOOP))
	{
		/* compile the instruction */
		if (!generate_opcode(rsp, block, compiler, desc))
		{
			UML_MOV(block, mem(&rsp->pc), desc->pc);                            // mov     [pc],desc->pc
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_unimplemented, rsp);                             // callc   cfunc_unimplemented
		}
	}
}

/*------------------------------------------------------------------
    generate_delay_slot_and_branch
------------------------------------------------------------------*/

static void generate_delay_slot_and_branch(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc, UINT8 linkreg)
{
	compiler_state compiler_temp = *compiler;
	UINT32 op = desc->opptr.l[0];

	/* fetch the target register if dynamic, in case it is modified by the delay slot */
	if (desc->targetpc == BRANCH_TARGET_DYNAMIC)
	{
		UML_AND(block, mem(&rsp->impstate->jmpdest), R32(RSREG), 0x00000fff);
		UML_OR(block, mem(&rsp->impstate->jmpdest), mem(&rsp->impstate->jmpdest), 0x1000);
	}

	/* set the link if needed -- before the delay slot */
	if (linkreg != 0)
	{
		UML_MOV(block, R32(linkreg), (INT32)(desc->pc + 8));                    // mov    <linkreg>,desc->pc + 8
	}

	/* compile the delay slot using temporary compiler state */
	assert(desc->delay.first() != NULL);
	generate_sequence_instruction(rsp, block, &compiler_temp, desc->delay.first());     // <next instruction>

	/* update the cycles and jump through the hash table to the target */
	if (desc->targetpc != BRANCH_TARGET_DYNAMIC)
	{
		generate_update_cycles(rsp, block, &compiler_temp, desc->targetpc, TRUE);   // <subtract cycles>
		if (desc->flags & OPFLAG_INTRABLOCK_BRANCH)
			UML_JMP(block, desc->targetpc | 0x80000000);                            // jmp     desc->targetpc
		else
			UML_HASHJMP(block, 0, desc->targetpc, *rsp->impstate->nocode);
																					// hashjmp <mode>,desc->targetpc,nocode
	}
	else
	{
		generate_update_cycles(rsp, block, &compiler_temp, mem(&rsp->impstate->jmpdest), TRUE);
																					// <subtract cycles>
		UML_HASHJMP(block, 0, mem(&rsp->impstate->jmpdest), *rsp->impstate->nocode);
																					// hashjmp <mode>,<rsreg>,nocode
	}

	/* update the label */
	compiler->labelnum = compiler_temp.labelnum;

	/* reset the mapvar to the current cycles and account for skipped slots */
	compiler->cycles += desc->skipslots;
	UML_MAPVAR(block, MAPVAR_CYCLES, compiler->cycles);                             // mapvar  CYCLES,compiler->cycles
}


/*-------------------------------------------------
    generate_opcode - generate code for a specific
    opcode
-------------------------------------------------*/

static int generate_vector_opcode(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = desc->opptr.l[0];
	// Opcode legend:
	//    E = VS2 element type
	//    S = VS1, Source vector 1
	//    T = VS2, Source vector 2
	//    D = Destination vector

	switch (op & 0x3f)
	{
		case 0x00:      /* VMULF */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmulf, rsp);
			return TRUE;

		case 0x01:      /* VMULU */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmulu, rsp);
			return TRUE;

		case 0x04:      /* VMUDL */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmudl, rsp);
			return TRUE;

		case 0x05:      /* VMUDM */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmudm, rsp);
			return TRUE;

		case 0x06:      /* VMUDN */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmudn, rsp);
			return TRUE;

		case 0x07:      /* VMUDH */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmudh, rsp);
			return TRUE;

		case 0x08:      /* VMACF */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmacf, rsp);
			return TRUE;

		case 0x09:      /* VMACU */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmacu, rsp);
			return TRUE;

		case 0x0c:      /* VMADL */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmadl, rsp);
			return TRUE;

		case 0x0d:      /* VMADM */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmadm, rsp);
			return TRUE;

		case 0x0e:      /* VMADN */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmadn, rsp);
			return TRUE;

		case 0x0f:      /* VMADH */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmadh, rsp);
			return TRUE;

		case 0x10:      /* VADD */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vadd, rsp);
			return TRUE;

		case 0x11:      /* VSUB */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vsub, rsp);
			return TRUE;

		case 0x13:      /* VABS */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vabs, rsp);
			return TRUE;

		case 0x14:      /* VADDC */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vaddc, rsp);
			return TRUE;

		case 0x15:      /* VSUBC */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vsubc, rsp);
			return TRUE;

		case 0x1d:      /* VSAW */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vsaw, rsp);
			return TRUE;

		case 0x20:      /* VLT */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vlt, rsp);
			return TRUE;

		case 0x21:      /* VEQ */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_veq, rsp);
			return TRUE;

		case 0x22:      /* VNE */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vne, rsp);
			return TRUE;

		case 0x23:      /* VGE */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vge, rsp);
			return TRUE;

		case 0x24:      /* VCL */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vcl, rsp);
			return TRUE;

		case 0x25:      /* VCH */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vch, rsp);
			return TRUE;

		case 0x26:      /* VCR */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vcr, rsp);
			return TRUE;

		case 0x27:      /* VMRG */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmrg, rsp);
			return TRUE;

		case 0x28:      /* VAND */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vand, rsp);
			return TRUE;

		case 0x29:      /* VNAND */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vnand, rsp);
			return TRUE;

		case 0x2a:      /* VOR */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vor, rsp);
			return TRUE;

		case 0x2b:      /* VNOR */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vnor, rsp);
			return TRUE;

		case 0x2c:      /* VXOR */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vxor, rsp);
			return TRUE;

		case 0x2d:      /* VNXOR */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vnxor, rsp);
			return TRUE;

		case 0x30:      /* VRCP */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vrcp, rsp);
			return TRUE;

		case 0x31:      /* VRCPL */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vrcpl, rsp);
			return TRUE;

		case 0x32:      /* VRCPH */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vrcph, rsp);
			return TRUE;

		case 0x33:      /* VMOV */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vmov, rsp);
			return TRUE;

		case 0x35:      /* VRSQL */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vrsql, rsp);
			return TRUE;

		case 0x36:      /* VRSQH */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_rsp_vrsqh, rsp);
			return TRUE;

		default:
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_unimplemented_opcode, rsp);
			return FALSE;
	}
}

static int generate_opcode(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	int in_delay_slot = ((desc->flags & OPFLAG_IN_DELAY_SLOT) != 0);
	UINT32 op = desc->opptr.l[0];
	UINT8 opswitch = op >> 26;
	code_label skip;

	switch (opswitch)
	{
		/* ----- sub-groups ----- */

		case 0x00:  /* SPECIAL - MIPS I */
			return generate_special(rsp, block, compiler, desc);

		case 0x01:  /* REGIMM - MIPS I */
			return generate_regimm(rsp, block, compiler, desc);

		/* ----- jumps and branches ----- */

		case 0x02:  /* J - MIPS I */
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);      // <next instruction + hashjmp>
			return TRUE;

		case 0x03:  /* JAL - MIPS I */
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 31);     // <next instruction + hashjmp>
			return TRUE;

		case 0x04:  /* BEQ - MIPS I */
			UML_CMP(block, R32(RSREG), R32(RTREG));                             // cmp    <rsreg>,<rtreg>
			UML_JMPc(block, COND_NE, skip = compiler->labelnum++);              // jmp    skip,NE
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);      // <next instruction + hashjmp>
			UML_LABEL(block, skip);                                             // skip:
			return TRUE;

		case 0x05:  /* BNE - MIPS I */
			UML_CMP(block, R32(RSREG), R32(RTREG));                             // dcmp    <rsreg>,<rtreg>
			UML_JMPc(block, COND_E, skip = compiler->labelnum++);                       // jmp     skip,E
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);      // <next instruction + hashjmp>
			UML_LABEL(block, skip);                                             // skip:
			return TRUE;

		case 0x06:  /* BLEZ - MIPS I */
			if (RSREG != 0)
			{
				UML_CMP(block, R32(RSREG), 0);                              // dcmp    <rsreg>,0
				UML_JMPc(block, COND_G, skip = compiler->labelnum++);                   // jmp     skip,G
				generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);  // <next instruction + hashjmp>
				UML_LABEL(block, skip);                                         // skip:
			}
			else
				generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);  // <next instruction + hashjmp>
			return TRUE;

		case 0x07:  /* BGTZ - MIPS I */
			UML_CMP(block, R32(RSREG), 0);                                  // dcmp    <rsreg>,0
			UML_JMPc(block, COND_LE, skip = compiler->labelnum++);                  // jmp     skip,LE
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);      // <next instruction + hashjmp>
			UML_LABEL(block, skip);                                             // skip:
			return TRUE;


		/* ----- immediate arithmetic ----- */

		case 0x0f:  /* LUI - MIPS I */
			if (RTREG != 0)
				UML_MOV(block, R32(RTREG), SIMMVAL << 16);                  // dmov    <rtreg>,SIMMVAL << 16
			return TRUE;

		case 0x08:  /* ADDI - MIPS I */
		case 0x09:  /* ADDIU - MIPS I */
			if (RTREG != 0)
			{
				UML_ADD(block, R32(RTREG), R32(RSREG), SIMMVAL);                // add     i0,<rsreg>,SIMMVAL,V
			}
			return TRUE;

		case 0x0a:  /* SLTI - MIPS I */
			if (RTREG != 0)
			{
				UML_CMP(block, R32(RSREG), SIMMVAL);                            // dcmp    <rsreg>,SIMMVAL
				UML_SETc(block, COND_L, R32(RTREG));                                    // dset    <rtreg>,l
			}
			return TRUE;

		case 0x0b:  /* SLTIU - MIPS I */
			if (RTREG != 0)
			{
				UML_CMP(block, R32(RSREG), SIMMVAL);                            // dcmp    <rsreg>,SIMMVAL
				UML_SETc(block, COND_B, R32(RTREG));                                    // dset    <rtreg>,b
			}
			return TRUE;


		case 0x0c:  /* ANDI - MIPS I */
			if (RTREG != 0)
				UML_AND(block, R32(RTREG), R32(RSREG), UIMMVAL);                // dand    <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		case 0x0d:  /* ORI - MIPS I */
			if (RTREG != 0)
				UML_OR(block, R32(RTREG), R32(RSREG), UIMMVAL);             // dor     <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		case 0x0e:  /* XORI - MIPS I */
			if (RTREG != 0)
				UML_XOR(block, R32(RTREG), R32(RSREG), UIMMVAL);                // dxor    <rtreg>,<rsreg>,UIMMVAL
			return TRUE;

		/* ----- memory load operations ----- */

		case 0x20:  /* LB - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, *rsp->impstate->read8);                                    // callh   read8
			if (RTREG != 0)
				UML_SEXT(block, R32(RTREG), I0, SIZE_BYTE);                     // dsext   <rtreg>,i0,byte
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x21:  /* LH - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, *rsp->impstate->read16);                               // callh   read16
			if (RTREG != 0)
				UML_SEXT(block, R32(RTREG), I0, SIZE_WORD);                     // dsext   <rtreg>,i0,word
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x23:  /* LW - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, *rsp->impstate->read32);                               // callh   read32
			if (RTREG != 0)
				UML_MOV(block, R32(RTREG), I0);
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x24:  /* LBU - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, *rsp->impstate->read8);                                    // callh   read8
			if (RTREG != 0)
				UML_AND(block, R32(RTREG), I0, 0xff);                   // dand    <rtreg>,i0,0xff
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x25:  /* LHU - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_CALLH(block, *rsp->impstate->read16);                               // callh   read16
			if (RTREG != 0)
				UML_AND(block, R32(RTREG), I0, 0xffff);                 // dand    <rtreg>,i0,0xffff
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x32:  /* LWC2 - MIPS I */
			return generate_lwc2(rsp, block, compiler, desc);


		/* ----- memory store operations ----- */

		case 0x28:  /* SB - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, I1, R32(RTREG));                                 // mov     i1,<rtreg>
			UML_CALLH(block, *rsp->impstate->write8);                               // callh   write8
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x29:  /* SH - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, I1, R32(RTREG));                                 // mov     i1,<rtreg>
			UML_CALLH(block, *rsp->impstate->write16);                              // callh   write16
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x2b:  /* SW - MIPS I */
			UML_ADD(block, I0, R32(RSREG), SIMMVAL);                        // add     i0,<rsreg>,SIMMVAL
			UML_MOV(block, I1, R32(RTREG));                                 // mov     i1,<rtreg>
			UML_CALLH(block, *rsp->impstate->write32);                              // callh   write32
			if (!in_delay_slot)
				generate_update_cycles(rsp, block, compiler, desc->pc + 4, TRUE);
			return TRUE;

		case 0x3a:  /* SWC2 - MIPS I */
			return generate_swc2(rsp, block, compiler, desc);
			//UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);     // mov     [arg0],desc->opptr.l
			//UML_CALLC(block, cfunc_swc2, rsp);                                        // callc   cfunc_mfc2
			//return TRUE;

		/* ----- coprocessor instructions ----- */

		case 0x10:  /* COP0 - MIPS I */
			return generate_cop0(rsp, block, compiler, desc);

		case 0x12:  /* COP2 - MIPS I */
			return generate_cop2(rsp, block, compiler, desc);
			//UML_EXH(block, rsp->impstate->exception[EXCEPTION_INVALIDOP], 0);// exh     invalidop,0
			//return TRUE;


		/* ----- unimplemented/illegal instructions ----- */

		//default:    /* ??? */       invalid_instruction(op);                                                break;
	}

	return FALSE;
}


/*-------------------------------------------------
    generate_special - compile opcodes in the
    'SPECIAL' group
-------------------------------------------------*/

static int generate_special(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = desc->opptr.l[0];
	UINT8 opswitch = op & 63;
	//code_label skip;

	switch (opswitch)
	{
		/* ----- shift instructions ----- */

		case 0x00:  /* SLL - MIPS I */
			if (RDREG != 0)
			{
				UML_SHL(block, R32(RDREG), R32(RTREG), SHIFT);
			}
			return TRUE;

		case 0x02:  /* SRL - MIPS I */
			if (RDREG != 0)
			{
				UML_SHR(block, R32(RDREG), R32(RTREG), SHIFT);
			}
			return TRUE;

		case 0x03:  /* SRA - MIPS I */
			if (RDREG != 0)
			{
				UML_SAR(block, R32(RDREG), R32(RTREG), SHIFT);
			}
			return TRUE;

		case 0x04:  /* SLLV - MIPS I */
			if (RDREG != 0)
			{
				UML_SHL(block, R32(RDREG), R32(RTREG), R32(RSREG));
			}
			return TRUE;

		case 0x06:  /* SRLV - MIPS I */
			if (RDREG != 0)
			{
				UML_SHR(block, R32(RDREG), R32(RTREG), R32(RSREG));
			}
			return TRUE;

		case 0x07:  /* SRAV - MIPS I */
			if (RDREG != 0)
			{
				UML_SAR(block, R32(RDREG), R32(RTREG), R32(RSREG));
			}
			return TRUE;

		/* ----- basic arithmetic ----- */

		case 0x20:  /* ADD - MIPS I */
		case 0x21:  /* ADDU - MIPS I */
			if (RDREG != 0)
			{
				UML_ADD(block, R32(RDREG), R32(RSREG), R32(RTREG));
			}
			return TRUE;

		case 0x22:  /* SUB - MIPS I */
		case 0x23:  /* SUBU - MIPS I */
			if (RDREG != 0)
			{
				UML_SUB(block, R32(RDREG), R32(RSREG), R32(RTREG));
			}
			return TRUE;

		/* ----- basic logical ops ----- */

		case 0x24:  /* AND - MIPS I */
			if (RDREG != 0)
			{
				UML_AND(block, R32(RDREG), R32(RSREG), R32(RTREG));             // dand     <rdreg>,<rsreg>,<rtreg>
			}
			return TRUE;

		case 0x25:  /* OR - MIPS I */
			if (RDREG != 0)
			{
				UML_OR(block, R32(RDREG), R32(RSREG), R32(RTREG));                  // dor      <rdreg>,<rsreg>,<rtreg>
			}
			return TRUE;

		case 0x26:  /* XOR - MIPS I */
			if (RDREG != 0)
			{
				UML_XOR(block, R32(RDREG), R32(RSREG), R32(RTREG));             // dxor     <rdreg>,<rsreg>,<rtreg>
			}
			return TRUE;

		case 0x27:  /* NOR - MIPS I */
			if (RDREG != 0)
			{
				UML_OR(block, I0, R32(RSREG), R32(RTREG));                  // dor      i0,<rsreg>,<rtreg>
				UML_XOR(block, R32(RDREG), I0, (UINT64)~0);             // dxor     <rdreg>,i0,~0
			}
			return TRUE;


		/* ----- basic comparisons ----- */

		case 0x2a:  /* SLT - MIPS I */
			if (RDREG != 0)
			{
				UML_CMP(block, R32(RSREG), R32(RTREG));                         // dcmp    <rsreg>,<rtreg>
				UML_SETc(block, COND_L, R32(RDREG));                                    // dset    <rdreg>,l
			}
			return TRUE;

		case 0x2b:  /* SLTU - MIPS I */
			if (RDREG != 0)
			{
				UML_CMP(block, R32(RSREG), R32(RTREG));                         // dcmp    <rsreg>,<rtreg>
				UML_SETc(block, COND_B, R32(RDREG));                                    // dset    <rdreg>,b
			}
			return TRUE;


		/* ----- jumps and branches ----- */

		case 0x08:  /* JR - MIPS I */
			generate_delay_slot_and_branch(rsp, block, compiler, desc, 0);      // <next instruction + hashjmp>
			return TRUE;

		case 0x09:  /* JALR - MIPS I */
			generate_delay_slot_and_branch(rsp, block, compiler, desc, RDREG);  // <next instruction + hashjmp>
			return TRUE;


		/* ----- system calls ----- */

		case 0x0d:  /* BREAK - MIPS I */
			UML_MOV(block, mem(&rsp->impstate->arg0), 3);                   // mov     [arg0],3
			UML_CALLC(block, cfunc_sp_set_status_cb, rsp);                      // callc   cfunc_sp_set_status_cb
			UML_MOV(block, mem(&rsp->icount), 0);                       // mov icount, #0

			UML_EXIT(block, EXECUTE_OUT_OF_CYCLES);
			return TRUE;
	}
	return FALSE;
}



/*-------------------------------------------------
    generate_regimm - compile opcodes in the
    'REGIMM' group
-------------------------------------------------*/

static int generate_regimm(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = desc->opptr.l[0];
	UINT8 opswitch = RTREG;
	code_label skip;

	switch (opswitch)
	{
		case 0x00:  /* BLTZ */
		case 0x10:  /* BLTZAL */
			if (RSREG != 0)
			{
				UML_CMP(block, R32(RSREG), 0);                              // dcmp    <rsreg>,0
				UML_JMPc(block, COND_GE, skip = compiler->labelnum++);              // jmp     skip,GE
				generate_delay_slot_and_branch(rsp, block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
				UML_LABEL(block, skip);                                         // skip:
			}
			return TRUE;

		case 0x01:  /* BGEZ */
		case 0x11:  /* BGEZAL */
			if (RSREG != 0)
			{
				UML_CMP(block, R32(RSREG), 0);                              // dcmp    <rsreg>,0
				UML_JMPc(block, COND_L, skip = compiler->labelnum++);                   // jmp     skip,L
				generate_delay_slot_and_branch(rsp, block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
				UML_LABEL(block, skip);                                         // skip:
			}
			else
				generate_delay_slot_and_branch(rsp, block, compiler, desc, (opswitch & 0x10) ? 31 : 0);
																					// <next instruction + hashjmp>
			return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    generate_cop2 - compile COP2 opcodes
-------------------------------------------------*/

static int generate_cop2(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = desc->opptr.l[0];
	UINT8 opswitch = RSREG;

	switch (opswitch)
	{
		case 0x00:  /* MFCz */
			if (RTREG != 0)
			{
				UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);    // mov     [arg0],desc->opptr.l
				UML_CALLC(block, cfunc_mfc2, rsp);                                  // callc   cfunc_mfc2
				//UML_SEXT(block, R32(RTREG), I0, DWORD);                      // dsext   <rtreg>,i0,dword
			}
			return TRUE;

		case 0x02:  /* CFCz */
			if (RTREG != 0)
			{
				UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);    // mov     [arg0],desc->opptr.l
				UML_CALLC(block, cfunc_cfc2, rsp);                                  // callc   cfunc_cfc2
				//UML_SEXT(block, R32(RTREG), I0, DWORD);                      // dsext   <rtreg>,i0,dword
			}
			return TRUE;

		case 0x04:  /* MTCz */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_mtc2, rsp);                                      // callc   cfunc_mtc2
			return TRUE;

		case 0x06:  /* CTCz */
			UML_MOV(block, mem(&rsp->impstate->arg0), desc->opptr.l[0]);        // mov     [arg0],desc->opptr.l
			UML_CALLC(block, cfunc_ctc2, rsp);                                      // callc   cfunc_ctc2
			return TRUE;

		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			return generate_vector_opcode(rsp, block, compiler, desc);
	}
	return FALSE;
}

/*-------------------------------------------------
    generate_cop0 - compile COP0 opcodes
-------------------------------------------------*/

static int generate_cop0(rsp_state *rsp, drcuml_block *block, compiler_state *compiler, const opcode_desc *desc)
{
	UINT32 op = desc->opptr.l[0];
	UINT8 opswitch = RSREG;

	switch (opswitch)
	{
		case 0x00:  /* MFCz */
			if (RTREG != 0)
			{
				UML_MOV(block, mem(&rsp->impstate->arg0), RDREG);               // mov     [arg0],<rdreg>
				UML_MOV(block, mem(&rsp->impstate->arg1), RTREG);               // mov     [arg1],<rtreg>
				UML_CALLC(block, cfunc_get_cop0_reg, rsp);                          // callc   cfunc_get_cop0_reg
				if(RDREG == 2)
				{
					generate_update_cycles(rsp, block, compiler, mem(&rsp->pc), TRUE);
					UML_HASHJMP(block, 0, mem(&rsp->pc), *rsp->impstate->nocode);
				}
			}
			return TRUE;

		case 0x04:  /* MTCz */
			UML_MOV(block, mem(&rsp->impstate->arg0), RDREG);                   // mov     [arg0],<rdreg>
			UML_MOV(block, mem(&rsp->impstate->arg1), R32(RTREG));                  // mov     [arg1],rtreg
			UML_CALLC(block, cfunc_set_cop0_reg, rsp);                              // callc   cfunc_set_cop0_reg
			return TRUE;
	}

	return FALSE;
}

static void cfunc_mfc2(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int el = (op >> 7) & 0xf;
#if USE_SIMD
	UINT16 out;
	SIMD_EXTRACT16(rsp->xv[VS1REG], out, (el >> 1));
	out >>= (1 - (el & 1)) * 8;
	out &= 0x00ff;

	el++;

	UINT16 temp;
	SIMD_EXTRACT16(rsp->xv[VS1REG], temp, (el >> 1));
	temp >>= (1 - (el & 1)) * 8;
	temp &= 0x00ff;

	rsp->r[RTREG] = (INT32)(INT16)((out << 8) | temp);
#else
	UINT16 b1 = VREG_B(VS1REG, (el+0) & 0xf);
	UINT16 b2 = VREG_B(VS1REG, (el+1) & 0xf);
	if (RTREG) RTVAL = (INT32)(INT16)((b1 << 8) | (b2));
#endif
}

static void cfunc_cfc2(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	if (RTREG)
	{
		switch(RDREG)
		{
			case 0:
				RTVAL = ((CARRY_FLAG(rsp, 0) & 1) << 0) |
						((CARRY_FLAG(rsp, 1) & 1) << 1) |
						((CARRY_FLAG(rsp, 2) & 1) << 2) |
						((CARRY_FLAG(rsp, 3) & 1) << 3) |
						((CARRY_FLAG(rsp, 4) & 1) << 4) |
						((CARRY_FLAG(rsp, 5) & 1) << 5) |
						((CARRY_FLAG(rsp, 6) & 1) << 6) |
						((CARRY_FLAG(rsp, 7) & 1) << 7) |
						((ZERO_FLAG(rsp, 0) & 1) << 8) |
						((ZERO_FLAG(rsp, 1) & 1) << 9) |
						((ZERO_FLAG(rsp, 2) & 1) << 10) |
						((ZERO_FLAG(rsp, 3) & 1) << 11) |
						((ZERO_FLAG(rsp, 4) & 1) << 12) |
						((ZERO_FLAG(rsp, 5) & 1) << 13) |
						((ZERO_FLAG(rsp, 6) & 1) << 14) |
						((ZERO_FLAG(rsp, 7) & 1) << 15);
				if (RTVAL & 0x8000) RTVAL |= 0xffff0000;
				break;
			case 1:
				RTVAL = ((COMPARE_FLAG(rsp, 0) & 1) << 0) |
						((COMPARE_FLAG(rsp, 1) & 1) << 1) |
						((COMPARE_FLAG(rsp, 2) & 1) << 2) |
						((COMPARE_FLAG(rsp, 3) & 1) << 3) |
						((COMPARE_FLAG(rsp, 4) & 1) << 4) |
						((COMPARE_FLAG(rsp, 5) & 1) << 5) |
						((COMPARE_FLAG(rsp, 6) & 1) << 6) |
						((COMPARE_FLAG(rsp, 7) & 1) << 7) |
						((CLIP2_FLAG(rsp, 0) & 1) << 8) |
						((CLIP2_FLAG(rsp, 1) & 1) << 9) |
						((CLIP2_FLAG(rsp, 2) & 1) << 10) |
						((CLIP2_FLAG(rsp, 3) & 1) << 11) |
						((CLIP2_FLAG(rsp, 4) & 1) << 12) |
						((CLIP2_FLAG(rsp, 5) & 1) << 13) |
						((CLIP2_FLAG(rsp, 6) & 1) << 14) |
						((CLIP2_FLAG(rsp, 7) & 1) << 15);
				if (RTVAL & 0x8000) RTVAL |= 0xffff0000;
				break;
			case 2:
				RTVAL = ((CLIP1_FLAG(rsp, 0) & 1) << 0) |
						((CLIP1_FLAG(rsp, 1) & 1) << 1) |
						((CLIP1_FLAG(rsp, 2) & 1) << 2) |
						((CLIP1_FLAG(rsp, 3) & 1) << 3) |
						((CLIP1_FLAG(rsp, 4) & 1) << 4) |
						((CLIP1_FLAG(rsp, 5) & 1) << 5) |
						((CLIP1_FLAG(rsp, 6) & 1) << 6) |
						((CLIP1_FLAG(rsp, 7) & 1) << 7);
				break;
		}
	}
}

static void cfunc_mtc2(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	int el = (op >> 7) & 0xf;
#if USE_SIMD
	SIMD_INSERT16(rsp->xv[VS1REG], RTVAL, el >> 1);
#else
	VREG_B(VS1REG, (el+0) & 0xf) = (RTVAL >> 8) & 0xff;
	VREG_B(VS1REG, (el+1) & 0xf) = (RTVAL >> 0) & 0xff;
#endif
}

static void cfunc_ctc2(void *param)
{
	rsp_state *rsp = (rsp_state*)param;
	UINT32 op = rsp->impstate->arg0;
	switch(RDREG)
	{
		case 0:
			CLEAR_CARRY_FLAGS();
			CLEAR_ZERO_FLAGS();
			rsp->vflag[0][0] = ((RTVAL >> 0) & 1) ? 0xffff : 0;
			rsp->vflag[0][1] = ((RTVAL >> 1) & 1) ? 0xffff : 0;
			rsp->vflag[0][2] = ((RTVAL >> 2) & 1) ? 0xffff : 0;
			rsp->vflag[0][3] = ((RTVAL >> 3) & 1) ? 0xffff : 0;
			rsp->vflag[0][4] = ((RTVAL >> 4) & 1) ? 0xffff : 0;
			rsp->vflag[0][5] = ((RTVAL >> 5) & 1) ? 0xffff : 0;
			rsp->vflag[0][6] = ((RTVAL >> 6) & 1) ? 0xffff : 0;
			rsp->vflag[0][7] = ((RTVAL >> 7) & 1) ? 0xffff : 0;
			if (RTVAL & (1 << 0))  { SET_CARRY_FLAG(0); }
			if (RTVAL & (1 << 1))  { SET_CARRY_FLAG(1); }
			if (RTVAL & (1 << 2))  { SET_CARRY_FLAG(2); }
			if (RTVAL & (1 << 3))  { SET_CARRY_FLAG(3); }
			if (RTVAL & (1 << 4))  { SET_CARRY_FLAG(4); }
			if (RTVAL & (1 << 5))  { SET_CARRY_FLAG(5); }
			if (RTVAL & (1 << 6))  { SET_CARRY_FLAG(6); }
			if (RTVAL & (1 << 7))  { SET_CARRY_FLAG(7); }
			rsp->vflag[3][0] = ((RTVAL >> 8) & 1) ? 0xffff : 0;
			rsp->vflag[3][1] = ((RTVAL >> 9) & 1) ? 0xffff : 0;
			rsp->vflag[3][2] = ((RTVAL >> 10) & 1) ? 0xffff : 0;
			rsp->vflag[3][3] = ((RTVAL >> 11) & 1) ? 0xffff : 0;
			rsp->vflag[3][4] = ((RTVAL >> 12) & 1) ? 0xffff : 0;
			rsp->vflag[3][5] = ((RTVAL >> 13) & 1) ? 0xffff : 0;
			rsp->vflag[3][6] = ((RTVAL >> 14) & 1) ? 0xffff : 0;
			rsp->vflag[3][7] = ((RTVAL >> 15) & 1) ? 0xffff : 0;
			if (RTVAL & (1 << 8))  { SET_ZERO_FLAG(0); }
			if (RTVAL & (1 << 9))  { SET_ZERO_FLAG(1); }
			if (RTVAL & (1 << 10)) { SET_ZERO_FLAG(2); }
			if (RTVAL & (1 << 11)) { SET_ZERO_FLAG(3); }
			if (RTVAL & (1 << 12)) { SET_ZERO_FLAG(4); }
			if (RTVAL & (1 << 13)) { SET_ZERO_FLAG(5); }
			if (RTVAL & (1 << 14)) { SET_ZERO_FLAG(6); }
			if (RTVAL & (1 << 15)) { SET_ZERO_FLAG(7); }
			break;
		case 1:
			CLEAR_COMPARE_FLAGS();
			CLEAR_CLIP2_FLAGS();
			rsp->vflag[1][0] = ((RTVAL >> 0) & 1) ? 0xffff : 0;
			rsp->vflag[1][1] = ((RTVAL >> 1) & 1) ? 0xffff : 0;
			rsp->vflag[1][2] = ((RTVAL >> 2) & 1) ? 0xffff : 0;
			rsp->vflag[1][3] = ((RTVAL >> 3) & 1) ? 0xffff : 0;
			rsp->vflag[1][4] = ((RTVAL >> 4) & 1) ? 0xffff : 0;
			rsp->vflag[1][5] = ((RTVAL >> 5) & 1) ? 0xffff : 0;
			rsp->vflag[1][6] = ((RTVAL >> 6) & 1) ? 0xffff : 0;
			rsp->vflag[1][7] = ((RTVAL >> 7) & 1) ? 0xffff : 0;
			if (RTVAL & (1 << 0)) { SET_COMPARE_FLAG(0); }
			if (RTVAL & (1 << 1)) { SET_COMPARE_FLAG(1); }
			if (RTVAL & (1 << 2)) { SET_COMPARE_FLAG(2); }
			if (RTVAL & (1 << 3)) { SET_COMPARE_FLAG(3); }
			if (RTVAL & (1 << 4)) { SET_COMPARE_FLAG(4); }
			if (RTVAL & (1 << 5)) { SET_COMPARE_FLAG(5); }
			if (RTVAL & (1 << 6)) { SET_COMPARE_FLAG(6); }
			if (RTVAL & (1 << 7)) { SET_COMPARE_FLAG(7); }
			rsp->vflag[4][0] = ((RTVAL >> 8) & 1) ? 0xffff : 0;
			rsp->vflag[4][1] = ((RTVAL >> 9) & 1) ? 0xffff : 0;
			rsp->vflag[4][2] = ((RTVAL >> 10) & 1) ? 0xffff : 0;
			rsp->vflag[4][3] = ((RTVAL >> 11) & 1) ? 0xffff : 0;
			rsp->vflag[4][4] = ((RTVAL >> 12) & 1) ? 0xffff : 0;
			rsp->vflag[4][5] = ((RTVAL >> 13) & 1) ? 0xffff : 0;
			rsp->vflag[4][6] = ((RTVAL >> 14) & 1) ? 0xffff : 0;
			rsp->vflag[4][7] = ((RTVAL >> 15) & 1) ? 0xffff : 0;
			if (RTVAL & (1 << 8))  { SET_CLIP2_FLAG(0); }
			if (RTVAL & (1 << 9))  { SET_CLIP2_FLAG(1); }
			if (RTVAL & (1 << 10)) { SET_CLIP2_FLAG(2); }
			if (RTVAL & (1 << 11)) { SET_CLIP2_FLAG(3); }
			if (RTVAL & (1 << 12)) { SET_CLIP2_FLAG(4); }
			if (RTVAL & (1 << 13)) { SET_CLIP2_FLAG(5); }
			if (RTVAL & (1 << 14)) { SET_CLIP2_FLAG(6); }
			if (RTVAL & (1 << 15)) { SET_CLIP2_FLAG(7); }
			break;
		case 2:
			CLEAR_CLIP1_FLAGS();
			rsp->vflag[2][0] = ((RTVAL >> 0) & 1) ? 0xffff : 0;
			rsp->vflag[2][1] = ((RTVAL >> 1) & 1) ? 0xffff : 0;
			rsp->vflag[2][2] = ((RTVAL >> 2) & 1) ? 0xffff : 0;
			rsp->vflag[2][3] = ((RTVAL >> 3) & 1) ? 0xffff : 0;
			rsp->vflag[2][4] = ((RTVAL >> 4) & 1) ? 0xffff : 0;
			rsp->vflag[2][5] = ((RTVAL >> 5) & 1) ? 0xffff : 0;
			rsp->vflag[2][6] = ((RTVAL >> 6) & 1) ? 0xffff : 0;
			rsp->vflag[2][7] = ((RTVAL >> 7) & 1) ? 0xffff : 0;
			if (RTVAL & (1 << 0)) { SET_CLIP1_FLAG(0); }
			if (RTVAL & (1 << 1)) { SET_CLIP1_FLAG(1); }
			if (RTVAL & (1 << 2)) { SET_CLIP1_FLAG(2); }
			if (RTVAL & (1 << 3)) { SET_CLIP1_FLAG(3); }
			if (RTVAL & (1 << 4)) { SET_CLIP1_FLAG(4); }
			if (RTVAL & (1 << 5)) { SET_CLIP1_FLAG(5); }
			if (RTVAL & (1 << 6)) { SET_CLIP1_FLAG(6); }
			if (RTVAL & (1 << 7)) { SET_CLIP1_FLAG(7); }
			break;
	}
}

/***************************************************************************
    CODE LOGGING HELPERS
***************************************************************************/

/*-------------------------------------------------
    log_add_disasm_comment - add a comment
    including disassembly of a RSP instruction
-------------------------------------------------*/

static void log_add_disasm_comment(rsp_state *rsp, drcuml_block *block, UINT32 pc, UINT32 op)
{
#if (LOG_UML)
	char buffer[100];
	rsp_dasm_one(buffer, pc, op);
	block->append_comment("%08X: %s", pc, buffer);                                  // comment
#endif
}


static CPU_SET_INFO( rsp )
{
	rsp_state *rsp = get_safe_token(device);

	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + RSP_PC:             rsp->pc = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R0:             rsp->r[0] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R1:             rsp->r[1] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R2:             rsp->r[2] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R3:             rsp->r[3] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R4:             rsp->r[4] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R5:             rsp->r[5] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R6:             rsp->r[6] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R7:             rsp->r[7] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R8:             rsp->r[8] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R9:             rsp->r[9] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R10:            rsp->r[10] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R11:            rsp->r[11] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R12:            rsp->r[12] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R13:            rsp->r[13] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R14:            rsp->r[14] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R15:            rsp->r[15] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R16:            rsp->r[16] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R17:            rsp->r[17] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R18:            rsp->r[18] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R19:            rsp->r[19] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R20:            rsp->r[20] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R21:            rsp->r[21] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R22:            rsp->r[22] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R23:            rsp->r[23] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R24:            rsp->r[24] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R25:            rsp->r[25] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R26:            rsp->r[26] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R27:            rsp->r[27] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R28:            rsp->r[28] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R29:            rsp->r[29] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_R30:            rsp->r[30] = info->i;        break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + RSP_R31:            rsp->r[31] = info->i;        break;
		case CPUINFO_INT_REGISTER + RSP_SR:             rsp->sr = info->i;           break;
		case CPUINFO_INT_REGISTER + RSP_NEXTPC:         rsp->nextpc = info->i;       break;
		case CPUINFO_INT_REGISTER + RSP_STEPCNT:        rsp->step_count = info->i;   break;
	}
}

CPU_GET_INFO( rsp_drc )
{
	rsp_state *rsp = (device != NULL && device->token() != NULL) ? get_safe_token(device) : NULL;

	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:                  info->i = sizeof(rsp_state);                    break;
		case CPUINFO_INT_INPUT_LINES:                   info->i = 1;                            break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:            info->i = 0;                            break;
		case CPUINFO_INT_ENDIANNESS:                    info->i = ENDIANNESS_BIG;               break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:              info->i = 1;                            break;
		case CPUINFO_INT_CLOCK_DIVIDER:                 info->i = 1;                            break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:         info->i = 4;                            break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:         info->i = 4;                            break;
		case CPUINFO_INT_MIN_CYCLES:                    info->i = 1;                            break;
		case CPUINFO_INT_MAX_CYCLES:                    info->i = 1;                            break;

		case CPUINFO_INT_DATABUS_WIDTH + AS_PROGRAM:    info->i = 32;                   break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_PROGRAM: info->i = 32;                  break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_PROGRAM: info->i = 0;                   break;
		case CPUINFO_INT_DATABUS_WIDTH + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_DATABUS_WIDTH + AS_IO:     info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_IO:     info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_IO:     info->i = 0;                    break;

		case CPUINFO_INT_INPUT_STATE:                   info->i = CLEAR_LINE;                   break;

		case CPUINFO_INT_PREVIOUSPC:                    info->i = rsp->ppc | 0x04000000;                        break;

		case CPUINFO_INT_PC:    /* intentional fallthrough */
		case CPUINFO_INT_REGISTER + RSP_PC:             info->i = rsp->pc | 0x04000000;                     break;

		case CPUINFO_INT_REGISTER + RSP_R0:             info->i = rsp->r[0];                        break;
		case CPUINFO_INT_REGISTER + RSP_R1:             info->i = rsp->r[1];                        break;
		case CPUINFO_INT_REGISTER + RSP_R2:             info->i = rsp->r[2];                        break;
		case CPUINFO_INT_REGISTER + RSP_R3:             info->i = rsp->r[3];                        break;
		case CPUINFO_INT_REGISTER + RSP_R4:             info->i = rsp->r[4];                        break;
		case CPUINFO_INT_REGISTER + RSP_R5:             info->i = rsp->r[5];                        break;
		case CPUINFO_INT_REGISTER + RSP_R6:             info->i = rsp->r[6];                        break;
		case CPUINFO_INT_REGISTER + RSP_R7:             info->i = rsp->r[7];                        break;
		case CPUINFO_INT_REGISTER + RSP_R8:             info->i = rsp->r[8];                        break;
		case CPUINFO_INT_REGISTER + RSP_R9:             info->i = rsp->r[9];                        break;
		case CPUINFO_INT_REGISTER + RSP_R10:            info->i = rsp->r[10];                   break;
		case CPUINFO_INT_REGISTER + RSP_R11:            info->i = rsp->r[11];                   break;
		case CPUINFO_INT_REGISTER + RSP_R12:            info->i = rsp->r[12];                   break;
		case CPUINFO_INT_REGISTER + RSP_R13:            info->i = rsp->r[13];                   break;
		case CPUINFO_INT_REGISTER + RSP_R14:            info->i = rsp->r[14];                   break;
		case CPUINFO_INT_REGISTER + RSP_R15:            info->i = rsp->r[15];                   break;
		case CPUINFO_INT_REGISTER + RSP_R16:            info->i = rsp->r[16];                   break;
		case CPUINFO_INT_REGISTER + RSP_R17:            info->i = rsp->r[17];                   break;
		case CPUINFO_INT_REGISTER + RSP_R18:            info->i = rsp->r[18];                   break;
		case CPUINFO_INT_REGISTER + RSP_R19:            info->i = rsp->r[19];                   break;
		case CPUINFO_INT_REGISTER + RSP_R20:            info->i = rsp->r[20];                   break;
		case CPUINFO_INT_REGISTER + RSP_R21:            info->i = rsp->r[21];                   break;
		case CPUINFO_INT_REGISTER + RSP_R22:            info->i = rsp->r[22];                   break;
		case CPUINFO_INT_REGISTER + RSP_R23:            info->i = rsp->r[23];                   break;
		case CPUINFO_INT_REGISTER + RSP_R24:            info->i = rsp->r[24];                   break;
		case CPUINFO_INT_REGISTER + RSP_R25:            info->i = rsp->r[25];                   break;
		case CPUINFO_INT_REGISTER + RSP_R26:            info->i = rsp->r[26];                   break;
		case CPUINFO_INT_REGISTER + RSP_R27:            info->i = rsp->r[27];                   break;
		case CPUINFO_INT_REGISTER + RSP_R28:            info->i = rsp->r[28];                   break;
		case CPUINFO_INT_REGISTER + RSP_R29:            info->i = rsp->r[29];                   break;
		case CPUINFO_INT_REGISTER + RSP_R30:            info->i = rsp->r[30];                   break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + RSP_R31:            info->i = rsp->r[31];                    break;
		case CPUINFO_INT_REGISTER + RSP_SR:             info->i = rsp->sr;                       break;
		case CPUINFO_INT_REGISTER + RSP_NEXTPC:         info->i = rsp->nextpc | 0x04000000;      break;
		case CPUINFO_INT_REGISTER + RSP_STEPCNT:        info->i = rsp->step_count;               break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_SET_INFO:                      info->setinfo = CPU_SET_INFO_NAME(rsp);         break;
		case CPUINFO_FCT_INIT:                          info->init = CPU_INIT_NAME(rsp);                    break;
		case CPUINFO_FCT_RESET:                         info->reset = CPU_RESET_NAME(rsp);              break;
		case CPUINFO_FCT_EXIT:                          info->exit = CPU_EXIT_NAME(rsp);                    break;
		case CPUINFO_FCT_EXECUTE:                       info->execute = CPU_EXECUTE_NAME(rsp);          break;
		case CPUINFO_FCT_BURN:                          info->burn = NULL;                      break;
		case CPUINFO_FCT_DISASSEMBLE:                   info->disassemble = CPU_DISASSEMBLE_NAME(rsp);          break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:           info->icount = &rsp->icount;                break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:                          strcpy(info->s, "RSP DRC");                 break;
		case CPUINFO_STR_SHORTNAME:                     strcpy(info->s, "rsp_drc");                 break;
		case CPUINFO_STR_FAMILY:                    strcpy(info->s, "RSP");                 break;
		case CPUINFO_STR_VERSION:                   strcpy(info->s, "1.0");                 break;
		case CPUINFO_STR_SOURCE_FILE:                       strcpy(info->s, __FILE__);              break;
		case CPUINFO_STR_CREDITS:                   strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;

		case CPUINFO_STR_FLAGS:                         strcpy(info->s, " ");                   break;

		case CPUINFO_STR_REGISTER + RSP_PC:             sprintf(info->s, "PC: %08X", rsp->pc | 0x04000000); break;

		case CPUINFO_STR_REGISTER + RSP_R0:             sprintf(info->s, "R0: %08X", rsp->r[0]); break;
		case CPUINFO_STR_REGISTER + RSP_R1:             sprintf(info->s, "R1: %08X", rsp->r[1]); break;
		case CPUINFO_STR_REGISTER + RSP_R2:             sprintf(info->s, "R2: %08X", rsp->r[2]); break;
		case CPUINFO_STR_REGISTER + RSP_R3:             sprintf(info->s, "R3: %08X", rsp->r[3]); break;
		case CPUINFO_STR_REGISTER + RSP_R4:             sprintf(info->s, "R4: %08X", rsp->r[4]); break;
		case CPUINFO_STR_REGISTER + RSP_R5:             sprintf(info->s, "R5: %08X", rsp->r[5]); break;
		case CPUINFO_STR_REGISTER + RSP_R6:             sprintf(info->s, "R6: %08X", rsp->r[6]); break;
		case CPUINFO_STR_REGISTER + RSP_R7:             sprintf(info->s, "R7: %08X", rsp->r[7]); break;
		case CPUINFO_STR_REGISTER + RSP_R8:             sprintf(info->s, "R8: %08X", rsp->r[8]); break;
		case CPUINFO_STR_REGISTER + RSP_R9:             sprintf(info->s, "R9: %08X", rsp->r[9]); break;
		case CPUINFO_STR_REGISTER + RSP_R10:            sprintf(info->s, "R10: %08X", rsp->r[10]); break;
		case CPUINFO_STR_REGISTER + RSP_R11:            sprintf(info->s, "R11: %08X", rsp->r[11]); break;
		case CPUINFO_STR_REGISTER + RSP_R12:            sprintf(info->s, "R12: %08X", rsp->r[12]); break;
		case CPUINFO_STR_REGISTER + RSP_R13:            sprintf(info->s, "R13: %08X", rsp->r[13]); break;
		case CPUINFO_STR_REGISTER + RSP_R14:            sprintf(info->s, "R14: %08X", rsp->r[14]); break;
		case CPUINFO_STR_REGISTER + RSP_R15:            sprintf(info->s, "R15: %08X", rsp->r[15]); break;
		case CPUINFO_STR_REGISTER + RSP_R16:            sprintf(info->s, "R16: %08X", rsp->r[16]); break;
		case CPUINFO_STR_REGISTER + RSP_R17:            sprintf(info->s, "R17: %08X", rsp->r[17]); break;
		case CPUINFO_STR_REGISTER + RSP_R18:            sprintf(info->s, "R18: %08X", rsp->r[18]); break;
		case CPUINFO_STR_REGISTER + RSP_R19:            sprintf(info->s, "R19: %08X", rsp->r[19]); break;
		case CPUINFO_STR_REGISTER + RSP_R20:            sprintf(info->s, "R20: %08X", rsp->r[20]); break;
		case CPUINFO_STR_REGISTER + RSP_R21:            sprintf(info->s, "R21: %08X", rsp->r[21]); break;
		case CPUINFO_STR_REGISTER + RSP_R22:            sprintf(info->s, "R22: %08X", rsp->r[22]); break;
		case CPUINFO_STR_REGISTER + RSP_R23:            sprintf(info->s, "R23: %08X", rsp->r[23]); break;
		case CPUINFO_STR_REGISTER + RSP_R24:            sprintf(info->s, "R24: %08X", rsp->r[24]); break;
		case CPUINFO_STR_REGISTER + RSP_R25:            sprintf(info->s, "R25: %08X", rsp->r[25]); break;
		case CPUINFO_STR_REGISTER + RSP_R26:            sprintf(info->s, "R26: %08X", rsp->r[26]); break;
		case CPUINFO_STR_REGISTER + RSP_R27:            sprintf(info->s, "R27: %08X", rsp->r[27]); break;
		case CPUINFO_STR_REGISTER + RSP_R28:            sprintf(info->s, "R28: %08X", rsp->r[28]); break;
		case CPUINFO_STR_REGISTER + RSP_R29:            sprintf(info->s, "R29: %08X", rsp->r[29]); break;
		case CPUINFO_STR_REGISTER + RSP_R30:            sprintf(info->s, "R30: %08X", rsp->r[30]); break;
		case CPUINFO_STR_REGISTER + RSP_R31:            sprintf(info->s, "R31: %08X", rsp->r[31]); break;

#if USE_SIMD
		case CPUINFO_STR_REGISTER + RSP_V0:             sprintf(info->s, "V0: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 0], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 0], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V1:             sprintf(info->s, "V1: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 1], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 1], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V2:             sprintf(info->s, "V2: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 2], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 2], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V3:             sprintf(info->s, "V3: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 3], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 3], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V4:             sprintf(info->s, "V4: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 4], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 4], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V5:             sprintf(info->s, "V5: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 5], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 5], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V6:             sprintf(info->s, "V6: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 6], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 6], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V7:             sprintf(info->s, "V7: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 7], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 7], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V8:             sprintf(info->s, "V8: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 8], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 8], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V9:             sprintf(info->s, "V9: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)_mm_extract_epi16(rsp->xv[ 9], 7), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 6), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 5), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 4), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 3), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 2), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 1), (UINT16)_mm_extract_epi16(rsp->xv[ 9], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V10:            sprintf(info->s, "V10: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[10], 7), (UINT16)_mm_extract_epi16(rsp->xv[10], 6), (UINT16)_mm_extract_epi16(rsp->xv[10], 5), (UINT16)_mm_extract_epi16(rsp->xv[10], 4), (UINT16)_mm_extract_epi16(rsp->xv[10], 3), (UINT16)_mm_extract_epi16(rsp->xv[10], 2), (UINT16)_mm_extract_epi16(rsp->xv[10], 1), (UINT16)_mm_extract_epi16(rsp->xv[10], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V11:            sprintf(info->s, "V11: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[11], 7), (UINT16)_mm_extract_epi16(rsp->xv[11], 6), (UINT16)_mm_extract_epi16(rsp->xv[11], 5), (UINT16)_mm_extract_epi16(rsp->xv[11], 4), (UINT16)_mm_extract_epi16(rsp->xv[11], 3), (UINT16)_mm_extract_epi16(rsp->xv[11], 2), (UINT16)_mm_extract_epi16(rsp->xv[11], 1), (UINT16)_mm_extract_epi16(rsp->xv[11], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V12:            sprintf(info->s, "V12: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[12], 7), (UINT16)_mm_extract_epi16(rsp->xv[12], 6), (UINT16)_mm_extract_epi16(rsp->xv[12], 5), (UINT16)_mm_extract_epi16(rsp->xv[12], 4), (UINT16)_mm_extract_epi16(rsp->xv[12], 3), (UINT16)_mm_extract_epi16(rsp->xv[12], 2), (UINT16)_mm_extract_epi16(rsp->xv[12], 1), (UINT16)_mm_extract_epi16(rsp->xv[12], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V13:            sprintf(info->s, "V13: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[13], 7), (UINT16)_mm_extract_epi16(rsp->xv[13], 6), (UINT16)_mm_extract_epi16(rsp->xv[13], 5), (UINT16)_mm_extract_epi16(rsp->xv[13], 4), (UINT16)_mm_extract_epi16(rsp->xv[13], 3), (UINT16)_mm_extract_epi16(rsp->xv[13], 2), (UINT16)_mm_extract_epi16(rsp->xv[13], 1), (UINT16)_mm_extract_epi16(rsp->xv[13], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V14:            sprintf(info->s, "V14: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[14], 7), (UINT16)_mm_extract_epi16(rsp->xv[14], 6), (UINT16)_mm_extract_epi16(rsp->xv[14], 5), (UINT16)_mm_extract_epi16(rsp->xv[14], 4), (UINT16)_mm_extract_epi16(rsp->xv[14], 3), (UINT16)_mm_extract_epi16(rsp->xv[14], 2), (UINT16)_mm_extract_epi16(rsp->xv[14], 1), (UINT16)_mm_extract_epi16(rsp->xv[14], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V15:            sprintf(info->s, "V15: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[15], 7), (UINT16)_mm_extract_epi16(rsp->xv[15], 6), (UINT16)_mm_extract_epi16(rsp->xv[15], 5), (UINT16)_mm_extract_epi16(rsp->xv[15], 4), (UINT16)_mm_extract_epi16(rsp->xv[15], 3), (UINT16)_mm_extract_epi16(rsp->xv[15], 2), (UINT16)_mm_extract_epi16(rsp->xv[15], 1), (UINT16)_mm_extract_epi16(rsp->xv[15], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V16:            sprintf(info->s, "V16: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[16], 7), (UINT16)_mm_extract_epi16(rsp->xv[16], 6), (UINT16)_mm_extract_epi16(rsp->xv[16], 5), (UINT16)_mm_extract_epi16(rsp->xv[16], 4), (UINT16)_mm_extract_epi16(rsp->xv[16], 3), (UINT16)_mm_extract_epi16(rsp->xv[16], 2), (UINT16)_mm_extract_epi16(rsp->xv[16], 1), (UINT16)_mm_extract_epi16(rsp->xv[16], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V17:            sprintf(info->s, "V17: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[17], 7), (UINT16)_mm_extract_epi16(rsp->xv[17], 6), (UINT16)_mm_extract_epi16(rsp->xv[17], 5), (UINT16)_mm_extract_epi16(rsp->xv[17], 4), (UINT16)_mm_extract_epi16(rsp->xv[17], 3), (UINT16)_mm_extract_epi16(rsp->xv[17], 2), (UINT16)_mm_extract_epi16(rsp->xv[17], 1), (UINT16)_mm_extract_epi16(rsp->xv[17], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V18:            sprintf(info->s, "V18: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[18], 7), (UINT16)_mm_extract_epi16(rsp->xv[18], 6), (UINT16)_mm_extract_epi16(rsp->xv[18], 5), (UINT16)_mm_extract_epi16(rsp->xv[18], 4), (UINT16)_mm_extract_epi16(rsp->xv[18], 3), (UINT16)_mm_extract_epi16(rsp->xv[18], 2), (UINT16)_mm_extract_epi16(rsp->xv[18], 1), (UINT16)_mm_extract_epi16(rsp->xv[18], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V19:            sprintf(info->s, "V19: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[19], 7), (UINT16)_mm_extract_epi16(rsp->xv[19], 6), (UINT16)_mm_extract_epi16(rsp->xv[19], 5), (UINT16)_mm_extract_epi16(rsp->xv[19], 4), (UINT16)_mm_extract_epi16(rsp->xv[19], 3), (UINT16)_mm_extract_epi16(rsp->xv[19], 2), (UINT16)_mm_extract_epi16(rsp->xv[19], 1), (UINT16)_mm_extract_epi16(rsp->xv[19], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V20:            sprintf(info->s, "V20: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[20], 7), (UINT16)_mm_extract_epi16(rsp->xv[20], 6), (UINT16)_mm_extract_epi16(rsp->xv[20], 5), (UINT16)_mm_extract_epi16(rsp->xv[20], 4), (UINT16)_mm_extract_epi16(rsp->xv[20], 3), (UINT16)_mm_extract_epi16(rsp->xv[20], 2), (UINT16)_mm_extract_epi16(rsp->xv[20], 1), (UINT16)_mm_extract_epi16(rsp->xv[20], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V21:            sprintf(info->s, "V21: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[21], 7), (UINT16)_mm_extract_epi16(rsp->xv[21], 6), (UINT16)_mm_extract_epi16(rsp->xv[21], 5), (UINT16)_mm_extract_epi16(rsp->xv[21], 4), (UINT16)_mm_extract_epi16(rsp->xv[21], 3), (UINT16)_mm_extract_epi16(rsp->xv[21], 2), (UINT16)_mm_extract_epi16(rsp->xv[21], 1), (UINT16)_mm_extract_epi16(rsp->xv[21], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V22:            sprintf(info->s, "V22: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[22], 7), (UINT16)_mm_extract_epi16(rsp->xv[22], 6), (UINT16)_mm_extract_epi16(rsp->xv[22], 5), (UINT16)_mm_extract_epi16(rsp->xv[22], 4), (UINT16)_mm_extract_epi16(rsp->xv[22], 3), (UINT16)_mm_extract_epi16(rsp->xv[22], 2), (UINT16)_mm_extract_epi16(rsp->xv[22], 1), (UINT16)_mm_extract_epi16(rsp->xv[22], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V23:            sprintf(info->s, "V23: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[23], 7), (UINT16)_mm_extract_epi16(rsp->xv[23], 6), (UINT16)_mm_extract_epi16(rsp->xv[23], 5), (UINT16)_mm_extract_epi16(rsp->xv[23], 4), (UINT16)_mm_extract_epi16(rsp->xv[23], 3), (UINT16)_mm_extract_epi16(rsp->xv[23], 2), (UINT16)_mm_extract_epi16(rsp->xv[23], 1), (UINT16)_mm_extract_epi16(rsp->xv[23], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V24:            sprintf(info->s, "V24: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[24], 7), (UINT16)_mm_extract_epi16(rsp->xv[24], 6), (UINT16)_mm_extract_epi16(rsp->xv[24], 5), (UINT16)_mm_extract_epi16(rsp->xv[24], 4), (UINT16)_mm_extract_epi16(rsp->xv[24], 3), (UINT16)_mm_extract_epi16(rsp->xv[24], 2), (UINT16)_mm_extract_epi16(rsp->xv[24], 1), (UINT16)_mm_extract_epi16(rsp->xv[24], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V25:            sprintf(info->s, "V25: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[25], 7), (UINT16)_mm_extract_epi16(rsp->xv[25], 6), (UINT16)_mm_extract_epi16(rsp->xv[25], 5), (UINT16)_mm_extract_epi16(rsp->xv[25], 4), (UINT16)_mm_extract_epi16(rsp->xv[25], 3), (UINT16)_mm_extract_epi16(rsp->xv[25], 2), (UINT16)_mm_extract_epi16(rsp->xv[25], 1), (UINT16)_mm_extract_epi16(rsp->xv[25], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V26:            sprintf(info->s, "V26: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[26], 7), (UINT16)_mm_extract_epi16(rsp->xv[26], 6), (UINT16)_mm_extract_epi16(rsp->xv[26], 5), (UINT16)_mm_extract_epi16(rsp->xv[26], 4), (UINT16)_mm_extract_epi16(rsp->xv[26], 3), (UINT16)_mm_extract_epi16(rsp->xv[26], 2), (UINT16)_mm_extract_epi16(rsp->xv[26], 1), (UINT16)_mm_extract_epi16(rsp->xv[26], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V27:            sprintf(info->s, "V27: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[27], 7), (UINT16)_mm_extract_epi16(rsp->xv[27], 6), (UINT16)_mm_extract_epi16(rsp->xv[27], 5), (UINT16)_mm_extract_epi16(rsp->xv[27], 4), (UINT16)_mm_extract_epi16(rsp->xv[27], 3), (UINT16)_mm_extract_epi16(rsp->xv[27], 2), (UINT16)_mm_extract_epi16(rsp->xv[27], 1), (UINT16)_mm_extract_epi16(rsp->xv[27], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V28:            sprintf(info->s, "V28: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[28], 7), (UINT16)_mm_extract_epi16(rsp->xv[28], 6), (UINT16)_mm_extract_epi16(rsp->xv[28], 5), (UINT16)_mm_extract_epi16(rsp->xv[28], 4), (UINT16)_mm_extract_epi16(rsp->xv[28], 3), (UINT16)_mm_extract_epi16(rsp->xv[28], 2), (UINT16)_mm_extract_epi16(rsp->xv[28], 1), (UINT16)_mm_extract_epi16(rsp->xv[28], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V29:            sprintf(info->s, "V29: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[29], 7), (UINT16)_mm_extract_epi16(rsp->xv[29], 6), (UINT16)_mm_extract_epi16(rsp->xv[29], 5), (UINT16)_mm_extract_epi16(rsp->xv[29], 4), (UINT16)_mm_extract_epi16(rsp->xv[29], 3), (UINT16)_mm_extract_epi16(rsp->xv[29], 2), (UINT16)_mm_extract_epi16(rsp->xv[29], 1), (UINT16)_mm_extract_epi16(rsp->xv[29], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V30:            sprintf(info->s, "V30: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[30], 7), (UINT16)_mm_extract_epi16(rsp->xv[30], 6), (UINT16)_mm_extract_epi16(rsp->xv[30], 5), (UINT16)_mm_extract_epi16(rsp->xv[30], 4), (UINT16)_mm_extract_epi16(rsp->xv[30], 3), (UINT16)_mm_extract_epi16(rsp->xv[30], 2), (UINT16)_mm_extract_epi16(rsp->xv[30], 1), (UINT16)_mm_extract_epi16(rsp->xv[30], 0)); break;
		case CPUINFO_STR_REGISTER + RSP_V31:            sprintf(info->s, "V31: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)_mm_extract_epi16(rsp->xv[31], 7), (UINT16)_mm_extract_epi16(rsp->xv[31], 6), (UINT16)_mm_extract_epi16(rsp->xv[31], 5), (UINT16)_mm_extract_epi16(rsp->xv[31], 4), (UINT16)_mm_extract_epi16(rsp->xv[31], 3), (UINT16)_mm_extract_epi16(rsp->xv[31], 2), (UINT16)_mm_extract_epi16(rsp->xv[31], 1), (UINT16)_mm_extract_epi16(rsp->xv[31], 0)); break;
#else
		case CPUINFO_STR_REGISTER + RSP_V0:             sprintf(info->s, "V0: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 0, 0), (UINT16)VREG_S( 0, 1), (UINT16)VREG_S( 0, 2), (UINT16)VREG_S( 0, 3), (UINT16)VREG_S( 0, 4), (UINT16)VREG_S( 0, 5), (UINT16)VREG_S( 0, 6), (UINT16)VREG_S( 0, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V1:             sprintf(info->s, "V1: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 1, 0), (UINT16)VREG_S( 1, 1), (UINT16)VREG_S( 1, 2), (UINT16)VREG_S( 1, 3), (UINT16)VREG_S( 1, 4), (UINT16)VREG_S( 1, 5), (UINT16)VREG_S( 1, 6), (UINT16)VREG_S( 1, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V2:             sprintf(info->s, "V2: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 2, 0), (UINT16)VREG_S( 2, 1), (UINT16)VREG_S( 2, 2), (UINT16)VREG_S( 2, 3), (UINT16)VREG_S( 2, 4), (UINT16)VREG_S( 2, 5), (UINT16)VREG_S( 2, 6), (UINT16)VREG_S( 2, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V3:             sprintf(info->s, "V3: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 3, 0), (UINT16)VREG_S( 3, 1), (UINT16)VREG_S( 3, 2), (UINT16)VREG_S( 3, 3), (UINT16)VREG_S( 3, 4), (UINT16)VREG_S( 3, 5), (UINT16)VREG_S( 3, 6), (UINT16)VREG_S( 3, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V4:             sprintf(info->s, "V4: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 4, 0), (UINT16)VREG_S( 4, 1), (UINT16)VREG_S( 4, 2), (UINT16)VREG_S( 4, 3), (UINT16)VREG_S( 4, 4), (UINT16)VREG_S( 4, 5), (UINT16)VREG_S( 4, 6), (UINT16)VREG_S( 4, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V5:             sprintf(info->s, "V5: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 5, 0), (UINT16)VREG_S( 5, 1), (UINT16)VREG_S( 5, 2), (UINT16)VREG_S( 5, 3), (UINT16)VREG_S( 5, 4), (UINT16)VREG_S( 5, 5), (UINT16)VREG_S( 5, 6), (UINT16)VREG_S( 5, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V6:             sprintf(info->s, "V6: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 6, 0), (UINT16)VREG_S( 6, 1), (UINT16)VREG_S( 6, 2), (UINT16)VREG_S( 6, 3), (UINT16)VREG_S( 6, 4), (UINT16)VREG_S( 6, 5), (UINT16)VREG_S( 6, 6), (UINT16)VREG_S( 6, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V7:             sprintf(info->s, "V7: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 7, 0), (UINT16)VREG_S( 7, 1), (UINT16)VREG_S( 7, 2), (UINT16)VREG_S( 7, 3), (UINT16)VREG_S( 7, 4), (UINT16)VREG_S( 7, 5), (UINT16)VREG_S( 7, 6), (UINT16)VREG_S( 7, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V8:             sprintf(info->s, "V8: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 8, 0), (UINT16)VREG_S( 8, 1), (UINT16)VREG_S( 8, 2), (UINT16)VREG_S( 8, 3), (UINT16)VREG_S( 8, 4), (UINT16)VREG_S( 8, 5), (UINT16)VREG_S( 8, 6), (UINT16)VREG_S( 8, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V9:             sprintf(info->s, "V9: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X",  (UINT16)VREG_S( 9, 0), (UINT16)VREG_S( 9, 1), (UINT16)VREG_S( 9, 2), (UINT16)VREG_S( 9, 3), (UINT16)VREG_S( 9, 4), (UINT16)VREG_S( 9, 5), (UINT16)VREG_S( 9, 6), (UINT16)VREG_S( 9, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V10:            sprintf(info->s, "V10: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(10, 0), (UINT16)VREG_S(10, 1), (UINT16)VREG_S(10, 2), (UINT16)VREG_S(10, 3), (UINT16)VREG_S(10, 4), (UINT16)VREG_S(10, 5), (UINT16)VREG_S(10, 6), (UINT16)VREG_S(10, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V11:            sprintf(info->s, "V11: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(11, 0), (UINT16)VREG_S(11, 1), (UINT16)VREG_S(11, 2), (UINT16)VREG_S(11, 3), (UINT16)VREG_S(11, 4), (UINT16)VREG_S(11, 5), (UINT16)VREG_S(11, 6), (UINT16)VREG_S(11, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V12:            sprintf(info->s, "V12: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(12, 0), (UINT16)VREG_S(12, 1), (UINT16)VREG_S(12, 2), (UINT16)VREG_S(12, 3), (UINT16)VREG_S(12, 4), (UINT16)VREG_S(12, 5), (UINT16)VREG_S(12, 6), (UINT16)VREG_S(12, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V13:            sprintf(info->s, "V13: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(13, 0), (UINT16)VREG_S(13, 1), (UINT16)VREG_S(13, 2), (UINT16)VREG_S(13, 3), (UINT16)VREG_S(13, 4), (UINT16)VREG_S(13, 5), (UINT16)VREG_S(13, 6), (UINT16)VREG_S(13, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V14:            sprintf(info->s, "V14: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(14, 0), (UINT16)VREG_S(14, 1), (UINT16)VREG_S(14, 2), (UINT16)VREG_S(14, 3), (UINT16)VREG_S(14, 4), (UINT16)VREG_S(14, 5), (UINT16)VREG_S(14, 6), (UINT16)VREG_S(14, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V15:            sprintf(info->s, "V15: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(15, 0), (UINT16)VREG_S(15, 1), (UINT16)VREG_S(15, 2), (UINT16)VREG_S(15, 3), (UINT16)VREG_S(15, 4), (UINT16)VREG_S(15, 5), (UINT16)VREG_S(15, 6), (UINT16)VREG_S(15, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V16:            sprintf(info->s, "V16: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(16, 0), (UINT16)VREG_S(16, 1), (UINT16)VREG_S(16, 2), (UINT16)VREG_S(16, 3), (UINT16)VREG_S(16, 4), (UINT16)VREG_S(16, 5), (UINT16)VREG_S(16, 6), (UINT16)VREG_S(16, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V17:            sprintf(info->s, "V17: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(17, 0), (UINT16)VREG_S(17, 1), (UINT16)VREG_S(17, 2), (UINT16)VREG_S(17, 3), (UINT16)VREG_S(17, 4), (UINT16)VREG_S(17, 5), (UINT16)VREG_S(17, 6), (UINT16)VREG_S(17, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V18:            sprintf(info->s, "V18: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(18, 0), (UINT16)VREG_S(18, 1), (UINT16)VREG_S(18, 2), (UINT16)VREG_S(18, 3), (UINT16)VREG_S(18, 4), (UINT16)VREG_S(18, 5), (UINT16)VREG_S(18, 6), (UINT16)VREG_S(18, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V19:            sprintf(info->s, "V19: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(19, 0), (UINT16)VREG_S(19, 1), (UINT16)VREG_S(19, 2), (UINT16)VREG_S(19, 3), (UINT16)VREG_S(19, 4), (UINT16)VREG_S(19, 5), (UINT16)VREG_S(19, 6), (UINT16)VREG_S(19, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V20:            sprintf(info->s, "V20: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(20, 0), (UINT16)VREG_S(20, 1), (UINT16)VREG_S(20, 2), (UINT16)VREG_S(20, 3), (UINT16)VREG_S(20, 4), (UINT16)VREG_S(20, 5), (UINT16)VREG_S(20, 6), (UINT16)VREG_S(20, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V21:            sprintf(info->s, "V21: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(21, 0), (UINT16)VREG_S(21, 1), (UINT16)VREG_S(21, 2), (UINT16)VREG_S(21, 3), (UINT16)VREG_S(21, 4), (UINT16)VREG_S(21, 5), (UINT16)VREG_S(21, 6), (UINT16)VREG_S(21, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V22:            sprintf(info->s, "V22: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(22, 0), (UINT16)VREG_S(22, 1), (UINT16)VREG_S(22, 2), (UINT16)VREG_S(22, 3), (UINT16)VREG_S(22, 4), (UINT16)VREG_S(22, 5), (UINT16)VREG_S(22, 6), (UINT16)VREG_S(22, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V23:            sprintf(info->s, "V23: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(23, 0), (UINT16)VREG_S(23, 1), (UINT16)VREG_S(23, 2), (UINT16)VREG_S(23, 3), (UINT16)VREG_S(23, 4), (UINT16)VREG_S(23, 5), (UINT16)VREG_S(23, 6), (UINT16)VREG_S(23, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V24:            sprintf(info->s, "V24: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(24, 0), (UINT16)VREG_S(24, 1), (UINT16)VREG_S(24, 2), (UINT16)VREG_S(24, 3), (UINT16)VREG_S(24, 4), (UINT16)VREG_S(24, 5), (UINT16)VREG_S(24, 6), (UINT16)VREG_S(24, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V25:            sprintf(info->s, "V25: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(25, 0), (UINT16)VREG_S(25, 1), (UINT16)VREG_S(25, 2), (UINT16)VREG_S(25, 3), (UINT16)VREG_S(25, 4), (UINT16)VREG_S(25, 5), (UINT16)VREG_S(25, 6), (UINT16)VREG_S(25, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V26:            sprintf(info->s, "V26: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(26, 0), (UINT16)VREG_S(26, 1), (UINT16)VREG_S(26, 2), (UINT16)VREG_S(26, 3), (UINT16)VREG_S(26, 4), (UINT16)VREG_S(26, 5), (UINT16)VREG_S(26, 6), (UINT16)VREG_S(26, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V27:            sprintf(info->s, "V27: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(27, 0), (UINT16)VREG_S(27, 1), (UINT16)VREG_S(27, 2), (UINT16)VREG_S(27, 3), (UINT16)VREG_S(27, 4), (UINT16)VREG_S(27, 5), (UINT16)VREG_S(27, 6), (UINT16)VREG_S(27, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V28:            sprintf(info->s, "V28: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(28, 0), (UINT16)VREG_S(28, 1), (UINT16)VREG_S(28, 2), (UINT16)VREG_S(28, 3), (UINT16)VREG_S(28, 4), (UINT16)VREG_S(28, 5), (UINT16)VREG_S(28, 6), (UINT16)VREG_S(28, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V29:            sprintf(info->s, "V29: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(29, 0), (UINT16)VREG_S(29, 1), (UINT16)VREG_S(29, 2), (UINT16)VREG_S(29, 3), (UINT16)VREG_S(29, 4), (UINT16)VREG_S(29, 5), (UINT16)VREG_S(29, 6), (UINT16)VREG_S(29, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V30:            sprintf(info->s, "V30: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(30, 0), (UINT16)VREG_S(30, 1), (UINT16)VREG_S(30, 2), (UINT16)VREG_S(30, 3), (UINT16)VREG_S(30, 4), (UINT16)VREG_S(30, 5), (UINT16)VREG_S(30, 6), (UINT16)VREG_S(30, 7)); break;
		case CPUINFO_STR_REGISTER + RSP_V31:            sprintf(info->s, "V31: %04X|%04X|%04X|%04X|%04X|%04X|%04X|%04X", (UINT16)VREG_S(31, 0), (UINT16)VREG_S(31, 1), (UINT16)VREG_S(31, 2), (UINT16)VREG_S(31, 3), (UINT16)VREG_S(31, 4), (UINT16)VREG_S(31, 5), (UINT16)VREG_S(31, 6), (UINT16)VREG_S(31, 7)); break;
#endif
		case CPUINFO_STR_REGISTER + RSP_SR:             sprintf(info->s, "SR: %08X",  rsp->sr);    break;
		case CPUINFO_STR_REGISTER + RSP_NEXTPC:         sprintf(info->s, "NPC: %08X", rsp->nextpc);break;
		case CPUINFO_STR_REGISTER + RSP_STEPCNT:        sprintf(info->s, "STEP: %d",  rsp->step_count);  break;
	}
}

DEFINE_LEGACY_CPU_DEVICE(RSP_DRC, rsp_drc);
