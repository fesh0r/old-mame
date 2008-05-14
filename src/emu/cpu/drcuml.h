/***************************************************************************

    drcuml.h

    Universal machine language for dynamic recompiling CPU cores.

    Copyright Aaron Giles
    Released for general non-commercial use under the MAME license
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __DRCUML_H__
#define __DRCUML_H__

#include "memory.h"
#include "drccache.h"
#include <setjmp.h>


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* these options are passed into drcuml_alloc() and control global behaviors */
#define DRCUML_OPTION_USE_C					0x0001		/* always use the C back-end */
#define DRCUML_OPTION_LOG_UML				0x0002		/* generate a UML disassembly of each block */
#define DRCUML_OPTION_LOG_NATIVE			0x0004		/* tell the back-end to generate a native disassembly of each block */


/* opcode parameter types */
enum _drcuml_ptype
{
	DRCUML_PTYPE_NONE = 0,
	DRCUML_PTYPE_IMMEDIATE,
	DRCUML_PTYPE_INT_REGISTER,
	DRCUML_PTYPE_FLOAT_REGISTER,
	DRCUML_PTYPE_MAPVAR,
	DRCUML_PTYPE_MEMORY,
	DRCUML_PTYPE_MAX
};
typedef enum _drcuml_ptype drcuml_ptype;


/* these define the registers for the UML */
enum
{
	DRCUML_REG_INVALID = 0,	/* 0 is invalid */

	/* integer registers */
	DRCUML_REG_I0 = 0x400,
	DRCUML_REG_I1,
	DRCUML_REG_I2,
	DRCUML_REG_I3,
	DRCUML_REG_I4,
	DRCUML_REG_I5,
	DRCUML_REG_I6,
	DRCUML_REG_I7,
	DRCUML_REG_I8,
	DRCUML_REG_I9,
	DRCUML_REG_I_END,

	/* floating point registers */
	DRCUML_REG_F0 = 0x800,
	DRCUML_REG_F1,
	DRCUML_REG_F2,
	DRCUML_REG_F3,
	DRCUML_REG_F4,
	DRCUML_REG_F5,
	DRCUML_REG_F6,
	DRCUML_REG_F7,
	DRCUML_REG_F8,
	DRCUML_REG_F9,
	DRCUML_REG_F_END,

	/* map variables */
	DRCUML_MAPVAR_M0 = 0xc00,
	DRCUML_MAPVAR_M1,
	DRCUML_MAPVAR_M2,
	DRCUML_MAPVAR_M3,
	DRCUML_MAPVAR_M4,
	DRCUML_MAPVAR_M5,
	DRCUML_MAPVAR_M6,
	DRCUML_MAPVAR_M7,
	DRCUML_MAPVAR_M8,
	DRCUML_MAPVAR_M9,
	DRCUML_MAPVAR_END
};


/* UML flag definitions */
enum
{
	DRCUML_FLAG_C = 1,					/* carry flag */
	DRCUML_FLAG_V = 2,					/* overflow flag (defined for integer only) */
	DRCUML_FLAG_Z = 4,					/* zero flag */
	DRCUML_FLAG_S = 8,					/* sign flag (defined for integer only) */
	DRCUML_FLAG_U = 16					/* unordered flag (defined for FP only) */
};


/* these define the conditions for the UML */
/* note that these are defined such that (condition ^ 1) is always the opposite */
/* they are also defined so as not to conflict with the flag bits above */
enum
{
	DRCUML_COND_ALWAYS = 0,

	DRCUML_COND_Z = 0x80,	/* requires Z */
	DRCUML_COND_NZ,			/* requires Z */
	DRCUML_COND_S,			/* requires S */
	DRCUML_COND_NS,			/* requires S */
	DRCUML_COND_C,			/* requires C */
	DRCUML_COND_NC,			/* requires C */
	DRCUML_COND_V,			/* requires V */
	DRCUML_COND_NV,			/* requires V */
	DRCUML_COND_U,			/* requires U */
	DRCUML_COND_NU,			/* requires U */
	DRCUML_COND_A,			/* requires CZ */
	DRCUML_COND_BE,			/* requires CZ */
	DRCUML_COND_G,			/* requires SVZ */
	DRCUML_COND_LE,			/* requires SVZ */
	DRCUML_COND_L,			/* requires SV */
	DRCUML_COND_GE,			/* requires SV */

