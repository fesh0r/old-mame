#pragma once

#ifndef __TX0_H__
#define __TX0_H__

#include "cpuintrf.h"


/* register ids for tx0_get_reg/tx0_set_reg */
enum
{
	TX0_MBR=1, TX0_AC, TX0_MAR, TX0_PC, TX0_IR, TX0_LR, TX0_XR, TX0_PF,
	TX0_TBR, TX0_TAC,
	TX0_TSS00, TX0_TSS01, TX0_TSS02, TX0_TSS03, TX0_TSS04, TX0_TSS05, TX0_TSS06, TX0_TSS07,
	TX0_TSS10, TX0_TSS11, TX0_TSS12, TX0_TSS13, TX0_TSS14, TX0_TSS15, TX0_TSS16, TX0_TSS17,
	TX0_CM_SEL, TX0_LR_SEL, TX0_GBL_CM_SEL,
	TX0_STOP_CYC0, TX0_STOP_CYC1,
	TX0_RUN, TX0_RIM,
	TX0_CYCLE, TX0_IOH, TX0_IOS,
	TX0_RESET,			/* hack, do not use directly, use tx0_pulse_reset instead */
	TX0_IO_COMPLETE		/* hack, do not use directly, use tx0_pulse_io_complete instead */
};

#define tx0_pulse_reset()		cpu_set_reg(machine->cpu[0], TX0_RESET, 0)
#define tx0_pulse_io_complete()	cpu_set_reg(machine->cpu[0], TX0_IO_COMPLETE, 0)

typedef struct _tx0_reset_param_t tx0_reset_param_t;
struct _tx0_reset_param_t
{
	/* 8 standard I/O handlers:
        0: cpy (8kW only)
        1: r1l
        2: dis
        3: r3l
        4: prt
        5: reserved (for unimplemented typ instruction?)
        6: p6h
        7: p7h */
	void (*io_handlers[8])(void);
	/* select instruction handler */
	void (*sel_handler)(void);
	/* callback called when reset line is pulsed: IO devices should reset */
	void (*io_reset_callback)(void);
};

/* PUBLIC FUNCTIONS */
CPU_GET_INFO( tx0_64kw );
CPU_GET_INFO( tx0_8kw );

#define READ_TX0_18BIT(A) ((signed)memory_read_dword_32be(tx0.program, (A)<<2))
#define WRITE_TX0_18BIT(A,V) (memory_write_dword_32be(tx0.program, (A)<<2,(V)))

CPU_DISASSEMBLE( tx0_64kw );
CPU_DISASSEMBLE( tx0_8kw );

#endif /* __TX0_H__ */
