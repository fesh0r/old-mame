/********************************************************************

Urashima Mahjong         UPL        68000 <unknown cpu> OKIM6295
Task Force Harrier       UPL        68000 Z80           YM2203 2xOKIM6295
Mustang                  UPL        68000 NMK004        YM2203 2xOKIM6295
Mustang (bootleg)        UPL        68000 Z80           YM3812 OKIM6295
Bio-ship Paladin         UPL        68000 <unknown cpu> YM2203(?) 2xOKIM6295
Vandyke                  UPL        68000 NMK004        YM2203 2xOKIM6295
Black Heart              UPL        68000 NMK004        YM2203 2xOKIM6295
Acrobat Mission          UPL        68000 NMK004        YM2203 2xOKIM6295
Strahl                   UPL        68000 <unknown cpu> YM2203 2xOKIM6295
Thunder Dragon           NMK/Tecmo  68000 <unknown cpu> YM2203 2xOKIM6295
Thunder Dragon (bootleg) NMK/Tecmo  68000 Z80           YM3812 OKIM6295
Hacha Mecha Fighter      NMK        68000 <unknown cpu> YM2203 2xOKIM6295
Macross                  Banpresto  68000 <unknown cpu> YM2203 2xOKIM6295
GunNail                  NMK/Tecmo  68000 NMK004        YM2203 2xOKIM6295
Macross II               Banpresto  68000 Z80           YM2203 2xOKIM6295
Thunder Dragon 2         NMK        68000 Z80           YM2203 2xOKIM6295
Saboten Bombers          NMK/Tecmo  68000               2xOKIM6295
Bombjack Twin            NMK        68000               2xOKIM6295
Nouryoku Koujou Iinkai   Tecmo      68000               2xOKIM6295
Many Block               Bee-Oh     68000 Z80           YM2203 2xOKIM6295
S.S. Mission             Comad      68000 Z80           OKIM6295

driver by Mirko Buffoni, Richard Bush, Nicola Salmoria, Bryan McPhail,
          David Haywood, and R. Belmont.

The NMK004 CPU might be a Toshiba TLCS-90 class CPU with internal ROM in the
0000-3fff range.

The later games have an higher resolution (384x224 instead of 256x224)
but the hardware is pretty much the same. It's obvious that the higher
res is an afterthought, because the tilemap layout is weird (the left
8 screen columns have to be taken from the rightmost 8 columns of the
tilemap), and the games rely on mirror addresses to access the tilemap
sequentially.

TODO:
- Protection is patched in several games, it's the same chip as Macross/Task
  Force Harrier so it can probably be emulated.  I don't think it's an actual MCU,
  just some type of state machine (Bryan).
- Input ports in Bio-ship Paladin, Strahl
- Sound communication in Mustang might be incorrectly implemented
- Several games use an unknown (custom?) CPU to drive sound and for protection.
  In macross and gunnail it's easy to work around, the others are more complex.
  In hachamf it seems that the two CPUs share some RAM (fe000-fefff), and the
  main CPU fetches pointers from that shared RAM to do important operations
  like reading the input ports. Some of them are easily deduced checking for
  similarities in macross and bjtwin; however another protection check involves
  (see the routine at 01429a) writing data to the fe100-fe1ff range, and then
  jumping to subroutines in that range (most likely function pointers since
  each one is only 0x10 bytes long), and heaven knows what those should do.
  On startup, hachamf does a RAM test, then copies some stuff and jumps to
  RAM at 0xfef00, where it sits in a loop. Maybe the RAM is shared with the
  sound CPU, and used as a protection. We patch around that by replacing the
  reset vector with the "real" one.
- Cocktail mode is supported, but tilemap.c has problems with asymmetrical
  visible areas.
- Macross2 dip switches (the ones currently listed match macross)
- Macross2 background is wrong in level 2 at the end of the vertical scroll.
  The tilemap layout is probably different from the one I used, the dimensions
  should be correct but the page order is likely different.
- Music timing in nouryoku is a little off.
- DSW's in Tdragon2
- In Bioship, there's an occasional flicker of one of the sprites composing big
  ships. Increasing CPU speed from 12 to 16 MHz improved it, but it's still not
  100% fixed.

----

IRQ1 controls audio output and coin/joysticks reads
IRQ4 controls DSW reads and vblank.
IRQ2 points to RTE (not used).

----

mustang and hachamf test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 1 button 2 14 (!) times

gunnail test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 2 button 1 3 times

bjtwin test mode:

1)  Press player 2 buttons 1+2 during reset.  "Ready?" will appear
2)	Press player 1 buttons in this sequence:
	2,2,2, 1,1,1, 2,2,2, 1,1,1
	The release date of this program will appear.

Some code has to be patched out for this to work (see below). The program
remaps button 2 and 3 to button 1, so you can't enter the above sequence.

---

Questions / Notes
The 2nd cpu program roms .. anyone got any idea what cpu they run on
areas 0x0000 - 0x3fff in several of them look like they contain nothing
of value.. maybe this area wouldn't be accessable really and there is
ram there in that cpu's address space (shared with the main cpu ?)
.. maybe the roms are bitswapped like the gfx.. needs investigating
really..

'manybloc' :

  - There are writes to 0x080010.w and 0x080012.w (MCU ?) in code between
    0x005000 to 0x005690, but I see no call to "main" routine at 0x005504 !
  - There are writes to 0x08001c.w and 0x08001e.w but I can't tell what
    the effect is ! Could it be related to sound and/or interrupts ?

  - In the "test mode", press BOTH player 1 buttons to exit

  - When help is available, press BUTTON2 twice within the timer to "solve"

---

Sound notes for games with a Z80:

mustangb and tdragonb use the Seibu Raiden sound hardware and a modified
Z80 program (but the music is intact and recognizable).  See sndhrdw/seibu.c
for more info on this.

tharrier and manybloc share a sound board.  manybloc has some weird/missing
sound effects and unknown writes at F600 and F700.  Possibly these are some
sort of OKIM6295 bankswitch?  But there's only 2 256k sample ROMs, and the
6295 can address 512k on it's own?

tharrier doesn't do the extra writes and doesn't appear to have any trouble.
If someone could fix the protection it'd be fully playable with sound and music...

********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/seibu.h"

extern data16_t *nmk_bgvideoram,*nmk_fgvideoram,*nmk_txvideoram;
extern data16_t *gunnail_scrollram;

READ16_HANDLER( nmk_bgvideoram_r );
WRITE16_HANDLER( nmk_bgvideoram_w );
READ16_HANDLER( nmk_fgvideoram_r );
WRITE16_HANDLER( nmk_fgvideoram_w );
READ16_HANDLER( nmk_txvideoram_r );
WRITE16_HANDLER( nmk_txvideoram_w );
WRITE16_HANDLER( nmk_scroll_w );
WRITE16_HANDLER( nmk_scroll_2_w );
WRITE16_HANDLER( gunnail_scrollx_w );
WRITE16_HANDLER( gunnail_scrolly_w );
WRITE16_HANDLER( nmk_flipscreen_w );
WRITE16_HANDLER( nmk_tilebank_w );
WRITE16_HANDLER( bioship_scroll_w );
WRITE16_HANDLER( bioship_bank_w );
WRITE16_HANDLER( mustang_scroll_w );
WRITE16_HANDLER( vandyke_scroll_w );

VIDEO_START( macross );
VIDEO_UPDATE( manybloc );
VIDEO_START( gunnail );
VIDEO_START( macross2 );
VIDEO_START( bjtwin );
VIDEO_START( bioship );
VIDEO_START( strahl );
VIDEO_UPDATE( bioship );
VIDEO_UPDATE( strahl );
VIDEO_UPDATE( macross );
VIDEO_UPDATE( gunnail );
VIDEO_UPDATE( bjtwin );
VIDEO_EOF( nmk );

static int respcount; // used with mcu function

static MACHINE_INIT( nmk16 )
{
	respcount = 0;
}

static MACHINE_INIT( mustang_sound )
{
	respcount = 0;
	machine_init_seibu_sound_1();
}

WRITE16_HANDLER ( ssmissin_sound_w )
{
	/* maybe .. */
	if (ACCESSING_LSB)
	{
		soundlatch_w(0,data & 0xff);
		cpu_set_irq_line(1,0, ASSERT_LINE);
	}

	if (ACCESSING_MSB)
		if ((data >> 8) & 0x80)
			cpu_set_irq_line(1,0, CLEAR_LINE);
}



WRITE_HANDLER ( ssmissin_soundbank_w )
{
	unsigned char *rom = memory_region(REGION_SOUND1);
	int bank;

	bank = data & 0x3;

	memcpy(rom + 0x20000,rom + 0x80000 + bank * 0x20000,0x20000);
}


static data16_t *ram;