	DRCUML_COND_MAX
};


/* basic condition code aliases */
enum
{
	DRCUML_COND_E = DRCUML_COND_Z,
	DRCUML_COND_NE = DRCUML_COND_NZ,
	DRCUML_COND_B = DRCUML_COND_C,
	DRCUML_COND_AE = DRCUML_COND_NC
};


/* floating point modes */
enum
{
	DRCUML_FMOD_TRUNC,		/* truncate */
	DRCUML_FMOD_ROUND,		/* round */
	DRCUML_FMOD_CEIL,		/* round up */
	DRCUML_FMOD_FLOOR		/* round down */
};


/* these define the opcodes for the UML */
enum _drcuml_opcode
{
	DRCUML_OP_INVALID,

	/* Compile-time opcodes */
	DRCUML_OP_HANDLE,		/* HANDLE  handle                 */
	DRCUML_OP_HASH,			/* HASH    mode,pc                */
	DRCUML_OP_LABEL,		/* LABEL   imm                    */
	DRCUML_OP_COMMENT,		/* COMMENT string                 */
	DRCUML_OP_MAPVAR,		/* MAPVAR  mapvar,value           */

	/* Control Flow Operations */
	DRCUML_OP_DEBUG,		/* DEBUG   pc                     */
	DRCUML_OP_EXIT,			/* EXIT    src1[,c]               */
	DRCUML_OP_HASHJMP,		/* HASHJMP mode,pc,handle         */
	DRCUML_OP_JMP,			/* JMP     imm[,c]                */
	DRCUML_OP_EXH,			/* EXH     handle,param[,c]       */
	DRCUML_OP_CALLH,		/* CALLH   handle[,c]             */
	DRCUML_OP_RET,			/* RET     [c]                    */
	DRCUML_OP_CALLC,		/* CALLC   func,ptr[,c]           */
	DRCUML_OP_RECOVER,		/* RECOVER dst,mapvar             */

	/* Internal Register Operations */
	DRCUML_OP_SETFMOD,		/* SETFMOD src                    */
	DRCUML_OP_GETFMOD,		/* GETFMOD dst                    */
	DRCUML_OP_GETEXP,		/* GETEXP  dst,index              */

