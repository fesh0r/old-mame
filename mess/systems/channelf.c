/******************************************************************
 *  Fairchild Channel F driver
 *
 *  Juergen Buchmueller & Frank Palazzolo
 *
 *  TBD:
 *      Verify timing on real unit
 *
 ******************************************************************/

#include "driver.h"
#include "image.h"
#include "vidhrdw/generic.h"
#include "includes/channelf.h"
#include "devices/cartslot.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)	/* x */
#endif

static UINT8 latch[4];

static void init_channelf(void)
{
	UINT8 *mem = memory_region(REGION_GFX1);
	int i;

    for (i = 0; i < 256; i++)
		mem[i] = i;
}

static DEVICE_LOAD( channelf_cart )
{
	return cartslot_load_generic(file, REGION_CPU1, 0x800, 0x800, 0x800, 0);
}

static READ_HANDLER( channelf_port_0_r )
{
	int data = readinputport(0);
	data = (data ^ 0xff) | latch[0];
    LOG(("port_0_r: $%02x\n",data));
	return data;
}

static READ_HANDLER( channelf_port_1_r )
{
	int data = readinputport(1);
	data = (data ^ 0xff) | latch[1];
    LOG(("port_1_r: $%02x\n",data));
	return data;
}

static READ_HANDLER( channelf_port_4_r )
{
	int data = readinputport(2);
	data = (data ^ 0xff) | latch[2];
    LOG(("port_4_r: $%02x\n",data));
	return data;
}

static READ_HANDLER( channelf_port_5_r )
{
	int data = 0xff;
	data = (data ^ 0xff) | latch[3];
    LOG(("port_5_r: $%02x\n",data));
	return data;
}

static WRITE_HANDLER( channelf_port_0_w )
{
	int offs;

	LOG(("port_0_w: $%02x\n",data));

/*
	if (data & 0x40)
		controller_enable = 1;
	else
		controller_enable = 0;
*/

    if (data & 0x20)
	{
		offs = channelf_row_reg*128+channelf_col_reg;
		if (videoram[offs] != channelf_val_reg)
			videoram[offs] = channelf_val_reg;
	}
	latch[0] = data;
}

static WRITE_HANDLER( channelf_port_1_w )
{
	LOG(("port_1_w: $%02x\n",data));

    channelf_val_reg = ((data ^ 0xff) >> 6) & 0x03;

	latch[1] = data;
}

static WRITE_HANDLER( channelf_port_4_w )
{
	LOG(("port_4_w: $%02x\n",data));

    channelf_col_reg = (data | 0x80) ^ 0xff;

    latch[2] = data;
}

static WRITE_HANDLER( channelf_port_5_w )
{
	LOG(("port_5_w: $%02x\n",data));

	channelf_sound_w((data>>6)&3);

    channelf_row_reg = (data | 0xc0) ^ 0xff;

    latch[3] = data;
}

static MEMORY_READ_START (readmem)
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x0fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START (writemem)
	{ 0x0000, 0x03ff, MWA_ROM },
	{ 0x0400, 0x07ff, MWA_ROM },
MEMORY_END

static PORT_READ_START (readport)
	{ 0x00, 0x00,	channelf_port_0_r }, /* Front panel switches */
	{ 0x01, 0x01,	channelf_port_1_r }, /* Right controller     */
	{ 0x04, 0x04,	channelf_port_4_r }, /* Left controller      */
	{ 0x05, 0x05,	channelf_port_5_r },
PORT_END

static PORT_WRITE_START (writeport)
	{ 0x00, 0x00,	channelf_port_0_w }, /* Enable Controllers & ARM WRT */
	{ 0x01, 0x01,	channelf_port_1_w }, /* Video Write Data */
	{ 0x04, 0x04,	channelf_port_4_w }, /* Video Horiz */
	{ 0x05, 0x05,	channelf_port_5_w }, /* Video Vert & Sound */
PORT_END

INPUT_PORTS_START( channelf )
	PORT_START /* Front panel buttons */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START )	/* START (1) */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON5 )	/* HOLD  (2) */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON6 )	/* MODE  (3) */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON7 )	/* TIME  (4) */
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START /* Right controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER1 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER1 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER1 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER1 ) /* PUSH DOWN   */

	PORT_START /* Left controller */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 	   | IPF_PLAYER2 ) /* C-CLOCKWISE */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON3 	   | IPF_PLAYER2 ) /* CLOCKWISE   */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 	   | IPF_PLAYER2 ) /* PULL UP     */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 	   | IPF_PLAYER2 ) /* PUSH DOWN   */

INPUT_PORTS_END

static struct CustomSound_interface channelf_sound_interface = {
	channelf_sh_custom_start,
	channelf_sh_stop,
	channelf_sh_custom_update
};


static MACHINE_DRIVER_START( channelf )
	/* basic machine hardware */
	MDRV_CPU_ADD(F8, 3579545/2)        /* Colorburst/2 */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*2, 64*2)
	MDRV_VISIBLE_AREA(1*2, 112*2 - 1, 0, 64*2 - 1)
	MDRV_PALETTE_LENGTH(8)
	MDRV_COLORTABLE_LENGTH(0)
	MDRV_PALETTE_INIT( channelf )

	MDRV_VIDEO_START( channelf )
	MDRV_VIDEO_UPDATE( channelf )

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, channelf_sound_interface)
MACHINE_DRIVER_END

ROM_START(channelf)
	ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("sl31253.rom",  0x0000, 0x0400, CRC(04694ed9))
		ROM_LOAD("sl31254.rom",  0x0400, 0x0400, CRC(9c047ba3))
	ROM_REGION(0x00100,REGION_GFX1,0)
		/* bit pattern is stored here */
ROM_END

SYSTEM_CONFIG_START(channelf)
	CONFIG_DEVICE_CARTSLOT_OPT( 1, "bin\0", NULL, NULL, device_load_channelf_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT		COMPANY		FULLNAME */
CONS( 1976, channelf, 0,		0,		channelf, channelf, channelf,	channelf,	"Fairchild", "Channel F" )


