/***************************************************************************

							  -= Metro Games =-

				driver by	Luca Elia (eliavit@unina.it)


Main  CPU    :  MC68000

Video Chips  :  Imagetek 14100 052 9227KK701	Or
                Imagetek 14220 071 9338EK707

Sound Chips  :	NEC78C10 (CPU, unemulated) + OKIM6295 + YM2143  or
                YRW801-M + YMF278B (unemulated)

Other        :  Memory Blitter

---------------------------------------------------------------------------
Year + Game						PCB			Issues / Notes
---------------------------------------------------------------------------
92	The Karate Tournament		?
92	Last Fortress - Toride		VG420
92	Pang Poms					VG420
92	Sky Alert					VG420
93? Moeyo Gonta!!               VG460-(B)
93	Poitto!						MTR5260-A
94	Dharma Doujou				?
94  Toride II Adauchi Gaiden	MTR5260-A
95	Daitoride					MTR5260-A
95	Puzzli 						MTR5260-A
95	Pururun						MTR5260-A
96	Bal Cube					?			Preliminary: wrong colors, some wrong sprites
94  Blazing Tornado             ?           Also has Konami 053936 gfx chip
---------------------------------------------------------------------------
Not dumped yet:
94	Gun Master
94	Toride II

To Do:

-	Priorities (pdrawgfxzoom). Priorities are particularly bad in blzntrnd.
-	Sprite palette marking doesn't know about 8bpp.
-	1 pixel granularity in the window's placement (8 pixels now, see daitorid)
-	Sound (as soon as the NEC78C10 gets emulated)
-	Coin lockout
-	Most games, in service mode, seem to require that you press
	start1&2 *exactly at once* in order to advance to the next
	screen (e.g. holding 1 then pressing 2 doesn't work)
-	Some gfx problems in ladykill
-	To save memory, 8bpp tiles are handled fetching data directly from the ROMs,
	without decoding them to a separate buffer, so screen rotation is not
	supported.

Notes:
- To enter service mode in Lady Killer, toggle the dip switch and reset keeping
  start 2 pressed.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/upd7810/upd7810.h"

/* Variables defined in vidhrdw: */

extern data16_t *metro_videoregs;
extern data16_t *metro_screenctrl;
extern data16_t *metro_scroll;
extern data16_t *metro_tiletable;
extern data16_t *metro_vram_0, *metro_vram_1, *metro_vram_2;
extern data16_t *metro_window;
extern data16_t *metro_K053936_ram,*metro_K053936_ctrl;
WRITE16_HANDLER( metro_K053936_w );


/* Functions defined in vidhrdw: */

WRITE16_HANDLER( metro_paletteram_w );

WRITE16_HANDLER( metro_tiletable_w );
WRITE16_HANDLER( metro_window_w );

WRITE16_HANDLER( metro_vram_0_w );
WRITE16_HANDLER( metro_vram_1_w );
WRITE16_HANDLER( metro_vram_2_w );


int  metro_vh_start_14100(void);
int  metro_vh_start_14220(void);
int  blzntrnd_vh_start(void);
void metro_vh_stop(void);

void metro_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************


								Interrupts


***************************************************************************/

static data16_t irq_enable;

static UINT8 irq_line;

static UINT8 vblank_irq;
static UINT8 blitter_irq;
static UINT8 unknown1_irq;
static UINT8 unknown2_irq;
static UINT8 unknown3_irq;
static UINT8 unknown4_irq;


WRITE16_HANDLER( metro_irq_enable_w )
{
	COMBINE_DATA( &irq_enable );
}


READ16_HANDLER( metro_irq_cause_r )
{
	return	vblank_irq   * 0x01 +
			unknown1_irq * 0x02 +
			blitter_irq  * 0x04 +
			unknown2_irq * 0x08 +
			unknown3_irq * 0x10 +
			unknown4_irq * 0x20 ;
}


/* Update the IRQ state based on all possible causes */
static void update_irq_state(void)
{
	int state =	(metro_irq_cause_r(0,0) & ~irq_enable) ? ASSERT_LINE : CLEAR_LINE;
	cpu_set_irq_line(0, irq_line, state);
}


WRITE16_HANDLER( metro_irq_cause_w )
{
	if (data & ~0x15)	logerror("CPU #0 PC %06X : unknown bits of irqcause written: %04X\n",cpu_get_pc(),data);

	if (ACCESSING_LSB)
	{
		data &= ~irq_enable;
		if (data & 0x01)	vblank_irq   = 0;
		if (data & 0x02)	unknown1_irq = 0;	// DAITORIDE, BALCUBE, KARATOUR
		if (data & 0x04)	blitter_irq  = 0;
		if (data & 0x08)	unknown2_irq = 0;	// KARATOUR
		if (data & 0x10)	unknown3_irq = 0;
		if (data & 0x20)	unknown4_irq = 0;	// KARATOUR, BLZNTRND
	}

	update_irq_state();
}


int metro_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 0:
			vblank_irq = 1;
			update_irq_state();
			break;

		default:
			unknown3_irq = 1;
			update_irq_state();
			break;
	}
	return ignore_interrupt();
}


static void vblank_end_callback(int param)
{
	unknown4_irq = param;
}

/* lev 2-7 (lev 1 seems sound related) */
int karatour_interrupt(void)
{
	switch ( cpu_getiloops() )
	{
		case 0:
			vblank_irq = 1;
			unknown4_irq = 1;	// write the scroll registers
			timer_set(TIME_IN_USEC(DEFAULT_REAL_60HZ_VBLANK_DURATION), 0, vblank_end_callback);
			update_irq_state();
			break;

		default:
			unknown3_irq = 1;
			update_irq_state();
			break;
	}
	return ignore_interrupt();
}

/***************************************************************************


							Sound Communication


***************************************************************************/

static int metro_io_callback(int ioline, int state)
{
	data8_t data;

    switch ( ioline )
	{
	case UPD7810_RXD:	/* read the RxD line */
		data = soundlatch_r(0);
		state = data & 1;
		soundlatch_w(0, data >> 1);
		break;
	default:
		logerror("upd7810 ioline %d not handled\n", ioline);
    }
	return state;
}

WRITE16_HANDLER( metro_soundlatch_w )
{
	if ( ACCESSING_LSB && (Machine->sample_rate != 0) )
	{
		soundlatch_w(0,data & 0xff);
		cpu_set_nmi_line( 1, PULSE_LINE );
	}
}

data16_t metro_soundstatus;

WRITE16_HANDLER( metro_soundstatus_w )
{
	if (ACCESSING_LSB)
	{
		metro_soundstatus = (~data) & 1;
	}
	if (data & ~1)	logerror("CPU #0 PC %06X : unknown bits of soundstatus written: %04X\n",cpu_get_pc(),data);
}


READ16_HANDLER( metro_soundstatus_r )
{
	return metro_soundstatus & 1;
}


READ16_HANDLER( dharma_soundstatus_r )
{
	return readinputport(0) | (metro_soundstatus ? 0x80 : 0);
}

static WRITE_HANDLER( daitorid_sound_rombank_w )
{
	unsigned char *RAM = memory_region(REGION_CPU2);
	int bank = (data >> 4) & 0x07;

	if ( data & ~0x70 ) 	logerror("CPU #1 - PC %04X: unknown bank bits: %02X\n",cpu_get_pc(),data);

	if (bank < 2)	RAM = &RAM[0x4000 * bank];
	else			RAM = &RAM[0x4000 * (bank-2) + 0x10000];

	cpu_setbank(1, RAM);
}

static data8_t chip_select;

static READ_HANDLER( daitorid_sound_chip_data_r )
{
	/* fake status to get the 7810 out of the tight loop waiting for a chip */
    static data8_t toggle_bit7;
    switch( chip_select )
	{
	case 0xb7: return YM2151_status_port_0_r(0);
	case 0xe7: return OKIM6295_status_0_r(0);
	default:
		logerror("CPU #1 PC %04X : reading from unknown chip: %02X\n",cpu_get_pc(),chip_select);
		toggle_bit7 ^= 0x80;
        return toggle_bit7;
	}
}

static WRITE_HANDLER( daitorid_sound_chip_data_w )
{
	soundlatch2_w(0,data);	// for debugging, the latch is internal
}

static READ_HANDLER( daitorid_sound_chip_select_r )
{
	return chip_select;
}

static WRITE_HANDLER( daitorid_sound_chip_select_w )
{
	chip_select = data;

	if ((chip_select & 0xf0) == 0xf0)	return;

	switch( chip_select )
	{
	case 0x7f: metro_soundstatus = 0; break;
	case 0xb9: YM2151_register_port_0_w(0,soundlatch2_r(0)); break;
	case 0xbb: YM2151_data_port_0_w(0,soundlatch2_r(0)); break;
	case 0xeb: OKIM6295_data_0_w(0,soundlatch2_r(0)); break;
	default:
		logerror("CPU #1 PC %04X : writing to unknown chip: %02X\n",cpu_get_pc(),chip_select);
	}
}

static MEMORY_READ_START( upd7810_readmem )
    { 0x0000, 0x3fff, MRA_ROM               },  /* External ROM */
    { 0x4000, 0x7fff, MRA_BANK1             },  /* External ROM (Banked) */
    { 0x8000, 0x87ff, MRA_RAM               },  /* External RAM */
    { 0xff00, 0xffff, MRA_RAM               },  /* Internal RAM */
MEMORY_END

static MEMORY_WRITE_START( upd7810_writemem )
    { 0x0000, 0x3fff, MWA_ROM               },  /* External ROM */
    { 0x4000, 0x7fff, MWA_ROM               },  /* External ROM (Banked) */
    { 0x8000, 0x87ff, MWA_RAM               },  /* External RAM */
    { 0xff00, 0xffff, MWA_RAM               },  /* Internal RAM */
MEMORY_END

static PORT_READ_START( upd7810_readport )
	{ UPD7810_PORTA, UPD7810_PORTA, daitorid_sound_chip_data_r		},
	{ UPD7810_PORTB, UPD7810_PORTB, daitorid_sound_chip_select_r	},
PORT_END

static PORT_WRITE_START( upd7810_writeport )
	{ UPD7810_PORTA, UPD7810_PORTA, daitorid_sound_chip_data_w		},
	{ UPD7810_PORTB, UPD7810_PORTB, daitorid_sound_chip_select_w	},
	{ UPD7810_PORTC, UPD7810_PORTC, daitorid_sound_rombank_w		},
PORT_END

static void metro_sound_irq_handler(int state)
{
	cpu_set_irq_line(1, UPD7810_INTF2, HOLD_LINE);
}

static struct YM2151interface daitorid_ym2151_interface =
{
	1,
	2000000,			/* ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ metro_sound_irq_handler },	/* irq handler */
	{ 0 }							/* port_write */
};

static struct OKIM6295interface daitorid_okim6295_interface =
{
	1,
	{ 8000 },	/* ? */
	{ REGION_SOUND1 },
	{ 50 }
};

/***************************************************************************


								Coin Lockout


***************************************************************************/

/* IT DOESN'T WORK PROPERLY */

