/* Status Register */
static UINT8 LF_bit(void) { return (SR & 0x8000) >> 15; }
static UINT8 FV_bit(void) { return (SR & 0x4000) >> 14; }
//static UINT8 S_bits(void); #define s1BIT ((SR & 0x0800) != 0) #define s0BIT ((SR & 0x0400) != 0)
static UINT8 I_bits(void) {	return (SR & 0x0300) >> 8; }
static UINT8 S_bit(void)  { return (SR & 0x0080) >> 7; }
static UINT8 L_bit(void)  { return (SR & 0x0040) >> 6; }
static UINT8 E_bit(void)  { return (SR & 0x0020) >> 5; }
static UINT8 U_bit(void)  { return (SR & 0x0010) >> 4; }
static UINT8 N_bit(void)  { return (SR & 0x0008) >> 3; }
static UINT8 Z_bit(void)  { return (SR & 0x0004) >> 2; }
static UINT8 V_bit(void)  { return (SR & 0x0002) >> 1; }
static UINT8 C_bit(void)  { return (SR & 0x0001) >> 0; }

static void LF_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x8000);
	SR |=  (value << 15);
}
static void FV_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x4000);
	SR |=  (value << 14);
}
static void S_bits_set(UINT8 value)
{
	value = value & 0x03;
	SR &= ~(0x0c00);
	SR |=  (value << 10);
}
static void I_bits_set(UINT8 value)
{
	value = value & 0x03;
	SR &= ~(0x0300);
	SR |=  (value << 8);
}
static void S_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0080);
	SR |=  (value << 7);
}
static void L_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0040);
	SR |=  (value << 6);
}
static void E_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0020);
	SR |=  (value << 5);
}
static void U_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0010);
	SR |=  (value << 4);
}
static void N_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0008);
	SR |=  (value << 3);
}
static void Z_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0004);
	SR |=  (value << 2);
}
static void V_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0002);
	SR |=  (value << 1);
}
static void C_bit_set(UINT8 value)
{
	value = value & 0x01;
	SR &= ~(0x0001);
	SR |=  (value << 0);
}

/* Operating Mode Register */
// static UINT8 MC_bit(void) { return ((OMR & 0x0004) != 0); } // #define mcBIT ((OMR & 0x0004) != 0)
static UINT8 MB_bit(void) { return ((OMR & 0x0002) != 0); } // #define mbBIT ((OMR & 0x0002) != 0)
static UINT8 MA_bit(void) { return ((OMR & 0x0001) != 0); } // #define maBIT ((OMR & 0x0001) != 0)

static void CD_bit_set(UINT8 value) { if (value) (OMR |= 0x0080); else (OMR &= (~0x0080)); }
static void SD_bit_set(UINT8 value) { if (value) (OMR |= 0x0040); else (OMR &= (~0x0040)); }
static void R_bit_set(UINT8 value)  { if (value) (OMR |= 0x0020); else (OMR &= (~0x0020)); }
static void SA_bit_set(UINT8 value) { if (value) (OMR |= 0x0010); else (OMR &= (~0x0010)); }
static void MC_bit_set(UINT8 value) { if (value) (OMR |= 0x0004); else (OMR &= (~0x0004)); }
static void MB_bit_set(UINT8 value) { if (value) (OMR |= 0x0002); else (OMR &= (~0x0002)); }
static void MA_bit_set(UINT8 value) { if (value) (OMR |= 0x0001); else (OMR &= (~0x0001)); }


/* Stack Pointer */
static UINT8 UF_bit() { return (SP & 0x0020) >> 5; }
static UINT8 SE_bit() { return (SP & 0x0010) >> 4; }

//static void UF_bit_set(UINT8 value) {};
//static void SE_bit_set(UINT8 value) {};


UINT8 dsp56k_operating_mode(void)
{
	return ((MB_bit() << 1) | MA_bit());
}


static void pcu_init(int index)
{
	// Init the irq table
	dsp56k_irq_table_init();
}

