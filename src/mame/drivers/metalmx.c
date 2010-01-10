/* Metal Maniax
 skeleton driver!

 lots of CPUS.. count 'em
 2 x TMS34020-40
 4 x DSP32C F33 DSPs
 and 2 stacks of
 1 x MC68EC020FG16 CPU
 1 x ADSP-2105 KP-40 DSP
 1 x TMS320C31PQL DSP

 That's 12 CPUs for one game.

 This should make an ideal starter project for anybody wanting to get into emulation :-)

*/


/*

Metal Maniax
Atari Prototype

This is a truly insane board stack. Side view:

 +----(1l)----+              +----(1r)----+
+-----(2l)------------+    +------(2r)------------+
+-----(3l)-------------+   +------(3r)------------+
+-----------------------(4)-----------------------+
+-----------------------(5)-----------------------+

The two bottom boards (4) and (5) are common. The top stack of 3 boards are duplicated twice. Here, in detail, is a list of components on each board:


----------------------
Layer 1: (1l) and (1r)
----------------------
Dimensions: 5" x 8"
ID: ROMCH31 A053443 ATARI GAMES (c) 94 MADE IN U.S.A.
Title: "Teenage Kicks Right Through the Night"

Programmable:
16 x 27C040-10 EPROMs, labelled datametl 0-15, 10/19/94 11:08:05
 1 x 27C040-10 EPROM, labelled bootbetl rel 34, 10/19/94 16:22:49
 1 x GAL16V8B, labelled 136103-1500

Logic:
 3 x SN74F245


----------------------
Layer 2: (2l) and (2r)
----------------------
Dimensions: 5.5" x 8"
ID: CH31.2 A053304 ATARI GAMES (c) 93 MADE IN U.S.A.
Title: "Silly Putty: Thixotropic or Dilatent"

Programmable:
 4 x unpopulated 42-pin sockets
 1 x unpopulated 32-pin socket
 1 x GAL16V8A-25, labelled DSP
 1 x GAL16V8A-25, labelled IRQ2
 1 x GAL16V8A-25, labelled XDEC

CPU/DSP:
 1 x TMS320C31PQL DSP
 1 x 33.8688MHz crystal

RAM:
 4 x CY7C199-20 32k*8 SRAM

Unknown:
 2 x Asahi Kasei AK4316-VS

Logic:
 1 x SN74F138
 4 x SN74LS374


----------------------
Layer 3: (3l) and (3r)
----------------------
Dimensions: 9" x 12"
ID: MCUBE A052137 ATARI GAMES (c) 93

Programmable:
 4 x 27C040-10 EPROMs, labelled tgs665e 0-3, 9/28/94 15:53:52
 4 x 27C040-10 EPROMs, labelled st665e 0-3, 9/28/94 15:56:02
 1 x 27C512-IJL EPROM, labelled MTL MANIAX 136103-2308PP
 1 x GAL20V8B-15, labelled 136103-1300

CPU/DSP:
 1 x ADSP-2105 KP-40 DSP
 1 x 10.000MHz crystal
 1 x MC68EC020FG16 CPU
 1 x 14.318180MHz crystal

RAM:
 1 x MK48Z02B-15 ZEROPOWER RAM
 1 x MK48T02B-15 TIMEKEEPER RAM
 4 x MOSEL 62256H-20NC 32k*8 SRAM

Unknown:
 1 x XILINX FPGA XC3142-3 TQ144C X25839M AIG9326
 1 x XILINX 1736DPC, labelled 3302 (checksum?)
 1 x DALLAS DS1232 MicroMonitor

ADCs:
 1 x AD7582KN 12-bit ADC
 1 x ADC0809CCN 8-bit ADC with 8-way mux

Logic:
 1 x SN74F138
 7 x SN74F244
15 x SN74F245
 1 x SN74LS240
 3 x SN74LS259
 5 x SN74LS374
 1 x SN75ALS194
 1 x SN75ALS195
 1 x 74F04PC

Misc:
 1 x 8-position DIP switch
 3 x green LEDs
 2 x red LEDs


------------
Layer 4: (4)
------------
Dimensions: 8" x 18"
ID: TGSMATH A052043 ATARI GAMES (c) 93 MADE IN U.S.A.

Programmable:
 2 x GAL22V10B, labelled 136103-1200
 1 x GAL22V10B, labelled 136103-1202
 1 x GAL22V10B, labelled 136103-1203
 1 x GAL22V10B, labelled 136103-1204
 1 x GAL22V10B, labelled 136103-1205
 1 x GAL22V10B, labelled 136103-1218
 1 x GAL20V8B, labelled 136103-1217
 4 x GAL16V8B, labelled 136103-1219

CPU/DSP:
 4 x DSP32C F33 DSPs, labelled Freda, Gus, Herb, Irene

RAM:
32 x MOSEL 62256H-20NC 32k*8 SRAM
16 x MT5C1008-20 128k*8 SRAM

Logic:
 4 x 74F04PC
 1 x SN74LS74
 2 x SN74LS174
16 x SN74ALS373
 2 x SN74ALS374


------------
Layer 5: (5)
------------
Dimensions: 12" x 18"
ID: ATARI GAMES (c) 93 MADE IN U.S.A. TGS A051985

Programmable:
 8 x 27C040-10, labelled TEX0-7 0
 8 x 27C040-10, labelled TEX0-7 1
 2 x GAL16V8B, labelled 136103-1114
 1 x GAL16V8B, labelled 136103-1116
 1 x GAL22V10B, labelled 136103-1106
 1 x GAL22V10B, labelled 136103-1107
 2 x GAL22V10B, labelled 136103-1108
 2 x GAL22V10B, labelled 136103-1111
 2 x GAL22V10B, labelled 136103-1112
 2 x N82S147 512*8 BPROM, labelled RED
 2 x N82S147 512*8 BPROM, labelled GREEN
 2 x N82S147 512*8 BPROM, labelled BLUE

CPU/DSP:
 2 x TMS34020-40
 1 x unknown crystal
 1 x 16.00000MHz crystal

RAM:
 4 x TMS45160DZ-80 256k*16 DRAM
 4 x TMS55160DGH-70 256k*16 VRAM

Unknown:
 1 x HP FPGA 1FY5-0003 9351-HONG KONG NPFY5B5880
 1 x XILINX FPGA XC3190-3 PQ160C X35728M AIG9427
 1 x XILINX FPGA CX4003-6 PQ100C X25788M ASG9325
 2 x XILINX 1765DPC

Logic:
 8 x SN74BCT652
20 x DM74ALS541
 5 x ACT7814-25 64x18 strobed FIFO
 1 x 74F04PC
 4 x 74F273
 1 x SN74ALS2232
 6 x 74HCT374
 1 x 74ALS164

*/