static WRITE16_HANDLER( macross_mcu_w )
{
	logerror("%04x: mcu_w %04x\n",activecpu_get_pc(),data);


	/* since its starting to look more unlikely every day that we'll be
	   able to emulate the NMK004 sound cpu this provides some sound in
	   gunnail and macross, its entirely guesswork, sadly music can't be
	   simulated in a similar way :-( what is the NMK004 anyway, are
	   these really code roms we have, or does it have internal code of
	   its own, BioShip Paladin's 'Sound Program' Rom looks cleaner than
	   the others */


	/* Note: this doesn't play correct samples, it plays drums and stuff.
	   That's not quite right so I'm leaving it disabled */
#if 0
	if (!strcmp(Machine->gamedrv->name,"gunnail")) {
		if (ACCESSING_LSB) {
			if ((data & 0xff) == 0x00) { /* unknown */
				/* ?? */
			} else if ((data & 0xff) == 0xcc) { /* unknown */
				/* ?? */
			} else if ((data & 0xff) == 0xd0) { /* fire normal? */
				OKIM6295_data_0_w(0, 0x80 | 0x01 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else if ((data & 0xff) == 0xd1) { /* fire purple? */
				OKIM6295_data_0_w(0, 0x80 | 0x03 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else if ((data & 0xff) == 0xd2) { /* fire red? */
				OKIM6295_data_0_w(0, 0x80 | 0x03 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else if ((data & 0xff) == 0xd3) { /* fire blue? */
				OKIM6295_data_0_w(0, 0x80 | 0x03 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else if ((data & 0xff) == 0xd8) { /* explosion1? */
				OKIM6295_data_0_w(0, 0x80 | 0x07 );
				OKIM6295_data_0_w(0, 0x00 | 0x20 );
			} else if ((data & 0xff) == 0xdb) { /* explosion2? */
				OKIM6295_data_0_w(0, 0x80 | 0x08 );
				OKIM6295_data_0_w(0, 0x00 | 0x20 );
			} else if ((data & 0xff) == 0xe0) { /* coin? */
				OKIM6295_data_0_w(0, 0x80 | 0x10 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else {
//				usrintf_showmessage("%04x: mcu_w %04x\n",activecpu_get_pc(),data);
			}
		}
	}


	if (!strcmp(Machine->gamedrv->name,"macross")) {
		if (ACCESSING_LSB) {
			if ((data & 0xff) == 0xc4) { /* unknown */
				/* ?? */
			} else if ((data & 0xff) == 0xc5) { /* with bomb? */
				/* ?? */
			} else if ((data & 0xff) == 0xc6) { /* unknown */
				/* ?? */
			} else if ((data & 0xff) == 0xcc) { /* unknown */
				/* ?? */
			} else if ((data & 0xff) == 0xce) { /* green ememy die? */
				OKIM6295_data_0_w(0, 0x80 | 0x0f );
				OKIM6295_data_0_w(0, 0x00 | 0x40 );
			} else if ((data & 0xff) == 0xd0) { /* coin */
				OKIM6295_data_0_w(0, 0x80 | 0x01 );
				OKIM6295_data_0_w(0, 0x00 | 0x10 );
			} else if ((data & 0xff) == 0xd1) { /* shoot? */
				OKIM6295_data_0_w(0, 0x80 | 0x03 );
				OKIM6295_data_0_w(0, 0x00 | 0x20 );
			} else if ((data & 0xff) == 0xdb) { /* explosion? */
				OKIM6295_data_0_w(0, 0x80 | 0x10 );
				OKIM6295_data_0_w(0, 0x00 | 0x40 );
			} else if ((data & 0xff) == 0xdd) { /* player death? */
				OKIM6295_data_0_w(0, 0x80 | 0x0e );
				OKIM6295_data_0_w(0, 0x00 | 0x40 );
			} else if ((data & 0xff) == 0xde) { /* bomb? */
				OKIM6295_data_0_w(0, 0x80 | 0x0d );
				OKIM6295_data_0_w(0, 0x00 | 0x40 );
			} else if ((data & 0xff) == 0xdf) { /* some enemy fire? */
				OKIM6295_data_0_w(0, 0x80 | 0x12 );
				OKIM6295_data_0_w(0, 0x00 | 0x20 );
			} else if ((data & 0xff) == 0xe5) { /* power up? */
				OKIM6295_data_0_w(0, 0x80 | 0x16 );
				OKIM6295_data_0_w(0, 0x00 | 0x80 );
			} else {
//				usrintf_showmessage("%04x: mcu_w %04x\n",activecpu_get_pc(),data);
			}
		}
	}
#endif
}


static READ16_HANDLER( macross_mcu_r )
{
	static int resp[] = {	0x82, 0xc7, 0x00,
							0x2c, 0x6c, 0x00,
							0x9f, 0xc7, 0x00,
							0x29, 0x69, 0x00,
							0x8b, 0xc7, 0x00 };
	int res;

	if (activecpu_get_pc()==0x8aa) res = (ram[0x064/2])|0x20; /* Task Force Harrier */
	else if (activecpu_get_pc()==0x8ce) res = (ram[0x064/2])|0x60; /* Task Force Harrier */
	else if (activecpu_get_pc() == 0x0332	/* Macross */
			||	activecpu_get_pc() == 0x64f4)	/* GunNail */
		res = ram[0x0f6/2];
	else
	{
		res = resp[respcount++];
		if (respcount >= sizeof(resp)/sizeof(resp[0])) respcount = 0;
	}

logerror("%04x: mcu_r %02x\n",activecpu_get_pc(),res);

	return res;
}

static READ16_HANDLER( urashima_mcu_r )
{
	static int resp[] = {	0x99, 0xd8, 0x00,
							0x2a, 0x6a, 0x00,
							0x9c, 0xd8, 0x00,
							0x2f, 0x6f, 0x00,
							0x22, 0x62, 0x00,
							0x25, 0x65, 0x00 };
	int res;

	res = resp[respcount++];
	if (respcount >= sizeof(resp)/sizeof(resp[0])) respcount = 0;

logerror("%04x: mcu_r %02x\n",activecpu_get_pc(),res);

	return res;
}

static WRITE16_HANDLER( tharrier_mcu_control_w )
{
//	logerror("%04x: mcu_control_w %02x\n",activecpu_get_pc(),data);
}

static READ16_HANDLER( tharrier_mcu_r )
{
	/* The MCU is mapped as the top byte for byte accesses only,
		all word accesses are to the input port */
	if (ACCESSING_MSB && !ACCESSING_LSB)
		return macross_mcu_r(offset,0)<<8;
	else
		return ~input_port_1_word_r(0,0);
}

static WRITE16_HANDLER( macross2_sound_command_w )
{
	if (ACCESSING_LSB)
		soundlatch_w(0,data & 0xff);
}

static READ16_HANDLER( macross2_sound_result_r )
{
	return soundlatch2_r(0);
}

static WRITE_HANDLER( macross2_sound_bank_w )
{
	const UINT8 *rom = memory_region(REGION_CPU2) + 0x10000;

	cpu_setbank(1,rom + (data & 0x07) * 0x4000);
}

static WRITE_HANDLER( macross2_oki6295_bankswitch_w )
{
	/* The OKI6295 ROM space is divided in four banks, each one indepentently
	   controlled. The sample table at the beginning of the addressing space is
	   divided in four pages as well, banked together with the sample data. */
	#define TABLESIZE 0x100
	#define BANKSIZE 0x10000
	int chip = (offset & 4) >> 2;
	int banknum = offset & 3;
	unsigned char *rom = memory_region(REGION_SOUND1 + chip);
	int size = memory_region_length(REGION_SOUND1 + chip) - 0x40000;
	int bankaddr = (data * BANKSIZE) & (size-1);

	/* copy the samples */
	memcpy(rom + banknum * BANKSIZE,rom + 0x40000 + bankaddr,BANKSIZE);

	/* and also copy the samples address table */
	rom += banknum * TABLESIZE;
	memcpy(rom,rom + 0x40000 + bankaddr,TABLESIZE);
}

static WRITE16_HANDLER( bjtwin_oki6295_bankswitch_w )
{
	if (ACCESSING_LSB)
		macross2_oki6295_bankswitch_w(offset,data & 0xff);
}

static READ16_HANDLER( hachamf_protection_hack_r )
{
	/* adresses for the input ports */
	static int pap[] = { 0x0008, 0x0000, 0x0008, 0x0002, 0x0008, 0x0008 };

	return pap[offset];
}

/***************************************************************************/

static MEMORY_READ16_START( urashima_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080004, 0x080005, urashima_mcu_r },
	{ 0x09e000, 0x09e7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
#if 0
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x09e000, 0x0a1fff, nmk_bgvideoram_r },
#endif
MEMORY_END

static MEMORY_WRITE16_START( urashima_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080014, 0x080015, macross_mcu_w },
	{ 0x09e000, 0x09e7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },	/* Work RAM again */
#if 0
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA16_NOP },	/* IRQ enable? */
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x09e000, 0x0a1fff, nmk_bgvideoram_w, &nmk_bgvideoram },
#endif
MEMORY_END


static MEMORY_READ16_START( vandyke_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x08000e, 0x08000f, macross_mcu_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( vandyke_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080016, 0x080017, MWA16_NOP },	/* IRQ enable? */
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x08001e, 0x08001f, macross_mcu_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, vandyke_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
//	{ 0x094000, 0x097fff, MWA16_RAM }, /* what is this */
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM }, /* not tested in tests .. hardly used probably some registers not ram */
MEMORY_END

static READ16_HANDLER(logr)
{
//logerror("Read input port 1 %05x\n",activecpu_get_pc());
return ~input_port_0_word_r(0,0);
}

static MEMORY_READ16_START( manybloc_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080004, 0x080005, input_port_2_word_r },
	{ 0x08001e, 0x08001f, soundlatch2_word_r },
	{ 0x088000, 0x0883ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09cfff, MRA16_RAM }, /* Scroll? */
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( manybloc_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080010, 0x080011, MWA16_NOP },			/* See notes at the top of the driver */
	{ 0x080012, 0x080013, MWA16_NOP },			/* See notes at the top of the driver */
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x08001c, 0x08001d, MWA16_NOP },			/* See notes at the top of the driver */
	{ 0x08001e, 0x08001f, soundlatch_word_w },
	{ 0x088000, 0x0883ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09cfff, MWA16_RAM }, /* Scroll? */
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },
MEMORY_END

static MEMORY_READ_START( manybloc_sound_readmem )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xf000, 0xf000, soundlatch_r },
	{ 0xf400, 0xf400, OKIM6295_status_0_r },
	{ 0xf500, 0xf500, OKIM6295_status_1_r },
MEMORY_END

static MEMORY_WRITE_START( manybloc_sound_writemem)
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch2_w },
	{ 0xf400, 0xf400, OKIM6295_data_0_w },
	{ 0xf500, 0xf500, OKIM6295_data_1_w },
	{ 0xf600, 0xf600, MWA_NOP },
	{ 0xf700, 0xf700, MWA_NOP },
MEMORY_END

static PORT_READ_START( manybloc_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
PORT_END

static PORT_WRITE_START( manybloc_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
PORT_END

static MEMORY_READ16_START( tharrier_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, logr },//input_port_0_word_r },
	{ 0x080002, 0x080003, tharrier_mcu_r }, //input_port_1_word_r },
//	{ 0x080004, 0x080005, input_port_2_word_r },
	{ 0x08000e, 0x08000f, soundlatch2_word_r },	/* from Z80 */
	{ 0x088000, 0x0883ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( tharrier_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080010, 0x080011, tharrier_mcu_control_w },
	{ 0x080012, 0x080013, macross_mcu_w },
//	{ 0x080014, 0x080015, nmk_flipscreen_w },
//	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x08001e, 0x08001f, soundlatch_word_w },
	{ 0x088000, 0x0883ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, MWA16_NOP }, /* Unused txvideoram area? */
	{ 0x09d000, 0x09d7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
MEMORY_END

//Read input port 1 030c8/  BAD
//3478  GOOD

static MEMORY_READ16_START( mustang_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080004, 0x080005, input_port_2_word_r },
	{ 0x08000e, 0x08000f, macross_mcu_r },
//	{ 0x08000e, 0x08000f, soundlatch2_word_r },	/* from Z80 bootleg only? */
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( mustang_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x08000e, 0x08000f, macross_mcu_w },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA16_NOP },	// frame number?
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, mustang_scroll_w },
//{ 0x08c000, 0x08c001, MWA16_NOP },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },	/* Work RAM */
MEMORY_END

static MEMORY_WRITE16_START( mustangb_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA16_NOP },	// frame number?
	{ 0x08001e, 0x08001f, seibu_main_mustb_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, mustang_scroll_w },
//{ 0x08c000, 0x08c001, MWA16_NOP },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },	/* Work RAM */
MEMORY_END

static MEMORY_READ16_START( acrobatm_readmem )
	{ 0x00000, 0x3ffff, MRA16_ROM },
	{ 0x80000, 0x8ffff, MRA16_RAM },
	{ 0xc0000, 0xc0001, input_port_0_word_r },
	{ 0xc0002, 0xc0003, input_port_1_word_r },
	{ 0xc0008, 0xc0009, input_port_2_word_r },
	{ 0xc000a, 0xc000b, input_port_3_word_r },
	{ 0xc4000, 0xc45ff, MRA16_RAM },
	{ 0xcc000, 0xcffff, nmk_bgvideoram_r },
	{ 0xd4000, 0xd47ff, nmk_txvideoram_r },
MEMORY_END

static MEMORY_WRITE16_START( acrobatm_writemem )
	{ 0x00000, 0x3ffff, MWA16_ROM },
	{ 0x88000, 0x88fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x80000, 0x8ffff, MWA16_RAM },
	{ 0xc0014, 0xc0015, nmk_flipscreen_w },
	{ 0xc4000, 0xc45ff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },
	{ 0xc8000, 0xc8007, nmk_scroll_w },
	{ 0xcc000, 0xcffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0xd4000, 0xd47ff, nmk_txvideoram_w, &nmk_txvideoram },
MEMORY_END

static MEMORY_READ16_START( hachamf_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0fe000, 0x0fe00b, hachamf_protection_hack_r },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( hachamf_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM },	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
MEMORY_END

static MEMORY_READ16_START( bioship_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bioship_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
//	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x084000, 0x084001, bioship_bank_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, mustang_scroll_w },
	{ 0x08c010, 0x08c017, bioship_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM },	/* Work RAM again (fe000-fefff is shared with the sound CPU) */
MEMORY_END

static MEMORY_READ16_START( tdragon_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x044022, 0x044023, MRA16_NOP },  /* No Idea */
	{ 0x0b0000, 0x0b7fff, MRA16_RAM },	/* Work RAM */
	{ 0x0b8000, 0x0b8fff, MRA16_RAM },	/* Sprite RAM */
	{ 0x0b9000, 0x0bffff, MRA16_RAM },	/* Work RAM */
	{ 0x0c8000, 0x0c87ff, MRA16_RAM },  /* Palette RAM */
	{ 0x0c0000, 0x0c0001, input_port_0_word_r },
	{ 0x0c0002, 0x0c0003, input_port_1_word_r },
	{ 0x0c0008, 0x0c0009, input_port_2_word_r },
	{ 0x0c000a, 0x0c000b, input_port_3_word_r },
	{ 0x0cc000, 0x0cffff, nmk_bgvideoram_r },
	{ 0x0d0000, 0x0d07ff, nmk_txvideoram_r },
MEMORY_END

static MEMORY_WRITE16_START( tdragon_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0b0000, 0x0b7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0b8000, 0x0b8fff, MWA16_RAM, &spriteram16, &spriteram_size },	/* Sprite RAM */
	{ 0x0b9000, 0x0bffff, MWA16_RAM },	/* Work RAM */
	{ 0x0c0014, 0x0c0015, nmk_flipscreen_w }, /* Maybe */
	{ 0x0c0018, 0x0c0019, nmk_tilebank_w }, /* Tile Bank ? */
	{ 0x0c001e, 0x0c001f, MWA16_NOP },
	{ 0x0c4000, 0x0c4007, nmk_scroll_w },
	{ 0x0c8000, 0x0c87ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x0cc000, 0x0cffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x0d0000, 0x0d07ff, nmk_txvideoram_w, &nmk_txvideoram },
MEMORY_END

static MEMORY_WRITE16_START( tdragonb_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0b0000, 0x0b7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0b8000, 0x0b8fff, MWA16_RAM, &spriteram16, &spriteram_size },	/* Sprite RAM */
	{ 0x0b9000, 0x0bffff, MWA16_RAM },	/* Work RAM */
	{ 0x0c0014, 0x0c0015, nmk_flipscreen_w }, /* Maybe */
	{ 0x0c0018, 0x0c0019, nmk_tilebank_w }, /* Tile Bank ? */
	{ 0x0c001e, 0x0c001f, seibu_main_mustb_w },
	{ 0x0c4000, 0x0c4007, nmk_scroll_w },
	{ 0x0c8000, 0x0c87ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x0cc000, 0x0cffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x0d0000, 0x0d07ff, nmk_txvideoram_w, &nmk_txvideoram },
MEMORY_END

static MEMORY_READ16_START( ssmissin_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x0b0000, 0x0b7fff, MRA16_RAM },	/* Work RAM */
	{ 0x0b8000, 0x0b8fff, MRA16_RAM },	/* Sprite RAM */
	{ 0x0b9000, 0x0bffff, MRA16_RAM },	/* Work RAM */
	{ 0x0c8000, 0x0c87ff, MRA16_RAM },  /* Palette RAM */
	{ 0x0c0000, 0x0c0001, input_port_0_word_r },
	{ 0x0c0004, 0x0c0005, input_port_1_word_r },
	{ 0x0c0006, 0x0c0007, input_port_2_word_r },
//	{ 0x0c000e, 0x0c000f, ?? },
	{ 0x0cc000, 0x0cffff, nmk_bgvideoram_r },
	{ 0x0d0000, 0x0d07ff, nmk_txvideoram_r },
MEMORY_END

static MEMORY_WRITE16_START( ssmissin_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0b0000, 0x0b7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0b8000, 0x0b8fff, MWA16_RAM, &spriteram16, &spriteram_size },	/* Sprite RAM */
	{ 0x0b9000, 0x0bffff, MWA16_RAM },	/* Work RAM */
	{ 0x0c0014, 0x0c0015, nmk_flipscreen_w }, /* Maybe */
	{ 0x0c0018, 0x0c0019, nmk_tilebank_w }, /* Tile Bank ? */
	{ 0x0c001e, 0x0c001f, ssmissin_sound_w },
	{ 0x0c4000, 0x0c4007, nmk_scroll_w },
	{ 0x0c8000, 0x0c87ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x0cc000, 0x0cffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x0d0000, 0x0d07ff, nmk_txvideoram_w, &nmk_txvideoram },
MEMORY_END



static MEMORY_READ_START( ssmissin_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( ssmissin_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, ssmissin_soundbank_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END


static MEMORY_READ16_START( strahl_readmem )
	{ 0x00000, 0x3ffff, MRA16_ROM },
	{ 0x80000, 0x80001, input_port_0_word_r },
	{ 0x80002, 0x80003, input_port_1_word_r },
	{ 0x80008, 0x80009, input_port_2_word_r },
	{ 0x8000a, 0x8000b, input_port_3_word_r },
	{ 0x8c000, 0x8c7ff, MRA16_RAM },
	{ 0x90000, 0x93fff, nmk_bgvideoram_r },
	{ 0x94000, 0x97fff, nmk_fgvideoram_r },
	{ 0x9c000, 0x9c7ff, nmk_txvideoram_r },
	{ 0xf0000, 0xf7fff, MRA16_RAM },
	{ 0xf8000, 0xfefff, MRA16_RAM },
	{ 0xff000, 0xfffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( strahl_writemem )
	{ 0x00000, 0x3ffff, MWA16_ROM },
	{ 0x80014, 0x80015, nmk_flipscreen_w },
	{ 0x8001e, 0x8001f, MWA16_NOP }, /* -> Sound cpu */
	{ 0x84000, 0x84007, nmk_scroll_w },
	{ 0x88000, 0x88007, nmk_scroll_2_w },
	{ 0x8c000, 0x8c7ff, paletteram16_RRRRGGGGBBBBxxxx_word_w, &paletteram16 },
	{ 0x90000, 0x93fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x94000, 0x97fff, nmk_fgvideoram_w, &nmk_fgvideoram },
	{ 0x9c000, 0x9c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0xf0000, 0xf7fff, MWA16_RAM },	/* Work RAM */
	{ 0xf8000, 0xfefff, MWA16_RAM, &ram },	/* Work RAM again */
	{ 0xff000, 0xfffff, MWA16_RAM, &spriteram16, &spriteram_size },
MEMORY_END

static MEMORY_READ16_START( macross_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x08000e, 0x08000f, macross_mcu_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_r },
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( macross_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA16_NOP },	/* IRQ enable? */
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x08001e, 0x08001f, macross_mcu_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c007, nmk_scroll_w },
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09c7ff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram },	/* Work RAM again */
MEMORY_END

static MEMORY_READ16_START( gunnail_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x08000e, 0x08000f, macross_mcu_r },
	{ 0x088000, 0x0887ff, MRA16_RAM }, /* palette ram */
	{ 0x090000, 0x093fff, nmk_bgvideoram_r },
	{ 0x09c000, 0x09cfff, nmk_txvideoram_r },
	{ 0x09d000, 0x09dfff, nmk_txvideoram_r },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( gunnail_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x08001e, 0x08001f, macross_mcu_w },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x080016, 0x080017, MWA16_NOP },	/* IRQ enable? */
	{ 0x080018, 0x080019, nmk_tilebank_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x08c000, 0x08c1ff, gunnail_scrollx_w, &gunnail_scrollram },
	{ 0x08c200, 0x08c201, gunnail_scrolly_w },
//	{ 0x08c202, 0x08c7ff, MWA16_RAM },	// unknown
	{ 0x090000, 0x093fff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09c000, 0x09cfff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x09d000, 0x09dfff, nmk_txvideoram_w },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM, &ram }, /* Work RAM again */
MEMORY_END

static MEMORY_READ16_START( macross2_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x100001, input_port_0_word_r },
	{ 0x100002, 0x100003, input_port_1_word_r },
	{ 0x100008, 0x100009, input_port_2_word_r },
	{ 0x10000a, 0x10000b, input_port_3_word_r },
	{ 0x10000e, 0x10000f, macross2_sound_result_r },	/* from Z80 */
	{ 0x120000, 0x1207ff, MRA16_RAM },
	{ 0x140000, 0x14ffff, nmk_bgvideoram_r },
	{ 0x170000, 0x170fff, nmk_txvideoram_r },
	{ 0x171000, 0x171fff, nmk_txvideoram_r },	/* mirror */
	{ 0x1f0000, 0x1f7fff, MRA16_RAM },
	{ 0x1f8000, 0x1f8fff, MRA16_RAM },
	{ 0x1f9000, 0x1fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( macross2_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100014, 0x100015, nmk_flipscreen_w },
	{ 0x100016, 0x100017, MWA16_NOP },	/* IRQ eanble? */
	{ 0x100018, 0x100019, nmk_tilebank_w },
	{ 0x10001e, 0x10001f, macross2_sound_command_w },	/* to Z80 */
	{ 0x120000, 0x1207ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x130000, 0x130007, nmk_scroll_w },
	{ 0x130008, 0x1307ff, MWA16_NOP },	/* 0 only? */
	{ 0x140000, 0x14ffff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x170000, 0x170fff, nmk_txvideoram_w, &nmk_txvideoram },
	{ 0x171000, 0x171fff, nmk_txvideoram_w },	/* mirror */
	{ 0x1f0000, 0x1f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x1f8000, 0x1f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x1f9000, 0x1fffff, MWA16_RAM, &ram },	/* Work RAM again */
MEMORY_END


static MEMORY_READ_START( macross2_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },	/* banked ROM */
	{ 0xa000, 0xa000, MRA_NOP },	/* IRQ ack? watchdog? */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xf000, 0xf000, soundlatch_r },	/* from 68000 */
MEMORY_END

static MEMORY_WRITE_START( macross2_sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe001, 0xe001, macross2_sound_bank_w },
	{ 0xf000, 0xf000, soundlatch2_w },	/* to 68000 */
MEMORY_END

static PORT_READ_START( macross2_sound_readport )
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x80, 0x80, OKIM6295_status_0_r },
	{ 0x88, 0x88, OKIM6295_status_1_r },
PORT_END

static PORT_WRITE_START( macross2_sound_writeport )
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x80, 0x80, OKIM6295_data_0_w },
	{ 0x88, 0x88, OKIM6295_data_1_w },
	{ 0x90, 0x97, macross2_oki6295_bankswitch_w },
PORT_END

static MEMORY_READ16_START( bjtwin_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080001, input_port_0_word_r },
	{ 0x080002, 0x080003, input_port_1_word_r },
	{ 0x080008, 0x080009, input_port_2_word_r },
	{ 0x08000a, 0x08000b, input_port_3_word_r },
	{ 0x084000, 0x084001, OKIM6295_status_0_lsb_r },
	{ 0x084010, 0x084011, OKIM6295_status_1_lsb_r },
	{ 0x088000, 0x0887ff, MRA16_RAM },
	{ 0x09c000, 0x09cfff, nmk_bgvideoram_r },
	{ 0x09d000, 0x09dfff, nmk_bgvideoram_r },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MRA16_RAM },
	{ 0x0f8000, 0x0f8fff, MRA16_RAM },
	{ 0x0f9000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( bjtwin_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080014, 0x080015, nmk_flipscreen_w },
	{ 0x084000, 0x084001, OKIM6295_data_0_lsb_w },
	{ 0x084010, 0x084011, OKIM6295_data_1_lsb_w },
	{ 0x084020, 0x08402f, bjtwin_oki6295_bankswitch_w },
	{ 0x088000, 0x0887ff, paletteram16_RRRRGGGGBBBBRGBx_word_w, &paletteram16 },
	{ 0x094000, 0x094001, nmk_tilebank_w },
	{ 0x094002, 0x094003, MWA16_NOP },	/* IRQ enable? */
	{ 0x09c000, 0x09cfff, nmk_bgvideoram_w, &nmk_bgvideoram },
	{ 0x09d000, 0x09dfff, nmk_bgvideoram_w },	/* mirror */
	{ 0x0f0000, 0x0f7fff, MWA16_RAM },	/* Work RAM */
	{ 0x0f8000, 0x0f8fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x0f9000, 0x0fffff, MWA16_RAM },	/* Work RAM again */
MEMORY_END


INPUT_PORTS_START( vandyke )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00,  "2" )
	PORT_DIPSETTING(    0x01,  "3" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( blkheart )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x02, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40,  "2" )
	PORT_DIPSETTING(    0xc0,  "3" )
	PORT_DIPSETTING(    0x80,  "4" )
	PORT_DIPSETTING(    0x00,  "5" )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( manybloc )
	PORT_START	/* IN0 - 0x080000 */
	PORT_BIT( 0x7fff, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )		// VBLANK ? Check code at 0x005640

	PORT_START	/* IN1 - 0x080002 */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// select fruits
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// help
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	// select fruits
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	// help
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START	/* DSW - 0x080004 -> 0x0f0036 */
	PORT_DIPNAME( 0x0001, 0x0000, "Slot System" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, "Explanation" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPSETTING(      0x0002, "Japanese" )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Cabinet ) )		// "Play Type"
	PORT_DIPSETTING(      0x0008, DEF_STR( Upright ) )		//   "Uplight" !
	PORT_DIPSETTING(      0x0000, DEF_STR( Cocktail ) )		//   "Table"
	PORT_SERVICE( 0x0010, IP_ACTIVE_HIGH )				// "Test Mode"
	PORT_DIPNAME( 0x0060, 0x0000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0060, "Easy" )				//   "Level 1
	PORT_DIPSETTING(      0x0000, "Normal" )				//   "Level 2
	PORT_DIPSETTING(      0x0020, "Hard" )				//   "Level 3
	PORT_DIPSETTING(      0x0040, "Hardest" )				//   "Level 4
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Flip_Screen ) )	// "Display"
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )			//   "Normal"
	PORT_DIPSETTING(      0x0080, DEF_STR( On ) )			//   "Inverse"
	PORT_DIPNAME( 0x0700, 0x0000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0700, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0600, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0500, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x3800, 0x0000, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x3000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x2800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0xc000, 0x0000, "Plate Probability" )
	PORT_DIPSETTING(      0xc000, "Bad" )
	PORT_DIPSETTING(      0x0000, "Normal" )
	PORT_DIPSETTING(      0x4000, "Better" )
	PORT_DIPSETTING(      0x8000, "Best" )
