/*

    dc.c - Sega Dreamcast skeleton driver
    by R. Belmont

    SH-4 @ 200 MHz
    ARM7DI @ 2.8223 MHz (no T or M extensions)
    PowerVR 3D video
    AICA audio
    GD-ROM drive (modified ATAPI interface)

            NTSC/N  NTSC/I   PAL/N   PAL/I   VGA
	    (x/240) (x/480) (x/240)  (x/480) (640x480)
    VTOTAL   262     524      312     624    524
    HTOTAL   857     857      863     863    857

    PCLKs = 26917135 (NTSC 480 @ 59.94), 26944080 (VGA 480 @ 60.0), 13458568 (NTSC 240 @ 59.94),
            25925600 (PAL 480 @ 50.00), 13462800 (PAL 240 @ 50.00)
*/

#include "driver.h"
#include "cpu/sh4/sh4.h"
#include "cpu/arm7/arm7core.h"
#include "sound/aica.h"
#include "dc.h"

#define CPU_CLOCK (200000000)

#define DC_VIDEO_TYPE	(3<<8)		// composite

static UINT32 *dc_sound_ram;

UINT32 dma_offset;

static UINT64 portA = 0x3, PDTRA, latched_PDTRA = 0, w;
static int state = 0;

static READ64_HANDLER( dc_pdtra_r )
{
	w = portA;
	if (!state)
	{
		if ((latched_PDTRA == 1) || (latched_PDTRA == 2))
		{
			w &= ~3;
		}
	}

	return w;
}

static WRITE64_HANDLER( dc_pdtra_w )
{
	if ((data) && (!PDTRA))
	{
		latched_PDTRA = data;
		state = 1;
	}
	else if ((!data) && (!PDTRA))
	{
		latched_PDTRA = 0;
	}
	else
	{
		state = 0;
	}

	COMBINE_DATA(&PDTRA);
}

static READ64_HANDLER( dc_arm_r )
{
	return *((UINT64 *)dc_sound_ram+offset);
}

static WRITE64_HANDLER( dc_arm_w )
{
	COMBINE_DATA((UINT64 *)dc_sound_ram + offset);
}

