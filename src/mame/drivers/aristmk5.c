/*

   Aristocrat MK5 / MKV hardware
   possibly 'Acorn Archimedes on a chip' hardware

   Note: ARM250 mapping is not identical to

*/

#include "driver.h"
#include "cpu/arm/arm.h"
#include "sound/dac.h"
#include "includes/archimds.h"

static ADDRESS_MAP_START( aristmk5_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x01ffffff) AM_READWRITE(memc_logical_r, memc_logical_w)
	AM_RANGE(0x02000000, 0x02ffffff) AM_RAM AM_BASE(&memc_physmem) /* physical RAM - 16 MB for now, should be 512k for the A310 */
	AM_RANGE(0x03000000, 0x033fffff) AM_READWRITE(ioc_r, ioc_w)
	AM_RANGE(0x03400000, 0x035fffff) AM_ROM AM_REGION("maincpu", 0) AM_WRITE(memc_page_w)
	AM_RANGE(0x03600000, 0x037fffff) AM_READWRITE(memc_r, memc_w)
	AM_RANGE(0x03800000, 0x039fffff) AM_READWRITE(vidc_r, vidc_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( aristmk5 )
INPUT_PORTS_END

static VIDEO_START(aristmk5)
{

}

static VIDEO_UPDATE(aristmk5)
{
	return 0;
}

static DRIVER_INIT( aristmk5 )
{
	archimedes_driver_init(machine);
}

static MACHINE_START( aristmk5 )
{
	archimedes_init(machine);

	// reset the DAC to centerline
	dac_signed_data_w(devtag_get_device(machine, "dac"), 0x80);
}

static MACHINE_RESET( aristmk5 )
{
	archimedes_reset(machine);
}

static MACHINE_DRIVER_START( aristmk5 )
	MDRV_CPU_ADD("maincpu", ARM, 10000000) // ?
	MDRV_CPU_PROGRAM_MAP(aristmk5_map)

	MDRV_MACHINE_RESET( aristmk5 )
	MDRV_MACHINE_START( aristmk5 )

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)

	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(aristmk5)
	MDRV_VIDEO_UPDATE(aristmk5)

	MDRV_SPEAKER_STANDARD_MONO("aristmk5")
	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(0, "aristmk5", 1.00)
MACHINE_DRIVER_END

ROM_START( reelrock )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "reelrock.u7",  0x000000, 0x80000, CRC(b60af34f) SHA1(1143380b765db234b3871c0fe04736472fde7de4) )
	ROM_LOAD32_WORD( "reelrock.u11", 0x000002, 0x80000, CRC(57e341d0) SHA1(9b0d50763bb74ca5fe404c9cd526633721cf6677) )
	ROM_LOAD32_WORD( "reelrock.u8",  0x100000, 0x80000, CRC(57eec667) SHA1(5f3888d75f48b6148f451d7ebb7f99e1a0939f3c) )
	ROM_LOAD32_WORD( "reelrock.u12", 0x100002, 0x80000, CRC(4ac20679) SHA1(0ac732ffe6a33806e4a06e87ec875a3e1314e06b) )
ROM_END

ROM_START( indiandr )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "indiandr.u7",  0x000000, 0x80000, CRC(0c924a3e) SHA1(499b4ae601e53173e3ba5f400a40e5ae7bbaa043) )
	ROM_LOAD32_WORD( "indiandr.u11", 0x000002, 0x80000, CRC(e371dc0f) SHA1(a01ab7fb63a19c144f2c465ecdfc042695124bdf) )
	ROM_LOAD32_WORD( "indiandr.u8",  0x100000, 0x80000, CRC(1c6bfb47) SHA1(7f751cb499a6185a0ab64eeec511583ceeee6ee8) )
	ROM_LOAD32_WORD( "indiandr.u12", 0x100002, 0x80000, CRC(4bbe67f6) SHA1(928f88387da66697f1de54f086531f600f80a15e) )
ROM_END