INPUT_PORTS_END

INPUT_PORTS_START( tharrier )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* Mcu status? */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
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
INPUT_PORTS_END

INPUT_PORTS_START( mustang )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// TEST in service mode
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x00e0, 0x00e0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c00, 0x0c00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0400, "Easy" )
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0800, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x4000, "2" )
	PORT_DIPSETTING(      0xc000, "3" )
	PORT_DIPSETTING(      0x8000, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
INPUT_PORTS_END

INPUT_PORTS_START( hachamf )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START  /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( strahl )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START  /* DSW B */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x40, "100k and every 200k" )
	PORT_DIPSETTING(    0x60, "200k and every 200k" )
	PORT_DIPSETTING(    0x20, "300k and every 300k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

INPUT_PORTS_START( acrobatm )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001C, 0x001C, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000C, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00E0, 0x00E0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00C0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00E0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00A0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )

	PORT_START /* DSW B */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x02, "50k and 100k" )
	PORT_DIPSETTING(    0x06, "100k and 100k" )
	PORT_DIPSETTING(    0x04, "100k and 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "Japanese" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x20, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x00, "5" )
INPUT_PORTS_END

INPUT_PORTS_START( bioship )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0006, 0x0006, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0006, "Normal" )
	PORT_DIPSETTING(      0x0002, "Hard" )
	PORT_DIPSETTING(      0x0004, "Hardest" )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( On ) )
	PORT_DIPNAME( 0x00C0, 0x00C0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x00C0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPSETTING(      0x0040, "5" )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x001C, 0x001C, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001C, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x000C, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00E0, 0x00E0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00C0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x00E0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x00A0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
INPUT_PORTS_END

INPUT_PORTS_START( tdragon )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// TEST in service mode
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0003, 0x0003, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0002, "2" )
	PORT_DIPSETTING(      0x0003, "3" )
	PORT_DIPSETTING(      0x0001, "4" )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
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
INPUT_PORTS_END

