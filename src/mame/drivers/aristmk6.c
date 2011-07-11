/*
Aristocrat MK6 hardware

  SH4 + PowerVR

GFX data looks 8bpp with external palette

Font is at 0x160000 onward in the BIOS set, in raw format
*/

#include "emu.h"
#include "cpu/sh4/sh4.h"


class aristmk6_state : public driver_device
{
public:
	aristmk6_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT32 m_test_x,m_test_y,m_start_offs;
	UINT8 m_type;
};


VIDEO_START(aristmk6)
{
}

SCREEN_UPDATE(aristmk6)
{
	aristmk6_state *state = screen->machine().driver_data<aristmk6_state>();

	int x,y,count;
	const UINT8 *blit_ram = screen->machine().region("maincpu")->base();

	if(screen->machine().input().code_pressed(KEYCODE_Z))
		state->m_test_x++;

	if(screen->machine().input().code_pressed(KEYCODE_X))
		state->m_test_x--;

	if(screen->machine().input().code_pressed(KEYCODE_A))
		state->m_test_y++;

	if(screen->machine().input().code_pressed(KEYCODE_S))
		state->m_test_y--;

	if(screen->machine().input().code_pressed(KEYCODE_Q))
		state->m_start_offs+=0x2000;

	if(screen->machine().input().code_pressed(KEYCODE_W))
		state->m_start_offs-=0x2000;

	if(screen->machine().input().code_pressed(KEYCODE_E))
		state->m_start_offs++;

	if(screen->machine().input().code_pressed(KEYCODE_R))
		state->m_start_offs--;

	if(screen->machine().input().code_pressed_once(KEYCODE_L))
		state->m_type^=1;

	popmessage("%d %d %04x %d",state->m_test_x,state->m_test_y,state->m_start_offs,state->m_type);

	bitmap_fill(bitmap,cliprect,get_black_pen(screen->machine()));

	count = (state->m_start_offs);

	for(y=0;y<state->m_test_y;y++)
	{
		for(x=0;x<state->m_test_x;x++)
		{
			if(state->m_type)
			{
				UINT16 vram;
				int r,g,b;

				vram = blit_ram[count+0] | blit_ram[count+1]<<8;

				r = (vram & 0x001f)>>0;
				g = (vram & 0x07e0)>>5;
				b = (vram & 0xf800)>>11;

				r = (r << 3) | (r & 0x7);
				g = (g << 2) | (g & 3);
				b = (b << 3) | (b & 0x7);

				if((x)<screen->visible_area().max_x && ((y)+0)<screen->visible_area().max_y)
					*BITMAP_ADDR32(bitmap, y, x) = r | g<<8 | b<<16;

				count+=2;
			}
			else
			{
				UINT8 color;

				color = blit_ram[count];

				if((x)<screen->visible_area().max_x && ((y)+0)<screen->visible_area().max_y)
					*BITMAP_ADDR32(bitmap, y, x) = screen->machine().pens[color];

				count++;
			}
		}
	}

	return 0;
}

static ADDRESS_MAP_START( aristmk6_map, AS_PROGRAM, 64 )
	AM_RANGE(0x00000000, 0x003fffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x0c000000, 0x0cffffff) AM_RAM // work ram
ADDRESS_MAP_END

static ADDRESS_MAP_START( aristmk6_port, AS_IO, 64 )
ADDRESS_MAP_END


static INPUT_PORTS_START( aristmk6 )
INPUT_PORTS_END

// ?
#define ARISTMK6_CPU_CLOCK XTAL_200MHz
// ?
static const struct sh4_config sh4cpu_config = {  1,  0,  1,  0,  0,  0,  1,  1,  0, ARISTMK6_CPU_CLOCK };

static MACHINE_CONFIG_START( aristmk6, aristmk6_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", SH4, ARISTMK6_CPU_CLOCK)
	MCFG_CPU_CONFIG(sh4cpu_config)
	MCFG_CPU_PROGRAM_MAP(aristmk6_map)
	MCFG_CPU_IO_MAP(aristmk6_port)
	MCFG_DEVICE_DISABLE()

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))  /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(aristmk6)

	MCFG_PALETTE_LENGTH(0x1000)

	MCFG_VIDEO_START(aristmk6)
MACHINE_CONFIG_END