WRITE16_HANDLER( metro_coin_lockout_1word_w )
{
	if (ACCESSING_LSB)
	{
//		coin_lockout_w(0, data & 1);
//		coin_lockout_w(1, data & 2);
	}
	if (data & ~3)	logerror("CPU #0 PC %06X : unknown bits of coin lockout written: %04X\n",cpu_get_pc(),data);
}


WRITE16_HANDLER( metro_coin_lockout_4words_w )
{
//	coin_lockout_w( (offset >> 1) & 1, offset & 1 );
	if (data & ~1)	logerror("CPU #0 PC %06X : unknown bits of coin lockout written: %04X\n",cpu_get_pc(),data);
}




/***************************************************************************


								Banked ROM access


***************************************************************************/

/*
	The main CPU has access to the ROMs that hold the graphics through
	a banked window of 64k. Those ROMs also usually store the tables for
	the virtual tiles set. The tile codes to be written to the tilemap
	memory to render the backgrounds are also stored here, in a format
	that the blitter can readily use (which is a form of compression)
*/

data16_t *metro_rombank;

READ16_HANDLER( metro_bankedrom_r )
{
	const int region = REGION_GFX1;

	data8_t *ROM = memory_region( region );
	size_t  len  = memory_region_length( region );

	offset = offset * 2 + 0x10000 * (*metro_rombank);

	if ( offset < len )	return ((ROM[offset+0]<<8)+ROM[offset+1])^0xffff;
	else				return 0xffff;
}




/***************************************************************************


									Blitter

	[ Registers ]

		Offset:		Value:

		0.l			Destination Tilemap      (1,2,3)
		4.l			Blitter Data Address     (byte offset into the gfx ROMs)
		8.l			Destination Address << 7 (byte offset into the tilemap)

		The Blitter reads a byte and looks at the most significative
		bits for the opcode, while the remaining bits define a value
		(usually how many bytes to write). The opcode byte may be
		followed by a number of other bytes:

			76------			Opcode
			--543210			N
			(at most N+1 bytes follow)


		The blitter is designed to write every other byte (e.g. it
		writes a byte and skips the next). Hence 2 blits are needed
		to fill a tilemap (first even, then odd addresses)

	[ Opcodes ]

			0		Copy the following N+1 bytes. If the whole byte
					is $00:	stop and generate an IRQ

			1		Fill N+1 bytes with a sequence, starting with
					the  value in the following byte

			2		Fill N+1 bytes with the value in the following
					byte

			3		Skip N+1 bytes. If the whole byte is $C0:
					skip to the next row of the tilemap (+0x200 bytes)
					but preserve the column passed at the start of the
					blit (destination address % 0x200)


***************************************************************************/

data16_t *metro_blitter_regs;

void metro_blit_done(int param)
{
	blitter_irq = 1;
	update_irq_state();
}

INLINE int blt_read(const data8_t *ROM, const int offs)
{
	return ROM[offs] ^ 0xff;
}

INLINE void blt_write(const int tmap, const offs_t offs, const data16_t data, const data16_t mask)
{
	switch( tmap )
	{
		case 1:	metro_vram_0_w(offs,data,mask);	break;
		case 2:	metro_vram_1_w(offs,data,mask);	break;
		case 3:	metro_vram_2_w(offs,data,mask);	break;
	}
//	logerror("CPU #0 PC %06X : Blitter %X] %04X <- %04X & %04X\n",cpu_get_pc(),tmap,offs,data,mask);
}