ROM_START( swthrt2v )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "swthrt2v.u7",  0x000000, 0x80000, CRC(f51b2faa) SHA1(dbcfdbee92af5f89a8a2611bbc687ee0cc907642) )
	ROM_LOAD32_WORD( "swthrt2v.u11", 0x000002, 0x80000, CRC(bd7ead91) SHA1(9f775428a4aa0b0a8ee17aed9be620edc2020c5e) )
ROM_END

ROM_START( margmgc )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "margmgc.u7",  0x000000, 0x80000, CRC(eee7ebaf) SHA1(bad0c08578877f84325c07d51c6ed76c40b70720) )
	ROM_LOAD32_WORD( "margmgc.u11", 0x000002, 0x80000, CRC(4901a166) SHA1(8afe6f08b4ac5c17744dff73939c4bc93124fdf1) )
	ROM_LOAD32_WORD( "margmgc.u8",  0x100000, 0x80000, CRC(b0d78efe) SHA1(bc8b345290f4d31c6553f1e2700bc8324b4eeeac) )
	ROM_LOAD32_WORD( "margmgc.u12", 0x100002, 0x80000, CRC(90ff59a8) SHA1(c9e342db2b5e8c3f45efa8496bc369385046e920) )
	ROM_LOAD32_WORD( "margmgc.u9",  0x200000, 0x80000, CRC(1f0ca910) SHA1(be7a2f395eae09a29faf99ba34551fbc38f20fdb) )
	ROM_LOAD32_WORD( "margmgc.u13", 0x200002, 0x80000, CRC(3f702945) SHA1(a6c9a848d059c1e564fdc5a65bf8c9600853edfa) )
ROM_END

ROM_START( adonis )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "adonis.u7",  0x000000, 0x80000, CRC(ab386ab0) SHA1(56c5baea4272866a9fe18bdc371a49f155251f86) )
	ROM_LOAD32_WORD( "adonis.u11", 0x000002, 0x80000, CRC(ce8c8449) SHA1(9894f0286f27147dcc437e4406870fe695a6f61a) )
	ROM_LOAD32_WORD( "adonis.u8",  0x100000, 0x80000, CRC(99097a82) SHA1(a08214ab4781b06b46fc3be5c48288e373230ef4) )
	ROM_LOAD32_WORD( "adonis.u12", 0x100002, 0x80000, CRC(443a7b6d) SHA1(c19a1c50fb8774826a1e12adacba8bbfce320891) )
ROM_END

ROM_START( dmdtouch )
	ROM_REGION( 0x800000, "maincpu", 0 ) /* ARM Code */
	ROM_LOAD32_WORD( "dmdtouch.u7",  0x000000, 0x80000, CRC(71b19365) SHA1(5a8ba1806af544d33e9acbcbbc0555805b4074e6) )
	ROM_LOAD32_WORD( "dmdtouch.u11", 0x000002, 0x80000, CRC(3d836342) SHA1(b015a4ba998b39ed86cdb6247c9c7f1365641b59) )
	ROM_LOAD32_WORD( "dmdtouch.u8",  0x100000, 0x80000, CRC(971bbf63) SHA1(082f81115209c7089c76fb207248da3c347a080b) )
	ROM_LOAD32_WORD( "dmdtouch.u12", 0x100002, 0x80000, CRC(9e0d08e2) SHA1(38b10f7c37f1cefe9271549073dc0a4fed409aec) )
ROM_END


GAME( 1995, swthrt2v, 0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Sweet Hearts II (C - 07/09/95, Venezuela version)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1997, dmdtouch, 0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Diamond Touch (E - 30-06-97, Local)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, adonis,   0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Adonis (A - 25-05-98, NSW/ACT)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, reelrock, 0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Reelin-n-Rockin (A - 13/07/98, Local)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1998, indiandr, 0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Indian Dreaming (B - 15/12/98, Local)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 2000, margmgc,  0, aristmk5, aristmk5, aristmk5, ROT0,  "Aristocrat", "Margarita Magic (A - 07/07/2000, NSW/ACT)", GAME_NOT_WORKING|GAME_NO_SOUND )
