/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "includes/slapfght.h"


/* Perform basic machine initialisation */
MACHINE_RESET( slapfight )
{
	slapfght_state *state = machine->driver_data<slapfght_state>();
	/* MAIN CPU */

	state->slapfight_status_state=0;
	state->slapfight_status = 0xc7;

	state->getstar_sequence_index = 0;
	state->getstar_sh_intenabled = 0;	/* disable sound cpu interrupts */

	/* SOUND CPU */
	cputag_set_input_line(machine, "audiocpu", INPUT_LINE_RESET, ASSERT_LINE);

	/* MCU */
	state->mcu_val = 0;
}

/* Slapfight CPU input/output ports

  These ports seem to control memory access

*/

/* Reset and hold sound CPU */
WRITE8_HANDLER( slapfight_port_00_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	cputag_set_input_line(space->machine, "audiocpu", INPUT_LINE_RESET, ASSERT_LINE);
	state->getstar_sh_intenabled = 0;
}

/* Release reset on sound CPU */
WRITE8_HANDLER( slapfight_port_01_w )
{
	cputag_set_input_line(space->machine, "audiocpu", INPUT_LINE_RESET, CLEAR_LINE);
}

/* Disable and clear hardware interrupt */
WRITE8_HANDLER( slapfight_port_06_w )
{
	interrupt_enable_w(space,0,0);
}

/* Enable hardware interrupt */
WRITE8_HANDLER( slapfight_port_07_w )
{
	interrupt_enable_w(space,0,1);
}

WRITE8_HANDLER( slapfight_port_08_w )
{
	UINT8 *RAM = space->machine->region("maincpu")->base();

	memory_set_bankptr(space->machine, "bank1",&RAM[0x10000]);
}

WRITE8_HANDLER( slapfight_port_09_w )
{
	UINT8 *RAM = space->machine->region("maincpu")->base();

	memory_set_bankptr(space->machine, "bank1",&RAM[0x14000]);
}


/* Status register */
READ8_HANDLER( slapfight_port_00_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	static const int states[3]={ 0xc7, 0x55, 0x00 };

	state->slapfight_status = states[state->slapfight_status_state];

	state->slapfight_status_state++;
	if (state->slapfight_status_state > 2) state->slapfight_status_state = 0;

	return state->slapfight_status;
}

READ8_HANDLER( slapfight_68705_portA_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	return (state->portA_out & state->ddrA) | (state->portA_in & ~state->ddrA);
}

WRITE8_HANDLER( slapfight_68705_portA_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portA_out = data;
}

WRITE8_HANDLER( slapfight_68705_ddrA_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrA = data;
}

READ8_HANDLER( slapfight_68705_portB_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	return (state->portB_out & state->ddrB) | (state->portB_in & ~state->ddrB);
}

WRITE8_HANDLER( slapfight_68705_portB_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	if ((state->ddrB & 0x02) && (~data & 0x02) && (state->portB_out & 0x02))
	{
		state->portA_in = state->from_main;

		if (state->main_sent)
			cputag_set_input_line(space->machine, "mcu", 0, CLEAR_LINE);

		state->main_sent = 0;
	}
	if ((state->ddrB & 0x04) && (data & 0x04) && (~state->portB_out & 0x04))
	{
		state->from_mcu = state->portA_out;
		state->mcu_sent = 1;
	}
	if ((state->ddrB & 0x08) && (~data & 0x08) && (state->portB_out & 0x08))
	{
		*state->slapfight_scrollx_lo = state->portA_out;
	}
	if ((state->ddrB & 0x10) && (~data & 0x10) && (state->portB_out & 0x10))
	{
		*state->slapfight_scrollx_hi = state->portA_out;
	}

	state->portB_out = data;
}

WRITE8_HANDLER( slapfight_68705_ddrB_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrB = data;
}

READ8_HANDLER( slapfight_68705_portC_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portC_in = 0;

	if (state->main_sent)
		state->portC_in |= 0x01;
	if (!state->mcu_sent)
		state->portC_in |= 0x02;

	return (state->portC_out & state->ddrC) | (state->portC_in & ~state->ddrC);
}

WRITE8_HANDLER( slapfight_68705_portC_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portC_out = data;
}

WRITE8_HANDLER( slapfight_68705_ddrC_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrC = data;
}