WRITE16_HANDLER( metro_blitter_w )
{
	COMBINE_DATA( &metro_blitter_regs[offset] );

	if (offset == 0xC/2)
	{
		const int region = REGION_GFX1;

		data8_t *src	=	memory_region(region);
		size_t  src_len	=	memory_region_length(region);

		UINT32 tmap		=	(metro_blitter_regs[ 0x00 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x02 / 2 ];
		UINT32 src_offs	=	(metro_blitter_regs[ 0x04 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x06 / 2 ];
		UINT32 dst_offs	=	(metro_blitter_regs[ 0x08 / 2 ] << 16 ) +
							 metro_blitter_regs[ 0x0a / 2 ];

		int shift			=	(dst_offs & 0x80) ? 0 : 8;
		data16_t mask		=	(dst_offs & 0x80) ? 0xff00 : 0x00ff;

		logerror("CPU #0 PC %06X : Blitter regs %08X, %08X, %08X\n",cpu_get_pc(),tmap,src_offs,dst_offs);

		dst_offs >>= 7+1;
		switch( tmap )
		{
			case 1:
			case 2:
			case 3:
				break;
			default:
				logerror("CPU #0 PC %06X : Blitter unknown destination: %08X\n",cpu_get_pc(),tmap);
				return;
		}

		while (1)
		{
			data16_t b1,b2,count;

			src_offs %= src_len;
			b1 = blt_read(src,src_offs);
//			logerror("CPU #0 PC %06X : Blitter opcode %02X at %06X\n",cpu_get_pc(),b1,src_offs);
			src_offs++;

			count = ((~b1) & 0x3f) + 1;

			switch( (b1 & 0xc0) >> 6 )
			{
				case 0:

					/* Stop and Generate an IRQ. We can't generate it now
					   both because it's unlikely that the blitter is so
					   fast and because some games (e.g. lastfort) need to
					   complete the blitter irq service routine before doing
					   another blit. */
					if (b1 == 0)
					{
						timer_set(TIME_IN_USEC(500),0,metro_blit_done);
						return;
					}

					/* Copy */
					while (count--)
					{
						src_offs %= src_len;
						b2 = blt_read(src,src_offs) << shift;
						src_offs++;

						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 1:

					/* Fill with an increasing value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs);
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2<<shift,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
						b2++;
					}
					break;


				case 2:

					/* Fill with a fixed value */
					src_offs %= src_len;
					b2 = blt_read(src,src_offs) << shift;
					src_offs++;

					while (count--)
					{
						dst_offs &= 0xffff;
						blt_write(tmap,dst_offs,b2,mask);
						dst_offs = ((dst_offs+1) & (0x100-1)) | (dst_offs & (~(0x100-1)));
					}
					break;


				case 3:

					/* Skip to the next line ?? */
					if (b1 == 0xC0)
					{
						dst_offs +=   0x100;
						dst_offs &= ~(0x100-1);
						dst_offs |=  (0x100-1) & (metro_blitter_regs[ 0x0a / 2 ] >> (7+1));
					}
					else
					{
						dst_offs += count;
					}
					break;


				default:
					logerror("CPU #0 PC %06X : Blitter unknown opcode %02X at %06X\n",cpu_get_pc(),b1,src_offs-1);
					return;
			}

		}
	}

}


/***************************************************************************


								Memory Maps


***************************************************************************/

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/

/***************************************************************************
									Bal Cube
***************************************************************************/

/* Really weird way of mapping 3 DSWs (only 2 used) */
static READ16_HANDLER( balcube_dsw_r )
{
	data16_t dsw = readinputport(2);

	switch (offset*2)
	{
		/* The 3rd (unused) DSW is read from bit 7 of the following range */
		case 0x1FFFC:	return (dsw & 0x0001) ? 0x0040 : 0x0000;
		case 0x1FFFA:	return (dsw & 0x0002) ? 0x0040 : 0x0000;
		case 0x1FFF6:	return (dsw & 0x0004) ? 0x0040 : 0x0000;
		case 0x1FFEE:	return (dsw & 0x0008) ? 0x0040 : 0x0000;
		case 0x1FFDE:	return (dsw & 0x0010) ? 0x0040 : 0x0000;
		case 0x1FFBE:	return (dsw & 0x0020) ? 0x0040 : 0x0000;
		case 0x1FF7E:	return (dsw & 0x0040) ? 0x0040 : 0x0000;
		case 0x1FEFE:	return (dsw & 0x0080) ? 0x0040 : 0x0000;

		case 0x1FDFE:	return (dsw & 0x0100) ? 0x0040 : 0x0000;
		case 0x1FBFE:	return (dsw & 0x0200) ? 0x0040 : 0x0000;
		case 0x1F7FE:	return (dsw & 0x0400) ? 0x0040 : 0x0000;
		case 0x1EFFE:	return (dsw & 0x0800) ? 0x0040 : 0x0000;
		case 0x1DFFE:	return (dsw & 0x1000) ? 0x0040 : 0x0000;
		case 0x1BFFE:	return (dsw & 0x2000) ? 0x0040 : 0x0000;
		case 0x17FFE:	return (dsw & 0x4000) ? 0x0040 : 0x0000;
		case 0x0FFFE:	return (dsw & 0x8000) ? 0x0040 : 0x0000;
	}
	logerror("CPU #0 PC %06X : unknown dsw address read: %04X\n",cpu_get_pc(),offset);
	return 0xffff;
}


static MEMORY_READ16_START( balcube_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0xf00000, 0xf0ffff, MRA16_RAM				},	// RAM
	{ 0x300000, 0x300001, MRA16_NOP				},	// Sound
	{ 0x400000, 0x41ffff, balcube_dsw_r			},	// DSW x 2
	{ 0x600000, 0x61ffff, MRA16_RAM				},	// Layer 0
	{ 0x620000, 0x63ffff, MRA16_RAM				},	// Layer 1
	{ 0x640000, 0x65ffff, MRA16_RAM				},	// Layer 2
	{ 0x660000, 0x66ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x670000, 0x673fff, MRA16_RAM				},	// Palette
	{ 0x674000, 0x674fff, MRA16_RAM				},	// Sprites
	{ 0x678000, 0x6787ff, MRA16_RAM				},	// Tiles Set
	{ 0x6788a2, 0x6788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x500000, 0x500001, input_port_0_word_r	},	// Inputs
	{ 0x500002, 0x500003, input_port_1_word_r	},	//
	{ 0x500006, 0x500007, MRA16_NOP				},	//
MEMORY_END

static MEMORY_WRITE16_START( balcube_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0xf00000, 0xf0ffff, MWA16_RAM						},	// RAM
	{ 0x300000, 0x30000b, MWA16_NOP						},	// Sound
	{ 0x500002, 0x500009, metro_coin_lockout_4words_w	},	// Coin Lockout
	{ 0x670000, 0x673fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x674000, 0x674fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x600000, 0x61ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x620000, 0x63ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x640000, 0x65ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x678000, 0x6787ff, metro_tiletable_w, &metro_tiletable		},	// Tiles Set
	{ 0x678840, 0x67884d, metro_blitter_w, &metro_blitter_regs		},	// Tiles Blitter
	{ 0x678860, 0x67886b, metro_window_w, &metro_window				},	// Tilemap Window
	{ 0x678870, 0x67887b, MWA16_RAM, &metro_scroll		},	// Scroll
	{ 0x678880, 0x678881, MWA16_NOP						},	// ? increasing
	{ 0x678890, 0x678891, MWA16_NOP						},	// ? increasing
	{ 0x6788a2, 0x6788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x6788a4, 0x6788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x6788aa, 0x6788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x6788ac, 0x6788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x679700, 0x679713, MWA16_RAM, &metro_videoregs	},	// Video Registers
MEMORY_END


/***************************************************************************
								Dai Toride
***************************************************************************/

static MEMORY_READ16_START( daitorid_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x800000, 0x80ffff, MRA16_RAM				},	// RAM
	{ 0x400000, 0x41ffff, MRA16_RAM				},	// Layer 0
	{ 0x420000, 0x43ffff, MRA16_RAM				},	// Layer 1
	{ 0x440000, 0x45ffff, MRA16_RAM				},	// Layer 2
	{ 0x460000, 0x46ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x470000, 0x473fff, MRA16_RAM				},	// Palette
	{ 0x474000, 0x474fff, MRA16_RAM				},	// Sprites
	{ 0x478000, 0x4787ff, MRA16_RAM				},	// Tiles Set
	{ 0x4788a2, 0x4788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0xc00000, 0xc00001, dharma_soundstatus_r	},	// Inputs
	{ 0xc00002, 0xc00003, input_port_1_word_r	},	//
	{ 0xc00004, 0xc00005, input_port_2_word_r	},	//
	{ 0xc00006, 0xc00007, input_port_3_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( daitorid_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0x800000, 0x80ffff, MWA16_RAM						},	// RAM
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x478000, 0x4787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x478860, 0x47886b, metro_window_w, &metro_window			},	// Tilemap Window
	{ 0x478870, 0x47887b, MWA16_RAM, &metro_scroll		},	// Scroll
	{ 0x478880, 0x478881, MWA16_NOP						},	// ? increasing
	{ 0x478890, 0x478891, MWA16_NOP						},	// ? increasing
	{ 0x4788a2, 0x4788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x4788a4, 0x4788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x4788a8, 0x4788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x4788aa, 0x4788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x4788ac, 0x4788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x479700, 0x479713, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	// To Sound CPU
	{ 0xc00002, 0xc00009, metro_coin_lockout_4words_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Dharma Doujou
***************************************************************************/

static MEMORY_READ16_START( dharma_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x400000, 0x40ffff, MRA16_RAM				},	// RAM
	{ 0x800000, 0x81ffff, MRA16_RAM				},	// Layer 0
	{ 0x820000, 0x83ffff, MRA16_RAM				},	// Layer 1
	{ 0x840000, 0x85ffff, MRA16_RAM				},	// Layer 2
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x870000, 0x873fff, MRA16_RAM				},	// Palette
	{ 0x874000, 0x874fff, MRA16_RAM				},	// Sprites
	{ 0x878000, 0x8787ff, MRA16_RAM				},	// Tiles Set
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0xc00000, 0xc00001, dharma_soundstatus_r	},	// Inputs
	{ 0xc00002, 0xc00003, input_port_1_word_r	},	//
	{ 0xc00004, 0xc00005, input_port_2_word_r	},	//
	{ 0xc00006, 0xc00007, input_port_3_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( dharma_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0x400000, 0x40ffff, MWA16_RAM						},	// RAM
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x878000, 0x8787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	// Scroll Regs
	{ 0x878880, 0x878881, MWA16_NOP						},	// ? increasing
	{ 0x878890, 0x878891, MWA16_NOP						},	// ? increasing
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x8788a4, 0x8788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x879700, 0x879713, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	// To Sound CPU
	{ 0xc00002, 0xc00009, metro_coin_lockout_4words_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Karate Tournament
***************************************************************************/

/* This game uses almost only the blitter to write to the tilemaps.
   The CPU can only access a "window" of 512x256 pixels in the upper
   left corner of the big tilemap */

#define KARATOUR_OFFS( _x_ ) ((_x_) & (0x40-1)) + (((_x_) & ~(0x40-1)) * (0x100 / 0x40))

#define KARATOUR_VRAM( _n_ ) \
static READ16_HANDLER( karatour_vram_##_n_##_r ) \
{ \
	return metro_vram_##_n_[KARATOUR_OFFS(offset)]; \
} \
static WRITE16_HANDLER( karatour_vram_##_n_##_w ) \
{ \
	metro_vram_##_n_##_w(KARATOUR_OFFS(offset),data,mem_mask); \
}

KARATOUR_VRAM( 0 )
KARATOUR_VRAM( 1 )
KARATOUR_VRAM( 2 )

static MEMORY_READ16_START( karatour_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0xffc000, 0xffffff, MRA16_RAM				},	// RAM
	{ 0x400000, 0x400001, metro_soundstatus_r	},	// From Sound CPU
	{ 0x400002, 0x400003, input_port_0_word_r	},	// Inputs
	{ 0x400004, 0x400005, input_port_1_word_r	},	//
	{ 0x400006, 0x400007, input_port_2_word_r	},	//
	{ 0x40000a, 0x40000b, input_port_3_word_r	},	//
	{ 0x40000c, 0x40000d, input_port_4_word_r	},	//
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x870000, 0x873fff, MRA16_RAM				},	// Palette
	{ 0x874000, 0x874fff, MRA16_RAM				},	// Sprites
	{ 0x875000, 0x875fff, karatour_vram_0_r		},	// Layer 0 (Part of)
	{ 0x876000, 0x876fff, karatour_vram_1_r		},	// Layer 1 (Part of)
	{ 0x877000, 0x877fff, karatour_vram_2_r		},	// Layer 2 (Part of)
	{ 0x878000, 0x8787ff, MRA16_RAM				},	// Tiles Set
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	// IRQ Cause
MEMORY_END

static MEMORY_WRITE16_START( karatour_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0xffc000, 0xffffff, MWA16_RAM						},	// RAM
	{ 0x400000, 0x400001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x400002, 0x400003, metro_coin_lockout_1word_w	},	// Coin Lockout
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x875000, 0x875fff, karatour_vram_0_w				},	// Layer 0 (Part of)
	{ 0x876000, 0x876fff, karatour_vram_1_w				},	// Layer 1 (Part of)
	{ 0x877000, 0x877fff, karatour_vram_2_w				},	// Layer 2 (Part of)
	{ 0x878000, 0x8787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	// Scroll
	{ 0x878880, 0x878881, MWA16_NOP						},	// ? increasing
	{ 0x878890, 0x878891, MWA16_NOP						},	// ? increasing
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x8788a4, 0x8788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
MEMORY_END


/***************************************************************************
								Last Fortress
***************************************************************************/

static MEMORY_READ16_START( lastfort_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x400000, 0x40ffff, MRA16_RAM				},	// RAM
	{ 0x800000, 0x81ffff, MRA16_RAM				},	// Layer 0
	{ 0x820000, 0x83ffff, MRA16_RAM				},	// Layer 1
	{ 0x840000, 0x85ffff, MRA16_RAM				},	// Layer 2
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x870000, 0x873fff, MRA16_RAM				},	// Palette
	{ 0x874000, 0x874fff, MRA16_RAM				},	// Sprites
	{ 0x878000, 0x8787ff, MRA16_RAM				},	// Tiles Set
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0xc00000, 0xc00001, metro_soundstatus_r	},	// From Sound CPU
	{ 0xc00002, 0xc00003, MRA16_NOP				},	//
	{ 0xc00004, 0xc00005, input_port_0_word_r	},	// Inputs
	{ 0xc00006, 0xc00007, input_port_1_word_r	},	//
	{ 0xc00008, 0xc00009, input_port_2_word_r	},	//
	{ 0xc0000a, 0xc0000b, input_port_3_word_r	},	//
	{ 0xc0000c, 0xc0000d, input_port_4_word_r	},	//
	{ 0xc0000e, 0xc0000f, input_port_5_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( lastfort_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0x400000, 0x40ffff, MWA16_RAM						},	// RAM
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x878000, 0x8787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	// Scroll
	{ 0x878880, 0x878881, MWA16_NOP						},	// ? increasing
	{ 0x878890, 0x878891, MWA16_NOP						},	// ? increasing
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x8788a4, 0x8788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0xc00000, 0xc00001, metro_soundstatus_w			},	// To Sound CPU
	{ 0xc00002, 0xc00003, metro_coin_lockout_1word_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Pang Poms
***************************************************************************/

static MEMORY_READ16_START( pangpoms_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0xc00000, 0xc0ffff, MRA16_RAM				},	// RAM
	{ 0x400000, 0x41ffff, MRA16_RAM				},	// Layer 0
	{ 0x420000, 0x43ffff, MRA16_RAM				},	// Layer 1
	{ 0x440000, 0x45ffff, MRA16_RAM				},	// Layer 2
	{ 0x460000, 0x46ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x470000, 0x473fff, MRA16_RAM				},	// Palette
	{ 0x474000, 0x474fff, MRA16_RAM				},	// Sprites
	{ 0x478000, 0x4787ff, MRA16_RAM				},	// Tiles Set
	{ 0x4788a2, 0x4788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x800000, 0x800001, metro_soundstatus_r	},	// From Sound CPU
	{ 0x800002, 0x800003, MRA16_NOP				},	//
	{ 0x800004, 0x800005, input_port_0_word_r	},	// Inputs
	{ 0x800006, 0x800007, input_port_1_word_r	},	//
	{ 0x800008, 0x800009, input_port_2_word_r	},	//
	{ 0x80000a, 0x80000b, input_port_3_word_r	},	//
	{ 0x80000c, 0x80000d, input_port_4_word_r	},	//
	{ 0x80000e, 0x80000f, input_port_5_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( pangpoms_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0xc00000, 0xc0ffff, MWA16_RAM						},	// RAM
	{ 0x400000, 0x41ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x420000, 0x43ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x440000, 0x45ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x470000, 0x473fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x474000, 0x474fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x478000, 0x4787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x478800, 0x478813, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x478840, 0x47884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x478860, 0x47886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0x478870, 0x47887b, MWA16_RAM, &metro_scroll		},	// Scroll Regs
	{ 0x478880, 0x478881, MWA16_NOP						},	// ? increasing
	{ 0x478890, 0x478891, MWA16_NOP						},	// ? increasing
	{ 0x4788a2, 0x4788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x4788a4, 0x4788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x4788a8, 0x4788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x4788aa, 0x4788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x4788ac, 0x4788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x800000, 0x800001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x800002, 0x800003, metro_coin_lockout_1word_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Poitto!
***************************************************************************/

static MEMORY_READ16_START( poitto_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x400000, 0x40ffff, MRA16_RAM				},	// RAM
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	// Layer 0
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	// Layer 1
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	// Layer 2
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	// Palette
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	// Sprites
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	// Tiles Set
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x800000, 0x800001, dharma_soundstatus_r	},	// Inputs
	{ 0x800002, 0x800003, input_port_1_word_r	},	//
	{ 0x800004, 0x800005, input_port_2_word_r	},	//
	{ 0x800006, 0x800007, input_port_3_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( poitto_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0x400000, 0x40ffff, MWA16_RAM						},	// RAM
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0xc78000, 0xc787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0xc78800, 0xc78813, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	// Scroll Regs
	{ 0xc78880, 0xc78881, MWA16_NOP						},	// ? increasing
	{ 0xc78890, 0xc78891, MWA16_NOP						},	// ? increasing
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0xc788a4, 0xc788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x800000, 0x800001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x800002, 0x800009, metro_coin_lockout_4words_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Sky Alert
***************************************************************************/

static MEMORY_READ16_START( skyalert_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0xc00000, 0xc0ffff, MRA16_RAM				},	// RAM
	{ 0x800000, 0x81ffff, MRA16_RAM				},	// Layer 0
	{ 0x820000, 0x83ffff, MRA16_RAM				},	// Layer 1
	{ 0x840000, 0x85ffff, MRA16_RAM				},	// Layer 2
	{ 0x860000, 0x86ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x870000, 0x873fff, MRA16_RAM				},	// Palette
	{ 0x874000, 0x874fff, MRA16_RAM				},	// Sprites
	{ 0x878000, 0x8787ff, MRA16_RAM				},	// Tiles Set
	{ 0x8788a2, 0x8788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x400000, 0x400001, metro_soundstatus_r	},	// From Sound CPU
	{ 0x400002, 0x400003, MRA16_NOP				},	//
	{ 0x400004, 0x400005, input_port_0_word_r	},	// Inputs
	{ 0x400006, 0x400007, input_port_1_word_r	},	//
	{ 0x400008, 0x400009, input_port_2_word_r	},	//
	{ 0x40000a, 0x40000b, input_port_3_word_r	},	//
	{ 0x40000c, 0x40000d, input_port_4_word_r	},	//
	{ 0x40000e, 0x40000f, input_port_5_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( skyalert_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0xc00000, 0xc0ffff, MWA16_RAM						},	// RAM
	{ 0x800000, 0x81ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x820000, 0x83ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x840000, 0x85ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x870000, 0x873fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x874000, 0x874fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x878000, 0x8787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0x878800, 0x878813, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x878840, 0x87884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0x878860, 0x87886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0x878870, 0x87887b, MWA16_RAM, &metro_scroll		},	// Scroll
	{ 0x878880, 0x878881, MWA16_NOP						},	// ? increasing
	{ 0x878890, 0x878891, MWA16_NOP						},	// ? increasing
	{ 0x8788a2, 0x8788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x8788a4, 0x8788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x8788a8, 0x8788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0x8788aa, 0x8788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x8788ac, 0x8788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x400000, 0x400001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x400002, 0x400003, metro_coin_lockout_1word_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
								Pururun
***************************************************************************/

static MEMORY_READ16_START( pururun_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM				},	// ROM
	{ 0x800000, 0x80ffff, MRA16_RAM				},	// RAM
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	// Layer 0
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	// Layer 1
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	// Layer 2
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	// Palette
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	// Sprites
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	// Tiles Set
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x400000, 0x400001, dharma_soundstatus_r	},	// Inputs
	{ 0x400002, 0x400003, input_port_1_word_r	},	//
	{ 0x400004, 0x400005, input_port_2_word_r	},	//
	{ 0x400006, 0x400007, input_port_3_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( pururun_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM						},	// ROM
	{ 0x800000, 0x80ffff, MWA16_RAM						},	// RAM
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0xc78000, 0xc787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	// Scroll Regs
	{ 0xc78880, 0xc78881, MWA16_NOP						},	// ? increasing
	{ 0xc78890, 0xc78891, MWA16_NOP						},	// ? increasing
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0xc788a4, 0xc788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0xc79700, 0xc79713, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x400000, 0x400001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x400002, 0x400009, metro_coin_lockout_4words_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
							Toride II Adauchi Gaiden
***************************************************************************/

static MEMORY_READ16_START( toride2g_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM				},	// ROM
	{ 0x400000, 0x4cffff, MRA16_RAM				},	// RAM (4xc000-4xffff mirrored?)
	{ 0xc00000, 0xc1ffff, MRA16_RAM				},	// Layer 0
	{ 0xc20000, 0xc3ffff, MRA16_RAM				},	// Layer 1
	{ 0xc40000, 0xc5ffff, MRA16_RAM				},	// Layer 2
	{ 0xc60000, 0xc6ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0xc70000, 0xc73fff, MRA16_RAM				},	// Palette
	{ 0xc74000, 0xc74fff, MRA16_RAM				},	// Sprites
	{ 0xc78000, 0xc787ff, MRA16_RAM				},	// Tiles Set
	{ 0xc788a2, 0xc788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0x800000, 0x800001, dharma_soundstatus_r	},	// Inputs
	{ 0x800002, 0x800003, input_port_1_word_r	},	//
	{ 0x800004, 0x800005, input_port_2_word_r	},	//
	{ 0x800006, 0x800007, input_port_3_word_r	},	//
MEMORY_END

static MEMORY_WRITE16_START( toride2g_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM						},	// ROM
	{ 0x400000, 0x4cffff, MWA16_RAM						},	// RAM (4xc000-4xffff mirrored?)
	{ 0xc00000, 0xc1ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0xc20000, 0xc3ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0xc40000, 0xc5ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0xc70000, 0xc73fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0xc74000, 0xc74fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0xc78000, 0xc787ff, metro_tiletable_w, &metro_tiletable	},	// Tiles Set
	{ 0xc78840, 0xc7884d, metro_blitter_w, &metro_blitter_regs	},	// Tiles Blitter
	{ 0xc78860, 0xc7886b, metro_window_w, &metro_window	},	// Tilemap Window
	{ 0xc78870, 0xc7887b, MWA16_RAM, &metro_scroll		},	// Scroll Regs
	{ 0xc78880, 0xc78881, MWA16_NOP						},	// ? increasing
	{ 0xc78890, 0xc78891, MWA16_NOP						},	// ? increasing
	{ 0xc788a2, 0xc788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0xc788a4, 0xc788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0xc788a8, 0xc788a9, metro_soundlatch_w			},	// To Sound CPU
	{ 0xc788aa, 0xc788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0xc788ac, 0xc788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0xc79700, 0xc79713, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x800000, 0x800001, metro_soundstatus_w			},	// To Sound CPU
	{ 0x800002, 0x800009, metro_coin_lockout_4words_w	},	// Coin Lockout
MEMORY_END


/***************************************************************************
							Blazing Tornado
***************************************************************************/

static MEMORY_READ16_START( blzntrnd_readmem )
	{ 0x000000, 0x1fffff, MRA16_ROM				},	// ROM
	{ 0xff0000, 0xffffff, MRA16_RAM				},	// RAM
//	{ 0x300000, 0x300001, MRA16_NOP				},	// Sound
	{ 0x200000, 0x21ffff, MRA16_RAM				},	// Layer 0
	{ 0x220000, 0x23ffff, MRA16_RAM				},	// Layer 1
	{ 0x240000, 0x25ffff, MRA16_RAM				},	// Layer 2
	{ 0x260000, 0x26ffff, metro_bankedrom_r		},	// Banked ROM
	{ 0x270000, 0x273fff, MRA16_RAM				},	// Palette
	{ 0x274000, 0x274fff, MRA16_RAM				},	// Sprites
	{ 0x278000, 0x2787ff, MRA16_RAM				},	// Tiles Set
	{ 0x2788a2, 0x2788a3, metro_irq_cause_r		},	// IRQ Cause
	{ 0xe00000, 0xe00001, input_port_0_word_r	},	// Inputs
	{ 0xe00002, 0xe00003, input_port_1_word_r	},	//
	{ 0xe00004, 0xe00005, input_port_2_word_r	},	//
	{ 0xe00006, 0xe00007, input_port_3_word_r	},	//
	{ 0xe00008, 0xe00009, input_port_4_word_r	},	//
	{ 0x400000, 0x43ffff, MRA16_RAM				},	// 053936
MEMORY_END

static MEMORY_WRITE16_START( blzntrnd_writemem )
	{ 0x000000, 0x1fffff, MWA16_ROM						},	// ROM
	{ 0xff0000, 0xffffff, MWA16_RAM						},	// RAM
//	{ 0x300000, 0x30000b, MWA16_NOP						},	// Sound
	{ 0xe00000, 0xe00001, MWA16_NOP						},	// ??????
//	{ 0xe00002, 0xe00009, metro_coin_lockout_4words_w	},	// Coin Lockout
	{ 0x270000, 0x273fff, metro_paletteram_w, &paletteram16	},	// Palette
	{ 0x274000, 0x274fff, MWA16_RAM, &spriteram16, &spriteram_size				},	// Sprites
	{ 0x200000, 0x21ffff, metro_vram_0_w, &metro_vram_0	},	// Layer 0
	{ 0x220000, 0x23ffff, metro_vram_1_w, &metro_vram_1	},	// Layer 1
	{ 0x240000, 0x25ffff, metro_vram_2_w, &metro_vram_2	},	// Layer 2
	{ 0x278000, 0x2787ff, metro_tiletable_w, &metro_tiletable		},	// Tiles Set
//	{ 0x278840, 0x27884d, metro_blitter_w, &metro_blitter_regs		},	// Tiles Blitter
	{ 0x278860, 0x27886b, metro_window_w, &metro_window				},	// Tilemap Window
	{ 0x278870, 0x27887b, MWA16_RAM, &metro_scroll		},	// Scroll
//	{ 0x278880, 0x278881, MWA16_NOP						},	// ? increasing
	{ 0x278890, 0x278891, MWA16_NOP						},	// ? increasing
	{ 0x2788a2, 0x2788a3, metro_irq_cause_w				},	// IRQ Acknowledge
	{ 0x2788a4, 0x2788a5, metro_irq_enable_w			},	// IRQ Enable
	{ 0x2788aa, 0x2788ab, MWA16_RAM, &metro_rombank		},	// Rom Bank
	{ 0x2788ac, 0x2788ad, MWA16_RAM, &metro_screenctrl	},	// Screen Control
	{ 0x279700, 0x279713, MWA16_RAM, &metro_videoregs	},	// Video Registers
	{ 0x260000, 0x26ffff, MWA16_NOP						},	// ??????
	{ 0x400000, 0x43ffff, metro_K053936_w, &metro_K053936_ram	},	// 053936
	{ 0x500000, 0x500fff, MWA16_RAM						},	// 053936 3D rotation control?
	{ 0x600000, 0x60001f, MWA16_RAM, &metro_K053936_ctrl},	// 053936 control
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/


#define JOY_LSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \


#define JOY_MSB(_n_, _b1_, _b2_, _b3_, _b4_) \
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	 | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_##_b1_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_##_b2_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_##_b3_         | IPF_PLAYER##_n_ ) \
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_##_b4_         | IPF_PLAYER##_n_ ) \


#define COINS \
	PORT_BIT(  0x0001, IP_ACTIVE_LOW,  IPT_SERVICE1 ) \
	PORT_BIT(  0x0002, IP_ACTIVE_LOW,  IPT_TILT     ) \
	PORT_BIT_IMPULSE(  0x0004, IP_ACTIVE_LOW,  IPT_COIN1, 2    ) \
	PORT_BIT_IMPULSE(  0x0008, IP_ACTIVE_LOW,  IPT_COIN2, 2    ) \
	PORT_BIT(  0x0010, IP_ACTIVE_LOW,  IPT_START1   ) \
	PORT_BIT(  0x0020, IP_ACTIVE_LOW,  IPT_START2   ) \
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN  ) \
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN  ) /* From Sound CPU in some games */


#define COINAGE_DSW \
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) ) \
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) ) \
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) ) \
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )



