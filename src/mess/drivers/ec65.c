/***************************************************************************

        EC-65

        16/07/2009 Initial driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/g65816/g65816.h"
#include "video/mc6845.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "machine/6551.h"
#include "machine/6850acia.h"

#define PIA6821_TAG "pia6821"
#define ACIA6850_TAG "acia6850"
#define ACIA6551_TAG "acia6551"
#define VIA6522_0_TAG "via6522_0"
#define VIA6522_1_TAG "via6522_1"
#define MC6845_TAG "mc6845"

static UINT8 *video_ram;

static ADDRESS_MAP_START(ec65_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xe003) AM_DEVREADWRITE(PIA6821_TAG, pia6821_r, pia6821_w)
	AM_RANGE(0xe010, 0xe010) AM_DEVREADWRITE(ACIA6850_TAG, acia6850_stat_r, acia6850_ctrl_w)
	AM_RANGE(0xe011, 0xe011) AM_DEVREADWRITE(ACIA6850_TAG, acia6850_data_r, acia6850_data_w)
	AM_RANGE(0xe100, 0xe10f) AM_DEVREADWRITE(VIA6522_0_TAG, via_r, via_w)
	AM_RANGE(0xe110, 0xe11f) AM_DEVREADWRITE(VIA6522_1_TAG, via_r, via_w)
	AM_RANGE(0xe130, 0xe133) AM_DEVREADWRITE(ACIA6551_TAG,  acia_6551_r, acia_6551_w )
	AM_RANGE(0xe140, 0xe140) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0xe141, 0xe141) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r , mc6845_register_w)
	AM_RANGE(0xe400, 0xe7ff) AM_RAM // 1KB on-board RAM
	AM_RANGE(0xe800, 0xefff) AM_RAM AM_BASE(&video_ram)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(ec65k_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static const pia6821_interface ec65_pia_interface=
{
	DEVCB_NULL,		/* port A input */
	DEVCB_NULL,		/* port B input */
	DEVCB_NULL,		/* CA1 input */
	DEVCB_NULL,		/* CB1 input */
	DEVCB_NULL,		/* CA2 input */
	DEVCB_NULL,		/* CB2 input */
	DEVCB_NULL, 	/* port A output */
	DEVCB_NULL,		/* port B output */
	DEVCB_NULL,		/* CA2 output */
	DEVCB_NULL,		/* CB2 output */
	DEVCB_NULL,		/* IRQA output */
	DEVCB_NULL		/* IRQB output */
};