#include "driver.h"
#include "cpu/m68000/m68000.h"

static UINT32* metalmx_fbram;

static VIDEO_START(metalmx)
{

}

static VIDEO_UPDATE(metalmx)
{
	int y, x, count;
	static const int xxx = 256, yyy = 204;

	bitmap_fill(bitmap, 0, get_black_pen(screen->machine));

	count = 0;
	for (y = 0; y < yyy; y++)
	{
		for(x = 0; x < xxx; x++)
		{
			*BITMAP_ADDR16(bitmap, y, (x*4)+0) =  ((metalmx_fbram[count]>>24)&0xff);
			*BITMAP_ADDR16(bitmap, y, (x*4)+1) =  ((metalmx_fbram[count]>>16)&0xff);
			*BITMAP_ADDR16(bitmap, y, (x*4)+2) =  ((metalmx_fbram[count]>>8)&0xff);
			*BITMAP_ADDR16(bitmap, y, (x*4)+3) =  ((metalmx_fbram[count]>>0)&0xff);
			count++;
		}
	}

	return 0;
}

static ADDRESS_MAP_START( metalmx_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x700000, 0x71ffff) AM_RAM AM_BASE(&metalmx_fbram)
	AM_RANGE(0x800000, 0x85ffff) AM_RAM

	AM_RANGE(0xf00000, 0xffffff) AM_RAM
ADDRESS_MAP_END


static INPUT_PORTS_START( metalmx )
	PORT_START("P1_P2")
INPUT_PORTS_END

static MACHINE_DRIVER_START( metalmx )
	MDRV_CPU_ADD("maincpu", M68EC020, 10000000) // ??
	MDRV_CPU_PROGRAM_MAP(metalmx_map)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)

	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(metalmx)
	MDRV_VIDEO_UPDATE(metalmx)
MACHINE_DRIVER_END