/***************************************************************************
									Bal Cube
***************************************************************************/

INPUT_PORTS_START( balcube )

	PORT_START	// IN0 - $500000
	COINS

	PORT_START	// IN1 - $500002
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - Strangely mapped in the 0x400000-0x41ffff range
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, "Difficulty?" )
	PORT_DIPSETTING(      0x0100, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0200, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPNAME( 0x0400, 0x0400, "2 Player Game" )
	PORT_DIPSETTING(      0x0400, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0800, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1000, 0x1000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
								Dai Toride
***************************************************************************/

INPUT_PORTS_START( daitorid )

	PORT_START	// IN0 - $c00000
	COINS

	PORT_START	// IN1 - $c00002
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $c00004
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPSETTING(      0x0100, "01" )
	PORT_DIPSETTING(      0x0200, "10" )
	PORT_DIPSETTING(      0x0300, "11" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $c00006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Puzzli
***************************************************************************/

INPUT_PORTS_START( puzzli )

	PORT_START	// IN0 - $c00000
	COINS

	PORT_START	// IN1 - $c00002
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON3, BUTTON3, UNKNOWN)

	PORT_START	// IN2 - $c00004
	COINAGE_DSW

	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $c00006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Dharma Doujou
***************************************************************************/

INPUT_PORTS_START( dharma )

	PORT_START	// IN0 - $c00000
	COINS

	PORT_START	// IN1 - $c00002
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $c00004
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPSETTING(      0x0100, "01" )
	PORT_DIPSETTING(      0x0200, "10" )
	PORT_DIPSETTING(      0x0300, "11" )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )	// crab's speed
	PORT_DIPSETTING(      0x0800, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, "2 Player Game" )
	PORT_DIPSETTING(      0x1000, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Freeze", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $c00006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Karate Tournament
***************************************************************************/

INPUT_PORTS_START( karatour )

	PORT_START	// IN0 - $400002
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN1 - $400004
	COINS

	PORT_START	// IN2 - $400006
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x000c, "11" )
	PORT_DIPSETTING(      0x0008, "10" )
	PORT_DIPSETTING(      0x0004, "01" )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0010, "60K" )
	PORT_DIPSETTING(      0x0000, "40K" )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $40000a
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_START	// IN4 - $40000c
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)