WRITE8_HANDLER( slapfight_mcu_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->from_main = data;
	state->main_sent = 1;
	cputag_set_input_line(space->machine, "mcu", 0, ASSERT_LINE);
}

READ8_HANDLER( slapfight_mcu_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->mcu_sent = 0;
	return state->from_mcu;
}

READ8_HANDLER( slapfight_mcu_status_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	int res = 0;

	if (!state->main_sent)
		res |= 0x02;
	if (!state->mcu_sent)
		res |= 0x04;

	return res;
}

/***************************************************************************

    Get Star MCU simulation

***************************************************************************/

READ8_HANDLER( getstar_e803_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	UINT16 tmp = 0;  /* needed for values computed on 16 bits */
	UINT8 getstar_val = 0;
	UINT8 phase_lookup_table[] = {0x00, 0x01, 0x03, 0xff, 0xff, 0x02, 0x05, 0xff, 0xff, 0x05}; /* table at 0x0e05 in 'gtstarb1' */
	UINT8 lives_lookup_table[] = {0x03, 0x05, 0x01, 0x02};                                     /* table at 0x0e62 in 'gtstarb1' */
	UINT8 lgsb2_lookup_table[] = {0x00, 0x03, 0x04, 0x05};                                     /* fake tanle for "test mode" in 'gtstarb2' */

	switch (state->getstar_id)
	{
		case GETSTAR:
		case GETSTARJ:
			switch (state->getstar_cmd)
			{
				case 0x20:  /* continue play */
					getstar_val = ((state->gs_a & 0x30) == 0x30) ? 0x20 : 0x80;
					break;
				case 0x21:  /* lose life */
					getstar_val = (state->gs_a << 1) | (state->gs_a >> 7);
					break;
				case 0x22:  /* starting difficulty */
					getstar_val = ((state->gs_a & 0x0c) >> 2) + 1;
					break;
				case 0x23:  /* starting lives */
					getstar_val = lives_lookup_table[state->gs_a];
					break;
				case 0x24:  /* game phase */
					getstar_val = phase_lookup_table[((state->gs_a & 0x18) >> 1) | (state->gs_a & 0x03)];
					break;
				case 0x25:  /* players inputs */
					getstar_val = BITSWAP8(state->gs_a, 3, 2, 1, 0, 7, 5, 6, 4);
					break;
				case 0x26:  /* background (1st read) */
					tmp = 0x8800 + (0x001f * state->gs_a);
					getstar_val = (tmp & 0x00ff) >> 0;
					state->getstar_cmd |= 0x80;     /* to allow a second consecutive read */
					break;
				case 0xa6:  /* background (2nd read) */
					tmp = 0x8800 + (0x001f * state->gs_a);
					getstar_val = (tmp & 0xff00) >> 8;
					break;
				case 0x29:  /* unknown effect */
					getstar_val = 0x00;
					break;
				case 0x2a:  /* change player (if 2 players game) */
					getstar_val = (state->gs_a ^ 0x40);
					break;
				case 0x37:  /* foreground (1st read) */
					tmp = ((0xd0 + ((state->gs_e >> 2) & 0x0f)) << 8) | (0x40 * (state->gs_e & 03) + state->gs_d);
					getstar_val = (tmp & 0x00ff) >> 0;
					state->getstar_cmd |= 0x80;     /* to allow a second consecutive read */
					break;
				case 0xb7:  /* foreground (2nd read) */
					tmp = ((0xd0 + ((state->gs_e >> 2) & 0x0f)) << 8) | (0x40 * (state->gs_e & 03) + state->gs_d);
					getstar_val = (tmp & 0xff00) >> 8;
					break;
				case 0x38:  /* laser position (1st read) */
					tmp = 0xf740 - (((state->gs_e >> 4) << 8) | ((state->gs_e & 0x08) ? 0x80 : 0x00)) + (0x02 + (state->gs_d >> 2));
					getstar_val = (tmp & 0x00ff) >> 0;
					state->getstar_cmd |= 0x80;     /* to allow a second consecutive read */
					break;
				case 0xb8:  /* laser position (2nd read) */
					tmp = 0xf740 - (((state->gs_e >> 4) << 8) | ((state->gs_e & 0x08) ? 0x80 : 0x00)) + (0x02 + (state->gs_d >> 2));
					getstar_val = (tmp & 0xff00) >> 8;
					break;
				case 0x73:  /* avoid "BAD HW" message */
					getstar_val = 0x76;
					break;
				default:
					logerror("%04x: getstar_e803_r - cmd = %02x\n",cpu_get_pc(space->cpu),state->getstar_cmd);
					break;
			}
			break;
		case GTSTARB1:
			/* value isn't computed by the bootleg but we want to please the "test mode" */
			if (cpu_get_pc(space->cpu) == 0x6b04) return (lives_lookup_table[state->gs_a]);
			break;
		case GTSTARB2:
			/*
            056B: 21 03 E8      ld   hl,$E803
            056E: 7E            ld   a,(hl)
            056F: BE            cp   (hl)
            0570: 28 FD         jr   z,$056F
            0572: C6 05         add  a,$05
            0574: EE 56         xor  $56
            0576: BE            cp   (hl)
            0577: C2 6E 05      jp   nz,$056E
            */
			if (cpu_get_pc(space->cpu) == 0x056e) return (getstar_val);
			if (cpu_get_pc(space->cpu) == 0x0570) return (getstar_val+1);
			if (cpu_get_pc(space->cpu) == 0x0577) return ((getstar_val+0x05) ^ 0x56);
			/* value isn't computed by the bootleg but we want to please the "test mode" */
			if (cpu_get_pc(space->cpu) == 0x6b04) return (lgsb2_lookup_table[state->gs_a]);
			break;
		default:
			logerror("%04x: getstar_e803_r - cmd = %02x - unknown set !\n",cpu_get_pc(space->cpu),state->getstar_cmd);
			break;
	}
	return getstar_val;
}