INPUT_PORTS_START( ssmissin )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )		// "Servise" in "test mode"
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )		// "Fire"
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )		// "Bomb"
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )		// "Fire"
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )		// "Bomb"
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0004, "Easy" )
	PORT_DIPSETTING(      0x000c, "Normal" )
	PORT_DIPSETTING(      0x0008, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0080, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#if 0
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )	// initialised but not read back
	PORT_DIPSETTING(      0x0400, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
#else
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
#endif
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( macross )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "Japanese" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( macross2 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "Japanese" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( tdragon2 )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( gunnail )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( sabotenb )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* shown in service mode, but no effect */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Language" )
	PORT_DIPSETTING(    0x02, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( bjtwin )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* shown in service mode, but no effect */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Maybe unused */

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0e, 0x0e, "Starting level" )
	PORT_DIPSETTING(    0x08, "Germany" )
	PORT_DIPSETTING(    0x04, "Thailand" )
	PORT_DIPSETTING(    0x0c, "Nevada" )
	PORT_DIPSETTING(    0x0e, "Japan" )
	PORT_DIPSETTING(    0x06, "Korea" )
	PORT_DIPSETTING(    0x0a, "England" )
	PORT_DIPSETTING(    0x02, "Hong Kong" )
	PORT_DIPSETTING(    0x00, "China" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END

INPUT_PORTS_START( nouryoku )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW A */
	PORT_DIPNAME( 0x03, 0x03, "Life Decrease Speed" )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_4C ) )

	PORT_START	/* DSW B */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static struct GfxDecodeInfo tharrier_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo macross_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo macross2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 32 },	/* color 0x100-0x2ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bjtwin_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX2, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo bioship_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX2, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ REGION_GFX3, 0, &tilelayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ REGION_GFX4, 0, &tilelayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ -1 }
};

static struct GfxDecodeInfo strahl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },	/* color 0x000-0x0ff */
	{ REGION_GFX2, 0, &tilelayout, 0x300, 16 },	/* color 0x300-0x3ff */
	{ REGION_GFX3, 0, &tilelayout, 0x100, 16 },	/* color 0x100-0x1ff */
	{ REGION_GFX4, 0, &tilelayout, 0x200, 16 },	/* color 0x200-0x2ff */
	{ -1 }
};


static void ym2203_irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface_15 =
{
	1,			/* 1 chip */
	1500000,	/* 2 MHz ??? */
	{ YM2203_VOL(90,90) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ ym2203_irqhandler }
};

static struct OKIM6295interface okim6295_interface_dual =
{
	2,              					/* 2 chips */
	{ 16000000/4/165, 16000000/4/165 },	/* 24242Hz frequency? */
	{ REGION_SOUND1, REGION_SOUND2 },	/* memory region */
	{ 40, 40 }							/* volume */
};

static struct OKIM6295interface okim6295_interface_ssmissin =
{
	1,              	/* 1 chip */
	{ 8000000/4/165 },	/* ? unknown */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }				/* volume */
};

static INTERRUPT_GEN( nmk_interrupt )
{
	if (cpu_getiloops() == 0) cpu_set_irq_line(0, 4, HOLD_LINE);
	else cpu_set_irq_line(0, 2, HOLD_LINE);
}

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4, 8000, REGION_SOUND1);