INPUT_PORTS_END


/***************************************************************************
								Lady Killer
***************************************************************************/

INPUT_PORTS_START( ladykill )

	PORT_START	// IN0 - $400002
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN1 - $400004
	COINS

	PORT_START	// IN2 - $400006
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0001, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0004, "Hard" )
	PORT_DIPSETTING(      0x0000, "Very Hard" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0020, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $40000a
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN4 - $40000c
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)


INPUT_PORTS_END


/***************************************************************************
								Last Fortress
***************************************************************************/

INPUT_PORTS_START( lastfort )

	PORT_START	// IN0 - $c00004
	COINS

	PORT_START	// IN1 - $c00006
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $c00008
	JOY_LSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN3 - $c0000a
	COINAGE_DSW

	PORT_START	// IN4 - $c0000c
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easiest" )
	PORT_DIPSETTING(      0x0001, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "2 Player Game" )
	PORT_DIPSETTING(      0x0010, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x0020, 0x0020, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN5 - $c0000e
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
							Last Fortress (Erotic)
***************************************************************************/

INPUT_PORTS_START( lastfero )

	PORT_START	// IN0 - $c00004
	COINS

	PORT_START	// IN1 - $c00006
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $c00008
	JOY_LSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN3 - $c0000a
	COINAGE_DSW

	PORT_START	// IN4 - $c0000c
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easiest" )
	PORT_DIPSETTING(      0x0001, "Easy" )
	PORT_DIPSETTING(      0x0003, "Normal" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "2 Player Game" )
	PORT_DIPSETTING(      0x0010, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x0020, 0x0020, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Tiles" )
	PORT_DIPSETTING(      0x0080, "Mahjong" )
	PORT_DIPSETTING(      0x0000, "Cards" )

	PORT_START	// IN5 - $c0000e
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Pang Poms
***************************************************************************/

INPUT_PORTS_START( pangpoms )

	PORT_START	// IN0 - $800004
	COINS

	PORT_START	// IN1 - $800006
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $800008
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN3 - $80000a
	COINAGE_DSW

	PORT_START	// IN4 - $80000c
	PORT_DIPNAME( 0x0003, 0x0003, "Time Speed" )
	PORT_DIPSETTING(      0x0000, "Slowest" )	// 60 (1 game sec. lasts x/60 real sec.)
	PORT_DIPSETTING(      0x0001, "Slow"    )	// 90
	PORT_DIPSETTING(      0x0003, "Normal"  )	// 120
	PORT_DIPSETTING(      0x0002, "Fast"    )	// 150
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0030, "11" )
	PORT_DIPSETTING(      0x0020, "10" )
	PORT_DIPSETTING(      0x0010, "01" )
	PORT_DIPSETTING(      0x0000, "00" )
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	// IN5 - $80000e
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Poitto!
***************************************************************************/

INPUT_PORTS_START( poitto )

	PORT_START	// IN0 - $800000
	COINS

	PORT_START	// IN1 - $800002
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $800004
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0200, "Hard" )
	PORT_DIPSETTING(      0x0100, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $800006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Sky Alert
***************************************************************************/

INPUT_PORTS_START( skyalert )

	PORT_START	// IN0 - $400004
	COINS

	PORT_START	// IN1 - $400006
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $400008
	JOY_LSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN3 - $40000a
	COINAGE_DSW

	PORT_START	// IN4 - $40000c
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, "006" )
	PORT_DIPSETTING(      0x0003, "106" )
	PORT_DIPSETTING(      0x0001, "206" )
	PORT_DIPSETTING(      0x0000, "306" )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0008, "1" )
	PORT_DIPSETTING(      0x0004, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x0030, 0x0030, DEF_STR( Bonus_Life ) )	// the game shows wrong values on screen ? (c0e2f2.l = score)
	PORT_DIPSETTING(      0x0030, "100K, Every 400K" )	// c <- other effect (difficulty?)
	PORT_DIPSETTING(      0x0020, "200K, Every 400K" )	// d
	PORT_DIPSETTING(      0x0010, "200K"  )				// e
	PORT_DIPSETTING(      0x0000, "None" )				// f
	PORT_DIPNAME( 0x0040, 0x0040, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )

	PORT_START	// IN5 - $40000e
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
								Pururun
***************************************************************************/

INPUT_PORTS_START( pururun )

	PORT_START	// IN0 - $400000
	COINS

	PORT_START	// IN1 - $400002
	JOY_LSB(1, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, BUTTON2, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $400004
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )	// distance to goal
	PORT_DIPSETTING(      0x0200, "Easiest" )
	PORT_DIPSETTING(      0x0100, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "2 Player Game" )
	PORT_DIPSETTING(      0x0800, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x1000, 0x1000, "Bombs" )
	PORT_DIPSETTING(      0x1000, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $400006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


/***************************************************************************
							Toride II Adauchi Gaiden
***************************************************************************/

INPUT_PORTS_START( toride2g )

	PORT_START	// IN0 - $800000
	COINS

	PORT_START	// IN1 - $800002
	JOY_LSB(1, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)
	JOY_MSB(2, BUTTON1, UNKNOWN, UNKNOWN, UNKNOWN)

	PORT_START	// IN2 - $800004
	COINAGE_DSW

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0200, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0400, "Normal" )
	PORT_DIPSETTING(      0x0000, "Hard" )
	PORT_DIPNAME( 0x0800, 0x0800, "Clear On Continue" )
	PORT_DIPSETTING(      0x0800, "Always" )
	PORT_DIPSETTING(      0x0000, "Ask Player" )
	PORT_DIPNAME( 0x1000, 0x1000, "2 Players Game" )
	PORT_DIPSETTING(      0x1000, "2 Credits" )
	PORT_DIPSETTING(      0x0000, "1 Credit" )
	PORT_DIPNAME( 0x2000, 0x2000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START	// IN3 - $800006
	PORT_BIT(  0xffff, IP_ACTIVE_LOW, IPT_UNKNOWN )	// BIT 6 !?

INPUT_PORTS_END


/***************************************************************************
							Blazing Tornado
***************************************************************************/

INPUT_PORTS_START( blzntrnd )
	PORT_START
	PORT_DIPNAME( 0x0007, 0x0004, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0007, "Beginner" )
	PORT_DIPSETTING(      0x0006, "Easiest" )
	PORT_DIPSETTING(      0x0005, "Easy" )
	PORT_DIPSETTING(      0x0004, "Normal" )
	PORT_DIPSETTING(      0x0003, "Hard" )
	PORT_DIPSETTING(      0x0002, "Hardest" )
	PORT_DIPSETTING(      0x0001, "Expert" )
	PORT_DIPSETTING(      0x0000, "Master" )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, "Allow Continue" )
	PORT_DIPSETTING(      0x0020, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x00c0, 0x0000, "Control Panel" )
	PORT_DIPSETTING(      0x0000, "4 Players" )
//	PORT_DIPSETTING(      0x0040, "4 Players" )
	PORT_DIPSETTING(      0x0080, "1P & 2P Tag only" )
	PORT_DIPSETTING(      0x00c0, "1P & 2P vs only" )
	PORT_DIPNAME( 0x0300, 0x0300, "Half Continue" )
	PORT_DIPSETTING(      0x0000, "6C to start, 3C to continue" )
	PORT_DIPSETTING(      0x0100, "4C to start, 2C to continue" )
	PORT_DIPSETTING(      0x0200, "2C to start, 1C to continue" )
	PORT_DIPSETTING(      0x0300, "Disabled" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(0x0080, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
//	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Service_Mode ) )
//	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, "CP Single" )
	PORT_DIPSETTING(      0x0300, "2:00" )
	PORT_DIPSETTING(      0x0200, "2:30" )
	PORT_DIPSETTING(      0x0100, "3:00" )
	PORT_DIPSETTING(      0x0000, "3:30" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "CP Tag" )
	PORT_DIPSETTING(      0x0c00, "2:00" )
	PORT_DIPSETTING(      0x0800, "2:30" )
	PORT_DIPSETTING(      0x0400, "3:00" )
	PORT_DIPSETTING(      0x0000, "3:30" )
	PORT_DIPNAME( 0x3000, 0x3000, "Vs Single" )
	PORT_DIPSETTING(      0x3000, "2:30" )
	PORT_DIPSETTING(      0x2000, "3:00" )
	PORT_DIPSETTING(      0x1000, "4:00" )
	PORT_DIPSETTING(      0x0000, "5:00" )
	PORT_DIPNAME( 0xc000, 0xc000, "Vs Tag" )
	PORT_DIPSETTING(      0xc000, "2:30" )
	PORT_DIPSETTING(      0x8000, "3:00" )
	PORT_DIPSETTING(      0x4000, "4:00" )
	PORT_DIPSETTING(      0x0000, "5:00" )

	PORT_START
	JOY_LSB(1, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
	JOY_MSB(2, BUTTON1, BUTTON3, BUTTON3, BUTTON4)

	PORT_START
	JOY_LSB(3, BUTTON1, BUTTON2, BUTTON3, BUTTON4)
	JOY_MSB(4, BUTTON1, BUTTON3, BUTTON3, BUTTON4)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT_IMPULSE(  0x0004, IP_ACTIVE_LOW, IPT_COIN1, 2    )
	PORT_BIT_IMPULSE(  0x0008, IP_ACTIVE_LOW, IPT_COIN2, 2    )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END



/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1) },
	{ 1*4,0*4,3*4,2*4,5*4,4*4,7*4,6*4 },
	{ STEP8(0,8*4) },
	8*8*4
};

