/****************************************************************************
*             real mode i286 emulator v1.4 by Fabrice Frances               *
*               (initial work based on David Hedley's pcemu)                *
****************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "host.h"


#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) mame_printf_debug x; } while (0)

/* All post-i286 CPUs have a 16MB address space */
#define AMASK   cpustate->amask


#define INPUT_LINE_A20      1

#include "i286.h"


#include "i86time.c"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/
/* I86 registers */
union i80286basicregs
{                   /* eight general registers */
	UINT16 w[8];    /* viewed as 16 bits registers */
	UINT8  b[16];   /* or as 8 bit registers */
};

struct i80286_state
{
	i80286basicregs regs;
	offs_t fetch_xor;
	UINT32  amask;          /* address mask */
	UINT32  pc;
	UINT32  prevpc;
	UINT16  flags;
	UINT16  msw;
	UINT32  base[4];
	UINT16  sregs[4];
	UINT16  limit[4];
	UINT8 rights[4];
	bool valid[4];
	struct {
		UINT32 base;
		UINT16 limit;
	} gdtr, idtr;
	struct {
		UINT16 sel;
		UINT32 base;
		UINT16 limit;
		UINT8 rights;
	} ldtr, tr;
	device_irq_acknowledge_callback irq_callback;
	legacy_cpu_device *device;
	address_space *program;
	direct_read_data *direct;
	address_space *io;
	INT32   AuxVal, OverVal, SignVal, ZeroVal, CarryVal, DirVal; /* 0 or non-0 valued flags */
	UINT8   ParityVal;
	UINT8   TF, IF;     /* 0 or 1 valued flags */
	INT8    nmi_state;
	INT8    irq_state;
	INT8    test_state;
	UINT8 rep_in_progress;
	INT32   extra_cycles;       /* extra cycles for interrupts */

	int halted;         /* Is the CPU halted ? */
	int trap_level;

	int icount;
	char seg_prefix;
	UINT8   prefix_seg;
	unsigned ea;
	UINT16 eo; /* HJB 12/13/98 effective offset of the address (before segment is added) */
	UINT8 ea_seg;   /* effective segment of the address */

	i80286_a20_cb a20_callback;
};

INLINE i80286_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == I80286);
	return (i80286_state *)downcast<legacy_cpu_device *>(device)->token();
}

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

static UINT8 parity_table[256];

static struct i80x86_timing timing;

/***************************************************************************/

#define I80286
#include "i86priv.h"
#define PREFIX(fname) i80286##fname
#define PREFIX86(fname) i80286##fname
#define PREFIX186(fname) i80286##fname
#define PREFIX286(fname) i80286##fname
#define i8086_state i80286_state

#include "ea.h"
#include "modrm286.h"
#include "instr86.h"
#include "instr186.h"
#include "instr286.h"
#include "table286.h"
#include "instr86.c"
#include "instr186.c"
#include "instr286.c"

static void i80286_urinit(void)
{
	unsigned int i,j,c;
	static const BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

	for (i = 0;i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;

		parity_table[i] = !(c & 1);
	}

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
	}
}

static void i80286_set_a20_line(i80286_state *cpustate, int state)
{
	if(cpustate->a20_callback)
		cpustate->amask = cpustate->a20_callback(state);
}

static CPU_RESET( i80286 )
{
	i80286_state *cpustate = get_safe_token(device);

	memset(&cpustate->regs, 0, sizeof(i80286basicregs));
	cpustate->sregs[CS] = 0xf000;
	cpustate->base[CS] = 0xff0000;
	cpustate->pc = 0xfffff0;
	cpustate->limit[CS]=cpustate->limit[SS]=cpustate->limit[DS]=cpustate->limit[ES]=0xffff;
	cpustate->sregs[DS]=cpustate->sregs[SS]=cpustate->sregs[ES]=0;
	cpustate->base[DS]=cpustate->base[SS]=cpustate->base[ES]=0;
	cpustate->rights[DS]=cpustate->rights[SS]=cpustate->rights[ES]=0x93;
	cpustate->rights[CS]=0x9a;
	cpustate->valid[CS]=cpustate->valid[SS]=cpustate->valid[DS]=cpustate->valid[ES]=1;
	cpustate->msw=0xfff0;
	cpustate->flags=2;
	ExpandFlags(cpustate->flags);
	cpustate->idtr.base=0;cpustate->idtr.limit=0x3ff;
	cpustate->gdtr.base=cpustate->ldtr.base=cpustate->tr.base=0;
	cpustate->gdtr.limit=cpustate->ldtr.limit=cpustate->tr.limit=0;
	cpustate->ldtr.rights=cpustate->tr.rights=0;
	cpustate->ldtr.sel=cpustate->tr.sel=0;
	cpustate->rep_in_progress = FALSE;
	cpustate->seg_prefix = FALSE;

	CHANGE_PC(cpustate->pc);

	cpustate->halted = 0;
}