WRITE8_HANDLER( getstar_e803_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	switch (state->getstar_id)
	{
		case GETSTAR:
			/* unknown effect - not read back */
			if (cpu_get_pc(space->cpu) == 0x00bf)
			{
				state->getstar_cmd = 0x00;
				GS_RESET_REGS
			}
			/* players inputs */
			if (cpu_get_pc(space->cpu) == 0x0560)
			{
				state->getstar_cmd = 0x25;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x056d)
			{
				state->getstar_cmd = 0x25;
				GS_SAVE_REGS
			}
			/* lose life */
			if (cpu_get_pc(space->cpu) == 0x0a0a)
			{
				state->getstar_cmd = 0x21;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0a17)
			{
				state->getstar_cmd = 0x21;
				GS_SAVE_REGS
			}
			/* unknown effect */
			if (cpu_get_pc(space->cpu) == 0x0a51)
			{
				state->getstar_cmd = 0x29;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0a6e)
			{
				state->getstar_cmd = 0x29;
				GS_SAVE_REGS
			}
			/* continue play */
			if (cpu_get_pc(space->cpu) == 0x0ae3)
			{
				state->getstar_cmd = 0x20;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0af0)
			{
				state->getstar_cmd = 0x20;
				GS_SAVE_REGS
			}
			/* unknown effect - not read back */
			if (cpu_get_pc(space->cpu) == 0x0b62)
			{
				state->getstar_cmd = 0x00;     /* 0x1f */
				GS_RESET_REGS
			}
			/* change player (if 2 players game) */
			if (cpu_get_pc(space->cpu) == 0x0bab)
			{
				state->getstar_cmd = 0x2a;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0bb8)
			{
				state->getstar_cmd = 0x2a;
				GS_SAVE_REGS
			}
			/* game phase */
			if (cpu_get_pc(space->cpu) == 0x0d37)
			{
				state->getstar_cmd = 0x24;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0d44)
			{
				state->getstar_cmd = 0x24;
				GS_SAVE_REGS
			}
			/* starting lives */
			if (cpu_get_pc(space->cpu) == 0x0d79)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0d8a)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			/* starting difficulty */
			if (cpu_get_pc(space->cpu) == 0x0dc1)
			{
				state->getstar_cmd = 0x22;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0dd0)
			{
				state->getstar_cmd = 0x22;
				GS_SAVE_REGS
			}
			/* starting lives (again) */
			if (cpu_get_pc(space->cpu) == 0x1011)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x101e)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			/* hardware test */
			if (cpu_get_pc(space->cpu) == 0x107a)
			{
				state->getstar_cmd = 0x73;
				GS_RESET_REGS
			}
			/* game phase (again) */
			if (cpu_get_pc(space->cpu) == 0x10c6)
			{
				state->getstar_cmd = 0x24;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x10d3)
			{
				state->getstar_cmd = 0x24;
				GS_SAVE_REGS
			}
			/* background */
			if (cpu_get_pc(space->cpu) == 0x1910)
			{
				state->getstar_cmd = 0x26;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x191d)
			{
				state->getstar_cmd = 0x26;
				GS_SAVE_REGS
			}
			/* foreground */
			if (cpu_get_pc(space->cpu) == 0x19d5)
			{
				state->getstar_cmd = 0x37;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x19e4)
			{
				state->getstar_cmd = 0x37;
				GS_SAVE_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x19f1)
			{
				state->getstar_cmd = 0x37;
				/* do NOT update the registers because there are 2 writes before 2 reads ! */
			}
			/* laser position */
			if (cpu_get_pc(space->cpu) == 0x26af)
			{
				state->getstar_cmd = 0x38;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x26be)
			{
				state->getstar_cmd = 0x38;
				GS_SAVE_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x26cb)
			{
				state->getstar_cmd = 0x38;
				/* do NOT update the registers because there are 2 writes before 2 reads ! */
			}
			/* starting lives (for "test mode") */
			if (cpu_get_pc(space->cpu) == 0x6a27)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x6a38)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			break;
		case GETSTARJ:
			/* unknown effect - not read back */
			if (cpu_get_pc(space->cpu) == 0x00bf)
			{
				state->getstar_cmd = 0x00;
				GS_RESET_REGS
			}
			/* players inputs */
			if (cpu_get_pc(space->cpu) == 0x0560)
			{
				state->getstar_cmd = 0x25;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x056d)
			{
				state->getstar_cmd = 0x25;
				GS_SAVE_REGS
			}
			/* lose life */
			if (cpu_get_pc(space->cpu) == 0x0ad5)
			{
				state->getstar_cmd = 0x21;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0ae2)
			{
				state->getstar_cmd = 0x21;
				GS_SAVE_REGS
			}
			/* unknown effect */
			if (cpu_get_pc(space->cpu) == 0x0b1c)
			{
				state->getstar_cmd = 0x29;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0b29)
			{
				state->getstar_cmd = 0x29;
				GS_SAVE_REGS
			}
			/* continue play */
			if (cpu_get_pc(space->cpu) == 0x0bae)
			{
				state->getstar_cmd = 0x20;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0bbb)
			{
				state->getstar_cmd = 0x20;
				GS_SAVE_REGS
			}
			/* unknown effect - not read back */
			if (cpu_get_pc(space->cpu) == 0x0c2d)
			{
				state->getstar_cmd = 0x00;     /* 0x1f */
				GS_RESET_REGS
			}
			/* change player (if 2 players game) */
			if (cpu_get_pc(space->cpu) == 0x0c76)
			{
				state->getstar_cmd = 0x2a;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0c83)
			{
				state->getstar_cmd = 0x2a;
				GS_SAVE_REGS
			}
			/* game phase */
			if (cpu_get_pc(space->cpu) == 0x0e02)
			{
				state->getstar_cmd = 0x24;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0e0f)
			{
				state->getstar_cmd = 0x24;
				GS_SAVE_REGS
			}
			/* starting lives */
			if (cpu_get_pc(space->cpu) == 0x0e44)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0e55)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			/* starting difficulty */
			if (cpu_get_pc(space->cpu) == 0x0e8c)
			{
				state->getstar_cmd = 0x22;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x0e9b)
			{
				state->getstar_cmd = 0x22;
				GS_SAVE_REGS
			}
			/* starting lives (again) */
			if (cpu_get_pc(space->cpu) == 0x10d6)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x10e3)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			/* hardware test */
			if (cpu_get_pc(space->cpu) == 0x113f)
			{
				state->getstar_cmd = 0x73;
				GS_RESET_REGS
			}
			/* game phase (again) */
			if (cpu_get_pc(space->cpu) == 0x118b)
			{
				state->getstar_cmd = 0x24;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x1198)
			{
				state->getstar_cmd = 0x24;
				GS_SAVE_REGS
			}
			/* background */
			if (cpu_get_pc(space->cpu) == 0x19f8)
			{
				state->getstar_cmd = 0x26;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x1a05)
			{
				state->getstar_cmd = 0x26;
				GS_SAVE_REGS
			}
			/* foreground */
			if (cpu_get_pc(space->cpu) == 0x1abd)
			{
				state->getstar_cmd = 0x37;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x1acc)
			{
				state->getstar_cmd = 0x37;
				GS_SAVE_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x1ad9)
			{
				state->getstar_cmd = 0x37;
				/* do NOT update the registers because there are 2 writes before 2 reads ! */
			}
			/* laser position */
			if (cpu_get_pc(space->cpu) == 0x2792)
			{
				state->getstar_cmd = 0x38;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x27a1)
			{
				state->getstar_cmd = 0x38;
				GS_SAVE_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x27ae)
			{
				state->getstar_cmd = 0x38;
				/* do NOT update the registers because there are 2 writes before 2 reads ! */
			}
			/* starting lives (for "test mode") */
			if (cpu_get_pc(space->cpu) == 0x6ae2)
			{
				state->getstar_cmd = 0x23;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x6af3)
			{
				state->getstar_cmd = 0x23;
				GS_SAVE_REGS
			}
			break;
		case GTSTARB1:
			/* "Test mode" doesn't compute the lives value :
                6ADA: 3E 23         ld   a,$23
                6ADC: CD 52 11      call $1152
                6ADF: 32 03 E8      ld   ($E803),a
                6AE2: DB 00         in   a,($00)
                6AE4: CB 4F         bit  1,a
                6AE6: 28 FA         jr   z,$6AE2
                6AE8: 3A 0A C8      ld   a,($C80A)
                6AEB: E6 03         and  $03
                6AED: CD 52 11      call $1152
                6AF0: 32 03 E8      ld   ($E803),a
                6AF3: DB 00         in   a,($00)
                6AF5: CB 57         bit  2,a
                6AF7: 20 FA         jr   nz,$6AF3
                6AF9: 00            nop
                6AFA: 00            nop
                6AFB: 00            nop
                6AFC: 00            nop
                6AFD: 00            nop
                6AFE: 00            nop
                6AFF: 00            nop
                6B00: 00            nop
                6B01: 3A 03 E8      ld   a,($E803)
               We save the regs though to hack it in 'getstar_e803_r' read handler.
            */
			if (cpu_get_pc(space->cpu) == 0x6ae2)
			{
				state->getstar_cmd = 0x00;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x6af3)
			{
				state->getstar_cmd = 0x00;
				GS_SAVE_REGS
			}
			break;
		case GTSTARB2:
			/* "Test mode" doesn't compute the lives value :
                6ADA: 3E 23         ld   a,$23
                6ADC: CD 52 11      call $1152
                6ADF: 32 03 E8      ld   ($E803),a
                6AE2: DB 00         in   a,($00)
                6AE4: CB 4F         bit  1,a
                6AE6: 00            nop
                6AE7: 00            nop
                6AE8: 3A 0A C8      ld   a,($C80A)
                6AEB: E6 03         and  $03
                6AED: CD 52 11      call $1152
                6AF0: 32 03 E8      ld   ($E803),a
                6AF3: DB 00         in   a,($00)
                6AF5: CB 57         bit  2,a
                6AF7: 00            nop
                6AF8: 00            nop
                6AF9: 00            nop
                6AFA: 00            nop
                6AFB: 00            nop
                6AFC: 00            nop
                6AFD: 00            nop
                6AFE: 00            nop
                6AFF: 00            nop
                6B00: 00            nop
                6B01: 3A 03 E8      ld   a,($E803)
               We save the regs though to hack it in 'getstar_e803_r' read handler.
            */
			if (cpu_get_pc(space->cpu) == 0x6ae2)
			{
				state->getstar_cmd = 0x00;
				GS_RESET_REGS
			}
			if (cpu_get_pc(space->cpu) == 0x6af3)
			{
				state->getstar_cmd = 0x00;
				GS_SAVE_REGS
			}
			break;
		default:
			logerror("%04x: getstar_e803_w - data = %02x - unknown set !\n",cpu_get_pc(space->cpu),data);
			break;
	}
}