static void pcu_reset(void)
{
	int i;

	// When reset is deasserted, set MA, MB, and MC from MODA, MODB, and MODC lines.
	MA_bit_set(core.modA_state);
	MB_bit_set(core.modB_state);
	MC_bit_set(core.modC_state);

	// Reset based on the operating mode.
	switch(dsp56k_operating_mode())
	{
		case 0x00:
			logerror("Dsp56k in Special Bootstrap Mode 1\n");

			// HACK - We don't need to put the bootstrap mode on here since
			//        we'll simulate it entirely in this function
			core.bootstrap_mode = BOOTSTRAP_OFF;

			// HACK - Simply copy over 0x1000 bytes of data located at program memory 0xc000.
			//        This, in actuality, is handled with the internal boot ROM.
			for (i = 0; i < 0x800; i++)
			{
				UINT32 mem_offset = (0xc0000<<1) + (i<<1);	/* TODO: TEST */

				// TODO - DO I HAVE TO FLIP THIS WORD?
				// P:$c000 -> Internal P:$0000 low byte
				// P:$c001 -> Internal P:$0000 high byte
				// ...
				// P:$cffe -> Internal P:$07ff low byte
				// P:$cfff -> Internal P:$07ff high byte
				UINT8 mem_value_low  = program_read_byte_16le(mem_offset);		/* TODO: IS THIS READING RIGHT? */
				UINT8 mem_value_high = program_read_byte_16be(mem_offset);
				dsp56k_program_ram[i] = (mem_value_high << 8) || mem_value_low;
			}

			// HACK - Set the PC to 0x0000 as per the boot ROM.
			PC = 0x0000;

			// HACK - All done!  Set the Operating Mode to 2 as per the boot ROM.
			MB_bit_set(1);
			MA_bit_set(0);
			core.PCU.reset_vector = 0xe000;
			break;

		case 0x01:
			logerror("Dsp56k in Special Bootstrap Mode 2\n");

			// HACK - Turn bootstrap mode on.  This hijacks the CPU execute loop and lets
			//        Either the host interface or the SSIO interface suck in all the data
			//        they need.  Once they've had their fill, they turn bootstrap mode off
			//        and the CPU begins execution at 0x0000;
			// HACK - Read bit 15 at 0xc000 to see if we're working with the SSIO or host interface.
			if (program_read_word_16le(0xc000<<1) & 0x8000)
			{
				core.bootstrap_mode = BOOTSTRAP_SSIX;
				logerror("DSP56k : Currently in (hacked) bootstrap mode - reading from SSIx.\n");
			}
			else
			{
				core.bootstrap_mode = BOOTSTRAP_HI;
				logerror("DSP56k : Currently in (hacked) bootstrap mode - reading from Host Interface.\n");
			}

			// HACK - Set the PC to 0x0000 as per the boot ROM.
			PC = 0x0000;

			// HACK - Not done yet, but set the Operating Mode to 2 in preparation.
			MB_bit_set(1);
			MA_bit_set(0);
			core.PCU.reset_vector = 0xe000;
			break;

		case 0x02:
			logerror("Dsp56k in Normal Expanded Mode\n");
			PC = 0xe000;
			core.PCU.reset_vector = 0xe000;
			break;

		case 0x03:
			logerror("Dsp56k in Development Expanded Mode\n");
			// TODO: Disable internal ROM, etc.  Likely a tricky thing for MAME?
			PC = 0x0000;
			core.PCU.reset_vector = 0x0000;
			break;
	}

	// Set registers properly
	// 1-17 Clear Interrupt Priority Register (IPR)
	IPR = 0x0000;

	// FM.5-4
//  I_bits_set(0x03); This is what the manual says, but i'm dubious!  Polygonet's HI interrupt wouldn't happen with the I bits set.
	I_bits_set(0x00);
	S_bits_set(0);
	L_bit_set(0);
	S_bit_set(0);
	FV_bit_set(0);

	// FM.7-25
	E_bit_set(0);
	U_bit_set(0);
	N_bit_set(0);
	V_bit_set(0);
	Z_bit_set(0);

	// FM.5-4+
	C_bit_set(0);
	LF_bit_set(0);
	SP = 0x0000;

	// FM.5-14 (OMR)
	SA_bit_set(0);
	R_bit_set(0);
	SD_bit_set(0);
	CD_bit_set(0);

	// Clear out the pending interrupt list
	dsp56k_clear_pending_interrupts();
}

/***************************************************************************
    INTERRUPT HANDLING
***************************************************************************/
typedef struct
{
	UINT16 irq_vector;
	char   irq_source[128];
} dsp56k_irq_data;

static dsp56k_irq_data dsp56k_interrupt_sources[32];