/****************************************************************************/

/* ASG 971222 -- added these interface functions */

static void set_irq_line(i80286_state *cpustate, int irqline, int state)
{
	if (state != CLEAR_LINE && cpustate->halted)
	{
		cpustate->halted = 0;
	}
	try
	{
		if (irqline == INPUT_LINE_NMI)
		{
			if (cpustate->nmi_state == state)
				return;
			cpustate->nmi_state = state;

			/* on a rising edge, signal the NMI */
			if (state != CLEAR_LINE)
				i80286_interrupt_descriptor(cpustate, I8086_NMI_INT_VECTOR, 2, -1);
		}
		else
		{
			cpustate->irq_state = state;

			/* if the IF is set, signal an interrupt */
			if (state != CLEAR_LINE && cpustate->IF)
				i80286_interrupt_descriptor(cpustate, (*cpustate->irq_callback)(cpustate->device, 0), 2, -1);

		}
	}
	catch (UINT32 e)
	{
		i80286_trap2(cpustate, e);
	}
}

static CPU_EXECUTE( i80286 )
{
	i80286_state *cpustate = get_safe_token(device);

	if (cpustate->halted)
	{
		cpustate->icount = 0;
		return;
	}

	/* copy over the cycle counts if they're not correct */
	if (timing.id != 80286)
		timing = i80286_cycles;

	/* adjust for any interrupts that came in */
	cpustate->icount -= cpustate->extra_cycles;
	cpustate->extra_cycles = 0;

	/* run until we're out */
	while(cpustate->icount>0)
	{
		LOG(("[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",cpustate->sregs[CS],cpustate->pc - cpustate->base[CS],ReadByte(cpustate->pc),cpustate->flags,cpustate->regs.w[AX],cpustate->regs.w[BX],cpustate->regs.w[CX],cpustate->regs.w[DX], cpustate->AuxVal?1:0, cpustate->OverVal?1:0, cpustate->SignVal?1:0, cpustate->ZeroVal?1:0, cpustate->CarryVal?1:0, cpustate->ParityVal?1:0,cpustate->TF, cpustate->IF, cpustate->DirVal<0?1:0));
		debugger_instruction_hook(device, cpustate->pc);

		cpustate->seg_prefix=FALSE;
		try
		{
			if (PM && ((cpustate->pc-cpustate->base[CS]) > cpustate->limit[CS]))
				throw TRAP(GENERAL_PROTECTION_FAULT, cpustate->sregs[CS] & ~3);
			cpustate->prevpc = cpustate->pc;

			TABLE286 // call instruction
		}
		catch (UINT32 e)
		{
			i80286_trap2(cpustate,e);
		}
	}

	/* adjust for any interrupts that came in */
	cpustate->icount -= cpustate->extra_cycles;
	cpustate->extra_cycles = 0;
}

extern int i386_dasm_one(char *buffer, UINT32 eip, const UINT8 *oprom, int mode);

static CPU_DISASSEMBLE( i80286 )
{
	return i386_dasm_one(buffer, pc, oprom, 2);
}