/* Enable hardware interrupt of sound cpu */
WRITE8_HANDLER( getstar_sh_intenable_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->getstar_sh_intenabled = 1;
	logerror("cpu #1 PC=%d: %d written to a0e0\n",cpu_get_pc(space->cpu),data);
}



/* Generate interrups only if they have been enabled */
INTERRUPT_GEN( getstar_interrupt )
{
	slapfght_state *state = device->machine->driver_data<slapfght_state>();
	if (state->getstar_sh_intenabled)
		cpu_set_input_line(device, INPUT_LINE_NMI, PULSE_LINE);
}

#ifdef UNUSED_FUNCTION
WRITE8_HANDLER( getstar_port_04_w )
{
//  cpu_halt(0,0);
}
#endif


/***************************************************************************

    Tiger Heli MCU

***************************************************************************/

READ8_HANDLER( tigerh_68705_portA_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	return (state->portA_out & state->ddrA) | (state->portA_in & ~state->ddrA);
}

WRITE8_HANDLER( tigerh_68705_portA_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portA_out = data;//?
	state->from_mcu = state->portA_out;
	state->mcu_sent = 1;
}

WRITE8_HANDLER( tigerh_68705_ddrA_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrA = data;
}

READ8_HANDLER( tigerh_68705_portB_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	return (state->portB_out & state->ddrB) | (state->portB_in & ~state->ddrB);
}