static MACHINE_DRIVER_START( urashima )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(urashima_readmem,urashima_writemem)
//	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)	/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vandyke )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(vandyke_readmem,vandyke_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	/* there's also a YM2203 */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( tharrier )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz */
	MDRV_CPU_MEMORY(tharrier_readmem,tharrier_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(manybloc_sound_readmem,manybloc_sound_writemem)
	MDRV_CPU_PORTS(manybloc_sound_readport,manybloc_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(tharrier_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( manybloc )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10? MHz - check */
	MDRV_CPU_MEMORY(manybloc_readmem,manybloc_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,60)/* is this is too high it breaks the game on this one, too low sprites flicker */

	MDRV_CPU_ADD(Z80, 3000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(manybloc_sound_readmem,manybloc_sound_writemem)
	MDRV_CPU_PORTS(manybloc_sound_readport,manybloc_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(tharrier_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_START(macross)
//	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(manybloc)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mustang )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(mustang_readmem,mustang_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)


	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mustangb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(mustang_readmem,mustangb_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( acrobatm )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? 12 MHz? */
	MDRV_CPU_MEMORY(acrobatm_readmem,acrobatm_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	/* there's also a YM2203? */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bioship )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000) /* 16 MHz ? */
	MDRV_CPU_MEMORY(bioship_readmem,bioship_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bioship_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bioship)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(bioship)

	/* sound hardware */
	/* there's also a YM2203 */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


/* bootleg using Raiden sound hardware */
static MACHINE_DRIVER_START( tdragonb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(tdragon_readmem,tdragonb_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ?? drives music */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(mustang_sound)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tdragon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(tdragon_readmem,tdragon_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ?? drives music */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ssmissin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(ssmissin_readmem,ssmissin_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112) /* input related */

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* 4 MHz ? */
	MDRV_CPU_MEMORY(ssmissin_sound_readmem,ssmissin_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_ssmissin)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( strahl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* 12 MHz ? */
	MDRV_CPU_MEMORY(strahl_readmem,strahl_writemem)
	MDRV_CPU_VBLANK_INT(nmk_interrupt,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(strahl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(strahl)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(strahl)

	/* sound hardware */
	/* there's also a YM2203 */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hachamf )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(hachamf_readmem,hachamf_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	/* there's also a YM2203 */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( macross )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(macross_readmem,macross_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	/* there's also a YM2203 */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gunnail )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz? */
	MDRV_CPU_MEMORY(gunnail_readmem,gunnail_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(gunnail)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(gunnail)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( macross2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz ? */
	MDRV_CPU_MEMORY(macross2_readmem,macross2_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ???????? */

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* 4 MHz ? */
	MDRV_CPU_MEMORY(macross2_sound_readmem,macross2_sound_writemem)
	MDRV_CPU_PORTS(macross2_sound_readport,macross2_sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(macross2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(macross2)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(macross)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, ym2203_interface_15)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bjtwin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000) /* 10 MHz? It's a P12, but xtals are 10MHz and 16MHz */
	MDRV_CPU_MEMORY(bjtwin_readmem,bjtwin_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,112)/* ?? drives music */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(nmk16)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(bjtwin_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(bjtwin)
	MDRV_VIDEO_EOF(nmk)
	MDRV_VIDEO_UPDATE(bjtwin)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface_dual)
MACHINE_DRIVER_END



ROM_START ( urashima )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "um-2.15d",  0x00000, 0x20000, 0xa90a47e3 )
	ROM_LOAD16_BYTE( "um-1.15c",  0x00001, 0x20000, 0x5f5c8f39 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "um-5.22j",		0x000000, 0x020000, 0x991776a2 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE ) /* 16x16 Tiles */
	ROM_LOAD( "um-7.4l",	0x000000, 0x080000, 0xd2a68cfb )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE ) /* Maybe there are no Sprites? */
	ROM_LOAD( "um-6.2l",	0x000000, 0x080000, 0x076be5b5 )

	ROM_REGION( 0x0240, REGION_PROMS, 0 )
	ROM_LOAD( "um-10.2b",      0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "um-11.2c",      0x0100, 0x0100, 0xff5660cf )	/* unknown */
	ROM_LOAD( "um-12.20c",     0x0200, 0x0020, 0xbdb66b02 )	/* unknown */
	ROM_LOAD( "um-13.10l",     0x0220, 0x0020, 0x4ce07ec0 )	/* unknown */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "um-3.22c",		0x000000, 0x080000, 0x9fd8c8fa )
ROM_END

ROM_START( vandyke )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "vdk-1.16",  0x00000, 0x20000, 0xc1d01c59 )
	ROM_LOAD16_BYTE( "vdk-2.15",  0x00001, 0x20000, 0x9d741cc2 )

	ROM_REGION(0x10000, REGION_CPU2, 0 ) /* 64k for sound cpu code */
	ROM_LOAD( "vdk-4.127",    0x00000, 0x10000, 0xeba544f0)

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-3.222",		0x000000, 0x010000, 0x5a547c1b )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vdk-01.13",		0x000000, 0x080000, 0x195a24be )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "vdk-07.202",	0x000000, 0x080000, 0x42d41f06 )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-06.203",	0x000001, 0x080000, 0xd54722a8 )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-04.2-1",	0x100000, 0x080000, 0x0a730547 )	/* Sprites */
	ROM_LOAD16_BYTE( "vdk-05.3-1",	0x100001, 0x080000, 0xba456d27 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-02.126",     0x000000, 0x080000, 0xb2103274 )	/* all banked */

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "vdk-03.165",     0x000000, 0x080000, 0x631776d3 )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "ic100.bpr", 0x0000, 0x0100, 0x98ed1c97 )	/* V-sync hw (unused) */
	ROM_LOAD( "ic101.bpr", 0x0100, 0x0100, 0xcfdbb86c )	/* H-sync hw (unused) */
ROM_END

ROM_START( tharrier )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2" ,   0x00000, 0x20000, 0x78923aaa )
	ROM_LOAD16_BYTE( "3" ,   0x00001, 0x20000, 0x99cea259 )

	ROM_REGION( 0x010000, REGION_CPU2, 0 )
	ROM_LOAD( "12" ,   0x00000, 0x10000, 0xb959f837 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1" ,        0x000000, 0x10000, 0xc7402e4a )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "89050-4" ,  0x000000, 0x80000, 0x64d7d687 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "89050-13",	0x000000, 0x80000, 0x24db3fa4 )	/* Sprites */
	ROM_LOAD16_BYTE( "89050-17",	0x000001, 0x80000, 0x7f715421 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-8",   0x00000, 0x80000, 0x11ee4c39 )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "89050-10",  0x00000, 0x80000, 0x893552ab )

	ROM_REGION( 0x140, REGION_PROMS, 0 )
	ROM_LOAD( "25",  0x00000, 0x100, 0xe0a009fe )
	ROM_LOAD( "23",  0x00100, 0x020, 0xfc3569f4 )
	ROM_LOAD( "26",  0x00120, 0x020, 0x0cbfb33e )
ROM_END

ROM_START( mustang )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2.bin",    0x00000, 0x20000, 0xbd9f7c89 )
	ROM_LOAD16_BYTE( "3.bin",    0x00001, 0x20000, 0x0eec36a5 )

	ROM_REGION(0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "90058-7",    0x00000, 0x10000, 0x920a93c8 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, 0x81ccfcad )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, 0xa07a2002 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, 0x560bff04 )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, 0xb9d72a03 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-5",    0x000000, 0x80000, 0xc60c883e )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-6",    0x000000, 0x80000, 0x233c1776 )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "10.bpr",  0x00000, 0x100, 0x633ab1c9 ) /* unknown */
	ROM_LOAD( "90058-11",  0x00100, 0x100, 0xcfdbb86c ) /* unknown */
ROM_END

ROM_START( mustangs )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "90058-2",    0x00000, 0x20000, 0x833aa458 )
	ROM_LOAD16_BYTE( "90058-3",    0x00001, 0x20000, 0xe4b80f06 )

	ROM_REGION(0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "90058-7",    0x00000, 0x10000, 0x920a93c8 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, 0x81ccfcad )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, 0xa07a2002 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, 0x560bff04 )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, 0xb9d72a03 )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-5",    0x000000, 0x80000, 0xc60c883e )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90058-6",    0x000000, 0x80000, 0x233c1776 )

	ROM_REGION( 0x200, REGION_PROMS, 0 )
	ROM_LOAD( "90058-10",  0x00000, 0x100, 0xde156d99 ) /* unknown */
	ROM_LOAD( "90058-11",  0x00100, 0x100, 0xcfdbb86c ) /* unknown */
ROM_END

ROM_START( mustangb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "mustang.14",    0x00000, 0x20000, 0x13c6363b )
	ROM_LOAD16_BYTE( "mustang.13",    0x00001, 0x20000, 0xd8ccce31 )

	ROM_REGION(0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "mustang.16",    0x00000, 0x8000, 0x99ee7505 )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-1",    0x00000, 0x20000, 0x81ccfcad )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90058-4",    0x000000, 0x80000, 0xa07a2002 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "90058-8",    0x00000, 0x80000, 0x560bff04 )
	ROM_LOAD16_BYTE( "90058-9",    0x00001, 0x80000, 0xb9d72a03 )

	ROM_REGION( 0x010000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "mustang.17",    0x00000, 0x10000, 0xf6f6c4bf )
ROM_END

ROM_START( acrobatm )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "02_ic100.bin",    0x00000, 0x20000, 0x3fe487f4 )
	ROM_LOAD16_BYTE( "01_ic101.bin",    0x00001, 0x20000, 0x17175753 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "03_ic79.bin",   0x000000, 0x10000, 0xd86c186e ) /* Characters */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "09_ic8.bin",  0x000000, 0x100000, 0x7c12afed ) /* Foreground */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "07_ic42.bin",  0x000000, 0x100000, 0x5672bdaa ) /* Sprites */
	ROM_LOAD( "08_ic29.bin",  0x100000, 0x080000, 0xb4c0ace3 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "04_ic74.bin",    0x00000, 0x10000, 0x176905fb )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "05_ic54.bin",    0x00000, 0x80000, 0x3b8c2b0e )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "06_ic53.bin",    0x00000, 0x80000, 0xc1517cd4 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "10_ic81.bin",    0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "11_ic80.bin",    0x0100, 0x0100, 0x633ab1c9 )	/* unknown */
ROM_END

ROM_START( bioship )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "2",    0x00000, 0x20000, 0xacf56afb )
	ROM_LOAD16_BYTE( "1",    0x00001, 0x20000, 0x820ef303 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",         0x000000, 0x10000, 0x2f3f5a10 ) /* Characters */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.01",  0x000000, 0x80000, 0x21302e78 ) /* Foreground */

	ROM_REGION( 0x80000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.03",  0x000000, 0x80000, 0x60e00d7b ) /* Sprites */

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "sbs-g.02",  0x000000, 0x80000, 0xf31eb668 ) /* Background */

	ROM_REGION16_BE(0x20000, REGION_GFX5, 0 )	/* Background tilemaps (used at runtime) */
	ROM_LOAD16_BYTE( "8",    0x00000, 0x10000, 0x75a46fea )
	ROM_LOAD16_BYTE( "9",    0x00001, 0x10000, 0xd91448ee )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "6",    0x00000, 0x10000, 0x5f39a980 )

	ROM_REGION(0x80000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "sbs-g.04",    0x00000, 0x80000, 0x7c74cc4e )

	ROM_REGION(0x80000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "sbs-g.05",    0x00000, 0x80000, 0xf0a782e3 )