static CPU_INIT( i80286 )
{
	i80286_state *cpustate = get_safe_token(device);

	device->save_item(NAME(cpustate->regs.w));
	device->save_item(NAME(cpustate->amask));
	device->save_item(NAME(cpustate->pc));
	device->save_item(NAME(cpustate->prevpc));
	device->save_item(NAME(cpustate->msw));
	device->save_item(NAME(cpustate->base));
	device->save_item(NAME(cpustate->sregs));
	device->save_item(NAME(cpustate->limit));
	device->save_item(NAME(cpustate->rights));
	device->save_item(NAME(cpustate->gdtr.base));
	device->save_item(NAME(cpustate->gdtr.limit));
	device->save_item(NAME(cpustate->idtr.base));
	device->save_item(NAME(cpustate->idtr.limit));
	device->save_item(NAME(cpustate->ldtr.sel));
	device->save_item(NAME(cpustate->ldtr.base));
	device->save_item(NAME(cpustate->ldtr.limit));
	device->save_item(NAME(cpustate->ldtr.rights));
	device->save_item(NAME(cpustate->tr.sel));
	device->save_item(NAME(cpustate->tr.base));
	device->save_item(NAME(cpustate->tr.limit));
	device->save_item(NAME(cpustate->tr.rights));
	device->save_item(NAME(cpustate->AuxVal));
	device->save_item(NAME(cpustate->OverVal));
	device->save_item(NAME(cpustate->SignVal));
	device->save_item(NAME(cpustate->ZeroVal));
	device->save_item(NAME(cpustate->CarryVal));
	device->save_item(NAME(cpustate->DirVal));
	device->save_item(NAME(cpustate->ParityVal));
	device->save_item(NAME(cpustate->TF));
	device->save_item(NAME(cpustate->IF));
	device->save_item(NAME(cpustate->nmi_state));
	device->save_item(NAME(cpustate->irq_state));
	device->save_item(NAME(cpustate->extra_cycles));
	device->save_item(NAME(cpustate->rep_in_progress));

	cpustate->irq_callback = irqcallback;
	cpustate->device = device;
	cpustate->program = &device->space(AS_PROGRAM);
	cpustate->io = &device->space(AS_IO);
	cpustate->direct = &cpustate->program->direct();

	if( device->static_config() )
		cpustate->a20_callback = ((i80286_interface *)device->static_config())->a20_callback;
	else
		cpustate->a20_callback = NULL;

	cpustate->amask = 0xffffff;

	cpustate->fetch_xor = BYTE_XOR_LE(0);

	i80286_urinit();
}

static CPU_TRANSLATE( i80286 )
{
	i80286_state *cpustate = get_safe_token(device);

	*address &= cpustate->amask;

	return TRUE;
}

/**************************************************************************
 * Generic set_info
 **************************************************************************/

static CPU_SET_INFO( i80286 )
{
	i80286_state *cpustate = get_safe_token(device);

	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:               set_irq_line(cpustate, 0, info->i);             break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:  set_irq_line(cpustate, INPUT_LINE_NMI, info->i);    break;

		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_A20:  i80286_set_a20_line(cpustate, info->i);         break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I286_PC:
			if (PM)
			{
				/* protected mode NYI */
			}
			else
			{
				if (info->i - cpustate->base[CS] >= 0x10000)
				{
					cpustate->base[CS] = info->i & 0xffff0;
					cpustate->sregs[CS] = cpustate->base[CS] >> 4;
				}
				cpustate->pc = info->i;
			}
			break;

		case CPUINFO_INT_REGISTER + I286_IP:          cpustate->pc = cpustate->base[CS] + info->i;            break;
		case CPUINFO_INT_SP:
			if (PM)
			{
				/* protected mode NYI */
			}
			else
			{
				if (info->i - cpustate->base[SS] < 0x10000)
				{
					cpustate->regs.w[SP] = info->i - cpustate->base[SS];
				}
				else
				{
					cpustate->base[SS] = info->i & 0xffff0;
					cpustate->sregs[SS] = cpustate->base[SS] >> 4;
					cpustate->regs.w[SP] = info->i & 0x0000f;
				}
			}
			break;

		case CPUINFO_INT_REGISTER + I286_SP:          cpustate->regs.w[SP] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_FLAGS:       cpustate->flags = info->i;  ExpandFlags(info->i); break;
		case CPUINFO_INT_REGISTER + I286_AX:          cpustate->regs.w[AX] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_CX:          cpustate->regs.w[CX] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_DX:          cpustate->regs.w[DX] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_BX:          cpustate->regs.w[BX] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_BP:          cpustate->regs.w[BP] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_SI:          cpustate->regs.w[SI] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_DI:          cpustate->regs.w[DI] = info->i;                 break;
		case CPUINFO_INT_REGISTER + I286_ES:          cpustate->sregs[ES] = info->i;  cpustate->base[ES] = SegBase(ES); break;
		case CPUINFO_INT_REGISTER + I286_CS:          cpustate->sregs[CS] = info->i;  cpustate->base[CS] = SegBase(CS); break;
		case CPUINFO_INT_REGISTER + I286_SS:          cpustate->sregs[SS] = info->i;  cpustate->base[SS] = SegBase(SS); break;
		case CPUINFO_INT_REGISTER + I286_DS:          cpustate->sregs[DS] = info->i;  cpustate->base[DS] = SegBase(DS); break;
	}
}