WRITE8_HANDLER( tigerh_68705_portB_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();

	if ((state->ddrB & 0x02) && (~data & 0x02) && (state->portB_out & 0x02))
	{
		state->portA_in = state->from_main;
		if (state->main_sent) cputag_set_input_line(space->machine, "mcu", 0, CLEAR_LINE);
		state->main_sent = 0;
	}
	if ((state->ddrB & 0x04) && (data & 0x04) && (~state->portB_out & 0x04))
	{
		state->from_mcu = state->portA_out;
		state->mcu_sent = 1;
	}

	state->portB_out = data;
}

WRITE8_HANDLER( tigerh_68705_ddrB_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrB = data;
}


READ8_HANDLER( tigerh_68705_portC_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portC_in = 0;
	if (!state->main_sent) state->portC_in |= 0x01;
	if (state->mcu_sent) state->portC_in |= 0x02;
	return (state->portC_out & state->ddrC) | (state->portC_in & ~state->ddrC);
}

WRITE8_HANDLER( tigerh_68705_portC_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->portC_out = data;
}

WRITE8_HANDLER( tigerh_68705_ddrC_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->ddrC = data;
}

WRITE8_HANDLER( tigerh_mcu_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->from_main = data;
	state->main_sent = 1;
	state->mcu_sent = 0;
	cputag_set_input_line(space->machine, "mcu", 0, ASSERT_LINE);
}

