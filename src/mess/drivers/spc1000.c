/***************************************************************************

        Samsung SPC-1000 driver by Miodrag Milanovic

        10/05/2009 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/m6847.h"
#include "sound/ay8910.h"
#include "sound/wave.h"
#include "devices/cassette.h"
#include "devices/messram.h"

static UINT8 IPLK;
static UINT8 *spc1000_video_ram;
static UINT8 GMODE;

static ADDRESS_MAP_START(spc1000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE( 0x8000, 0xffff ) AM_READ_BANK("bank3") AM_WRITE_BANK("bank4")
ADDRESS_MAP_END

static WRITE8_HANDLER(spc1000_iplk_w)
{
	IPLK = IPLK ? 0 : 1;
	if (IPLK == 1) {
		memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "maincpu"));
		memory_set_bankptr(space->machine, "bank3", memory_region(space->machine, "maincpu"));
	} else {
		memory_set_bankptr(space->machine, "bank1", messram_get_ptr(devtag_get_device(space->machine, "messram")));
		memory_set_bankptr(space->machine, "bank3", messram_get_ptr(devtag_get_device(space->machine, "messram")) + 0x8000);
	}
}

static READ8_HANDLER(spc1000_iplk_r)
{
	IPLK = IPLK ? 0 : 1;
	if (IPLK == 1) {
		memory_set_bankptr(space->machine, "bank1", memory_region(space->machine, "maincpu"));
		memory_set_bankptr(space->machine, "bank3", memory_region(space->machine, "maincpu"));
	} else {
		memory_set_bankptr(space->machine, "bank1", messram_get_ptr(devtag_get_device(space->machine, "messram")));
		memory_set_bankptr(space->machine, "bank3", messram_get_ptr(devtag_get_device(space->machine, "messram")) + 0x8000);
	}
	return 0;
}



static WRITE8_HANDLER(spc1000_video_ram_w)
{
	spc1000_video_ram[offset] = data;
}

static READ8_HANDLER(spc1000_video_ram_r)
{
	return spc1000_video_ram[offset];
}

static READ8_HANDLER(spc1000_keyboard_r) {
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4",
		"LINE5", "LINE6", "LINE7", "LINE8", "LINE9"
	};
	return input_port_read(space->machine, keynames[offset]);
}

static WRITE8_DEVICE_HANDLER(spc1000_gmode_w)
{
	GMODE = data;

	// GMODE layout: CSS|NA|PS2|PS1|~A/G|GM0|GM1|NA
	//	[PS2,PS1] is used to set screen 0/1 pages
	mc6847_gm1_w(device, BIT(data, 1));
	mc6847_gm0_w(device, BIT(data, 2));
	mc6847_ag_w(device, BIT(data, 3));
	mc6847_css_w(device, BIT(data, 7));	
}

static READ8_DEVICE_HANDLER(spc1000_gmode_r)
{
	return GMODE;
}

static ADDRESS_MAP_START( spc1000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_READWRITE(spc1000_video_ram_r, spc1000_video_ram_w)
	AM_RANGE(0x2000, 0x3fff) AM_DEVREADWRITE("mc6847", spc1000_gmode_r, spc1000_gmode_w)
	AM_RANGE(0x8000, 0x8009) AM_READ(spc1000_keyboard_r)
	AM_RANGE(0xA000, 0xA000) AM_READWRITE(spc1000_iplk_r, spc1000_iplk_w)
	AM_RANGE(0x4000, 0x4000) AM_DEVWRITE("ay8910", ay8910_address_w)
	AM_RANGE(0x4001, 0x4001) AM_DEVREADWRITE("ay8910", ay8910_r, ay8910_data_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( spc1000 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_RCONTROL) PORT_CODE(KEYCODE_LCONTROL)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_PAUSE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Graph") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_START("LINE9")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
INPUT_PORTS_END


static MACHINE_RESET(spc1000)
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_read_bank(space, 0x0000, 0x7fff, 0, 0, "bank1");
	memory_install_read_bank(space, 0x8000, 0xffff, 0, 0, "bank3");

	memory_install_write_bank(space, 0x0000, 0x7fff, 0, 0, "bank2");
	memory_install_write_bank(space, 0x8000, 0xffff, 0, 0, "bank4");

	memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu"));
	memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")));
	memory_set_bankptr(machine, "bank3", memory_region(machine, "maincpu"));
	memory_set_bankptr(machine, "bank4", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x8000);

	IPLK = 1;
}

static READ8_DEVICE_HANDLER( spc1000_mc6847_videoram_r )
{	
	// GMODE layout: CSS|NA|PS2|PS1|~A/G|GM0|GM1|NA
	//	[PS2,PS1] is used to set screen 0/1 pages
	if ( !BIT(GMODE, 3) ) {	// text mode (~A/G set to A)
		unsigned int page = (BIT(GMODE, 5) << 1) | BIT(GMODE, 4);
		mc6847_inv_w(device, BIT(spc1000_video_ram[offset+page*0x200+0x800], 0));
		mc6847_css_w(device, BIT(spc1000_video_ram[offset+page*0x200+0x800], 1));
		mc6847_as_w(device, BIT(spc1000_video_ram[offset+page*0x200+0x800], 2));
		mc6847_intext_w(device, BIT(spc1000_video_ram[offset+page*0x200+0x800], 3));
		return spc1000_video_ram[offset+page*0x200];
	} else {	// graphics mode: uses full 6KB of VRAM
		return spc1000_video_ram[offset];
	}
}


static UINT8 spc1000_get_char_rom(running_machine *machine, UINT8 ch, int line)
{
	return spc1000_video_ram[0x1000+(ch&0x7F)*16+line];
}

static VIDEO_START( spc1000 )
{
	spc1000_video_ram = auto_alloc_array(machine, UINT8, 0x2000);
}

static VIDEO_UPDATE( spc1000 )
{
	const device_config *mc6847 = devtag_get_device(screen->machine, "mc6847");
	return mc6847_update(mc6847, bitmap, cliprect);
}

static const ay8910_interface spc1000_ay_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_NULL
};

static const cassette_config spc1000_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};

static const mc6847_interface spc1000_mc6847_intf =
{
	DEVCB_HANDLER(spc1000_mc6847_videoram_r),	// data fetch
	DEVCB_LINE_VCC,								// GM2 [hardwired to 1]
	DEVCB_NULL,									// GM1
	DEVCB_NULL,									// GM0
	DEVCB_NULL,									// INVEXT
	DEVCB_NULL,									// INV
	DEVCB_NULL,									// ~A/S
	DEVCB_NULL,									// ~A/G
	DEVCB_NULL,									// CSS
	DEVCB_NULL,									// FS (output)
	DEVCB_NULL,									// HS (output)
	DEVCB_NULL									// RS (output)
};

static MACHINE_DRIVER_START( spc1000 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MDRV_CPU_PROGRAM_MAP(spc1000_mem)
	MDRV_CPU_IO_MAP(spc1000_io)

	MDRV_MACHINE_RESET(spc1000)

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	MDRV_VIDEO_START(spc1000)
	MDRV_VIDEO_UPDATE(spc1000)

	MDRV_MC6847_ADD("mc6847", spc1000_mc6847_intf)
	MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_NTSC)
	MDRV_MC6847_CHAR_ROM(spc1000_get_char_rom)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("ay8910", AY8910, XTAL_4MHz / 1)
	MDRV_SOUND_CONFIG(spc1000_ay_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CASSETTE_ADD( "cassette", spc1000_cassette_config )

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( spc1000 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "spcall.rom", 0x0000, 0x8000, CRC(19638fc9))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1982, spc1000,  0,       0, 	spc1000, 	spc1000, 	 0,  "Samsung",   "SPC-1000",		GAME_NOT_WORKING)