ROM_END

ROM_START( blkheart )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "blkhrt.7",  0x00000, 0x20000, 0x5bd248c0 )
	ROM_LOAD16_BYTE( "blkhrt.6",  0x00001, 0x20000, 0x6449e50d )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "4.bin",      0x00000, 0x10000, 0x7cefa295 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",    0x000000, 0x020000, 0xa1ab3a16 )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90068-5.bin", 0x000000, 0x100000, 0xa1ab4f24 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "90068-8.bin", 0x000000, 0x100000, 0x9d3204b2 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-1.bin", 0x000000, 0x080000, 0xe7af69d2 )	/* all banked */

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-2.bin", 0x000000, 0x080000, 0x3a583184 )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, 0x98ed1c97 )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0100, 0x0100, 0xcfdbb86c )	/* unknown */
ROM_END

ROM_START( blkhearj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "7.bin",  0x00000, 0x20000, 0xe0a5c667 )
	ROM_LOAD16_BYTE( "6.bin",  0x00001, 0x20000, 0x7cce45e8 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "4.bin",      0x00000, 0x10000, 0x7cefa295 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "3.bin",    0x000000, 0x020000, 0xa1ab3a16 )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "90068-5.bin", 0x000000, 0x100000, 0xa1ab4f24 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "90068-8.bin", 0x000000, 0x100000, 0x9d3204b2 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-1.bin", 0x000000, 0x080000, 0xe7af69d2 )	/* all banked */

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "90068-2.bin", 0x000000, 0x080000, 0x3a583184 )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, 0x98ed1c97 )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0100, 0x0100, 0xcfdbb86c )	/* unknown */
ROM_END

ROM_START( tdragon )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code -bitswapped- */
	ROM_LOAD16_BYTE( "thund.8",  0x00000, 0x20000, 0xedd02831 )
	ROM_LOAD16_BYTE( "thund.7",  0x00001, 0x20000, 0x52192fe5 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "thund.6",		0x000000, 0x20000, 0xfe365920 )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "thund.5",		0x000000, 0x100000, 0xd0bde826 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "thund.4",	0x000000, 0x100000, 0x3eedc2fe )	/* Sprites */

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "thund.1",      0x00000, 0x10000, 0xbf493d74 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples? */
	ROM_LOAD( "thund.2",     0x00000, 0x80000, 0xecfea43e )
	ROM_LOAD( "thund.3",     0x80000, 0x80000, 0xae6875a8 )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bin",  0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "10.bin", 0x0100, 0x0100, 0xe6ead349 )	/* unknown */
ROM_END

ROM_START( tdragonb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code -bitswapped- */
	ROM_LOAD16_BYTE( "td_04.bin",  0x00000, 0x20000, 0xe8a62d3e )
	ROM_LOAD16_BYTE( "td_03.bin",  0x00001, 0x20000, 0x2fa1aa04 )

	ROM_REGION(0x20000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "td_02.bin",    0x00000, 0x8000, 0x99ee7505 )
	ROM_CONTINUE(             0x010000, 0x08000 )
	ROM_COPY( REGION_CPU2, 0, 0x018000, 0x08000 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "td_08.bin",		0x000000, 0x20000, 0x5144dc69 )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "td_06.bin",		0x000000, 0x80000, 0xc1be8a4d )	/* 16x16 tiles */
	ROM_LOAD( "td_07.bin",		0x080000, 0x80000, 0x2c3e371f )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "td_10.bin",	0x000000, 0x080000, 0xbfd0ec5d )	/* Sprites */
	ROM_LOAD16_BYTE( "td_09.bin",	0x000001, 0x080000, 0xb6e074eb )	/* Sprites */

	ROM_REGION( 0x010000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "td_01.bin",     0x00000, 0x10000, 0xf6f6c4bf )
ROM_END

ROM_START( ssmissin )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "ssm14.165",    0x00001, 0x20000, 0xeda61b74 )
	ROM_LOAD16_BYTE( "ssm15.166",    0x00000, 0x20000, 0xaff15927 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ssm16.172",		0x000000, 0x20000, 0x5cf6eb1f )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ssm17.147",		0x000000, 0x080000, 0xc9c28455 )	/* 16x16 tiles */
	ROM_LOAD( "ssm18.148",		0x080000, 0x080000, 0xebfdaad6 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "ssm20.34",		0x000001, 0x080000, 0xa0c16c4d )	/* 16x16 tiles */
	ROM_LOAD16_BYTE( "ssm19.33",		0x000000, 0x080000, 0xb1943657 )	/* 16x16 tiles */

	ROM_REGION( 0x010000, REGION_CPU2, 0 )		/* Code for Sound CPU */
	ROM_LOAD( "ssm11.188",      0x00000, 0x08000, 0x8be6dce3 )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* OKIM6295 samples? */
	ROM_LOAD( "ssm13.190",     0x00000, 0x20000, 0x618f66f0 )
	ROM_LOAD( "ssm12.189",     0x80000, 0x80000, 0xe8219c83 ) /* banked */

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "ssm-pr2.113",  0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "ssm-pr1.114",  0x0100, 0x0200, 0xed0bd072 )	/* unknown */
ROM_END


ROM_START( strahl )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "strahl-2.82", 0x00000, 0x20000, 0xc9d008ae )
	ROM_LOAD16_BYTE( "strahl-1.83", 0x00001, 0x20000, 0xafc3c4d6 )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "strahl-3.73",  0x000000, 0x10000, 0x2273b33e ) /* Characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "str7b2r0.275", 0x000000, 0x40000, 0x5769e3e1 ) /* Tiles */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "strl3-01.32",  0x000000, 0x80000, 0xd8337f15 ) /* Sprites */
	ROM_LOAD( "strl4-02.57",  0x080000, 0x80000, 0x2a38552b )
	ROM_LOAD( "strl5-03.58",  0x100000, 0x80000, 0xa0e7d210 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "str6b1w1.776", 0x000000, 0x80000, 0xbb1bb155 ) /* Tiles */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "strahl-4.66",    0x00000, 0x10000, 0x60a799c4 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "str8pmw1.540",    0x00000, 0x80000, 0x01d6bb6a )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "str9pew1.639",    0x00000, 0x80000, 0x6bb3eb9f )
ROM_END

ROM_START( strahla )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "rom2", 0x00000, 0x20000, 0xf80a22ef )
	ROM_LOAD16_BYTE( "rom1", 0x00001, 0x20000, 0x802ecbfc )

	ROM_REGION( 0x20000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "strahl-3.73",  0x000000, 0x10000, 0x2273b33e ) /* Characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "str7b2r0.275", 0x000000, 0x40000, 0x5769e3e1 ) /* Tiles */

	ROM_REGION( 0x180000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "strl3-01.32",  0x000000, 0x80000, 0xd8337f15 ) /* Sprites */
	ROM_LOAD( "strl4-02.57",  0x080000, 0x80000, 0x2a38552b )
	ROM_LOAD( "strl5-03.58",  0x100000, 0x80000, 0xa0e7d210 )

	ROM_REGION( 0x80000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "str6b1w1.776", 0x000000, 0x80000, 0xbb1bb155 ) /* Tiles */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "strahl-4.66",    0x00000, 0x10000, 0x60a799c4 )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* Oki sample data */
	ROM_LOAD( "str8pmw1.540",    0x00000, 0x80000, 0x01d6bb6a )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* Oki sample data */
	ROM_LOAD( "str9pew1.639",    0x00000, 0x80000, 0x6bb3eb9f )
ROM_END

ROM_START( hachamf )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "hmf_07.rom",  0x00000, 0x20000, 0x9d847c31 )
	ROM_LOAD16_BYTE( "hmf_06.rom",  0x00001, 0x20000, 0xde6408a0 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* unknown  - sound cpu ?????? */
	ROM_LOAD( "hmf_01.rom",  0x00000, 0x10000, 0x9e6f48fc )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "hmf_05.rom",  0x000000, 0x020000, 0x29fb04a2 )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "hmf_04.rom",  0x000000, 0x080000, 0x05a624e3 )	/* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "hmf_08.rom",  0x000000, 0x100000, 0x7fd0f556 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "hmf_02.rom",  0x000000, 0x080000, 0x3f1e67f2 )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "hmf_03.rom",  0x000000, 0x080000, 0xb25ed93b )
ROM_END

ROM_START( macross )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "921a03",        0x00000, 0x80000, 0x33318d55 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* sound program (unknown CPU) */
	ROM_LOAD( "921a02",      0x00000, 0x10000, 0x77c082c7 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "921a01",      0x000000, 0x020000, 0xbbd8242d )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "921a04",      0x000000, 0x200000, 0x4002e4bb )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "921a07",      0x000000, 0x200000, 0x7d2bf112 )	/* Sprites */

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "921a05",      0x000000, 0x080000, 0xd5a1eddd )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "921a06",      0x000000, 0x080000, 0x89461d0f )

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "921a08",      0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "921a09",      0x0100, 0x0100, 0x633ab1c9 )	/* unknown */
	ROM_LOAD( "921a10",      0x0200, 0x0020, 0x8371e42d )	/* unknown */
ROM_END

ROM_START( gunnail )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "3e.bin",  0x00000, 0x40000, 0x61d985b2 )
	ROM_LOAD16_BYTE( "3o.bin",  0x00001, 0x40000, 0xf114e89c )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Code for (unknown?) CPU */
	ROM_LOAD( "92077_2.bin",      0x00000, 0x10000, 0xcd4e55f8 )

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",    0x000000, 0x020000, 0x3d00a9f4 )	/* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "92077-4.bin", 0x000000, 0x100000, 0xa9ea2804 )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "92077-7.bin", 0x000000, 0x200000, 0xd49169b3 )	/* Sprites */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "92077-6.bin", 0x000000, 0x080000, 0x6d133f0d )	/* all banked */

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "92077-5.bin", 0x000000, 0x080000, 0xfeb83c73 )	/* all banked */

	ROM_REGION( 0x0220, REGION_PROMS, 0 )
	ROM_LOAD( "8.bpr",      0x0000, 0x0100, 0x4299776e )	/* unknown */
	ROM_LOAD( "9.bpr",      0x0100, 0x0100, 0x633ab1c9 )	/* unknown */
	ROM_LOAD( "10.bpr",     0x0200, 0x0020, 0xc60103c8 )	/* unknown */
ROM_END

ROM_START( macross2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mcrs2j.3",      0x00000, 0x80000, 0x36a618fe )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "mcrs2j.2",    0x00000, 0x20000, 0xb4aa8ac7 )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mcrs2j.1",    0x000000, 0x020000, 0xc7417410 )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "bp932an.a04", 0x000000, 0x200000, 0xc4d77ff0 )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "bp932an.a07", 0x000000, 0x200000, 0xaa1b21b9 )	/* Sprites */
	ROM_LOAD16_WORD_SWAP( "bp932an.a08", 0x200000, 0x200000, 0x67eb2901 )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a06", 0x040000, 0x200000, 0xef0ffec0 )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "bp932an.a05", 0x040000, 0x100000, 0xb5335abb )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "mcrs2bpr.9",  0x0000, 0x0100, 0x435653a2 )	/* unknown */
	ROM_LOAD( "mcrs2bpr.10", 0x0100, 0x0100, 0xe6ead349 )	/* unknown */