/* 8x8x8 tiles for later games */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

static struct GfxDecodeInfo gfxdecodeinfo_4bit[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0x0, 0x200 }, // [0] 4 Bit Tiles
	{ REGION_GFX2, 0, &layout_8x8x4, 0x0, 0x200 }, // [1] Fake Tiles
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_8bit[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0x0, 0x200 }, // [0] 4 Bit Tiles
	{ REGION_GFX2, 0, &layout_8x8x4, 0x0, 0x200 }, // [1] Fake Tiles
//	{ REGION_GFX1, 0, &layout_8x8x8, 0x0,  0x20 }, // [2] 8 Bit Tiles (handled directly from ROM data, no decoding)
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_blzntrnd[] =
{
	{ REGION_GFX1, 0, &layout_8x8x4, 0x0, 0x200 }, // [0] 4 Bit Tiles
	{ REGION_GFX2, 0, &layout_8x8x4, 0x0, 0x200 }, // [1] Fake Tiles
	{ REGION_GFX3, 0, &layout_8x8x8, 0x0,  0x20 }, // [3] 053936 Tiles
//	{ REGION_GFX1, 0, &layout_8x8x8, 0x0,  0x20 }, // [2] 8 Bit Tiles (handled directly from ROM data, no decoding)
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static const struct MachineDriver machine_driver_balcube =
{
	{
		{
			CPU_M68000,
			16000000,
			balcube_readmem, balcube_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 0, 320-1, 0, 224-1 },
	gfxdecodeinfo_8bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14220,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	/* YMF278B (unemulated) + YRW801-M (Standard Samples ROM) */
	},
};


static const struct MachineDriver machine_driver_daitorid =
{
	{
		{
			CPU_M68000,
			16000000,
			daitorid_readmem, daitorid_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},
		{
			CPU_UPD7810,
			12000000,
			upd7810_readmem, upd7810_writemem, upd7810_readport, upd7810_writeport,
			ignore_interrupt, 0,
			0, 0,
			(void *)metro_io_callback
        },
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 0, 320-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14220,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
        {   SOUND_YM2151,   &daitorid_ym2151_interface      },
		{	SOUND_OKIM6295, &daitorid_okim6295_interface	}
	},
};


static const struct MachineDriver machine_driver_dharma =
{
	{
		{
			CPU_M68000,
			12000000,
			dharma_readmem, dharma_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 0, 320-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295
	},
};


static const struct MachineDriver machine_driver_karatour =
{
	{
		{
			CPU_M68000,
			12000000,
			karatour_readmem, karatour_writemem,0,0,
//			metro_interrupt, 10	/* ? */
			karatour_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 240, { 0, 320-1, 0, 240-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2143
	},
};


static const struct MachineDriver machine_driver_lastfort =
{
	{
		{
			CPU_M68000,
			12000000,
			lastfort_readmem, lastfort_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	360, 224, { 0, 360-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2143
	},
};


static const struct MachineDriver machine_driver_pangpoms =
{
	{
		{
			CPU_M68000,
			12000000,
			pangpoms_readmem, pangpoms_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	360, 224, { 0, 360-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2143
	},
};


static const struct MachineDriver machine_driver_poitto =
{
	{
		{
			CPU_M68000,
			12000000,
			poitto_readmem, poitto_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	360, 224, { 0, 360-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2143
	},
};


static const struct MachineDriver machine_driver_pururun =
{
	{
		{
			CPU_M68000,
			12000000,
			pururun_readmem, pururun_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 0, 320-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2151, Y3012
	},
};


static const struct MachineDriver machine_driver_skyalert =
{
	{
		{
			CPU_M68000,
			12000000,
			skyalert_readmem, skyalert_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	360, 224, { 0, 360-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2143
	},
};


static const struct MachineDriver machine_driver_toride2g =
{
	{
		{
			CPU_M68000,
			12000000,
			toride2g_readmem, toride2g_writemem,0,0,
			metro_interrupt, 10	/* ? */
		},

		/* Sound CPU is unemulated */
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 0, 320-1, 0, 224-1 },
	gfxdecodeinfo_4bit,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	metro_vh_start_14100,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	// M6295, YM2413
	},
};

static const struct MachineDriver machine_driver_blzntrnd =
{
	{
		{
			CPU_M68000,
			16000000,
			blzntrnd_readmem, blzntrnd_writemem,0,0,
//			metro_interrupt, 10	/* ? */
			karatour_interrupt, 10	/* ? */
		},
#if 0
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			16000000/2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1
		}
#endif
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	320, 224, { 8, 320-8-1, 0, 224-1 },
	gfxdecodeinfo_blzntrnd,
	0x2000, 0x2000,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	blzntrnd_vh_start,
	metro_vh_stop,
	metro_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{ 0 },	/* YMF286K (unemulated) + YRW801-M? (Standard Samples ROM) */
	},
};



/***************************************************************************


								ROMs Loading


***************************************************************************/

static void init_metro(void)
{
	int i;

	/*
	  Tiles can be either 4-bit or 8-bit, and both depths can be used at the same
	  time. The transparent pen is the last one, that is 15 or 255. To make
	  tilemap.c handle that, we invert gfx data so the transparent pen becomes 0
	  for both tile depths.
	*/
	for (i = 0;i < memory_region_length(REGION_GFX1);i++)
		memory_region(REGION_GFX1)[i] ^= 0xff;

	if (memory_region(REGION_GFX3))	/* blzntrnd */
		for (i = 0;i < memory_region_length(REGION_GFX3);i++)
			memory_region(REGION_GFX3)[i] ^= 0xff;

	vblank_irq   = 0;
	blitter_irq  = 0;
	unknown1_irq = 0;
	unknown2_irq = 0;
	unknown3_irq = 0;
	unknown4_irq = 0;

	irq_line   = 2;
	irq_enable = 0;
}


void init_karatour(void)
{
	data16_t *RAM = (data16_t *) memory_region( REGION_USER1 );
	metro_vram_0 = RAM + (0x20000/2) * 0;
	metro_vram_1 = RAM + (0x20000/2) * 1;
	metro_vram_2 = RAM + (0x20000/2) * 2;
	init_metro();
}

/* Unscramble the GFX ROMs */
static void init_balcube(void)
{
	const int region	=	REGION_GFX1;

	const size_t len	=	memory_region_length(region);
	data8_t *src		=	memory_region(region);
	data8_t *end		=	memory_region(region) + len;

	while(src < end)
	{
		const unsigned char scramble[16] =
		 { 0x0,0x8,0x4,0xc,0x2,0xa,0x6,0xe,0x1,0x9,0x5,0xd,0x3,0xb,0x7,0xf };

		unsigned char data;

		data  =  *src;
		*src  =  (scramble[data & 0xF] << 4) | scramble[data >> 4];
		src  +=  2;
	}

	init_metro();
	irq_line = 1;
}


static void init_blzntrnd(void)
{
	init_metro();
	irq_line = 1;
}


/* Fake Tiles used to support a feature of this hardware: it can draw tiles
   filled with one single color (e.g. not stored in the GFX ROMs) */

#define METRO_FAKE_TILES(_rgn_)	\
	ROM_REGION( 0x200, _rgn_, ROMREGION_DISPOSE ) \
	ROM_FILL( 0x000, 0x20, 0xff ) \
	ROM_FILL( 0x020, 0x20, 0xee ) \
	ROM_FILL( 0x040, 0x20, 0xdd ) \
	ROM_FILL( 0x060, 0x20, 0xcc ) \
	ROM_FILL( 0x080, 0x20, 0xbb ) \
	ROM_FILL( 0x0a0, 0x20, 0xaa ) \
	ROM_FILL( 0x0c0, 0x20, 0x99 ) \
	ROM_FILL( 0x0e0, 0x20, 0x88 ) \
	ROM_FILL( 0x100, 0x20, 0x77 ) \
	ROM_FILL( 0x120, 0x20, 0x66 ) \
	ROM_FILL( 0x140, 0x20, 0x55 ) \
	ROM_FILL( 0x160, 0x20, 0x44 ) \
	ROM_FILL( 0x180, 0x20, 0x33 ) \
	ROM_FILL( 0x1a0, 0x20, 0x22 ) \
	ROM_FILL( 0x1c0, 0x20, 0x11 ) \
	ROM_FILL( 0x1e0, 0x20, 0x00 )



/***************************************************************************

Bal Cube
Metro 1996

            7                             1
            YRW801-M                      2
   33.369MHz YMF278B                      3
                                          4



                     16MHz           Imagetek
                6     5              14220
                84256 84256
                68000-16                 52258-20  61C640-20
                             26.666MHz   52258-20

***************************************************************************/

ROM_START( balcube )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "6", 0x000000, 0x040000, 0xc400f84d )
	ROM_LOAD16_BYTE( "5", 0x000001, 0x040000, 0x15313e3f )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "2", 0x000000, 0x080000, 0x492ca8f0, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "4", 0x000002, 0x080000, 0xd1acda2c, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "1", 0x000004, 0x080000, 0x0ea3d161, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "3", 0x000006, 0x080000, 0xeef1d3b4, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "7", 0x000000, 0x080000, 0xf769287d )	// PCM 16 Bit (Signed)

	ROM_REGION( 0x200000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* ? YRW801-M ? */
	ROM_LOAD( "yrw801m", 0x000000, 0x200000, 0x00000000 )	// Yamaha YRW801 2MB ROM with samples for the OPL4.
ROM_END


/***************************************************************************

Daitoride
Metro 1995

MTR5260-A

                                 12MHz  6116
                   YM2151          DT7  DT8
                            M6295
     7C199                             78C10
     7C199       Imagetek14220
     61C64

                  68000-16             DT1
                  32MHz    52258       DT2
   SW1                     52258       DT3
   SW2            DT6  DT5             DT4

***************************************************************************/

ROM_START( daitorid )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "dt-ja-5.19e", 0x000000, 0x020000, 0x441efd77 )
	ROM_LOAD16_BYTE( "dt-ja-6.19c", 0x000001, 0x020000, 0x494f9cc3 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "dt-ja-8.3h", 0x000000, 0x020000, 0x0351ad5b )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "dt-ja-2.14h", 0x000000, 0x080000, 0x56881062, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-4.18h", 0x000002, 0x080000, 0x85522e3b, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-1.12h", 0x000004, 0x080000, 0x2a220bf2, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "dt-ja-3.16h", 0x000006, 0x080000, 0xfd1f58e0, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "dt-ja-7.3f", 0x000000, 0x040000, 0x0d888cde )
ROM_END


/***************************************************************************

Dharma Doujou
Metro 1994


                  M6395  JA-7 JA-8

     26.666MHz          NEC78C10
      7C199
      7C199
      7C199               JB-1
                          JB-2
                          JB-3
           68000-12       JB-4

           24MHz
                  6264
                  6264
           JC-5 JC-6

***************************************************************************/

ROM_START( dharma )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "jc-5", 0x000000, 0x020000, 0xb5d44426 )
	ROM_LOAD16_BYTE( "jc-6", 0x000001, 0x020000, 0xbc5a202e )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ja-8", 0x000000, 0x020000, 0xaf7ebc4c )	// (c)1992 Imagetek (11xxxxxxxxxxxxxxx = 0xFF)

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "jb-2", 0x000000, 0x080000, 0x2c07c29b, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-4", 0x000002, 0x080000, 0xfe15538e, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-1", 0x000004, 0x080000, 0xe6ca9bf6, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "jb-3", 0x000006, 0x080000, 0x6ecbe193, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ja-7", 0x000000, 0x040000, 0x7ce817eb )
ROM_END


/***************************************************************************

Karate Tournament

68000-12
NEC D78C10ACN
OKI6295
YM2413
OSC:  24.000MHz,  20.000MHz,   XTAL 3579545

On board, location for but unused things...
Unused DIP#3
Unused BAT1

I can see a large square surface-mounted chip with
these markings...

ImageTek Inc.
14100
052
9227KK702

Filename	Type		Location
KT001.BIN	27C010	 	1I
KT002.BIN	27C2001		8G
KT003.BIN	27C2001		10G
KT008.BIN	27C2001		1D

Filename	Chip Markings	Location
KTMASK1.BIN	361A04 9241D	15F
KTMASK2.BIN	361A05 9239D	17F
KTMASK3.BIN	361A06 9239D	15D
KTMASK4.BIN	361A07 9239D	17D

***************************************************************************/

ROM_START( karatour )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "kt002.8g",  0x000000, 0x040000, 0x316a97ec )
	ROM_LOAD16_BYTE( "kt003.10g", 0x000001, 0x040000, 0xabe1b991 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "kt001.1i", 0x000000, 0x020000, 0x1dd2008c )	// 11xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ktmask.15f", 0x000000, 0x100000, 0xf6bf20a5, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.17d", 0x000002, 0x100000, 0x794cc1c0, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.17f", 0x000004, 0x100000, 0xea9c11fc, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ktmask.15d", 0x000006, 0x100000, 0x7e15f058, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "kt008.1d", 0x000000, 0x040000, 0x47cf9fa1 )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END


/***************************************************************************

Moeyo Gonta!! (Lady Killer)
(c)1993 Yanyaka
VG460-(B)

CPU  : TMP68HC000P-16
Sound: D78C10ACW YM2413 M6295
OSC  : 3.579545MHz(XTAL1) 20.0000MHz(XTAL2) 24.0000MHz(XTAL3)

ROMs:
e1.1i - Sound program (27c010)

j2.8g  - Main programs (27c020)
j3.10g /

ladyj-4.15f - Graphics (mask, read as 27c800)
ladyj-5.17f |
ladyj-6.15d |
ladyj-7.17d /

e8j.1d - Samples (27c020)

Others:
Imagetek I4100 052 9330EK712

***************************************************************************/

ROM_START( ladykill )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "j2.8g",     0x000000, 0x040000, 0xaa18d130 )
	ROM_LOAD16_BYTE( "j3.10g",    0x000001, 0x040000, 0xb555e6ab )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "e1.1i",    0x000000, 0x020000, 0xa4d95cfb )	// 11xxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ladyj-4.15f", 0x000000, 0x100000, 0x65e5906c, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-7.17d", 0x000002, 0x100000, 0x56bd64a5, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-5.17f", 0x000004, 0x100000, 0xa81ffaa3, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "ladyj-6.15d", 0x000006, 0x100000, 0x3a34913a, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "e8j.1d",   0x000000, 0x040000, 0xf66c2a80 )

	/* Additional memory for the layers' ram */
	ROM_REGION( 0x20000*3, REGION_USER1, 0 )
ROM_END


/***************************************************************************

Last Fortress - Toride
Metro 1992
VG420

                                     TR_JB12 5216
                     SW2 SW1           NEC78C10   3.579MHz

                                                          6269
                                                          TR_JB11
  55328 55328 55328       24MHz

                           4064   4064   TR_   TR_          68000-12
       Imagetek                          JC10  JC09
       14100

    TR_  TR_  TR_  TR_  TR_  TR_  TR_  TR_
    JC08 JC07 JC06 JC05 JC04 JC03 JC02 JC01

***************************************************************************/

ROM_START( lastfort )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tr_jc09", 0x000000, 0x020000, 0x8b98a49a )
	ROM_LOAD16_BYTE( "tr_jc10", 0x000001, 0x020000, 0x8d04da04 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr_jb12", 0x000000, 0x020000, 0x8a8f5fef )	// (c)1992 Imagetek (11xxxxxxxxxxxxxxx = 0xFF)

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tr_jc02", 0x000000, 0x020000, 0xdb3c5b79, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc04", 0x000001, 0x020000, 0xf8ab2f9b, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc06", 0x000002, 0x020000, 0x47a7f397, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc08", 0x000003, 0x020000, 0xd7ba5e26, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc01", 0x000004, 0x020000, 0x3e3dab03, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc03", 0x000005, 0x020000, 0x87ac046f, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc05", 0x000006, 0x020000, 0x3fbbe49c, ROM_SKIP(7))
	ROMX_LOAD( "tr_jc07", 0x000007, 0x020000, 0x05e1456b, ROM_SKIP(7))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr_jb11", 0x000000, 0x020000, 0x83786a09 )
ROM_END


/***************************************************************************

Last Fortress - Toride (Erotic)
Metro Corporation.

Board number VG420

CPU: MC68000P12
SND: OKI M6295+ YM2413 + NEC D78C10ACW + NEC D4016 (ram?)
DSW: see manual (scanned in sub-directory Manual)
OSC: 24.000 MHz

***************************************************************************/

ROM_START( lastfero )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tre_jc09", 0x000000, 0x020000, 0x32f43390 )
	ROM_LOAD16_BYTE( "tre_jc10", 0x000001, 0x020000, 0x9536369c )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr_jb12", 0x000000, 0x020000, 0x8a8f5fef )	// (c)1992 Imagetek (11xxxxxxxxxxxxxxx = 0xFF)

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tre_jc02", 0x000000, 0x020000, 0x11cfbc84, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc04", 0x000001, 0x020000, 0x32bf9c26, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc06", 0x000002, 0x020000, 0x16937977, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc08", 0x000003, 0x020000, 0x6dd96a9b, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc01", 0x000004, 0x020000, 0xaceb44b3, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc03", 0x000005, 0x020000, 0xf18f1248, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc05", 0x000006, 0x020000, 0x79f769dd, ROM_SKIP(7))
	ROMX_LOAD( "tre_jc07", 0x000007, 0x020000, 0xb6feacb2, ROM_SKIP(7))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr_jb11", 0x000000, 0x020000, 0x83786a09 )
ROM_END


/***************************************************************************

Pang Poms (c) 1992 Metro

Pcb code:  VG420 (Same as Toride)

Cpus:  M68000, Z80
Clocks: 24 MHz, 3.579 MHz
Sound: M6295, YM2143, _unused_ slot for a YM2151

Custom graphics chip - Imagetek 14100 052 9227KK701 (same as Karate Tournament)

***************************************************************************/

ROM_START( pangpoms )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "ppoms09.bin", 0x000000, 0x020000, 0x0c292dbc )
	ROM_LOAD16_BYTE( "ppoms10.bin", 0x000001, 0x020000, 0x0bc18853 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ppoms12.bin", 0x000000, 0x020000, 0xa749357b )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ppoms02.bin", 0x000000, 0x020000, 0x88f902f7, ROM_SKIP(7))
	ROMX_LOAD( "ppoms04.bin", 0x000001, 0x020000, 0x9190c2a0, ROM_SKIP(7))
	ROMX_LOAD( "ppoms06.bin", 0x000002, 0x020000, 0xed15c93d, ROM_SKIP(7))
	ROMX_LOAD( "ppoms08.bin", 0x000003, 0x020000, 0x9a3408b9, ROM_SKIP(7))
	ROMX_LOAD( "ppoms01.bin", 0x000004, 0x020000, 0x11ac3810, ROM_SKIP(7))
	ROMX_LOAD( "ppoms03.bin", 0x000005, 0x020000, 0xe595529e, ROM_SKIP(7))
	ROMX_LOAD( "ppoms05.bin", 0x000006, 0x020000, 0x02226214, ROM_SKIP(7))
	ROMX_LOAD( "ppoms07.bin", 0x000007, 0x020000, 0x48471c87, ROM_SKIP(7))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ppoms11.bin", 0x000000, 0x020000, 0xe89bd565 )
ROM_END

ROM_START( pangpomm )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pa.c09", 0x000000, 0x020000, 0xe01a7a08 )
	ROM_LOAD16_BYTE( "pa.c10", 0x000001, 0x020000, 0x5e509cee )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "ppoms12.bin", 0x000000, 0x020000, 0xa749357b )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "ppoms02.bin", 0x000000, 0x020000, 0x88f902f7, ROM_SKIP(7))
	ROMX_LOAD( "pj.e04",      0x000001, 0x020000, 0x54bf2f10, ROM_SKIP(7))
	ROMX_LOAD( "pj.e06",      0x000002, 0x020000, 0xc8b6347d, ROM_SKIP(7))
	ROMX_LOAD( "ppoms08.bin", 0x000003, 0x020000, 0x9a3408b9, ROM_SKIP(7))
	ROMX_LOAD( "ppoms01.bin", 0x000004, 0x020000, 0x11ac3810, ROM_SKIP(7))
	ROMX_LOAD( "pj.e03",      0x000005, 0x020000, 0xd126e774, ROM_SKIP(7))
	ROMX_LOAD( "pj.e05",      0x000006, 0x020000, 0x79c0ec1e, ROM_SKIP(7))
	ROMX_LOAD( "ppoms07.bin", 0x000007, 0x020000, 0x48471c87, ROM_SKIP(7))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "ppoms11.bin", 0x000000, 0x020000, 0xe89bd565 )
ROM_END


/***************************************************************************

Poitto! (c)1993 Metro corp
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2413
OSC  : 24.0000MHz  (OSC1)
                   (OSC2)
                   (OSC3)
       3.579545MHz (OSC4)
                   (OSC5)

ROMs:
pt-1.13i - Graphics (23c4000)
pt-2.15i |
pt-3.17i |
pt-4.19i /

pt-jd05.20e - Main programs (27c010)
pt-jd06.20c /

pt-jc07.3g - Sound data (27c020)
pt-jc08.3i - Sound program (27c010)

Others:
Imagetek 14100 052 9309EK701 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( poitto )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pt-jd05.20e", 0x000000, 0x020000, 0x6b1be034 )
	ROM_LOAD16_BYTE( "pt-jd06.20c", 0x000001, 0x020000, 0x3092d9d4 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pt-jc08.3i", 0x000000, 0x020000, 0xf32d386a )	// 1xxxxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pt-2.15i", 0x000000, 0x080000, 0x05d15d01, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-4.19i", 0x000002, 0x080000, 0x8a39edb5, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-1.13i", 0x000004, 0x080000, 0xea6e2289, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pt-3.17i", 0x000006, 0x080000, 0x522917c1, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pt-jc07.3g", 0x000000, 0x040000, 0x5ae28b8d )
ROM_END


/***************************************************************************

Puzzli
Metro/Banpresto 1995

MTR5260-A                3.5759MHz  12MHz
               YM2151                         6116
   26.666MHz           M6295    PZ.JB7  PZ.JB8
                                     78C10
      7C199         Imagetek
      7C199           14220
      61C64

                                          PZ.JB1
           68000-16                       PZ.JB2
               32MHz   6164               PZ.JB3
                       6164               PZ.JB4
    SW      PZ.JB6 PZ.JB5
    SW

***************************************************************************/

ROM_START( puzzli )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pz.jb5",       0x000000, 0x020000, 0x33bbbd28 )
	ROM_LOAD16_BYTE( "pz.jb6",       0x000001, 0x020000, 0xe0bdea18 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pz.jb8",      0x000000, 0x020000, 0xc652da32 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pz.jb2",       0x000000, 0x080000, 0x0c0997d4, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb4",       0x000002, 0x080000, 0x576bc5c2, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb1",       0x000004, 0x080000, 0x29f01eb3, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pz.jb3",       0x000006, 0x080000, 0x6753e282, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pz.jb7",      0x000000, 0x040000, 0xb3aab610 )
ROM_END


/***************************************************************************

Pururun (c)1995 Metro/Banpresto
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2151 Y3012
OSC  : 24.000MHz   (OSC1)
                   (OSC2)
       26.6660MHz  (OSC3)
                   (OSC4)
       3.579545MHz (OSC5)

ROMs:
pu9-19-1.12i - Graphics (27c4096)
pu9-19-2.14i |
pu9-19-3.16i |
pu9-19-4.18i /

pu9-19-5.20e - Main programs (27c010)
pu9-19-6.20c /

pu9-19-7.3g - Sound data (27c020)
pu9-19-8.3i - Sound program (27c010)

Others:
Imagetek 14220 071 9338EK707 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( pururun )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "pu9-19-5.20e", 0x000000, 0x020000, 0x5a466a1b )
	ROM_LOAD16_BYTE( "pu9-19-6.20c", 0x000001, 0x020000, 0xd155a53c )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "pu9-19-8.3i", 0x000000, 0x020000, 0xedc3830b )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "pu9-19-2.14i", 0x000000, 0x080000, 0x21550b26, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-4.18i", 0x000002, 0x080000, 0x3f3e216d, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-1.12i", 0x000004, 0x080000, 0x7e83a75f, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "pu9-19-3.16i", 0x000006, 0x080000, 0xd15485c5, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "pu9-19-7.3g", 0x000000, 0x040000, 0x51ae4926 )
ROM_END


/***************************************************************************

Sky Alert (JPN Ver.)
(c)1992 Metro
VG420

CPU 	:MC68000P12
Sound	:YM2413,OKI M6295
OSC 	:24.0000MHz,3.579545MHz
other	:D78C10ACW,Imagetek Inc 14100 052

***************************************************************************/

ROM_START( skyalert )
	ROM_REGION( 0x040000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "sa_c_09.bin", 0x000000, 0x020000, 0x6f14d9ae )
	ROM_LOAD16_BYTE( "sa_c_10.bin", 0x000001, 0x020000, 0xf10bb216 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "sa_b_12.bin", 0x000000, 0x020000, 0xf358175d )	// (c)1992 Imagetek (1xxxxxxxxxxxxxxxx = 0xFF)

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "sa_a_02.bin", 0x000000, 0x040000, 0xf4f81d41, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_04.bin", 0x000001, 0x040000, 0x7d071e7e, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_06.bin", 0x000002, 0x040000, 0x77e4d5e1, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_08.bin", 0x000003, 0x040000, 0xf2a5a093, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_01.bin", 0x000004, 0x040000, 0x41ec6491, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_03.bin", 0x000005, 0x040000, 0xe0dff10d, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_05.bin", 0x000006, 0x040000, 0x62169d31, ROM_SKIP(7))
	ROMX_LOAD( "sa_a_07.bin", 0x000007, 0x040000, 0xa6f5966f, ROM_SKIP(7))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "sa_a_11.bin", 0x000000, 0x020000, 0x04842a60 )
