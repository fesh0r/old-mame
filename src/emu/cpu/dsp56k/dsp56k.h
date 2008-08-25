/***************************************************************************

    dsp56k.h
    Interface file for the portable Motorola/Freescale DSP56k emulator.
    Written by Andrew Gardner

***************************************************************************/


#pragma once

#ifndef __DSP56K_H__
#define __DSP56K_H__

#include "cpuintrf.h"

/***************************************************************************
    REGISTER ENUMERATION
***************************************************************************/

// IRQ Lines
// MODA and MODB are also known as IRQA and IRQB
#define DSP56K_IRQ_MODA  0
#define DSP56K_IRQ_MODB  1
#define DSP56K_IRQ_MODC  2
#define DSP56K_IRQ_RESET 3	/* Is this needed? */

enum
{
	// PCU
	DSP56K_PC=1,
	DSP56K_SR,
	DSP56K_LC,
	DSP56K_LA,
	DSP56K_SP,
	DSP56K_OMR,

	// ALU
	DSP56K_X, DSP56K_Y,
	DSP56K_A, DSP56K_B,

	// AGU
	DSP56K_R0,DSP56K_R1,DSP56K_R2,DSP56K_R3,
	DSP56K_N0,DSP56K_N1,DSP56K_N2,DSP56K_N3,
	DSP56K_M0,DSP56K_M1,DSP56K_M2,DSP56K_M3,
	DSP56K_TEMP,
	DSP56K_STATUS,

	// CPU STACK
	DSP56K_ST0,
	DSP56K_ST1,
	DSP56K_ST2,
	DSP56K_ST3,
	DSP56K_ST4,
	DSP56K_ST5,
	DSP56K_ST6,
	DSP56K_ST7,
	DSP56K_ST8,
	DSP56K_ST9,
	DSP56K_ST10,
	DSP56K_ST11,
	DSP56K_ST12,
	DSP56K_ST13,
	DSP56K_ST14,
	DSP56K_ST15
};

// For Debugger and opcodes
enum _parallel_move_type
{
	PARALLEL_TYPE_XMDM,
	PARALLEL_TYPE_XMDM_SPECIAL,
	PARALLEL_TYPE_NODM,
	PARALLEL_TYPE_ARU,
	PARALLEL_TYPE_RRDM
};
typedef enum _parallelMoveType parallelMoveType;

extern void dsp56k_get_info(UINT32 state, cpuinfo *info);

void  dsp56k_host_interface_write(UINT8 offset, UINT8 data);
UINT8 dsp56k_host_interface_read(UINT8 offset);

UINT16 dsp56k_get_peripheral_memory(UINT16 addr);

#endif /* __DSP56K_H__ */
