/***************************************************************************

  Atari VCS 2600 driver

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"
#include "cpu/m6502/m6502.h"
#include "sound/tiaintf.h"
#include "devices/cartslot.h"
#include "inputx.h"
#include "zlib.h"

extern PALETTE_INIT( tia_NTSC );
extern PALETTE_INIT( tia_PAL );
extern VIDEO_START( tia );
extern VIDEO_UPDATE( tia );
extern READ_HANDLER( tia_r );
extern WRITE_HANDLER( tia_w );

extern void tia_init(void);

#define CART memory_region(REGION_USER1)


enum
{
	mode2K,
	mode4K,
	mode8K,
	mode12,
	mode16,
	mode32,
	modeAV,
	modePB,
	modeTV,
	modeUA,
	modeMN,
	modeDC,
	modeCV
};


static char* extra_RAM;

static unsigned cart_size;
static unsigned current_bank;


static const UINT32 games[][2] =
{
	{ 0x91b8f1b2, modeAV }, // Decathlon
	{ 0x2452adab, modeAV }, // Decathlon PAL
	{ 0xe127c012, modeAV }, // Robot Tank
	{ 0xcded5569, modeAV }, // Robot Tank PAL
	{ 0xdd210cf3, modeAV }, // Space Shuttle
	{ 0x600e7c77, modeAV }, // Space Shuttle PAL
	{ 0xb60ab310, modeAV }, // Thwocker Prototype
	{ 0x3ba0d9bf, modePB }, // Frogger II
	{ 0x09cdd3ea, modePB }, // Frogger II PAL
	{ 0x0d78e8a9, modePB }, // Gyruss
	{ 0xba14b37b, modePB }, // Gyruss PAL
	{ 0x34d3ffc8, modePB }, // James Bond 007
	{ 0x59b96db3, modePB }, // Lord of the Rings Prototype
	{ 0xe680a1c9, modePB }, // Montezuma's Revenge
	{ 0x044735b9, modePB }, // Mr Do's Castle
	{ 0x7d287f20, modePB }, // Popeye
	{ 0x742ac749, modePB }, // Popeye PAL
	{ 0xa87be8fd, modePB }, // Q-bert's Qubes
	{ 0xd9f499c5, modePB }, // Q-bert's Qubes
	{ 0xde97103d, modePB }, // Super Cobra
	{ 0x380d78b3, modePB }, // Super Cobra PAL
	{ 0x0886a55d, modePB }, // Star Wars Death Star Battle
	{ 0x2a2bd248, modePB }, // Star Wars Death Star Battle PAL
	{ 0xd11E05fe, modePB }, // Star Wars Ewok Adventure Prototype PAL
	{ 0x65c31ca4, modePB }, // Star Wars Arcade Game
	{ 0x273bda48, modePB }, // Star Wars Arcade Game PAL
	{ 0x47efd61d, modePB }, // Star Wars Arcade Game Prototype
	{ 0xfd8c81e5, modePB }, // Tooth Protectors
	{ 0xec959bf2, modePB }, // Tutankham
	{ 0x8fbe2b84, modePB }, // Tutankham PAL
	{ 0x34b80a97, modeTV }, // Espial
	{ 0xbd08d915, modeTV }, // Miner 2049er
	{ 0xbfa477cd, modeTV }, // Miner 2049er Volume II
	{ 0xdb376663, modeTV }, // Polaris
	{ 0x25b7f8f9, modeTV }, // Polaris
	{ 0x71ecefaf, modeTV }, // Polaris
	{ 0xc820bd75, modeTV }, // River Patrol
	{ 0xdd183a4f, modeTV }, // Springer
	{ 0xb53b33f1, modeUA }, // Funky Fish Prototype
	{ 0x35589cec, modeUA }, // Pleiads Prototype
	{ 0xdf2bc303, modeMN }, // Bump 'n' Jump
	{ 0xc18E0bbc, modeMN }, // Burgertime
	{ 0x0603e177, modeMN }, // Masters of the Universe
	{ 0x14f126c0, modeCV }, // Magicard
	{ 0x34b0b5c2, modeCV }  // Video Life
};


static int detect_super_chip(void)
{
	int i;

	for (i = 0x1000; i < cart_size; i += 0x1000)
	{
		if (memcmp(CART, CART + i, 0x100))
		{
			return 0;
		}
	}

	return 1;
}


static DEVICE_LOAD( a2600_cart )
{
	cart_size = mame_fsize(file);

	switch (cart_size)
	{
	case 0x00800:
	case 0x01000:
	case 0x02000:
	case 0x03000:
	case 0x04000:
	case 0x08000:
	case 0x10000:
		break;

	default:
		return 1; /* unsupported image format */
	}

	current_bank = 0;

	mame_fread(file, CART, cart_size);

	return 0;
}


static int next_bank(void)
{
	return current_bank = (current_bank + 1) % 16;
}


void mode8K_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x1000 * offset);
}
void mode12_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x1000 * offset);
}
void mode16_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x1000 * offset);
}
void mode32_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x1000 * offset);
}
void modeTV_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x800 * (data & 3));
}
void modeUA_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + (offset >> 6) * 0x1000);
}
void modePB_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1 + (offset >> 3), CART + 0x400 * (offset & 7));
}
void modeMN_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x800 * offset);
}
void modeDC_switch(UINT16 offset, UINT8 data)
{
	cpu_setbank(1, CART + 0x1000 * next_bank());
}


static READ_HANDLER(mode8K_switch_r) { mode8K_switch(offset, 0); return 0; }
static READ_HANDLER(mode12_switch_r) { mode12_switch(offset, 0); return 0; }
static READ_HANDLER(mode16_switch_r) { mode16_switch(offset, 0); return 0; }
static READ_HANDLER(mode32_switch_r) { mode32_switch(offset, 0); return 0; }
static READ_HANDLER(modePB_switch_r) { modePB_switch(offset, 0); return 0; }
static READ_HANDLER(modeMN_switch_r) { modeMN_switch(offset, 0); return 0; }
static READ_HANDLER(modeTV_switch_r) { modeTV_switch(offset, 0); return 0; }
static READ_HANDLER(modeUA_switch_r) { modeUA_switch(offset, 0); return 0; }
static READ_HANDLER(modeDC_switch_r) { modeDC_switch(offset, 0); return 0; }


static WRITE_HANDLER(mode8K_switch_w) { mode8K_switch(offset, data); }
static WRITE_HANDLER(mode12_switch_w) { mode12_switch(offset, data); }
static WRITE_HANDLER(mode16_switch_w) { mode16_switch(offset, data); }
static WRITE_HANDLER(mode32_switch_w) { mode32_switch(offset, data); }
static WRITE_HANDLER(modePB_switch_w) {	modePB_switch(offset, data); }
static WRITE_HANDLER(modeMN_switch_w) {	modeMN_switch(offset, data); }
static WRITE_HANDLER(modeTV_switch_w) { modeTV_switch(offset, data); }
static WRITE_HANDLER(modeUA_switch_w) { modeUA_switch(offset, data); }
static WRITE_HANDLER(modeDC_switch_w) { modeDC_switch(offset, data); }


static READ_HANDLER(current_bank_r)
{
	return current_bank;
}