static ADDRESS_MAP_START( dc_map, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM	AM_WRITENOP				// BIOS
	AM_RANGE(0x00200000, 0x0021ffff) AM_ROM AM_REGION("main", 0x200000)	// flash
	AM_RANGE(0x005f6800, 0x005f69ff) AM_READWRITE( dc_sysctrl_r, dc_sysctrl_w )
	AM_RANGE(0x005f6c00, 0x005f6cff) AM_READWRITE( dc_maple_r, dc_maple_w )
	AM_RANGE(0x005f7000, 0x005f70ff) AM_READWRITE( dc_gdrom_r, dc_gdrom_w )
	AM_RANGE(0x005f7400, 0x005f74ff) AM_READWRITE( dc_g1_ctrl_r, dc_g1_ctrl_w )
	AM_RANGE(0x005f7800, 0x005f78ff) AM_READWRITE( dc_g2_ctrl_r, dc_g2_ctrl_w )
	AM_RANGE(0x005f7c00, 0x005f7cff) AM_READWRITE( pvr_ctrl_r, pvr_ctrl_w )
	AM_RANGE(0x005f8000, 0x005f9fff) AM_READWRITE( pvr_ta_r, pvr_ta_w )
	AM_RANGE(0x00600000, 0x006007ff) AM_READWRITE( dc_modem_r, dc_modem_w )
	AM_RANGE(0x00700000, 0x00707fff) AM_READWRITE( dc_aica_reg_r, dc_aica_reg_w )
	AM_RANGE(0x00710000, 0x0071000f) AM_READWRITE( dc_rtc_r, dc_rtc_w )
	AM_RANGE(0x00800000, 0x009fffff) AM_READWRITE( dc_arm_r, dc_arm_w )
	AM_RANGE(0x04000000, 0x04ffffff) AM_RAM	AM_SHARE(2)	// texture memory
	AM_RANGE(0x05000000, 0x05ffffff) AM_RAM AM_SHARE(2)	// mirror of texture RAM
	AM_RANGE(0x0c000000, 0x0cffffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x0d000000, 0x0dffffff) AM_RAM AM_SHARE(1)	// mirror of main RAM
	AM_RANGE(0x10000000, 0x107fffff) AM_WRITE( ta_fifo_poly_w )
	AM_RANGE(0x10800000, 0x10ffffff) AM_WRITE( ta_fifo_yuv_w )
	AM_RANGE(0x11000000, 0x11ffffff) AM_RAM AM_SHARE(2)	// another mirror of texture memory
	AM_RANGE(0xa0000000, 0xa01fffff) AM_ROM AM_REGION("main", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dc_port, ADDRESS_SPACE_IO, 64 )
	AM_RANGE(0x00000000, 0x00000007) AM_READWRITE( dc_pdtra_r, dc_pdtra_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( dc_audio_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&dc_sound_ram)		/* shared with SH-4 */
	AM_RANGE(0x00800000, 0x00807fff) AM_READWRITE(dc_arm_aica_r, dc_arm_aica_w)
ADDRESS_MAP_END

static MACHINE_RESET( dc_console )
{
	MACHINE_RESET_CALL(dc);
	aica_set_ram_base(0, dc_sound_ram, 2*1024*1024);
}

static void aica_irq(running_machine *machine, int irq)
{
	cpunum_set_input_line(machine, 1, ARM7_FIRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
}

static const aica_interface dc_aica_interface =
{
	0,
	aica_irq
};

static INTERRUPT_GEN( dc_dispatch_vblank )
{
	dc_vblank(machine);
}

static const struct sh4_config sh4cpu_config = {  1,  0,  1,  0,  0,  0,  1,  1,  0, CPU_CLOCK };

static MACHINE_DRIVER_START( dc )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", SH4, CPU_CLOCK)
	MDRV_CPU_CONFIG(sh4cpu_config)
	MDRV_CPU_PROGRAM_MAP(dc_map,0)
	MDRV_CPU_IO_MAP(dc_port,0)
	MDRV_CPU_VBLANK_INT("main", dc_dispatch_vblank)

	MDRV_CPU_ADD("sound", ARM7, ((XTAL_33_8688MHz*2)/3)/8)	// AICA bus clock is 2/3rds * 33.8688.  ARM7 gets 1 bus cycle out of each 8.
	MDRV_CPU_PROGRAM_MAP(dc_audio_map, 0)

	MDRV_MACHINE_RESET( dc_console )

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_RAW_PARAMS((524*857*59.94), 857, 0xa4, 640+0xa4, 524, 0x12, 480+0x12)
	
	MDRV_VIDEO_START(dc)
	MDRV_VIDEO_UPDATE(dc)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD("aica", AICA, 0)
	MDRV_SOUND_CONFIG(dc_aica_interface)
	MDRV_SOUND_ROUTE(0, "left", 2.0)
	MDRV_SOUND_ROUTE(0, "right", 2.0)
MACHINE_DRIVER_END

ROM_START(dc)
	ROM_REGION(0x220000, "main", 0)
        ROM_LOAD( "dc101d_us.bin", 0x000000, 0x200000, CRC(89f2b1a1) SHA1(8951d1bb219ab2ff8583033d2119c899cc81f18c) )	// BIOS
        ROM_LOAD( "dcus_ntsc.bin", 0x200000, 0x020000, CRC(c611b498) SHA1(94d44d7f9529ec1642ba3771ed3c5f756d5bc872) )	// Flash
ROM_END

ROM_START( dceu )
	ROM_REGION(0x220000, "main", 0)
        ROM_LOAD( "dc101d_eu.bin", 0x000000, 0x200000, CRC(a2564fad) SHA1(edc5d3d70a93c935703d26119b37731fd317d2bf) )	// BIOS
        ROM_LOAD( "dceu_pal.bin", 0x200000, 0x020000, CRC(b7e5aeeb) SHA1(11e02433e13b793ec7ffe0ae2356750bb8a575b4) )	// Flash
ROM_END

ROM_START( dcjp )
	ROM_REGION(0x220000, "main", 0)
        ROM_LOAD( "dc1004jp.bin", 0x000000, 0x200000, CRC(5454841f) SHA1(1ea132c0fbbf07ef76789eadc07908045c089bd6) )	// BIOS
        /* ROM_LOAD( "dcjp_ntsc.bad", 0x200000, 0x020000, BAD_DUMP CRC(307a7035) SHA1(1411423a9d071340ea52c56e19c1aafc4e1309ee) )      // Hacked Flash */
        ROM_LOAD( "dcjp_ntsc.bin", 0x200000, 0x020000, CRC(5F92BF76) SHA1(BE78B834F512AB2CF3D67B96E377C9F3093FF82A) )  // Flash
ROM_END

ROM_START( dcdev )
	ROM_REGION(0x220000, "main", 0)
        ROM_LOAD( "hkt-0120.bin", 0x000000, 0x200000, CRC(2186E0E5) SHA1(6BD18FB83F8FDB56F1941E079580E5DD672A6DAD) )		// BIOS
        ROM_LOAD( "hkt-0120-flash.bin", 0x200000, 0x020000, CRC(7784C304) SHA1(31EF57F550D8CD13E40263CBC657253089E53034) )	// Flash
ROM_END

static SYSTEM_CONFIG_START(dc)
SYSTEM_CONFIG_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    CONFIG  COMPANY FULLNAME */
CONS( 1999, dc, 	dcjp, 	0, 		dc, 	0, 		0, 	dc, 	"Sega", "Dreamcast (US NTSC)", GAME_NOT_WORKING )
CONS( 1998, dcjp, 	0, 		0, 		dc, 	0, 		0, 	dc, 	"Sega", "Dreamcast (Japan NTSC)", GAME_NOT_WORKING )
CONS( 1999, dceu, 	dcjp, 	0, 		dc, 	0, 		0, 	dc, 	"Sega", "Dreamcast (European PAL)", GAME_NOT_WORKING )
CONS( 1998, dcdev, 	dcjp, 	0, 		dc, 	0, 		0, 	dc, 	"Sega", "HKT-0120 Sega Dreamcast Development Box", GAME_NOT_WORKING )