READ8_HANDLER( tigerh_mcu_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	state->mcu_sent = 0;
	return state->from_mcu;
}

READ8_HANDLER( tigerh_mcu_status_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	int res = 0;
	if (!state->main_sent) res |= 0x02;
	if (!state->mcu_sent) res |= 0x04;
	return res;
}


READ8_HANDLER( tigerhb_e803_r )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	UINT8 tigerhb_val = 0;
	switch (state->tigerhb_cmd)
	{
		case 0x73:  /* avoid "BAD HW" message */
			tigerhb_val = 0x83;
			break;
		default:
			logerror("%04x: tigerhb_e803_r - cmd = %02x\n",cpu_get_pc(space->cpu),state->getstar_cmd);
			break;
	}
	return tigerhb_val;
}

WRITE8_HANDLER( tigerhb_e803_w )
{
	slapfght_state *state = space->machine->driver_data<slapfght_state>();
	switch (data)
	{
		/* hardware test */
		case 0x73:
			state->tigerhb_cmd = 0x73;
			break;
		default:
			logerror("%04x: tigerhb_e803_w - data = %02x\n",cpu_get_pc(space->cpu),data);
			state->tigerhb_cmd = 0x00;
			break;
	}
}


/***************************************************************************

    Performan

***************************************************************************/

READ8_HANDLER( perfrman_port_00_r )
{
	/* TODO */
	return space->machine->rand() & 1;
}