ROM_START( metalmx )
	/* ----------------------------------------
       Layer 1 (there are 2 of these boards)
    ---------------------------------------- */

	ROM_REGION( 0x80000, "boot", 0 )
	ROM_LOAD( "bootmetl.r34", 0x00000, 0x80000, CRC(ec799644) SHA1(32c77abb70fee1da8e3d7141bce2032e73e0eb35) )

	ROM_REGION( 0x80000, "data", 0 )
	ROM_LOAD( "datametl.0", 0x00000, 0x80000, CRC(004dc445) SHA1(e52e539cc38afa917d1c769f9ad1794f4bd833b2) )
	ROM_LOAD( "datametl.1", 0x00000, 0x80000, CRC(e0465fc5) SHA1(cd8584e48b6cf33bc103cdfdb68d32eef3f2bec5) )
	ROM_LOAD( "datametl.2", 0x00000, 0x80000, CRC(2fb4b470) SHA1(b5ec1f9579200684cfd7e46c7774687df9330e19) )
	ROM_LOAD( "datametl.3", 0x00000, 0x80000, CRC(00f2bab2) SHA1(20ee4b185c9ac637be28cdc96c003c8f3c7ba3ee) )
	ROM_LOAD( "datametl.4", 0x00000, 0x80000, CRC(45f3d3ed) SHA1(cc83e61f4572ebe0a32c12b7c463daa1db0b7d09) )
	ROM_LOAD( "datametl.5", 0x00000, 0x80000, CRC(866bad4d) SHA1(67ed2f183a8e0424ca8963032d34c24c1f0a1e9a) )
	ROM_LOAD( "datametl.6", 0x00000, 0x80000, CRC(82c52a2f) SHA1(4250749c5e7281248315986150040f00529439c4) )
	ROM_LOAD( "datametl.7", 0x00000, 0x80000, CRC(6d4f81ab) SHA1(94fcad3f9348e95022327562803583b2e0b94caa) )
	ROM_LOAD( "datametl.8", 0x00000, 0x80000, CRC(697dda37) SHA1(2d5b8c1c5ce52f10472c3bb0c4aa3d14cb67be27) )
	ROM_LOAD( "datametl.9", 0x00000, 0x80000, CRC(a54ab7a9) SHA1(c2cba71188aec74766413961aa78444e64005634) )
	ROM_LOAD( "datametl.10",0x00000, 0x80000, CRC(532ace72) SHA1(84f3b112a7edba794c9ce7fdd0d81c57157203c8) )
	ROM_LOAD( "datametl.11",0x00000, 0x80000, CRC(3a7ce25d) SHA1(2dc4cad858207b645b4cb71ce5be74e3ce5383d3) )
	ROM_LOAD( "datametl.12",0x00000, 0x80000, CRC(13b2b1c7) SHA1(ab37b04e04b8a6a6929e5f039c2bcf6fbd330545) )
	ROM_LOAD( "datametl.13",0x00000, 0x80000, CRC(e8894400) SHA1(7c094502404b897347950f943c79f6fa80c984a1) )
	ROM_LOAD( "datametl.14",0x00000, 0x80000, CRC(709df01f) SHA1(639d8d4cf79ae7e0406d11ed1c475986f3e595f6) )
	ROM_LOAD( "datametl.15",0x00000, 0x80000, CRC(c55dbfbf) SHA1(63de1b24f5024e94601bcdffa7a3e418a195342f) )

	ROM_REGION( 0x80000, "palslayer1", 0 )
	ROM_LOAD( "103-1500.bin",  0x000, 0x117, CRC(9883af90) SHA1(62b4a0cce5832628149c48e6810055ad3919cc5b) )

	/* ----------------------------------------
       Layer 2 (there are 2 of these boards)
    ---------------------------------------- */

	ROM_REGION( 0x80000, "palslayer2", 0 )
	ROM_LOAD( "dsp.bin",   0x000, 0x117, CRC(321b5250) SHA1(8d9c2ed9b0375c4624ea3efa80ba0a78135b756c) )
	ROM_LOAD( "irq2q.bin", 0x000, 0x117, CRC(983cb6e1) SHA1(0222fd2b79691b3b249f6b353f5687be83e291a7) )
	ROM_LOAD( "xdec.bin",  0x000, 0x117, CRC(fcb37143) SHA1(c95a6714f151868d42722684c8b67e9356f70544) )

	/* ----------------------------------------
       Layer 3 (there are 2 of these boards)
    ---------------------------------------- */

	ROM_REGION( 0x200000, "maincpu", 0 ) /* 68020 code */
	ROM_LOAD32_BYTE( "st665e.0", 0x00000, 0x80000, CRC(b2a90fd0) SHA1(ae483ab0aa68493904ea8d1906e22fdaa16a8a27) )
	ROM_LOAD32_BYTE( "st665e.1", 0x00001, 0x80000, CRC(559fecb7) SHA1(092e7e358d02f179a59849db0cafad0b4a95c0ed) )
	ROM_LOAD32_BYTE( "st665e.2", 0x00002, 0x80000, CRC(ee64b773) SHA1(8b1f51450804f16e73045ac9198daa7bd92d993b) )
	ROM_LOAD32_BYTE( "st665e.3", 0x00003, 0x80000, CRC(42b78cde) SHA1(a441fcd1cd34e1a8234be44ab33e56fbb73bac79) )

	ROM_REGION( 0x200000, "tgs", 0 )
	ROM_LOAD32_BYTE( "tgs665e.0", 0x00000, 0x80000, CRC(2a4102f1) SHA1(2d21956b27cc9ac3d418f4595c1b8aa08a0298f6) )
	ROM_LOAD32_BYTE( "tgs665e.1", 0x00001, 0x80000, CRC(7d0eff8f) SHA1(c815cfa55619c3363c86c843047fe8487daaa6c1) )
	ROM_LOAD32_BYTE( "tgs665e.2", 0x00002, 0x80000, CRC(3f965f1a) SHA1(7333e1cd5a9428d78236a5532b6a60203fd1485b) )
	ROM_LOAD32_BYTE( "tgs665e.3", 0x00003, 0x80000, CRC(bb0ea984) SHA1(92a273675b32a2e1782012d49a404c9e8658eb2d) )

	ROM_REGION( 0x80000, "103_2308", 0 )
	ROM_LOAD( "103-2308.bin", 0x00000, 0x10000, CRC(af1781d0) SHA1(f3f69e2fd83a949b447b71410ba9e165229e5aad) )

	ROM_REGION( 0x80000, "palslayer3", 0 )
	ROM_LOAD( "103-1300.bin",  0x000, 0x157, CRC(eec18a29) SHA1(cc41cc921f59d385df805217f3b09bc289379f79) )

	/* ----------------------------
       Layer 4
    ---------------------------- */

	ROM_REGION( 0x80000, "palslayer4", 0 )
	ROM_LOAD( "103-1200.bin",  0x000, 0x2e5, CRC(cf1d4df4) SHA1(2ae592df2f16af070620766b7ebf60918c7f725d) )
	ROM_LOAD( "103-1202.bin",  0x000, 0x2e5, CRC(dab3b17f) SHA1(ac825063a293a32ae6ba5cbf5cfb52aa4aea62c9) )
	ROM_LOAD( "103-1204.bin",  0x000, 0x2e5, CRC(bb675ce0) SHA1(e3b188ebfe13e826e3d14c1cc456810d85750314) )
	ROM_LOAD( "103-1217.bin",  0x000, 0x157, CRC(a6418db0) SHA1(5b23fcb6123f0402f7483ffd38625846b0432efa) )
	ROM_LOAD( "103-1219.bin",  0x000, 0x117, CRC(bf08d1e8) SHA1(5710cbc941830594087320ed2b22bcbb2b5c48de) )

	/* ----------------------------
       Layer 5
    ---------------------------- */

	ROM_REGION( 0x100000, "tex", 0 )
	ROM_LOAD16_BYTE( "tex0h.bin", 0x00001, 0x80000, CRC(b4dc459e) SHA1(cd2c6616951bdedeb5bb0c81179b4eee42ed5148) )
	ROM_LOAD16_BYTE( "tex0l.bin", 0x00000, 0x80000, CRC(370f9dca) SHA1(e26633f0f78f821fc59d8070ad927507f33d04d2) )
	ROM_LOAD16_BYTE( "tex1h.bin", 0x00001, 0x80000, CRC(e9224c54) SHA1(741f34cfad23d54c5bb70536489d661e7a505812) )
	ROM_LOAD16_BYTE( "tex1l.bin", 0x00000, 0x80000, CRC(0cb0c44b) SHA1(463736719dc0d00df8b22455effb17d00355ffd7) )
	ROM_LOAD16_BYTE( "tex2h.bin", 0x00001, 0x80000, CRC(f7096b75) SHA1(cf976039e8c0e4a5b5155fa4f108ae6a19712dc8) )
	ROM_LOAD16_BYTE( "tex2l.bin", 0x00000, 0x80000, CRC(33c47b1e) SHA1(20f6aff0ef31d71244ce3d0277d264fac7fa5229) )
	ROM_LOAD16_BYTE( "tex3h.bin", 0x00001, 0x80000, CRC(0cf25571) SHA1(08d78dfcb80742fb1f15b92e2f686b554109b01b) )
	ROM_LOAD16_BYTE( "tex3l.bin", 0x00000, 0x80000, CRC(56ab7d89) SHA1(0f88bba8b685f4cc7264ce042b3950dbaadbb495) )
	ROM_LOAD16_BYTE( "tex4h.bin", 0x00001, 0x80000, CRC(900d9ecf) SHA1(d0f0cb6dcfc25cd0ff10716d8242a82e4b000773) )
	ROM_LOAD16_BYTE( "tex4l.bin", 0x00000, 0x80000, CRC(d87d909c) SHA1(0eb26e37449ffd3d5a132189cdcbfc9129808c50) )
	ROM_LOAD16_BYTE( "tex5h.bin", 0x00001, 0x80000, CRC(7fcf4516) SHA1(4bd307afda9f499d26c9a802775bf57989fd7376) )
	ROM_LOAD16_BYTE( "tex5l.bin", 0x00000, 0x80000, CRC(da3fa060) SHA1(338dc2510a0fbb242c054b03264bd88ba9aa152f) )
	ROM_LOAD16_BYTE( "tex6h.bin", 0x00001, 0x80000, CRC(b6e6a0b3) SHA1(e302c997be5a612b37bb947245331d2f941df8fa) )
	ROM_LOAD16_BYTE( "tex6l.bin", 0x00000, 0x80000, CRC(c0b0638f) SHA1(11805da9efff04ec835042c91cec861a80f27b6d) )
	ROM_LOAD16_BYTE( "tex7h.bin", 0x00001, 0x80000, CRC(ef32fb90) SHA1(393c0f219ff550c9e9d60dbd872349d770c33580) )
	ROM_LOAD16_BYTE( "tex7l.bin", 0x00000, 0x80000, CRC(d9278afd) SHA1(6d86e0608cbb7697d63d3dbb74365d46be74074b) )

	ROM_REGION( 0x80000, "proms", 0 )
	ROM_LOAD( "blue.bpr",  0x000, 0x200, CRC(58e48ec5) SHA1(b2dfc6ad2b60fa3d9898c5d7a5fcc65af39117a1) )
	ROM_LOAD( "green.bpr", 0x000, 0x200, CRC(a80fd47d) SHA1(5bdffe022cbe2527c89c17f37e90e7e49c48be99) )
	ROM_LOAD( "red.bpr",   0x000, 0x200, CRC(bc8a3266) SHA1(87f98ea3657ae08ae80282c7940f00f31a407035) )

	ROM_REGION( 0x80000, "palslayer5", 0 )
	ROM_LOAD( "103-1106.bin",  0x000, 0x2e5, CRC(fd8a872e) SHA1(03f4033d77617d2c372f4656cb1cdb6ea6bd20d6) )
	ROM_LOAD( "103-1107.bin",  0x000, 0x2e5, CRC(c16e71fe) SHA1(6d1e9fa1894778381bb6b8954023e4d1816241bb) )
	ROM_LOAD( "103-1108.bin",  0x000, 0x2e5, CRC(b3680191) SHA1(36d006ca6bff5592a84fba812df43f2f5ddfe927) )
	ROM_LOAD( "103-1109.bin",  0x000, 0x2e5, CRC(ce803542) SHA1(9da822cedf0015399939524639fec9a0720626d4) )
	ROM_LOAD( "103-1111.bin",  0x000, 0x2e5, CRC(a48b2dc1) SHA1(18713aea428c8109fda4b6abec2ef0bb75f38078) )
	ROM_LOAD( "103-1112.bin",  0x000, 0x2e5, CRC(087bfb83) SHA1(330daf872e2e7dbdc30d65d74e487904e2091374) )
	ROM_LOAD( "103-1114.bin",  0x000, 0x117, CRC(47443136) SHA1(83ea193d9d10d74fed941d5a14dd84c8a03d229f) )
	ROM_LOAD( "103-1116.bin",  0x000, 0x117, CRC(37edc36c) SHA1(be53131c52e84cb3fe055af5ca4e2f6aa5442ff0) )
ROM_END

GAME( 1994, metalmx,    0,        metalmx,    metalmx,    0, ROT0,  "Atari", "Metal Maniax (prototype)", GAME_NO_SOUND | GAME_NOT_WORKING )