ROM_END

ROM_START( tdragon2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "6.bin",      0x00000, 0x80000, 0x310d6bca )

	ROM_REGION( 0x30000, REGION_CPU2, 0 )		/* Z80 code */
	ROM_LOAD( "5.bin",    0x00000, 0x20000, 0xb870be61 )
	ROM_RELOAD(              0x10000, 0x20000 )				/* banked */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1.bin",    0x000000, 0x020000, 0xd488aafa )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ww930914.2", 0x000000, 0x200000, 0xf968c65d )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ww930917.7", 0x000000, 0x200000, 0xb98873cb )	/* Sprites */
	ROM_LOAD16_WORD_SWAP( "ww930918.8", 0x200000, 0x200000, 0xbaee84b2 )

	ROM_REGION( 0x240000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930916.4", 0x040000, 0x200000, 0x07c35fe6 )	/* all banked */

	ROM_REGION( 0x240000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ww930915.3", 0x040000, 0x200000, 0x82025bab )	/* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "9.bpr",  0x0000, 0x0100, 0x435653a2 )	/* unknown */
	ROM_LOAD( "10.bpr", 0x0100, 0x0100, 0xe6ead349 )	/* unknown */
ROM_END

ROM_START( sabotenb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "ic76.sb1",  0x00000, 0x40000, 0xb2b0b2cf )
	ROM_LOAD16_BYTE( "ic75.sb2",  0x00001, 0x40000, 0x367e87b7 )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic35.sb3",		0x000000, 0x010000, 0xeb7bc99d )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic32.sb4",		0x000000, 0x200000, 0x24c62205 )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ic100.sb5",	0x000000, 0x200000, 0xb20f166e )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.sb6",    0x040000, 0x100000, 0x288407af )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.sb7",    0x040000, 0x100000, 0x43e33a7e )	/* all banked */
ROM_END

ROM_START( bjtwin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )  /* 68000 code */
	ROM_LOAD16_BYTE( "93087-1.bin",  0x00000, 0x20000, 0x93c84e2d )
	ROM_LOAD16_BYTE( "93087-2.bin",  0x00001, 0x20000, 0x30ff678a )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "93087-3.bin",  0x000000, 0x010000, 0xaa13df7c ) /* 8x8 tiles */

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "93087-4.bin",  0x000000, 0x100000, 0x8a4f26d0 ) /* 16x16 tiles */

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "93087-5.bin", 0x000000, 0x100000, 0xbb06245d ) /* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 ) /* OKIM6295 samples */
	ROM_LOAD( "93087-6.bin",    0x040000, 0x100000, 0x372d46dd ) /* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 ) /* OKIM6295 samples */
	ROM_LOAD( "93087-7.bin",    0x040000, 0x100000, 0x8da67808 ) /* all banked */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "8.bpr",      0x0000, 0x0100, 0x633ab1c9 ) /* unknown */
	ROM_LOAD( "9.bpr",      0x0000, 0x0100, 0x435653a2 ) /* unknown */
ROM_END


ROM_START( nouryoku )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "ic76.1",  0x00000, 0x40000, 0x26075988 )
	ROM_LOAD16_BYTE( "ic75.2",  0x00001, 0x40000, 0x75ab82cd )

	ROM_REGION( 0x010000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ic35.3",		0x000000, 0x010000, 0x03d0c3b1 )	/* 8x8 tiles */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ic32.4",		0x000000, 0x200000, 0x88d454fd )	/* 16x16 tiles */

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD16_WORD_SWAP( "ic100.5",	0x000000, 0x200000, 0x24d3e24e )	/* Sprites */

	ROM_REGION( 0x140000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic30.6",     0x040000, 0x100000, 0xfeea34f4 )	/* all banked */

	ROM_REGION( 0x140000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "ic27.7",     0x040000, 0x100000, 0x8a69fded )	/* all banked */
ROM_END

ROM_START( manybloc )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )		/* 68000 code */
	ROM_LOAD16_BYTE( "1-u33.bin",  0x00001, 0x20000, 0x07473154  )
	ROM_LOAD16_BYTE( "2-u35.bin",  0x00000, 0x20000, 0x04acd8c1  )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80? CPU */
	ROM_LOAD( "3-u146.bin",      0x00000, 0x10000, 0x7bf5fafa  )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "12-u39.bin",    0x000000, 0x10000, 0x413b5438  )	/* 8x8 tiles */

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE ) /* 16x16 sprite tiles */
	ROM_LOAD16_BYTE( "8-u54b.bin",  0x000000, 0x20000, 0x03eede77 )
	ROM_LOAD16_BYTE( "10-u86b.bin", 0x000001, 0x20000, 0x9eab216f )
	ROM_LOAD16_BYTE( "9-u53b.bin",  0x040000, 0x20000, 0xdfcfa040 )
	ROM_LOAD16_BYTE( "11-u85b.bin", 0x040001, 0x20000, 0xfe747dd5 )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "4-u96.bin", 0x000000, 0x40000, 0x28af2640 )
	ROM_LOAD( "5-u97.bin", 0x040000, 0x40000, 0x536699e6 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "6-u131.bin", 0x000000, 0x40000, 0x79a4ae75   )

	ROM_REGION( 0x40000, REGION_SOUND2, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "7-u132.bin", 0x000000, 0x40000, 0x21db875e   )

	ROM_REGION( 0x20220, REGION_PROMS, 0 )
	ROM_LOAD( "u7.bpr",      0x0000, 0x0100, 0xcfdbb86c )	/* unknown */
	ROM_LOAD( "u120.bpr",      0x0100, 0x0100, 0x576c5984  )	/* unknown */
	ROM_LOAD( "u200.bpr",      0x0200, 0x0020, 0x1823600b  )	/* unknown */
	ROM_LOAD( "u10.bpr",      0x0100, 0x0200, 0x8e9b569a   )	/* unknown */
ROM_END

static unsigned char decode_byte(unsigned char src, unsigned char *bitp)
{
	unsigned char ret, i;

	ret = 0;
	for (i=0; i<8; i++)
		ret |= (((src >> bitp[i]) & 1) << (7-i));

	return ret;
}

static unsigned long bjtwin_address_map_bg0(unsigned long addr)
{
   return ((addr&0x00004)>> 2) | ((addr&0x00800)>> 10) | ((addr&0x40000)>>16);
}


static unsigned short decode_word(unsigned short src, unsigned char *bitp)
{
	unsigned short ret, i;

	ret=0;
	for (i=0; i<16; i++)
		ret |= (((src >> bitp[i]) & 1) << (15-i));

	return ret;
}


static unsigned long bjtwin_address_map_sprites(unsigned long addr)
{
   return ((addr&0x00010)>> 4) | ((addr&0x20000)>>16) | ((addr&0x100000)>>18);
}


static void decode_gfx(void)
{
	/* GFX are scrambled.  We decode them here.  (BIG Thanks to Antiriad for descrambling info) */
	unsigned char *rom;
	int A;

	static unsigned char decode_data_bg[8][8] =
	{
		{0x3,0x0,0x7,0x2,0x5,0x1,0x4,0x6},
		{0x1,0x2,0x6,0x5,0x4,0x0,0x3,0x7},
		{0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0x7,0x6,0x5,0x0,0x1,0x4,0x3,0x2},
		{0x2,0x0,0x1,0x4,0x3,0x5,0x7,0x6},
		{0x5,0x3,0x7,0x0,0x4,0x6,0x2,0x1},
		{0x2,0x7,0x0,0x6,0x5,0x3,0x1,0x4},
		{0x3,0x4,0x7,0x6,0x2,0x0,0x5,0x1},
	};

	static unsigned char decode_data_sprite[8][16] =
	{
		{0x9,0x3,0x4,0x5,0x7,0x1,0xb,0x8,0x0,0xd,0x2,0xc,0xe,0x6,0xf,0xa},
		{0x1,0x3,0xc,0x4,0x0,0xf,0xb,0xa,0x8,0x5,0xe,0x6,0xd,0x2,0x7,0x9},
		{0xf,0xe,0xd,0xc,0xb,0xa,0x9,0x8,0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0},
		{0xf,0xe,0xc,0x6,0xa,0xb,0x7,0x8,0x9,0x2,0x3,0x4,0x5,0xd,0x1,0x0},

		{0x1,0x6,0x2,0x5,0xf,0x7,0xb,0x9,0xa,0x3,0xd,0xe,0xc,0x4,0x0,0x8}, /* Haze 20/07/00 */
		{0x7,0x5,0xd,0xe,0xb,0xa,0x0,0x1,0x9,0x6,0xc,0x2,0x3,0x4,0x8,0xf}, /* Haze 20/07/00 */
		{0x0,0x5,0x6,0x3,0x9,0xb,0xa,0x7,0x1,0xd,0x2,0xe,0x4,0xc,0x8,0xf}, /* Antiriad, Corrected by Haze 20/07/00 */
		{0x9,0xc,0x4,0x2,0xf,0x0,0xb,0x8,0xa,0xd,0x3,0x6,0x5,0xe,0x1,0x7}, /* Antiriad, Corrected by Haze 20/07/00 */
	};


	/* background */
	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_bg[bjtwin_address_map_bg0(A)]);
	}

	/* sprites */
	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A += 2)
	{
		unsigned short tmp = decode_word( rom[A+1]*256 + rom[A], decode_data_sprite[bjtwin_address_map_sprites(A)]);
		rom[A+1] = tmp >> 8;
		rom[A] = tmp & 0xff;
	}
}

static void decode_tdragonb(void)
{
	/* Descrambling Info Again Taken from Raine, Huge Thanks to Antiriad and the Raine Team for
	   going Open Source, best of luck in future development. */

	unsigned char *rom;
	int A;

	/* The Main 68k Program of the Bootleg is Bitswapped */
	static unsigned char decode_data_tdragonb[1][16] =
	{
		{0xe,0xc,0xa,0x8,0x7,0x5,0x3,0x1,0xf,0xd,0xb,0x9,0x6,0x4,0x2,0x0},
	};

	/* Graphic Roms Could Also Do With Rearranging to make things simpler */
	static unsigned char decode_data_tdragonbgfx[1][8] =
	{
		{0x7,0x6,0x5,0x3,0x4,0x2,0x1,0x0},
	};

	rom = memory_region(REGION_CPU1);
	for (A = 0;A < memory_region_length(REGION_CPU1);A += 2)
	{
#ifdef LSB_FIRST
		unsigned short tmp = decode_word( rom[A+1]*256 + rom[A], decode_data_tdragonb[0]);
		rom[A+1] = tmp >> 8;
		rom[A] = tmp & 0xff;
#else
		unsigned short tmp = decode_word( rom[A]*256 + rom[A+1], decode_data_tdragonb[0]);
		rom[A] = tmp >> 8;
		rom[A+1] = tmp & 0xff;
#endif
	}

	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}

	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}
}