ROM_START( aristmk6 )
	ROM_REGION( 0x0400000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("right.bin", 0x0000000, 0x0200000, CRC(bf21a975) SHA1(a251b1a7342387300689cd50fe4ce7975b903ac5) )
	ROM_LOAD32_WORD("left.bin",  0x0000002, 0x0200000, CRC(c02e14b0) SHA1(6bf98927813519dfe60e582dbe5be3ccd87f7c91) )
ROM_END

// note the 32-bit checksums listed don't appear to be CRC32s (or everything is badly dumped?)
// they apparently come from the eeprom programmer, the chips are marked with the game ID number instead

ROM_START( antcleo )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86 - 516be56e.bin", 0x0000000, 0x0400000, CRC(4897f4ed) SHA1(0a071528b0c2cb4c42d4535bed406849a6187d9d) )
	ROM_LOAD32_WORD("u73 - 3e37a4c6.bin", 0x0000002, 0x0400000, CRC(41b7d75d) SHA1(5c25e0bc65560b17b80c4430ae9d925a0f245e6c) )
	ROM_LOAD32_WORD("u85 - e9f4a69d.bin", 0x0800000, 0x0400000, CRC(909a5a6c) SHA1(abb86f82184f32fad578d5c3a6d034afaa78e3c3) )
	ROM_LOAD32_WORD("u72 - e98c941e.bin", 0x0800002, 0x0400000, CRC(121751b2) SHA1(caf5d7c2d3774575ef0a12146cbdd72d0a421c58) )
ROM_END

ROM_START( 50lions )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86 - 570ba079.bin", 0x0000000, 0x0400000, CRC(c3791531) SHA1(b9c60be9624463eb591f2baf421ff90b8763449b) )
	ROM_LOAD32_WORD("u73 - 35634c6d.bin", 0x0000002, 0x0400000, CRC(ec1b699b) SHA1(5a6ad7c7eb02443e42ee6a88525ae95a2b0a3195) )
ROM_END

ROM_START( 50lionsa )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("10120511.u86", 0x0000000, 0x0400000, CRC(0e5c86f1) SHA1(84e329e664ace697f9ea4ace08612089e0964732) )
	ROM_LOAD32_WORD("10120511.u73", 0x0000002, 0x0400000, CRC(1c1f2297) SHA1(13fb8c83d8ce2340ef554490c21a38da7b47c666) )
ROM_END

ROM_START( choysun )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86.bin", 0x0000000, 0x0400000, CRC(06f78c92) SHA1(e8bd3f18831dfb5c644321541fa9e75ae9e83688) )
	ROM_LOAD32_WORD("u73.bin", 0x0000002, 0x0400000, CRC(5b2468b6) SHA1(085aa44343f11fdf5ab7cc1ca56ddb0ba5cafc36) )
	ROM_LOAD32_WORD("u85.bin", 0x0800000, 0x0400000, CRC(6973dffd) SHA1(4350e0cdfeb9135e708f15bf2de325b8412c1434) )
	ROM_LOAD32_WORD("u72.bin", 0x0800002, 0x0400000 ,CRC(1c9a77d1) SHA1(ef31215be73b09531039fda1a2f3e875045180f3) )
ROM_END

ROM_START( cristals )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86 - (cdd5955b).bin", 0x0000000, 0x0400000, CRC(b046ea06) SHA1(0c0310bc0afb8bac630ac0570d5b9df6a992cfdb) )
	ROM_LOAD32_WORD("u73 - (b0d2564e).bin", 0x0000002, 0x0400000, CRC(b52cac8a) SHA1(65bb5d73933df6d53a079e4efe00ea29649e3201) )
ROM_END

ROM_START( indianmm )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("indianmm.u86", 0x0000000, 0x0400000, CRC(db13eaf5) SHA1(c2e743b72c2a280266d55642e40c3a7a740052db) )
	ROM_LOAD32_WORD("indianmm.u73", 0x0000002, 0x0400000, CRC(a5e3dca5) SHA1(e585841064dc98398169bcd0cd04269bbcfaf77c) )
	ROM_LOAD32_WORD("indianmm.u85", 0x0800000, 0x0400000, CRC(988f10da) SHA1(9c21cb8ebebcd603b25329331de89e9aaa36368a) )
	ROM_LOAD32_WORD("indianmm.u72", 0x0800002, 0x0400000, CRC(efe15e58) SHA1(8ae7b1436664189a5d07f523d41611bca6d1936c) )
ROM_END

ROM_START( wheregld )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86.bin", 0x0000000, 0x0400000, CRC(d09fd746) SHA1(6c805f7c535a6c08a836b94c0351d498751ac9e9) )
	ROM_LOAD32_WORD("u73.bin", 0x0000002, 0x0400000, CRC(3007f8d1) SHA1(a76a227b8157d80b08e224807345a56da4c326fd) )
	ROM_LOAD32_WORD("u85.bin", 0x0800000, 0x0400000, CRC(fb8c24e7) SHA1(ffa62c59f1857dcd670c733397d20c50710d9a85) )
	ROM_LOAD32_WORD("u72.bin", 0x0800002, 0x0400000, CRC(9d0b5bdf) SHA1(0018c93df75b9a54f43d660a24e00dc801a68084) )
	ROM_LOAD32_WORD("u84.bin", 0x1000000, 0x0400000, CRC(be31366b) SHA1(a82fb5bd41079c6727a93115fa74af5fe44ba4a9) )
	ROM_LOAD32_WORD("u71.bin", 0x1000002, 0x0400000, CRC(6781f074) SHA1(d99a6b0a86317e7c8eb79d362338eb39b58aed54) )
ROM_END

ROM_START( whalecsh )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86.bin", 0x0000000, 0x0400000, CRC(11bcb378) SHA1(56de7fee7631c2e468a1f1845ff9d74db56051f0) )
	ROM_LOAD32_WORD("u73.bin", 0x0000002, 0x0400000, CRC(3b6d2292) SHA1(87e50f3ed6629c697cff59ec425b098704450993) )
	ROM_LOAD32_WORD("u85.bin", 0x0800000, 0x0400000, CRC(50afc633) SHA1(ee237d806044bbab3f17210e4e668a8f0961ad92) )
	ROM_LOAD32_WORD("u72.bin", 0x0800002, 0x0400000, CRC(e799e162) SHA1(aad5d6bff8afcb7f208326c4a663e8462d753f9f) )
ROM_END

ROM_START( wildways )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("u86 - (00257141).bin", 0x0000000, 0x0400000, CRC(2968765c) SHA1(ba2c67c4be4063d8506cc8127c31b4df2609650b) )
	ROM_LOAD32_WORD("u73 - (e7e29273).bin", 0x0000002, 0x0400000, CRC(a1e0d77e) SHA1(df4d45d8c4dcfdb1fae4b5d5a0adfa0464c61828) )
	ROM_LOAD32_WORD("u85 - (101d515b).bin", 0x0800000, 0x0400000, CRC(d87426d5) SHA1(ba755f8fc426dcd8abf4f6ccee423ae0504bf6fe) )
	ROM_LOAD32_WORD("u72 - (0f0e787b).bin", 0x0800002, 0x0400000, CRC(5f2c30d1) SHA1(dfc68903e8375597477acc2fc403dcd9b838ae6c) )
ROM_END

ROM_START( thaiprin )
	ROM_REGION( 0x1800000, "maincpu", ROMREGION_ERASEFF)
	ROM_LOAD32_WORD("thai princess (u86] -27v322.bin", 0x0000000, 0x0400000, CRC(3cb5124b) SHA1(70f6d66793c433054557da4c9f2f033bbb640bd6) )
	ROM_LOAD32_WORD("thai princess (u73] -27v322.bin", 0x0000002, 0x0400000, CRC(531f05ab) SHA1(929285de219c033bdee5f8011e9a0a07b344375e) )
	ROM_LOAD32_WORD("thai princess (u85) -27v322.bin", 0x0800000, 0x0400000, CRC(90c345e0) SHA1(1cf5b237eca68749a7baa147b9b15b3e139d7951) )
	ROM_LOAD32_WORD("thai princess (u72) -27v322.bin", 0x0800002, 0x0100000, BAD_DUMP CRC(613cea6b) SHA1(f04a3ee53074b7cd84879d752df5dbb80437475e) ) // wrong size?!
ROM_END


GAME( 200?, aristmk6,  0,          aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Aristocrat MK-6 Base (15011025)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_IS_BIOS_ROOT)
GAME( 200?, antcleo,   aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Antony and Cleopatra (10177211)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, 50lions,   aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "50 Lions (set 1, 10156111)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, 50lionsa,  50lions,    aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "50 Lions (set 2, 10120511)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, choysun,   aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Choy Sun Doa (20131511)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, cristals,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Cristal Springs (10155811)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, indianmm,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Indian Dreaming (10130711) (Maximillion$ Jackpot Version)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, wheregld,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Wheres The Gold (20177111)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, whalecsh,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Whales Of Cash (20155711)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, wildways,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Wild Ways (10130111)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 200?, thaiprin,  aristmk6,   aristmk6,    aristmk6,    0, ROT0, "Aristocrat", "Thai Princess (30127721)", GAME_NOT_WORKING | GAME_NO_SOUND )

/* Other Games on this HW (are any dumped?)

    AFRICAN DUSK
    GOLDEN PYRAMIDS (also on Mk5)
    SPRING CARNIVAL
    LOUISE GOLD
    BIG RED
    KING OF THE NILE
    GEISHA
    MR CASHMAN
    DOLLAR TREE
    MISS KITTY
    GRIZZLY
    MONEY TREE
    CALL OF THE WILD
    RED BARON
    MYSTIC MERMAID
    KOLA MINT
    LOVE STRUCK
    ENCHANTRESS
    ROLL UP ROLL UP
    VENETIAN
    POMPE11

*/