static ACIA6850_INTERFACE( ec65_acia_intf )
{
	0,
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static READ8_DEVICE_HANDLER(ec65_via_read_a)
{
	return 0;
}
static READ8_DEVICE_HANDLER( ec65_read_ca1 )
{
	return 0;
}
static READ8_DEVICE_HANDLER(ec65_via_read_b)
{
	return 0xff;
}
static WRITE8_DEVICE_HANDLER( ec65_via_write_a )
{
}
static WRITE8_DEVICE_HANDLER( ec65_via_write_b )
{
}

static const via6522_interface ec65_via_1_intf=
{
	DEVCB_NULL,   /* in_a_func */
	DEVCB_HANDLER(ec65_via_read_b),   /* in_b_func */
	DEVCB_NULL,     /* in_ca1_func */
	DEVCB_NULL,                       /* in_cb1_func */
	DEVCB_NULL,                       /* in_ca2_func */
	DEVCB_NULL,                       /* in_cb2_func */
	DEVCB_HANDLER(ec65_via_write_a),  /* out_a_func */
	DEVCB_HANDLER(ec65_via_write_b),  /* out_b_func */
	DEVCB_NULL,                       /* out_ca1_func */
	DEVCB_NULL,                       /* out_cb1_func */
	DEVCB_NULL,                       /* out_ca2_func */
	DEVCB_NULL,                       /* out_cb2_func */
	DEVCB_NULL,                       /* irq_func */
};

static const via6522_interface ec65_via_0_intf=
{
	DEVCB_HANDLER(ec65_via_read_a),      /* in_a_func */
	DEVCB_NULL,      /* in_b_func */
	DEVCB_HANDLER(ec65_read_ca1),      /* in_ca1_func */
	DEVCB_NULL,      /* in_cb1_func */
	DEVCB_NULL,      /* in_ca2_func */
	DEVCB_NULL,      /* in_cb2_func */
	DEVCB_NULL,      /* out_a_func */
	DEVCB_NULL,      /* out_b_func */
	DEVCB_NULL,      /* out_ca1_func */
	DEVCB_NULL,      /* out_cb1_func */
	DEVCB_NULL,      /* out_ca2_func */
	DEVCB_NULL,      /* out_cb2_func */
	DEVCB_NULL,      /* irq_func */
};

/* Input ports */
static INPUT_PORTS_START( ec65 )

INPUT_PORTS_END


static MACHINE_RESET(ec65)
{
}

static VIDEO_START( ec65 )
{
}

static VIDEO_UPDATE( ec65 )
{
	const device_config *mc6845 = devtag_get_device(screen->machine, MC6845_TAG);
	mc6845_update(mc6845, bitmap, cliprect);
	return 0;
}

static MC6845_UPDATE_ROW( ec65_update_row )
{
	UINT8 *charrom = memory_region(device->machine, "chargen");
	int column, bit;

	for (column = 0; column < x_count; column++)
	{
		UINT8 code = video_ram[ma + column];
		UINT16 addr = code << 4 | (ra & 0x0f);
		UINT8 data = charrom[(addr & 0xfff)];
		if (column == cursor_x)
		{
			data = 0xff;
		}

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;
			int color = BIT(data, 7) ? 1 : 0;

			*BITMAP_ADDR16(bitmap, y, x) = color;

			data <<= 1;
		}
	}
}
static const mc6845_interface ec65_crtc6845_interface =
{
	"screen",
	8 /*?*/,
	NULL,
	ec65_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


static MACHINE_DRIVER_START( ec65 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M6502, XTAL_4MHz / 4)
    MDRV_CPU_PROGRAM_MAP(ec65_mem)

    MDRV_MACHINE_RESET(ec65)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz / 8, ec65_crtc6845_interface)

    MDRV_VIDEO_START(ec65)
    MDRV_VIDEO_UPDATE(ec65)

    /* devices */
	MDRV_PIA6821_ADD( PIA6821_TAG, ec65_pia_interface )
	MDRV_ACIA6850_ADD(ACIA6850_TAG, ec65_acia_intf)
	MDRV_VIA6522_ADD(VIA6522_0_TAG, XTAL_4MHz / 4, ec65_via_0_intf)
	MDRV_VIA6522_ADD(VIA6522_1_TAG, XTAL_4MHz / 4, ec65_via_1_intf)
	MDRV_ACIA6551_ADD(ACIA6551_TAG)     // have XTAL of 1.8432MHz connected
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ec65k )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",G65816, XTAL_4MHz) // can use 4,2 or 1 MHz
    MDRV_CPU_PROGRAM_MAP(ec65k_mem)

    MDRV_MACHINE_RESET(ec65)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 640 - 1, 0, 200 - 1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_16MHz / 8, ec65_crtc6845_interface)

    MDRV_VIDEO_START(ec65)
    MDRV_VIDEO_UPDATE(ec65)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( ec65 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ec65.ic6", 0xf000, 0x1000, CRC(acd928ed) SHA1(e02a688a057ff77294717cf7b887425fed0b1153))
	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen.ic19", 0x0000, 0x1000, CRC(9b56a28d) SHA1(41c04fd9fb542c50287bc0e366358a61fc4b0cd4))	// Located on VDU card
ROM_END

ROM_START( ec65k )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
  	ROM_LOAD( "ec65k.ic19",  0xf000, 0x1000, CRC(5e5a890a) SHA1(daa006f2179fd156833e11c73b37881cafe5dede))
	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen.ic19", 0x0000, 0x1000, CRC(9b56a28d) SHA1(41c04fd9fb542c50287bc0e366358a61fc4b0cd4))	// Located on VDU card
ROM_END
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1985, ec65,  0,       0, 		ec65, 	ec65, 	 0,  	  0,  	 "Elektor Electronics",   "EC-65",		GAME_NOT_WORKING)
COMP( 1985, ec65k, ec65,    0, 		ec65k, 	ec65, 	 0,  	  0,  	 "Elektor Electronics",   "EC-65K",		GAME_NOT_WORKING)