	/* Integer Operations */
	DRCUML_OP_LOAD1U,		/* LOAD1U  dst,base,index         */
	DRCUML_OP_LOAD1S,		/* LOAD1S  dst,base,index         */
	DRCUML_OP_LOAD2U,		/* LOAD2U  dst,base,index         */
	DRCUML_OP_LOAD2S,		/* LOAD2S  dst,base,index         */
	DRCUML_OP_LOAD4U,		/* LOAD4U  dst,base,index         */
	DRCUML_OP_LOAD4S,		/* LOAD4S  dst,base,index         */
	DRCUML_OP_LOAD8U,		/* LOAD8U  dst,base,index         */
	DRCUML_OP_STORE1,		/* STORE1  base,index,src         */
	DRCUML_OP_STORE2,		/* STORE2  base,index,src         */
	DRCUML_OP_STORE4,		/* STORE4  base,index,src         */
	DRCUML_OP_STORE8,		/* STORE8  base,index,src         */
	DRCUML_OP_READ1U,		/* READ1U  dst,space,src1         */
	DRCUML_OP_READ1S,		/* READ1S  dst,space,src1         */
	DRCUML_OP_READ2U,		/* READ2U  dst,space,src1         */
	DRCUML_OP_READ2S,		/* READ2S  dst,space,src1         */
	DRCUML_OP_READ2M,		/* READ2M  dst,space,src1,mask    */
	DRCUML_OP_READ4U,		/* READ4U  dst,space,src1         */
	DRCUML_OP_READ4S,		/* READ4S  dst,space,src1         */
	DRCUML_OP_READ4M,		/* READ4M  dst,space,src1,mask    */
	DRCUML_OP_READ8U,		/* READ8U  dst,space,src1         */
	DRCUML_OP_READ8M,		/* READ8M  dst,space,src1,mask    */
	DRCUML_OP_WRITE1,		/* WRITE1  space,dst,src1         */
	DRCUML_OP_WRITE2,		/* WRITE2  space,dst,src1         */
	DRCUML_OP_WRIT2M,		/* WRIT2M  space,dst,mask,src1    */
	DRCUML_OP_WRITE4,		/* WRITE4  space,dst,src1         */
	DRCUML_OP_WRIT4M,		/* WRIT4M  space,dst,mask,src1    */
	DRCUML_OP_WRITE8,		/* WRITE8  space,dst,src1         */
	DRCUML_OP_WRIT8M,		/* WRIT8M  space,dst,mask,src1    */
	DRCUML_OP_FLAGS,		/* FLAGS   dst,mask,table         */
	DRCUML_OP_MOV,			/* MOV     dst,src[,c]            */
	DRCUML_OP_ZEXT1,		/* ZEXT1   dst,src                */
	DRCUML_OP_ZEXT2,		/* ZEXT2   dst,src                */
	DRCUML_OP_ZEXT4,		/* ZEXT4   dst,src                */
	DRCUML_OP_SEXT1,		/* SEXT1   dst,src                */
	DRCUML_OP_SEXT2,		/* SEXT2   dst,src                */
	DRCUML_OP_SEXT4,		/* SEXT4   dst,src                */
	DRCUML_OP_ADD,			/* ADD     dst,src1,src2[,f]      */
	DRCUML_OP_ADDC,			/* ADDC    dst,src1,src2[,f]      */
	DRCUML_OP_SUB,			/* SUB     dst,src1,src2[,f]      */
	DRCUML_OP_SUBB,			/* SUBB    dst,src1,src2[,f]      */
	DRCUML_OP_CMP,			/* CMP     src1,src2[,f]          */
	DRCUML_OP_MULU,			/* MULU    dst,edst,src1,src2[,f] */
	DRCUML_OP_MULS,			/* MULS    dst,edst,src1,src2[,f] */
	DRCUML_OP_DIVU,			/* DIVU    dst,edst,src1,src2[,f] */
	DRCUML_OP_DIVS,			/* DIVS    dst,edst,src1,src2[,f] */
	DRCUML_OP_AND,			/* AND     dst,src1,src2[,f]      */
	DRCUML_OP_TEST,			/* TEST    src1,src2[,f]          */
	DRCUML_OP_OR,			/* OR      dst,src1,src2[,f]      */
	DRCUML_OP_XOR,			/* XOR     dst,src1,src2[,f]      */
	DRCUML_OP_SHL,			/* SHL     dst,src,count[,f]      */
	DRCUML_OP_SHR,			/* SHR     dst,src,count[,f]      */
	DRCUML_OP_SAR,			/* SAR     dst,src,count[,f]      */
	DRCUML_OP_ROL,			/* ROL     dst,src,count[,f]      */
	DRCUML_OP_ROLC,			/* ROLC    dst,src,count[,f]      */
	DRCUML_OP_ROR,			/* ROL     dst,src,count[,f]      */
	DRCUML_OP_RORC,			/* ROLC    dst,src,count[,f]      */

