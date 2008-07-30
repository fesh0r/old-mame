/*

    SCSP (YMF292-F) header
*/

#ifndef _SCSP_H_
#define _SCSP_H_

struct SCSPinterface
{
	int roffset;				/* offset in the region */
	void (*irq_callback)(running_machine *machine, int state);	/* irq callback */
};

void SCSP_set_ram_base(int which, void *base);

// SCSP register access
READ16_HANDLER( SCSP_0_r );
WRITE16_HANDLER( SCSP_0_w );
READ16_HANDLER( SCSP_1_r );
WRITE16_HANDLER( SCSP_1_w );

// MIDI I/O access (used for comms on Model 2/3)
WRITE16_HANDLER( SCSP_MidiIn );
READ16_HANDLER( SCSP_MidiOutR );

extern UINT32* stv_scu;

#endif