/****************************************************************************
 * Generic get_info
 ****************************************************************************/

CPU_GET_INFO( i80286 )
{
	i80286_state *cpustate = (device != NULL && device->token() != NULL) ? get_safe_token(device) : NULL;

	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:                  info->i = sizeof(i80286_state);                 break;
		case CPUINFO_INT_INPUT_LINES:                   info->i = 1;                            break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:            info->i = 0xff;                         break;
		case CPUINFO_INT_ENDIANNESS:                    info->i = ENDIANNESS_LITTLE;                    break;
		case CPUINFO_INT_CLOCK_MULTIPLIER:              info->i = 1;                            break;
		case CPUINFO_INT_CLOCK_DIVIDER:                 info->i = 1;                            break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:         info->i = 1;                            break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:         info->i = 10;                           break;
		case CPUINFO_INT_MIN_CYCLES:                    info->i = 1;                            break;
		case CPUINFO_INT_MAX_CYCLES:                    info->i = 50;                           break;

		case CPUINFO_INT_DATABUS_WIDTH + AS_PROGRAM:    info->i = 16;                   break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_PROGRAM: info->i = 24;                  break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_PROGRAM: info->i = 0;                   break;
		case CPUINFO_INT_DATABUS_WIDTH + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_DATA:   info->i = 0;                    break;
		case CPUINFO_INT_DATABUS_WIDTH + AS_IO:     info->i = 16;                   break;
		case CPUINFO_INT_ADDRBUS_WIDTH + AS_IO:     info->i = 16;                   break;
		case CPUINFO_INT_ADDRBUS_SHIFT + AS_IO:     info->i = 0;                    break;

		case CPUINFO_INT_INPUT_STATE + 0:               info->i = cpustate->irq_state;                  break;
		case CPUINFO_INT_INPUT_STATE + INPUT_LINE_NMI:  info->i = cpustate->nmi_state;                  break;

		case CPUINFO_INT_PREVIOUSPC:                    info->i = cpustate->prevpc;                     break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + I286_PC:          info->i = cpustate->pc;                         break;
		case CPUINFO_INT_REGISTER + I286_IP:          info->i = cpustate->pc - cpustate->base[CS];            break;
		case CPUINFO_INT_SP:                            info->i = cpustate->base[SS] + cpustate->regs.w[SP];    break;
		case CPUINFO_INT_REGISTER + I286_SP:          info->i = cpustate->regs.w[SP];                 break;
		case CPUINFO_INT_REGISTER + I286_FLAGS:       cpustate->flags = CompressFlags(); info->i = cpustate->flags; break;
		case CPUINFO_INT_REGISTER + I286_AX:          info->i = cpustate->regs.w[AX];                 break;
		case CPUINFO_INT_REGISTER + I286_CX:          info->i = cpustate->regs.w[CX];                 break;
		case CPUINFO_INT_REGISTER + I286_DX:          info->i = cpustate->regs.w[DX];                 break;
		case CPUINFO_INT_REGISTER + I286_BX:          info->i = cpustate->regs.w[BX];                 break;
		case CPUINFO_INT_REGISTER + I286_BP:          info->i = cpustate->regs.w[BP];                 break;
		case CPUINFO_INT_REGISTER + I286_SI:          info->i = cpustate->regs.w[SI];                 break;
		case CPUINFO_INT_REGISTER + I286_DI:          info->i = cpustate->regs.w[DI];                 break;
		case CPUINFO_INT_REGISTER + I286_CS:          info->i = cpustate->sregs[CS];                  break;
		case CPUINFO_INT_REGISTER + I286_SS:          info->i = cpustate->sregs[SS];                  break;
		case CPUINFO_INT_REGISTER + I286_DS:          info->i = cpustate->sregs[DS];                  break;
		case CPUINFO_INT_REGISTER + I286_ES:          info->i = cpustate->sregs[ES];                  break;
		case CPUINFO_INT_REGISTER + I286_CS_BASE:     info->i = cpustate->base[CS];                  break;
		case CPUINFO_INT_REGISTER + I286_SS_BASE:     info->i = cpustate->base[SS];                  break;
		case CPUINFO_INT_REGISTER + I286_DS_BASE:     info->i = cpustate->base[DS];                  break;
		case CPUINFO_INT_REGISTER + I286_ES_BASE:     info->i = cpustate->base[ES];                  break;
		case CPUINFO_INT_REGISTER + I286_CS_LIMIT:    info->i = cpustate->limit[CS];                  break;
		case CPUINFO_INT_REGISTER + I286_SS_LIMIT:    info->i = cpustate->limit[SS];                  break;
		case CPUINFO_INT_REGISTER + I286_DS_LIMIT:    info->i = cpustate->limit[DS];                  break;
		case CPUINFO_INT_REGISTER + I286_ES_LIMIT:    info->i = cpustate->limit[ES];                  break;
		case CPUINFO_INT_REGISTER + I286_CS_FLAGS:    info->i = cpustate->rights[CS];                  break;
		case CPUINFO_INT_REGISTER + I286_SS_FLAGS:    info->i = cpustate->rights[SS];                  break;
		case CPUINFO_INT_REGISTER + I286_DS_FLAGS:    info->i = cpustate->rights[DS];                  break;
		case CPUINFO_INT_REGISTER + I286_ES_FLAGS:    info->i = cpustate->rights[ES];                  break;
		case CPUINFO_INT_REGISTER + I286_MSW:         info->i = cpustate->msw;                        break;
		case CPUINFO_INT_REGISTER + I286_GDTR_BASE:   info->i = cpustate->gdtr.base;                  break;
		case CPUINFO_INT_REGISTER + I286_GDTR_LIMIT:  info->i = cpustate->gdtr.limit;                 break;
		case CPUINFO_INT_REGISTER + I286_IDTR_BASE:   info->i = cpustate->idtr.base;                  break;
		case CPUINFO_INT_REGISTER + I286_IDTR_LIMIT:  info->i = cpustate->idtr.limit;                 break;
		case CPUINFO_INT_REGISTER + I286_LDTR_BASE:   info->i = cpustate->ldtr.base;                  break;
		case CPUINFO_INT_REGISTER + I286_LDTR_LIMIT:  info->i = cpustate->ldtr.limit;                 break;
		case CPUINFO_INT_REGISTER + I286_TR_BASE:     info->i = cpustate->tr.base;                    break;
		case CPUINFO_INT_REGISTER + I286_TR_LIMIT:    info->i = cpustate->tr.limit;                   break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_FCT_SET_INFO:                      info->setinfo = CPU_SET_INFO_NAME(i80286);      break;
		case CPUINFO_FCT_INIT:                          info->init = CPU_INIT_NAME(i80286);             break;
		case CPUINFO_FCT_RESET:                         info->reset = CPU_RESET_NAME(i80286);               break;
		case CPUINFO_FCT_EXIT:                          info->exit = NULL;                      break;
		case CPUINFO_FCT_EXECUTE:                       info->execute = CPU_EXECUTE_NAME(i80286);           break;
		case CPUINFO_FCT_BURN:                          info->burn = NULL;                      break;
		case CPUINFO_FCT_DISASSEMBLE:                   info->disassemble = CPU_DISASSEMBLE_NAME(i80286);       break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:           info->icount = &cpustate->icount;           break;
		case CPUINFO_FCT_TRANSLATE:                     info->translate = CPU_TRANSLATE_NAME(i80286); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:                          strcpy(info->s, "80286");               break;
		case CPUINFO_STR_FAMILY:                    strcpy(info->s, "Intel 80286");         break;
		case CPUINFO_STR_VERSION:                   strcpy(info->s, "1.4");                 break;
		case CPUINFO_STR_SOURCE_FILE:                       strcpy(info->s, __FILE__);              break;
		case CPUINFO_STR_CREDITS:                   strcpy(info->s, "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work cpustate->based on David Hedley's pcemu)"); break;

		case CPUINFO_STR_FLAGS:
			cpustate->flags = CompressFlags();
			sprintf(info->s, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
					cpustate->flags & 0x8000 ? '0' : '.',
					cpustate->flags & 0x4000 ? 'N' : '.',
					cpustate->flags & 0x2000 ? 'I' : '.',
					cpustate->flags & 0x1000 ? 'I' : '.',
					cpustate->flags & 0x0800 ? 'O' : '.',
					cpustate->flags & 0x0400 ? 'D' : '.',
					cpustate->flags & 0x0200 ? 'I' : '.',
					cpustate->flags & 0x0100 ? 'T' : '.',
					cpustate->flags & 0x0080 ? 'S' : '.',
					cpustate->flags & 0x0040 ? 'Z' : '.',
					cpustate->flags & 0x0020 ? '0' : '.',
					cpustate->flags & 0x0010 ? 'A' : '.',
					cpustate->flags & 0x0008 ? '0' : '.',
					cpustate->flags & 0x0004 ? 'P' : '.',
					cpustate->flags & 0x0002 ? '1' : '.',
					cpustate->flags & 0x0001 ? 'C' : '.');
			break;

		case CPUINFO_STR_REGISTER + I286_PC:            sprintf(info->s, "PC: %06X", cpustate->pc);     break;
		case CPUINFO_STR_REGISTER + I286_IP:           sprintf(info->s, "IP: %04X", cpustate->pc - cpustate->base[CS]);   break;
		case CPUINFO_STR_REGISTER + I286_AL:            sprintf(info->s, "~AL: %02X", cpustate->regs.b[AL]); break;
		case CPUINFO_STR_REGISTER + I286_AH:            sprintf(info->s, "~AH: %02X", cpustate->regs.b[AH]); break;
		case CPUINFO_STR_REGISTER + I286_BL:            sprintf(info->s, "~BL: %02X", cpustate->regs.b[BL]); break;
		case CPUINFO_STR_REGISTER + I286_BH:            sprintf(info->s, "~BH: %02X", cpustate->regs.b[BH]); break;
		case CPUINFO_STR_REGISTER + I286_CL:            sprintf(info->s, "~CL: %02X", cpustate->regs.b[CL]); break;
		case CPUINFO_STR_REGISTER + I286_CH:            sprintf(info->s, "~CH: %02X", cpustate->regs.b[CH]); break;
		case CPUINFO_STR_REGISTER + I286_DL:            sprintf(info->s, "~DL: %02X", cpustate->regs.b[DL]); break;
		case CPUINFO_STR_REGISTER + I286_DH:            sprintf(info->s, "~DH: %02X", cpustate->regs.b[DH]); break;
		case CPUINFO_STR_REGISTER + I286_AX:           sprintf(info->s, "AX: %04X", cpustate->regs.w[AX]); break;
		case CPUINFO_STR_REGISTER + I286_BX:           sprintf(info->s, "BX: %04X", cpustate->regs.w[BX]); break;
		case CPUINFO_STR_REGISTER + I286_CX:           sprintf(info->s, "CX: %04X", cpustate->regs.w[CX]); break;
		case CPUINFO_STR_REGISTER + I286_DX:           sprintf(info->s, "DX: %04X", cpustate->regs.w[DX]); break;
		case CPUINFO_STR_REGISTER + I286_BP:           sprintf(info->s, "BP: %04X", cpustate->regs.w[BP]); break;
		case CPUINFO_STR_REGISTER + I286_SP:           sprintf(info->s, "SP: %04X", cpustate->regs.w[SP]); break;
		case CPUINFO_STR_REGISTER + I286_SI:           sprintf(info->s, "SI: %04X", cpustate->regs.w[SI]); break;
		case CPUINFO_STR_REGISTER + I286_DI:           sprintf(info->s, "DI: %04X", cpustate->regs.w[DI]); break;
		case CPUINFO_STR_REGISTER + I286_FLAGS:        sprintf(info->s, "FLAGS: %04X", cpustate->flags); break;
		case CPUINFO_STR_REGISTER + I286_CS:            sprintf(info->s, "CS: %04X", cpustate->sregs[CS]); break;
		case CPUINFO_STR_REGISTER + I286_CS_BASE:       sprintf(info->s, "CSBASE: %06X", cpustate->base[CS]); break;
		case CPUINFO_STR_REGISTER + I286_CS_LIMIT:      sprintf(info->s, "CSLIMIT: %04X", cpustate->limit[CS]); break;
		case CPUINFO_STR_REGISTER + I286_CS_FLAGS:      sprintf(info->s, "CSFLAGS: %02X", cpustate->rights[CS]); break;
		case CPUINFO_STR_REGISTER + I286_SS:            sprintf(info->s, "SS: %04X", cpustate->sregs[SS]); break;
		case CPUINFO_STR_REGISTER + I286_SS_BASE:       sprintf(info->s, "SSBASE: %06X", cpustate->base[SS]); break;
		case CPUINFO_STR_REGISTER + I286_SS_LIMIT:      sprintf(info->s, "SSLIMIT: %04X", cpustate->limit[SS]); break;
		case CPUINFO_STR_REGISTER + I286_SS_FLAGS:      sprintf(info->s, "SSFLAGS: %02X", cpustate->rights[SS]); break;
		case CPUINFO_STR_REGISTER + I286_DS:            sprintf(info->s, "DS: %04X", cpustate->sregs[DS]); break;
		case CPUINFO_STR_REGISTER + I286_DS_BASE:       sprintf(info->s, "DSBASE: %06X", cpustate->base[DS]); break;
		case CPUINFO_STR_REGISTER + I286_DS_LIMIT:      sprintf(info->s, "DSLIMIT: %04X", cpustate->limit[DS]); break;
		case CPUINFO_STR_REGISTER + I286_DS_FLAGS:      sprintf(info->s, "DSFLAGS: %02X", cpustate->rights[DS]); break;
		case CPUINFO_STR_REGISTER + I286_ES:            sprintf(info->s, "ES: %04X", cpustate->sregs[ES]); break;
		case CPUINFO_STR_REGISTER + I286_ES_BASE:       sprintf(info->s, "ESBASE: %06X", cpustate->base[ES]); break;
		case CPUINFO_STR_REGISTER + I286_ES_LIMIT:      sprintf(info->s, "ESLIMIT: %04X", cpustate->limit[ES]); break;
		case CPUINFO_STR_REGISTER + I286_ES_FLAGS:      sprintf(info->s, "ESFLAGS: %02X", cpustate->rights[ES]); break;
		case CPUINFO_STR_REGISTER + I286_GDTR_BASE:     sprintf(info->s, "GDTRBASE: %06X", cpustate->gdtr.base); break;
		case CPUINFO_STR_REGISTER + I286_GDTR_LIMIT:    sprintf(info->s, "GDTRLIMIT: %04X", cpustate->gdtr.limit); break;
		case CPUINFO_STR_REGISTER + I286_IDTR_BASE:     sprintf(info->s, "IDTRBASE: %06X", cpustate->idtr.base); break;
		case CPUINFO_STR_REGISTER + I286_IDTR_LIMIT:    sprintf(info->s, "IDTRLIMIT: %04X", cpustate->idtr.limit); break;
		case CPUINFO_STR_REGISTER + I286_LDTR:          sprintf(info->s, "LDTR: %04X", cpustate->ldtr.sel); break;
		case CPUINFO_STR_REGISTER + I286_LDTR_BASE:     sprintf(info->s, "LDTRBASE: %06X", cpustate->ldtr.base); break;
		case CPUINFO_STR_REGISTER + I286_LDTR_LIMIT:    sprintf(info->s, "LDTRLIMIT: %04X", cpustate->ldtr.limit); break;
		case CPUINFO_STR_REGISTER + I286_LDTR_FLAGS:    sprintf(info->s, "LDTRFLAGS: %02X", cpustate->ldtr.rights); break;
		case CPUINFO_STR_REGISTER + I286_TR:            sprintf(info->s, "TR: %04X", cpustate->tr.sel); break;
		case CPUINFO_STR_REGISTER + I286_TR_BASE:       sprintf(info->s, "TRBASE: %06X", cpustate->tr.base); break;
		case CPUINFO_STR_REGISTER + I286_TR_LIMIT:      sprintf(info->s, "TRLIMIT: %04X", cpustate->tr.limit); break;
		case CPUINFO_STR_REGISTER + I286_TR_FLAGS:      sprintf(info->s, "TRFLAGS: %02X", cpustate->tr.rights); break;
		case CPUINFO_STR_REGISTER + I286_MSW:           sprintf(info->s, "MSW: %04X", cpustate->msw); break;
	}
}

#undef I80286
DEFINE_LEGACY_CPU_DEVICE(I80286, i80286);