ROM_END


/***************************************************************************

Toride II Adauchi Gaiden (c)1994 Metro corp
MTR5260-A

CPU  : TMP68HC000P-16
Sound: D78C10ACW M6295 YM2413
OSC  : 24.0000MHz  (OSC1)
                   (OSC2)
       26.6660MHz  (OSC3)
       3.579545MHz (OSC4)
                   (OSC5)

ROMs:
tr2aja-1.12i - Graphics (27c4096)
tr2aja-2.14i |
tr2aja-3.16i |
tr2aja-4.18i /

tr2aja-5.20e - Main programs (27c020)
tr2aja-6.20c /

tr2aja-7.3g - Sound data (27c010)
tr2aja-8.3i - Sound program (27c010)

Others:
Imagetek 14220 071 9338EK700 (208pin PQFP)
AMD MACH110-20 (CPLD)

***************************************************************************/

ROM_START( toride2g )
	ROM_REGION( 0x080000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tr2aja-5.20e", 0x000000, 0x040000, 0xb96a52f6 )
	ROM_LOAD16_BYTE( "tr2aja-6.20c", 0x000001, 0x040000, 0x2918b6b4 )

	ROM_REGION( 0x020000, REGION_CPU2, 0 )		/* NEC78C10 Code */
	ROM_LOAD( "tr2aja-8.3i", 0x000000, 0x020000, 0xfdd29146 )

	ROM_REGION( 0x200000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "tr2aja-2.14i", 0x000000, 0x080000, 0x5c73f629, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-4.18i", 0x000002, 0x080000, 0x67ebaf1b, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-1.12i", 0x000004, 0x080000, 0x96245a5c, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "tr2aja-3.16i", 0x000006, 0x080000, 0x49013f5d, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x020000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "tr2aja-7.3g", 0x000000, 0x020000, 0x630c6193 )
ROM_END


/***************************************************************************

Blazing Tornado
(c)1994 Human

CPU:	68000-16
Sound:	Z80-8
	YMF286K
OSC:	16.0000MHz
	26.666MHz
Chips:	Imagetek 14220 071
	Konami 053936 (PSAC2)

***************************************************************************/

ROM_START( blzntrnd )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 68000 */
	ROM_LOAD16_BYTE( "1k.bin", 0x000000, 0x80000, 0xb007893b )
	ROM_LOAD16_BYTE( "2k.bin", 0x000001, 0x80000, 0xec173252 )
	ROM_LOAD16_BYTE( "3k.bin", 0x100000, 0x80000, 0x1e230ba2 )
	ROM_LOAD16_BYTE( "4k.bin", 0x100001, 0x80000, 0xe98ca99e )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 */
	ROM_LOAD( "rom5.bin", 0x0000, 0x20000, 0x7e90b774 )

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )	/* Gfx + Data (Addressable by CPU & Blitter) */
	ROMX_LOAD( "rom142.bin", 0x0000000, 0x200000, 0xa7200598, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom186.bin", 0x0000002, 0x200000, 0x6ee28ea7, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom131.bin", 0x0000004, 0x200000, 0xc77e75d3, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom175.bin", 0x0000006, 0x200000, 0x04a84f9b, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom242.bin", 0x0800000, 0x200000, 0x1182463f, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom286.bin", 0x0800002, 0x200000, 0x384424fc, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom231.bin", 0x0800004, 0x200000, 0xf0812362, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom275.bin", 0x0800006, 0x200000, 0x184cb129, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom342.bin", 0x1000000, 0x200000, 0xe527fee5, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom386.bin", 0x1000002, 0x200000, 0xd10b1401, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom331.bin", 0x1000004, 0x200000, 0x4d909c28, ROM_GROUPWORD | ROM_SKIP(6))
	ROMX_LOAD( "rom375.bin", 0x1000006, 0x200000, 0x6eb4f97c, ROM_GROUPWORD | ROM_SKIP(6))

	METRO_FAKE_TILES( REGION_GFX2 )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 053936 gfx data */
	ROM_LOAD( "rom9.bin", 0x000000, 0x200000, 0x37ca3570 )

	ROM_REGION( 0x080000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom8.bin", 0x000000, 0x080000, 0x565a4086 )

	ROM_REGION( 0x400000, REGION_SOUND2, ROMREGION_SOUNDONLY )	/* ? YRW801-M ? */
	ROM_LOAD( "rom6.bin", 0x000000, 0x200000, 0x8b8819fc )
	ROM_LOAD( "rom7.bin", 0x200000, 0x200000, 0x0089a52b )
ROM_END



/***************************************************************************


								Game Drivers


***************************************************************************/

GAMEX( 1992, karatour, 0,        karatour, karatour, karatour, ROT0,       "Mitchell",           "The Karate Tournament",    GAME_NO_SOUND )
GAMEX( 1993?,ladykill, 0,        karatour, ladykill, karatour, ROT90,      "Yanyaka",            "Moeyo Gonta!! (Japan)",    GAME_NO_SOUND )
GAMEX( 1992, pangpoms, 0,        pangpoms, pangpoms, metro,    ROT0,       "Metro",              "Pang Poms",                GAME_NO_SOUND )
GAMEX( 1992, pangpomm, pangpoms, pangpoms, pangpoms, metro,    ROT0,       "Metro (Mitchell license)", "Pang Poms (Mitchell)", GAME_NO_SOUND )
GAMEX( 1992, skyalert, 0,        skyalert, skyalert, metro,    ROT270,     "Metro",              "Sky Alert",                GAME_NO_SOUND )
GAMEX( 1993, poitto,   0,        poitto,   poitto,   metro,    ROT0,       "Metro / Able Corp.", "Poitto!",                  GAME_NO_SOUND )
GAMEX( 1994, dharma,   0,        dharma,   dharma,   metro,    ROT0,       "Metro",              "Dharma Doujou",            GAME_NO_SOUND )
GAMEX( 1994, lastfort, 0,        lastfort, lastfort, metro,    ROT0,       "Metro",              "Last Fortress - Toride",   GAME_NO_SOUND )
GAMEX( 1994, lastfero, lastfort, lastfort, lastfero, metro,    ROT0,       "Metro",              "Last Fortress - Toride (Erotic)", GAME_NO_SOUND )
GAMEX( 1994, toride2g, 0,        toride2g, toride2g, metro,    ROT0,       "Metro",              "Toride II Adauchi Gaiden", GAME_NO_SOUND )
GAMEX( 1995, daitorid, 0,        daitorid, daitorid, metro,    ROT0,       "Metro",              "Dai Toride",               GAME_NO_SOUND )
GAMEX( 1995, puzzli,   0,        daitorid, puzzli,   metro,    ROT0_16BIT, "Metro / Banpresto",  "Puzzli",                   GAME_NO_SOUND )
GAMEX( 1995, pururun,  0,        pururun,  pururun,  metro,    ROT0,       "Metro / Banpresto",  "Pururun",                  GAME_NO_SOUND )
GAMEX( 1996, balcube,  0,        balcube,  balcube,  balcube,  ROT0_16BIT, "Metro",              "Bal Cube",                 GAME_NO_SOUND )

GAMEX( 1994, blzntrnd, 0,        blzntrnd, blzntrnd, blzntrnd, ROT0_16BIT, "Human Amusement",    "Blazing Tornado",          GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS )
