/****************************************************************************

 PC-Engine / Turbo Grafx 16 driver
 by Charles Mac Donald
 E-Mail: cgfm2@hooked.net

 Thanks to David Shadoff and Brian McPhail for help with the driver.

****************************************************************************/

/**********************************************************************
          To-Do List:
- convert h6280-based drivers to internal memory map for the I/O region
- test sprite collision and overflow interrupts
- sprite precaching
- rewrite the base renderer loop
- Add CD support
- Add 6 button joystick support
- Add 263 line mode
- Sprite DMA should use vdc VRAM functions
**********************************************************************/

/**********************************************************************
                          Known Bugs
***********************************************************************
- TV Sports Basketball game freezes.
- Ankuku Densetsu: graphics flake out during intro (DMA issues)
**********************************************************************/

#include "driver.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "devices/cartslot.h"
#include "sound/c6280.h"
#include "hash.h"

#define	MAIN_CLOCK	21477270


ADDRESS_MAP_START( pce_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x000000, 0x07FFFF) AM_ROMBANK(1)
	AM_RANGE( 0x080000, 0x0FFFFF) AM_ROMBANK(2)
	AM_RANGE( 0x100000, 0x1EDFFF) AM_ROMBANK(3)
	AM_RANGE( 0x1EE000, 0x1EFFFF) AM_RAM AM_BASE( &pce_nvram )
	AM_RANGE( 0x1F0000, 0x1F1FFF) AM_RAM AM_MIRROR(0x6000) AM_BASE( &pce_user_ram )
	AM_RANGE( 0x1FE000, 0x1FE3FF) AM_READWRITE( vdc_0_r, vdc_0_w )
	AM_RANGE( 0x1FE400, 0x1FE7FF) AM_READWRITE( vce_r, vce_w )
	AM_RANGE( 0x1FE800, 0x1FEBFF) AM_READWRITE( C6280_r, C6280_0_w )
	AM_RANGE( 0x1FEC00, 0x1FEFFF) AM_READWRITE( H6280_timer_r, H6280_timer_w )
	AM_RANGE( 0x1FF000, 0x1FF3FF) AM_READWRITE( pce_joystick_r, pce_joystick_w )
	AM_RANGE( 0x1FF400, 0x1FF7FF) AM_READWRITE( H6280_irq_status_r, H6280_irq_status_w )
	AM_RANGE( 0x1FF800, 0x1FFBFF) AM_READWRITE( pce_cd_intf_r, pce_cd_intf_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( pce_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0x03) AM_READWRITE( vdc_0_r, vdc_0_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( sgx_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x000000, 0x07FFFF) AM_ROMBANK(1)
	AM_RANGE( 0x080000, 0x0FFFFF) AM_ROMBANK(2)
	AM_RANGE( 0x100000, 0x1EDFFF) AM_ROMBANK(3)
	AM_RANGE( 0x1EE000, 0x1EFFFF) AM_RAM AM_BASE( &pce_nvram )
	AM_RANGE( 0x1F0000, 0x1F7FFF) AM_RAM AM_BASE( &pce_user_ram )
	AM_RANGE( 0x1FE000, 0x1FE007) AM_READWRITE( vdc_0_r, vdc_0_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE008, 0x1FE00F) AM_READWRITE( vpc_r, vpc_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE010, 0x1FE017) AM_READWRITE( vdc_1_r, vdc_1_w ) AM_MIRROR(0x03E0)
	AM_RANGE( 0x1FE400, 0x1FE7FF) AM_READWRITE( vce_r, vce_w )
	AM_RANGE( 0x1FE800, 0x1FEBFF) AM_READWRITE( C6280_r, C6280_0_w )
	AM_RANGE( 0x1FEC00, 0x1FEFFF) AM_READWRITE( H6280_timer_r, H6280_timer_w )
	AM_RANGE( 0x1FF000, 0x1FF3FF) AM_READWRITE( pce_joystick_r, pce_joystick_w )
	AM_RANGE( 0x1FF400, 0x1FF7FF) AM_READWRITE( H6280_irq_status_r, H6280_irq_status_w )
	AM_RANGE( 0x1FF800, 0x1FFBFF) AM_READWRITE( pce_cd_intf_r, pce_cd_intf_w )
ADDRESS_MAP_END

ADDRESS_MAP_START( sgx_io , ADDRESS_SPACE_IO, 8)
	AM_RANGE( 0x00, 0x03) AM_READWRITE( sgx_vdc_r, sgx_vdc_w )
ADDRESS_MAP_END

/* todo: alternate forms of input (multitap, mouse, etc.) */
INPUT_PORTS_START( pce )

    PORT_START  /* Player 1 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* button I */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* button II */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* select */
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* run */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
#if 0
    PORT_START  /* Fake dipswitches for system config */
    PORT_DIPNAME( 0x01, 0x01, "Console type")
    PORT_DIPSETTING(    0x00, "Turbo-Grafx 16")
    PORT_DIPSETTING(    0x01, "PC-Engine")

    PORT_DIPNAME( 0x01, 0x01, "Joystick type")
    PORT_DIPSETTING(    0x00, "2 Button")
    PORT_DIPSETTING(    0x01, "6 Button")
#endif
INPUT_PORTS_END


#if 0
static gfx_layout pce_bg_layout =
{
        8, 8,
        2048,
        4,
        {0x00*8, 0x01*8, 0x10*8, 0x11*8 },
        {0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
        32*8,
};

static gfx_layout pce_obj_layout =
{
        16, 16,
        512,
        4,
        {0x00*8, 0x20*8, 0x40*8, 0x60*8},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
        128*8,
};

static gfx_decode pce_gfxdecodeinfo[] =
{
   { 1, 0x0000, &pce_bg_layout, 0, 0x10 },
   { 1, 0x0000, &pce_obj_layout, 0x100, 0x10 },
	{-1}
};
#endif


static MACHINE_DRIVER_START( pce )
	/* basic machine hardware */
	MDRV_CPU_ADD(H6280, MAIN_CLOCK/3)
	MDRV_CPU_PROGRAM_MAP(pce_mem, 0)
	MDRV_CPU_IO_MAP(pce_io, 0)
	MDRV_CPU_VBLANK_INT(pce_interrupt, VDC_LPF)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS(MAIN_CLOCK/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)
	/* MDRV_GFXDECODE( pce_gfxdecodeinfo ) */
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT( vce )
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD(C6280, MAIN_CLOCK/6)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(1, "right", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sgx )
	/* basic machine hardware */
	MDRV_CPU_ADD(H6280, MAIN_CLOCK/3)
	MDRV_CPU_PROGRAM_MAP(sgx_mem, 0)
	MDRV_CPU_IO_MAP(sgx_io, 0)
	MDRV_CPU_VBLANK_INT(sgx_interrupt, VDC_LPF)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_ADD("main",0)
	MDRV_SCREEN_RAW_PARAMS(MAIN_CLOCK/2, VDC_WPF, 70, 70 + 512 + 32, VDC_LPF, 14, 14+242)
	MDRV_PALETTE_LENGTH(1024)
	MDRV_PALETTE_INIT( vce )
	MDRV_COLORTABLE_LENGTH(1024)

	MDRV_VIDEO_START( pce )
	MDRV_VIDEO_UPDATE( pce )

	MDRV_NVRAM_HANDLER( pce )
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD(C6280, MAIN_CLOCK/6)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(1, "right", 1.00)
MACHINE_DRIVER_END

static void pce_partialhash(char *dest, const unsigned char *data,
        unsigned long length, unsigned int functions)
{
        if ( ( length <= PCE_HEADER_SIZE ) || ( length & PCE_HEADER_SIZE ) ) {
	        hash_compute(dest, &data[PCE_HEADER_SIZE], length - PCE_HEADER_SIZE, functions);
	} else {
		hash_compute(dest, data, length, functions);
	}
}

static void pce_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;
		case DEVINFO_INT_MUST_BE_LOADED:				info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_pce_cart; break;
		case DEVINFO_PTR_PARTIAL_HASH:					info->partialhash = pce_partialhash; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "pce"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(pce)
	CONFIG_DEVICE(pce_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

#define rom_pce NULL
#define rom_tg16 NULL
#define rom_sgx NULL

/*	  YEAR  NAME    PARENT	COMPAT	MACHINE	INPUT	 INIT	CONFIG  COMPANY	 FULLNAME */
CONS( 1987, pce,    0,      0,      pce,    pce,     pce,   pce,	"Nippon Electronic Company", "PC Engine", GAME_IMPERFECT_SOUND )
CONS( 1989, tg16,   pce,    0,      pce,    pce,     tg16,  pce,	"Nippon Electronic Company", "TurboGrafx 16", GAME_IMPERFECT_SOUND )
CONS( 1989,	sgx,	pce,	0,		sgx,	pce,	sgx,	pce,	"Nippon Electronic Company", "SuperGrafx", GAME_NOT_WORKING )