	/* Floating Point Operations */
	DRCUML_OP_FLOAD,		/* FLOAD   dst,base,index         */
	DRCUML_OP_FSTORE,		/* FSTORE  base,index,src         */
	DRCUML_OP_FREAD,		/* FREAD   dst,space,src1         */
	DRCUML_OP_FWRITE,		/* FWRITE  space,dst,src1         */
	DRCUML_OP_FMOV,			/* FSMOV   dst,src1[,c]           */
	DRCUML_OP_FTOI4,		/* FTOI4   dst,src1               */
	DRCUML_OP_FTOI4T,		/* FTOI4T  dst,src1               */
	DRCUML_OP_FTOI4R,		/* FTOI4R  dst,src1               */
	DRCUML_OP_FTOI4F,		/* FTOI4F  dst,src1               */
	DRCUML_OP_FTOI4C,		/* FTOI4C  dst,src1               */
	DRCUML_OP_FTOI8,		/* FTOI8   dst,src1               */
	DRCUML_OP_FTOI8T,		/* FTOI8T  dst,src1               */
	DRCUML_OP_FTOI8R,		/* FTOI8R  dst,src1               */
	DRCUML_OP_FTOI8F,		/* FTOI8F  dst,src1               */
	DRCUML_OP_FTOI8C,		/* FTOI8C  dst,src1               */
	DRCUML_OP_FFRFS,		/* FFRFS   dst,src1               */
	DRCUML_OP_FFRFD,		/* FFRFD   dst,src1               */
	DRCUML_OP_FFRI4,		/* FFRI4   dst,src1               */
	DRCUML_OP_FFRI8,		/* FFRI8   dst,src1               */
	DRCUML_OP_FADD,			/* FADD    dst,src1,src2          */
	DRCUML_OP_FSUB,			/* FSUB    dst,src1,src2          */
	DRCUML_OP_FCMP,			/* FCMP    src1,src2              */
	DRCUML_OP_FMUL,			/* FMUL    dst,src1,src2          */
	DRCUML_OP_FDIV,			/* FDIV    dst,src1,src2          */
	DRCUML_OP_FNEG,			/* FNEG    dst,src1               */
	DRCUML_OP_FABS,			/* FABS    dst,src1               */
	DRCUML_OP_FSQRT,		/* FSQRT   dst,src1               */
	DRCUML_OP_FRECIP,		/* FRECIP  dst,src1               */
	DRCUML_OP_FRSQRT,		/* FRSQRT  dst,src1               */

	DRCUML_OP_MAX
};
typedef enum _drcuml_opcode drcuml_opcode;



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* represents a reference to a local label in the code */
typedef UINT32 drcuml_codelabel;

/* represents the value of an opcode parameter */
typedef UINT64 drcuml_pvalue;


/* opaque structure describing UML generation state */
typedef struct _drcuml_state drcuml_state;

/* opaque structure describing UML codegen block */
typedef struct _drcuml_block drcuml_block;

/* opaque structure describing back-end state */
typedef struct _drcbe_state drcbe_state;

/* opaque structure describing a code handle */
typedef struct _drcuml_codehandle drcuml_codehandle;


/* a parameter for a UML instructon is encoded like this */
typedef struct _drcuml_parameter drcuml_parameter;
struct _drcuml_parameter
{
	drcuml_ptype 	type;				/* parameter type */
	drcuml_pvalue	value;				/* parameter value */
};


/* a single UML instructon is encoded like this */
typedef struct _drcuml_instruction drcuml_instruction;
struct _drcuml_instruction
{
	drcuml_opcode	opcode;				/* opcode */
	UINT8			condflags;			/* condition or flags */
	UINT8			size;				/* operation size */
	UINT8			numparams;			/* number of parameters */
	drcuml_parameter param[4];			/* up to 4 parameters */
};


/* typedefs for back-end callback functions */
typedef drcbe_state *(*drcbe_alloc_func)(drcuml_state *drcuml, drccache *cache, UINT32 flags, int modes, int addrbits, int ignorebits);
typedef void (*drcbe_free_func)(drcbe_state *state);
typedef void (*drcbe_reset_func)(drcbe_state *state);
typedef int	(*drcbe_execute_func)(drcbe_state *state, drcuml_codehandle *entry);
typedef void (*drcbe_generate_func)(drcbe_state *state, drcuml_block *block, const drcuml_instruction *instlist, UINT32 numinst);
typedef int (*drcbe_hash_exists)(drcbe_state *state, UINT32 mode, UINT32 pc);