static ADDRESS_MAP_START(a2600_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_FLAGS( AMEF_ABITS(13) )
	AM_RANGE(0x0000, 0x007F) AM_MIRROR(0x0100) AM_READWRITE(tia_r, tia_w)
	AM_RANGE(0x0080, 0x00FF) AM_MIRROR(0x0100) AM_RAM
	AM_RANGE(0x0280, 0x029F)                   AM_READWRITE(r6532_0_r, r6532_0_w)
	AM_RANGE(0x1000, 0x1FFF)                   AM_ROMBANK(1)
ADDRESS_MAP_END


static READ_HANDLER( switch_A_r )
{
	UINT8 val = 0;

	int control0 = readinputport(9) % 16;
	int control1 = readinputport(9) / 16;

	val |= readinputport(6 + control0) & 0x0F;
	val |= readinputport(6 + control1) & 0xF0;

	return val;
}


static struct R6532interface r6532_interface =
{
	switch_A_r,
	input_port_8_r,
	NULL,
	NULL
};


static struct TIAinterface tia_interface =
{
	31400, 255, TIA_DEFAULT_GAIN
};


static void install_banks(int count, unsigned init)
{
	int i;

	for (i = 0; i < count; i++)
	{
		static const read8_handler handler[] =
		{
			MRA8_BANK1,
			MRA8_BANK2,
			MRA8_BANK3,
			MRA8_BANK4,
		};

		install_mem_read_handler(0,
			0x1000 + (i + 0) * 0x1000 / count - 0,
			0x1000 + (i + 1) * 0x1000 / count - 1, handler[i]);

		cpu_setbank(i + 1, memory_region(REGION_USER1) + init);
	}
}


static MACHINE_INIT( a2600 )
{
	int mode = readinputport(10);
	int chip = readinputport(11);

	r6532_init(0, &r6532_interface);

	tia_init();

	install_mem_write_handler(0, 0x00, 0x7f, tia_w);
	install_mem_read_handler(0, 0x00, 0x7f, tia_r);

	install_mem_write_handler(0, 0x200, 0x27f, MWA8_NOP);
	install_mem_read_handler(0, 0x200, 0x27f, MRA8_NOP);

	install_mem_write_handler(0, 0x1000, 0x1fff, MWA8_NOP);
	install_mem_read_handler(0, 0x1000, 0x1fff, MRA8_NOP);

	/* auto-detect bank mode */

	if (mode == 0xff)
	{
		UINT32 crc = crc32(0, CART, cart_size);

		int i;

		switch (cart_size)
		{
		case 0x800:
			mode = mode2K;
			break;
		case 0x1000:
			mode = mode4K;
			break;
		case 0x2000:
			mode = mode8K;
			break;
		case 0x3000:
			mode = mode12;
			break;
		case 0x4000:
			mode = mode16;
			break;
		case 0x8000:
			mode = mode32;
			break;
		case 0x10000:
			mode = modeDC;
			break;
		}

		for (i = 0; 8 * i < sizeof games; i++)
		{
			if (games[i][0] == crc)
			{
				mode = games[i][1];
			}
		}
	}

	/* auto-detect super chip */

	if (chip == 0xff)
	{
		UINT32 crc = crc32(0, CART, cart_size);

		chip = 0;

		if (cart_size == 0x2000 || cart_size == 0x4000 || cart_size == 0x8000)
		{
			chip = detect_super_chip();
		}

		if (crc == 0xee7b80d1) chip = 1; // Dig Dug
		if (crc == 0xa09779ea) chip = 1; // Off the Wall
	}

	/* set up ROM banks */

	switch (mode)
	{
	case mode2K:
		install_banks(2, 0x0000);
		break;

	case mode4K:
		install_banks(1, 0x0000);
		break;

	case mode8K:
		install_banks(1, 0x1000);
		break;

	case mode12:
		install_banks(1, 0x2000);
		break;

	case mode16:
		install_banks(1, 0x3000);
		break;

	case mode32:
		install_banks(1, 0x7000);
		break;

	case modeAV:
		install_banks(1, 0x0000);
		break;

	case modePB:
		install_banks(4, 0x1c00);
		break;

	case modeTV:
		install_banks(2, 0x1800);
		break;

	case modeUA:
		install_banks(1, 0x1000);
		break;

	case modeMN:
		install_banks(2, 0x3800);
		break;

	case modeDC:
		install_banks(1, 0x1000 * current_bank);
		break;

	case modeCV:
		install_banks(2, 0x0000);
		break;
	}

	/* set up bank counter */

	if (mode == modeDC)
	{
		install_mem_read_handler(0, 0x1fec, 0x1fec, current_bank_r);
	}

	/* set up bank switch registers */

	switch (mode)
	{
	case mode8K:
		install_mem_write_handler(0, 0x1ff8, 0x1ff9, mode8K_switch_w);
		install_mem_read_handler(0, 0x1ff8, 0x1ff9, mode8K_switch_r);
		break;

	case mode12:
		install_mem_write_handler(0, 0x1ff8, 0x1ffa, mode12_switch_w);
		install_mem_read_handler(0, 0x1ff8, 0x1ffa, mode12_switch_r);
		break;

	case mode16:
		install_mem_write_handler(0, 0x1ff6, 0x1ff9, mode16_switch_w);
		install_mem_read_handler(0, 0x1ff6, 0x1ff9, mode16_switch_r);
		break;

	case mode32:
		install_mem_write_handler(0, 0x1ff4, 0x1ffb, mode32_switch_w);
		install_mem_read_handler(0, 0x1ff4, 0x1ffb, mode32_switch_r);
		break;

	case modePB:
		install_mem_write_handler(0, 0x1fe0, 0x1ff8, modePB_switch_w);
		install_mem_read_handler(0, 0x1fe0, 0x1ff8, modePB_switch_r);
		break;

	case modeTV:
		install_mem_write_handler(0, 0x00, 0x3f, modeTV_switch_w);
		install_mem_read_handler(0, 0x00, 0x3f, modeTV_switch_r);
		break;

	case modeUA:
		install_mem_write_handler(0, 0x200, 0x27f, modeUA_switch_w);
		install_mem_read_handler(0, 0x200, 0x27f, modeUA_switch_r);
		break;

	case modeMN:
		install_mem_write_handler(0, 0x1fe0, 0x1fe7, modeMN_switch_w);
		install_mem_read_handler(0, 0x1fe0, 0x1fe7, modeMN_switch_r);
		break;

	case modeDC:
		install_mem_write_handler(0, 0x1ff0, 0x1ff0, modeDC_switch_w);
		install_mem_read_handler(0, 0x1ff0, 0x1ff0, modeDC_switch_r);
		break;
	}

	/* set up extra RAM */

	if (mode == mode12)
	{
		install_mem_write_handler(0, 0x1000, 0x10ff, MWA8_BANK9);
		install_mem_read_handler(0, 0x1100, 0x11ff, MRA8_BANK9);

		cpu_setbank(9, extra_RAM);
	}

	if (mode == modeCV)
	{
		install_mem_write_handler(0, 0x1400, 0x17ff, MWA8_BANK9);
		install_mem_read_handler(0, 0x1000, 0x13ff, MRA8_BANK9);

		cpu_setbank(9, extra_RAM);
	}

	if (chip)
	{
		install_mem_write_handler(0, 0x1000, 0x107f, MWA8_BANK9);
		install_mem_read_handler(0, 0x1080, 0x10ff, MRA8_BANK9);

		cpu_setbank(9, extra_RAM);
	}
}


INPUT_PORTS_START( a2600 )

	PORT_START /* [0] */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER1 | IPF_REVERSE, 40, 10, 0, 255 ) PORT_CATEGORY(2)
	PORT_START /* [1] */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER2 | IPF_REVERSE, 40, 10, 0, 255 ) PORT_CATEGORY(2)
	PORT_START /* [2] */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER3 | IPF_REVERSE, 40, 10, 0, 255 ) PORT_CATEGORY(4)
	PORT_START /* [3] */
	PORT_ANALOG( 0xff, 0x80, IPT_PADDLE | IPF_PLAYER4 | IPF_REVERSE, 40, 10, 0, 255 ) PORT_CATEGORY(4)

	PORT_START /* [4] */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) PORT_CATEGORY(1)

	PORT_START /* [5] */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) PORT_CATEGORY(3)

	PORT_START /* [6] SWCHA joystick */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 ) PORT_CATEGORY(3)
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) PORT_CATEGORY(3)
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 ) PORT_CATEGORY(3)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) PORT_CATEGORY(3)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) PORT_CATEGORY(1)
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) PORT_CATEGORY(1)
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) PORT_CATEGORY(1)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) PORT_CATEGORY(1)

	PORT_START /* [7] SWCHA paddles */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 ) PORT_CATEGORY(4)
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 ) PORT_CATEGORY(4)
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) PORT_CATEGORY(2)
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 ) PORT_CATEGORY(2)

	PORT_START /* [8] SWCHB */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, 0, "Reset Game", KEYCODE_ENTER, IP_JOY_DEFAULT )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "Select Game", KEYCODE_BACKSPACE, IP_JOY_DEFAULT )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "TV Type" )
	PORT_DIPSETTING(    0x08, "Color" )
	PORT_DIPSETTING(    0x00, "B&W" )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, "Left Diff. Switch" )
	PORT_DIPSETTING(    0x40, "A" )
	PORT_DIPSETTING(    0x00, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Right Diff. Switch" )
	PORT_DIPSETTING(    0x80, "A" )
	PORT_DIPSETTING(    0x00, "B" )

	PORT_START /* [9] */
	PORT_CATEGORY_CLASS( 0xf0, 0x00, "Left Controller" )
	PORT_CATEGORY_ITEM(    0x00, "Joystick", 1 )
	PORT_CATEGORY_ITEM(    0x10, "Paddles", 2 )
	PORT_CATEGORY_CLASS( 0x0f, 0x00, "Right Controller" )
	PORT_CATEGORY_ITEM(    0x00, "Joystick", 3 )
	PORT_CATEGORY_ITEM(    0x01, "Paddles", 4 )

	PORT_START /* [10] */
	PORT_CONFNAME( 0xff, 0xff, "Bank Mode" )
	PORT_CONFSETTING(    0xff, "Auto" )
	PORT_CONFSETTING(    mode2K, "2K" )
	PORT_CONFSETTING(    mode4K, "4K" )
	PORT_CONFSETTING(    mode8K, "8K" )
	PORT_CONFSETTING(    mode12, "12K" )
	PORT_CONFSETTING(    mode16, "16K" )
	PORT_CONFSETTING(    mode32, "32K" )
	PORT_CONFSETTING(    modeAV, "Activision" )
	PORT_CONFSETTING(    modeCV, "CommaVid" )
	PORT_CONFSETTING(    modeDC, "Dynacom" )
	PORT_CONFSETTING(    modeMN, "M Network" )
	PORT_CONFSETTING(    modePB, "Parker Brothers" )
	PORT_CONFSETTING(    modeTV, "Tigervision" )
	PORT_CONFSETTING(    modeUA, "UA Limited" )

	PORT_START /* [11] */
	PORT_CONFNAME( 0xff, 0xff, "Super Chip" )
	PORT_CONFSETTING(    0xff, "Auto" )
	PORT_CONFSETTING(    0x00, DEF_STR( Off ))
	PORT_CONFSETTING(    0x01, DEF_STR( On ))
INPUT_PORTS_END


static DRIVER_INIT( a2600 )
{
	extra_RAM = auto_malloc(0x400);
}


static MACHINE_DRIVER_START( a2600 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 3584160 / 3)	/* actually M6507 */
	MDRV_CPU_PROGRAM_MAP(a2600_mem, 0)

	MDRV_FRAMES_PER_SECOND(60)

	MDRV_MACHINE_INIT(a2600)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(160, 262)
	MDRV_VISIBLE_AREA(0, 159, 36, 261)
	MDRV_PALETTE_LENGTH(128)
	MDRV_PALETTE_INIT(tia_NTSC)

	MDRV_VIDEO_START(tia)
	MDRV_VIDEO_UPDATE(tia)

	/* sound hardware */
	MDRV_SOUND_ADD(TIA, tia_interface)
MACHINE_DRIVER_END


ROM_START( a2600 )
	ROM_REGION( 0x2000, REGION_CPU1, 0 )
	ROM_REGION( 0x10000, REGION_USER1, 0 )
ROM_END


SYSTEM_CONFIG_START(a2600)
	CONFIG_DEVICE_CARTSLOT_REQ( 1, "bin\0a26\0", NULL, NULL, device_load_a2600_cart, NULL, NULL, NULL )
SYSTEM_CONFIG_END


/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1977,	a2600,	0,		0,		a2600,	a2600,	a2600,	a2600,	"Atari",	"Atari 2600 (NTSC)" )