// TODO: Figure out how to switch on level versus edge-triggered.
static void pcu_service_interrupts(void)
{
	int i;

	// Count list of pending interrupts
	int num_servicable = dsp56k_count_pending_interrupts();

	if (num_servicable == 0)
		return;

	// Sort list according to priority
	dsp56k_sort_pending_interrupts(num_servicable);

	// Service each interrupt in order
	// TODO: This just *can't* be right :)
	for (i = 0; i < num_servicable; i++)
	{
		const int interrupt_index = core.PCU.pending_interrupts[i];

		// Get the priority of the interrupt - a return value of -1 means disabled!
		INT8 priority = dsp56k_get_irq_priority(interrupt_index);

		// 1-12 Make sure you're not masked out against the Interrupt Mask Bits (disabled is handled for free here)
		if (priority >= I_bits())
		{
			// Are you anything but the Host Command interrupt?
			if (interrupt_index != 22)
			{
				// Execute a normal interrupt
				PC = dsp56k_interrupt_sources[interrupt_index].irq_vector;
			}
			else
			{
				// The host command input has a floating vector.
				const UINT16 irq_vector = HV_bits() << 1;
				PC = irq_vector;

				// TODO: 5-9 5-11 Gotta' Clear HC (HCP gets it too) when taking this exception!
				HC_bit_set(0);
			}
		}
	}

	dsp56k_clear_pending_interrupts();
}

//
// The function the CPU core will call to add an interrupt to the list
// (The API as it were)
//
static void dsp56k_add_pending_interrupt(const char* name)
{
	int i;
	int irq_index = dsp56k_get_irq_index_by_tag(name);

	for (i = 0; i < 32; i++)
	{
		if (core.PCU.pending_interrupts[i] == -1)
		{
			core.PCU.pending_interrupts[i] = irq_index;
			break;
		}
	}
}

static void dsp56k_set_irq_source(UINT8 irq_num, UINT16 iv, const char* source)
{
	dsp56k_interrupt_sources[irq_num].irq_vector = iv;
	strcpy(dsp56k_interrupt_sources[irq_num].irq_source, source);
}

// 1-14 + 1-18
static void dsp56k_irq_table_init(void)
{
	// TODO: Cull host command stuff appropriately
	/* array index . vector . token */
	dsp56k_set_irq_source(0,  0x0000, "Hardware RESET");
	dsp56k_set_irq_source(1,  0x0002, "Illegal Instruction");
	dsp56k_set_irq_source(2,  0x0004, "Stack Error");
	dsp56k_set_irq_source(3,  0x0006, "Reserved");
	dsp56k_set_irq_source(4,  0x0008, "SWI");
	dsp56k_set_irq_source(5,  0x000a, "IRQA");
	dsp56k_set_irq_source(6,  0x000c, "IRQB");
	dsp56k_set_irq_source(7,  0x000e, "Reserved");
	dsp56k_set_irq_source(8,  0x0010, "SSI0 Receive Data with Exception");
	dsp56k_set_irq_source(9,  0x0012, "SSI0 Receive Data");
	dsp56k_set_irq_source(10, 0x0014, "SSI0 Transmit Data with Exception");
	dsp56k_set_irq_source(11, 0x0016, "SSI0 Transmit Data");
	dsp56k_set_irq_source(12, 0x0018, "SSI1 Receive Data with Exception");
	dsp56k_set_irq_source(13, 0x001a, "SSI1 Receive Data");
	dsp56k_set_irq_source(14, 0x001c, "SSI1 Transmit Data with Exception");
	dsp56k_set_irq_source(15, 0x001e, "SSI1 Transmit Data");
	dsp56k_set_irq_source(16, 0x0020, "Timer Overflow");
	dsp56k_set_irq_source(17, 0x0022, "Timer Compare");
	dsp56k_set_irq_source(18, 0x0024, "Host DMA Receive Data");
	dsp56k_set_irq_source(19, 0x0026, "Host DMA Transmit Data");
	dsp56k_set_irq_source(20, 0x0028, "Host Receive Data");
	dsp56k_set_irq_source(21, 0x002a, "Host Transmit Data");
	dsp56k_set_irq_source(22, 0x002c, "Host Command");				/* Default vector for the host command */
	dsp56k_set_irq_source(23, 0x002e, "Codec Receive/Transmit");
	dsp56k_set_irq_source(24, 0x0030, "Host Command 1");
	dsp56k_set_irq_source(25, 0x0032, "Host Command 2");
	dsp56k_set_irq_source(26, 0x0034, "Host Command 3");
	dsp56k_set_irq_source(27, 0x0036, "Host Command 4");
	dsp56k_set_irq_source(28, 0x0038, "Host Command 5");
	dsp56k_set_irq_source(29, 0x003a, "Host Command 6");
	dsp56k_set_irq_source(30, 0x003c, "Host Command 7");
	dsp56k_set_irq_source(31, 0x003e, "Host Command 8");
}