/* interface structure for a back-end */
typedef struct _drcbe_interface drcbe_interface;
struct _drcbe_interface
{
	drcbe_alloc_func	be_alloc;
	drcbe_free_func		be_free;
	drcbe_reset_func	be_reset;
	drcbe_execute_func	be_execute;
	drcbe_generate_func	be_generate;
	drcbe_hash_exists	be_hash_exists;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* ----- initialization/teardown ----- */

/* allocate state for the code generator and initialize the back-end */
drcuml_state *drcuml_alloc(drccache *cache, UINT32 flags, int modes, int addrbits, int ignorebits);

/* reset the state completely, flushing the cache and all information */
void drcuml_reset(drcuml_state *drcuml);

/* free state for the code generator and the back-end */
void drcuml_free(drcuml_state *drcuml);



/* ----- code block generation ----- */

/* begin a new code block */
drcuml_block *drcuml_block_begin(drcuml_state *drcuml, UINT32 maxinst, jmp_buf *errorbuf);

/* append an opcode to the block, with 0-4 parameters */
void drcuml_block_append_0(drcuml_block *block, drcuml_opcode op, UINT8 size, UINT8 condflags);
void drcuml_block_append_1(drcuml_block *block, drcuml_opcode op, UINT8 size, UINT8 condflags, drcuml_ptype p0type, drcuml_pvalue p0value);
void drcuml_block_append_2(drcuml_block *block, drcuml_opcode op, UINT8 size, UINT8 condflags, drcuml_ptype p0type, drcuml_pvalue p0value, drcuml_ptype p1type, drcuml_pvalue p1value);
void drcuml_block_append_3(drcuml_block *block, drcuml_opcode op, UINT8 size, UINT8 condflags, drcuml_ptype p0type, drcuml_pvalue p0value, drcuml_ptype p1type, drcuml_pvalue p1value, drcuml_ptype p2type, drcuml_pvalue p2value);
void drcuml_block_append_4(drcuml_block *block, drcuml_opcode op, UINT8 size, UINT8 condflags, drcuml_ptype p0type, drcuml_pvalue p0value, drcuml_ptype p1type, drcuml_pvalue p1value, drcuml_ptype p2type, drcuml_pvalue p2value, drcuml_ptype p3type, drcuml_pvalue p3value);

/* complete a code block and commit it to the cache via the back-end */
void drcuml_block_end(drcuml_block *block);

/* abort a code block in progress due to cache allocation failure */
void drcuml_block_abort(drcuml_block *block);

/* return true if a hash entry exists for the given mode/pc */
int drcuml_hash_exists(drcuml_state *drcuml, UINT32 mode, UINT32 pc);



/* ----- code execution ----- */

/* execute at the given PC/mode for the specified cycles */
int drcuml_execute(drcuml_state *drcuml, drcuml_codehandle *entry);



/* ----- code handles ----- */

/* allocate a new handle */
drcuml_codehandle *drcuml_handle_alloc(drcuml_state *drcuml, const char *name);

/* set the handle pointer */
void drcuml_handle_set_codeptr(drcuml_codehandle *handle, drccodeptr code);

/* get the pointer from a handle */
drccodeptr drcuml_handle_codeptr(const drcuml_codehandle *handle);

/* get the address of the pointer from a handle */
drccodeptr *drcuml_handle_codeptr_addr(drcuml_codehandle *handle);

/* get the name of a handle */
const char *drcuml_handle_name(const drcuml_codehandle *handle);



/* ----- code logging ----- */

/* directly printf to the UML log if generated */
void drcuml_log_printf(drcuml_state *state, const char *format, ...) ATTR_PRINTF(2,3);

/* attach a comment to the current output location in the specified block */
void drcuml_add_comment(drcuml_block *block, const char *format, ...) ATTR_PRINTF(2,3);


#endif