static void decode_ssmissin(void)
{
	/* Like Thunder Dragon Bootleg without the Program Rom Swapping */
	unsigned char *rom;
	int A;

	/* Graphic Roms Could Also Do With Rearranging to make things simpler */
	static unsigned char decode_data_tdragonbgfx[1][8] =
	{
		{0x7,0x6,0x5,0x3,0x4,0x2,0x1,0x0},
	};

	rom = memory_region(REGION_GFX2);
	for (A = 0;A < memory_region_length(REGION_GFX2);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}

	rom = memory_region(REGION_GFX3);
	for (A = 0;A < memory_region_length(REGION_GFX3);A++)
	{
		rom[A] = decode_byte( rom[A], decode_data_tdragonbgfx[0]);
	}
}


static DRIVER_INIT( nmk )
{
	decode_gfx();
}

static DRIVER_INIT( hachamf )
{
	data16_t *rom = (data16_t *)memory_region(REGION_CPU1);

	rom[0x0006/2] = 0x7dc2;	/* replace reset vector with the "real" one */
}

static DRIVER_INIT( acrobatm )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	RAM[0x724/2] = 0x4e71; /* Protection */
	RAM[0x726/2] = 0x4e71;
	RAM[0x728/2] = 0x4e71;

	RAM[0x9d8/2] = 0x3e3C; /* Checksum */
	RAM[0x9da/2] = 0x0;
	RAM[0x97c/2] = 0x3e3C;
	RAM[0x97e/2] = 0x0;
}

static DRIVER_INIT( tdragonb )
{
	data16_t *ROM = (data16_t *)memory_region(REGION_CPU1);

	decode_tdragonb();

	/* The Following Patch is taken from Raine, Otherwise the game has no Sprites in Attract Mode or After Level 1
	   which is rather odd considering its a bootleg.. */
	ROM[0x00308/2] = 0x4e71; /* Sprite Problem */
}

static DRIVER_INIT( tdragon )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	RAM[0x94b0/2] = 0; /* Patch out JMP to shared memory (protection) */
	RAM[0x94b2/2] = 0x92f4;
}

static DRIVER_INIT( ssmissin )
{
	decode_ssmissin();
}


static DRIVER_INIT( strahl )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	RAM[0x79e/2] = 0x4e71; /* Protection */
	RAM[0x7a0/2] = 0x4e71;
	RAM[0x7a2/2] = 0x4e71;

	RAM[0x968/2] = 0x4e71; /* Checksum error */
	RAM[0x96a/2] = 0x4e71;
	RAM[0x8e0/2] = 0x4e71; /* Checksum error */
	RAM[0x8e2/2] = 0x4e71;
}

static DRIVER_INIT( bioship )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	RAM[0xe78a/2] = 0x4e71; /* Protection */
	RAM[0xe78c/2] = 0x4e71;

	RAM[0xe798/2] = 0x4e71; /* Checksum */
	RAM[0xe79a/2] = 0x4e71;
}


int is_blkheart;

// see raine's games/nmk.c

static WRITE16_HANDLER ( test_2a_w )
{
	data = data >> 8;

	ram[0x2a/2] = (data << 8) | data;

	if (data == 1 || data == 2) {
		ram[0x68/2] = 11;
		ram[0x6a/2] = 0;
	}
}

static WRITE16_HANDLER ( test_2a_mustang_w )
{
	data = data >> 8;

	ram[0x2a/2] = (data << 8) | data;

	if (data == 1 || data == 2) {
		ram[0x2e/2] = 10;
		ram[0x2c/2] = 0;
	}

}

static DRIVER_INIT( blkheart )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	is_blkheart = 1; // sprite enable is different?

   // see raine's games/nmk.c
	RAM[0x872/2] = 0x4e71;
   	RAM[0x874/2] = 0x4e71;
   	RAM[0x876/2] = 0x4e71;
   	RAM[0x8d6/2] = 0x0300;
   	RAM[0xe1c/2] = 0x0300;
   	RAM[0x23dc/2] = 0x0300;
   	RAM[0x3dea/2] = 0x0300;

	install_mem_write16_handler(0, 0xf902a, 0xf902b, test_2a_w );
}

static DRIVER_INIT( mustang )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);

	is_blkheart = 1; // sprite enable is different?

   // see raine's games/nmk.c
	RAM[0x85c/2] = 0x4e71;
	RAM[0x85e/2] = 0x4e71;
	RAM[0x860/2] = 0x4e71;
	RAM[0x8c0/2] = 0x0300;
  	RAM[0xc00/2] = 0x0300;
  	RAM[0x30b2/2] = 0x0300;

	install_mem_write16_handler(0, 0xf902a, 0xf902b, test_2a_mustang_w );
}

static DRIVER_INIT( bjtwin )
{
	init_nmk();

	/* Patch rom to enable test mode */

/*	008F54: 33F9 0008 0000 000F FFFC move.w  $80000.l, $ffffc.l
 *	008F5E: 3639 0008 0002           move.w  $80002.l, D3
 *	008F64: 3003                     move.w  D3, D0				\
 *	008F66: 3203                     move.w  D3, D1				|	This code remaps
 *	008F68: 0041 BFBF                ori.w   #-$4041, D1		|   buttons 2 and 3 to
 *	008F6C: E441                     asr.w   #2, D1				|   button 1, so
 *	008F6E: 0040 DFDF                ori.w   #-$2021, D0		|   you can't enter
 *	008F72: E240                     asr.w   #1, D0				|   service mode7
 *	008F74: C640                     and.w   D0, D3				|
 *	008F76: C641                     and.w   D1, D3				/
 *	008F78: 33C3 000F FFFE           move.w  D3, $ffffe.l
 *	008F7E: 207C 000F 9000           movea.l #$f9000, A0
 */

//	data 16_t *rom = (data16_t *)memory_region(REGION_CPU1);
//	rom[0x09172/2] = 0x6006;	/* patch checksum error */
//	rom[0x08f74/2] = 0x4e71);
}



GAMEX( 1989, urashima, 0,       urashima, macross,  0,        ROT0,   "UPL",							"Urashima Mahjong", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING ) /* Similar Hardware? */
GAMEX( 1989, tharrier, 0,       tharrier, tharrier, 0,        ROT270, "UPL (American Sammy license)",	"Task Force Harrier", GAME_UNEMULATED_PROTECTION )
GAMEX( 1990, mustang,  0,       mustang,  mustang,  mustang,  ROT0,   "UPL",							"US AAF Mustang (Japan)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND) // Playable but there are Still Protection Problems
GAMEX( 1990, mustangs, mustang, mustang,  mustang,  mustang,  ROT0,   "UPL (Seoul Trading license)",	"US AAF Mustang (Seoul Trading)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND ) // Playable but there are Still Protection Problems
GAMEX( 1990, mustangb, mustang, mustangb, mustang,  mustang,  ROT0,   "bootleg",						"US AAF Mustang (bootleg)", GAME_UNEMULATED_PROTECTION ) // Playable but there are Still Protection Problems
GAMEX( 1990, bioship,  0,       bioship,  bioship,  bioship,  ROT0,   "UPL (American Sammy license)",	"Bio-ship Paladin", GAME_NO_SOUND )
GAMEX( 1990, vandyke,  0,       vandyke,  vandyke,  0,        ROT270, "UPL",							"Vandyke (Japan)",  GAME_NO_SOUND )
GAMEX( 1991, manybloc, 0,       manybloc, manybloc, 0,        ROT270, "Bee-Oh",                         "Many Block", GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
GAMEX( 1991, blkheart, 0,       macross,  blkheart, blkheart, ROT0,   "UPL",							"Black Heart", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND  ) // Playable but there are Still Protection Problems
GAMEX( 1991, blkhearj, blkheart,macross,  blkheart, blkheart, ROT0,   "UPL",							"Black Heart (Japan)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND ) // Playable but there are Still Protection Problems
GAMEX( 1991, acrobatm, 0,       acrobatm, acrobatm, acrobatm, ROT270, "UPL (Taito license)",			"Acrobat Mission", GAME_NO_SOUND )
GAMEX( 1992, strahl,   0,       strahl,   strahl,   strahl,   ROT0,   "UPL",							"Koutetsu Yousai Strahl (Japan set 1)", GAME_NO_SOUND )
GAMEX( 1992, strahla,  strahl,  strahl,   strahl,   strahl,   ROT0,   "UPL",							"Koutetsu Yousai Strahl (Japan set 2)", GAME_NO_SOUND )
GAMEX( 1991, tdragon,  0,       tdragon,  tdragon,  tdragon,  ROT270, "NMK / Tecmo",					"Thunder Dragon", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING | GAME_NO_SOUND )
GAME(  1991, tdragonb, tdragon, tdragonb, tdragon,  tdragonb, ROT270, "NMK / Tecmo",					"Thunder Dragon (Bootleg)" )
GAMEX( 1992, ssmissin, 0,       ssmissin, ssmissin, ssmissin, ROT270, "Comad",				            "S.S. Mission", GAME_NO_COCKTAIL )
GAMEX( 1991, hachamf,  0,       hachamf,  hachamf,  hachamf,  ROT0,   "NMK",							"Hacha Mecha Fighter", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1992, macross,  0,       macross,  macross,  nmk,      ROT270, "Banpresto",						"Super Spacefortress Macross / Chou-Jikuu Yousai Macross", GAME_NO_SOUND )
GAMEX( 1993, gunnail,  0,       gunnail,  gunnail,  nmk,      ROT270, "NMK / Tecmo",					"GunNail", GAME_NO_SOUND )
GAMEX( 1993, macross2, 0,       macross2, macross2, 0,        ROT0,   "Banpresto",						"Super Spacefortress Macross II / Chou-Jikuu Yousai Macross II", GAME_NO_COCKTAIL )
GAMEX( 1993, tdragon2, 0,       macross2, tdragon2, 0,        ROT270, "NMK",				         	"Thunder Dragon 2", GAME_NO_COCKTAIL )
GAMEX( 1992, sabotenb, 0,       bjtwin,   sabotenb, nmk,      ROT0,   "NMK / Tecmo",					"Saboten Bombers", GAME_NO_COCKTAIL )
GAMEX( 1993, bjtwin,   0,       bjtwin,   bjtwin,   bjtwin,   ROT270, "NMK",							"Bombjack Twin", GAME_NO_COCKTAIL )
GAMEX( 1995, nouryoku, 0,       bjtwin,   nouryoku, nmk,      ROT0,   "Tecmo",							"Nouryoku Koujou Iinkai", GAME_NO_COCKTAIL )