static void dsp56k_clear_pending_interrupts(void)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		core.PCU.pending_interrupts[i] = -1;
	}
}

static int dsp56k_count_pending_interrupts(void)
{
	int numI = 0;
	while (core.PCU.pending_interrupts[numI] != -1)
	{
		numI++;
	}

	return numI;
}

static void dsp56k_sort_pending_interrupts(int num)
{
	int i, j;

	// We're going to be sorting the priorities
	int priority_list[32];
	for (i = 0; i < num; i++)
	{
		priority_list[i] = dsp56k_get_irq_priority(core.PCU.pending_interrupts[i]);
	}

	// Bubble sort should be good enough for us
	for (i = 0; i < num; i++)
	{
	    for(j = 0; j < num-1; j++)
		{
			if (priority_list[j] > priority_list[j+1])
			{
				int holder;

				// Swap priorities
				holder = priority_list[j+1];
				priority_list[j+1] = priority_list[j];
				priority_list[j] = holder;

				// Swap irq indices.
				holder = core.PCU.pending_interrupts[j+1];
				core.PCU.pending_interrupts[j+1] = core.PCU.pending_interrupts[j];
				core.PCU.pending_interrupts[j] = holder;
			}
		}
	}

	// TODO: 1-17 Now sort each of the priority levels within their categories.
}

static INT8 dsp56k_get_irq_priority(int index)
{
	// 1-12
	switch (index)
	{
		// Non-maskable
		case 0:  return 3; // Hardware RESET
		case 1:  return 3; // Illegal Instruction
		case 2:  return 3; // Stack Error
		case 3:  return 3; // Reserved
		case 4:  return 3; // SWI

		// Poll the IPR for these guys.
		case 5:  return irqa_ipl();  // IRQA
		case 6:  return irqb_ipl();  // IRQB
		case 7:  return -1;          // Reserved
		case 8:  return ssi0_ipl();  // SSI0 Receive Data with Exception
		case 9:  return ssi0_ipl();  // SSI0 Receive Data
		case 10: return ssi0_ipl();  // SSI0 Transmit Data with Exception
		case 11: return ssi0_ipl();  // SSI0 Transmit Data
		case 12: return ssi1_ipl();  // SSI1 Receive Data with Exception
		case 13: return ssi1_ipl();  // SSI1 Receive Data
		case 14: return ssi1_ipl();  // SSI1 Transmit Data with Exception
		case 15: return ssi1_ipl();  // SSI1 Transmit Data
		case 16: return tm_ipl();    // Timer Overflow
		case 17: return tm_ipl();    // Timer Compare
		case 18: return host_ipl();  // Host DMA Receive Data
		case 19: return host_ipl();  // Host DMA Transmit Data
		case 20: return host_ipl();  // Host Receive Data
		case 21: return host_ipl();  // Host Transmit Data
		case 22: return host_ipl();  // Host Command 0 (Default)
		case 23: return codec_ipl(); // Codec Receive/Transmit
		case 24: return host_ipl();  // Host Command 1              // TODO: Are all host ipl's the same?
		case 25: return host_ipl();  // Host Command 2
		case 26: return host_ipl();  // Host Command 3
		case 27: return host_ipl();  // Host Command 4
		case 28: return host_ipl();  // Host Command 5
		case 29: return host_ipl();  // Host Command 6
		case 30: return host_ipl();  // Host Command 7
		case 31: return host_ipl();  // Host Command 8

		default: break;
	}

	return -1;
}

static int dsp56k_get_irq_index_by_tag(const char* tag)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		if (strcmp(tag, dsp56k_interrupt_sources[i].irq_source) == 0)
		{
			return i;
		}
	}

	fatalerror("DSP56K ERROR : IRQ TAG specified incorrectly (get_vector_by_tag) : %s.\n", tag);
	return -1;
}
