/*
Double Dealer (c)NMK 1991

Based on jalmah.c and nmk.c drivers
Skeleton driver by Tomasz Slanina
Protection vectors provided by Angelo Salese

TODO:
-check Video Registers memory at 0x8c1e0-0x8c1ef.They appear to control cards movements,game should be in a playable state;
-finish the vector protection;
-implement coin simulation;
-merge this with the nmk16 driver.Appears to be similar to Hacha Mecha Fighter


--

pcb marked  GD91071

68000P10
YM2203C
91071-3 (????) marked as "3" on the pcb
NMK-110 8131 ( Mitsubishi M50747 MCU ?)
NMK 901
NMK 902
NMK 903 x2
82S135N ("5")
82S129N ("6")
xtals 16.000 MHz and  6.000 MHz
DSW x2

--

Few words about protection:

- Work RAM at $fe000 - $fffff is shared with MCU . Maybe whole $f0000-$fffff is shared ...
- After boot, game writes random-looking data to work RAM:

 00052C: 33FC 1234 000F E086        move.w  #$1234, $fe086.l
 000534: 33FC 5678 000F E164        move.w  #$5678, $fe164.l
 00053C: 33FC 9CA3 000F E62E        move.w  #$9ca3, $fe62e.l
 000544: 33FC ABA2 000F E734        move.w  #$aba2, $fe734.l
 00054C: 33FC B891 000F E828        move.w  #$b891, $fe828.l
 000554: 33FC C760 000F E950        move.w  #$c760, $fe950.l
 00055C: 33FC D45F 000F EA7C        move.w  #$d45f, $fea7c.l
 000564: 33FC E32E 000F ED4A        move.w  #$e32e, $fed4a.l

  Some (or maybe all ?) of above enables random generator at $fe010 - $fe017

- There's also MCU response (write/read/test) test just after these writes.
  (probably data used in the check depends on above writes). It's similar to
  jalmah.c tests, but num of responses is different, and  shared ram is
  used to communicate with MCU

- After last check (or maybe durning tests ... no idea)
  MCU writes $4ef900000604 (jmp $604) to $fe000 and game jumps to this address.

- code at $604  writes $20.w to $fe018 and $1.w to $fe01e.
  As result shared ram $fe000 - $fe007 is cleared.

  Also many, many other reads/writes  from/to shared mem.
  Few checks every interrupt:

    interrupt, lvl1

    000796: 007C 0700                  ori     #$700, SR
    00079A: 48E7 FFFE                  movem.l D0-D7/A0-A6, -(A7)
    00079E: 33FC 0001 000F E006        move.w  #$1, $fe006.l ; shared ram W (watchdog ?)
    0007A6: 4A79 000F E000             tst.w   $fe000.l ; shared ram R
    0007AC: 6600 0012                  bne     $7c0
    0007B0: 4A79 000F 02FE             tst.w   $f02fe.l
    0007B6: 6600 0008                  bne     $7c0
    0007BA: 4279 000F C880             clr.w   $fc880.l
+-0007C0: 6100 0236                  bsr     $9f8
| 0007C4: 4EB9 0003 0056             jsr     $30056.l
|   0007CA: 33FC 00FF 000F C880        move.w  #$ff, $fc880.l
|   0007D2: 007C 2000                  ori     #$2000, SR
|   0007D6: 4CDF 7FFF                  movem.l (A7)+, D0-D7/A0-A6
|   0007DA: 4E73                       rte
|
|
+->0009F8: 4A79 000F 02C0             tst.w   $f02c0.l
     0009FE: 6700 0072                  beq     $a72
     000A02: 4A79 000F 02F6             tst.w   $f02f6.l
     000A08: 6600 0068                  bne     $a72
     000A0C: 3439 000F E002             move.w  $fe002.l, D2 ; shared ram R
     000A12: 3602                       move.w  D2, D3
     000A14: 0242 00FF                  andi.w  #$ff, D2
     000A18: 0243 FF00                  andi.w  #$ff00, D3
     000A1C: E04B                       lsr.w   #8, D3
     000A1E: 3039 000F E000             move.w  $fe000.l, D0  ; shared ram R
     000A24: 3239 000F 0010             move.w  $f0010.l, D1
     000A2A: B041                       cmp.w   D1, D0
     000A2C: 6200 002A                  bhi     $a58
     000A30: 6600 002E                  bne     $a60
     000A34: 3039 000F 0012             move.w  $f0012.l, D0
     000A3A: B440                       cmp.w   D0, D2
     000A3C: 6200 001A                  bhi     $a58
     000A40: 6600 001E                  bne     $a60
     000A44: 3039 000F 0014             move.w  $f0014.l, D0
     000A4A: B640                       cmp.w   D0, D3
     000A4C: 6200 000A                  bhi     $a58
     000A50: 6600 000E                  bne     $a60
     000A54: 6000 001C                  bra     $a72
     000A58: 33FC 0007 000F C880        move.w  #$7, $fc880.l ; used later in the code...
     000A60: 33C0 000F 0010             move.w  D0, $f0010.l ;update mem, used in next test
     000A66: 33C2 000F 0012             move.w  D2, $f0012.l
     000A6C: 33C3 000F 0014             move.w  D3, $f0014.l
     000A72: 4E75                       rts

*/


#include "driver.h"
#include "cpu/m68000/m68000.h"

static UINT16 *mcu_shared_ram;
static UINT16 *sc3_vram,*sc0_vram;
static UINT16 *ddealer_vregs;
static tilemap *sc3_tilemap,*sc0_tilemap;

static int respcount;

static TILEMAP_MAPPER( bg_scan )
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0xff) << 4) + ((row & 0x70) << 8);
}

static TILE_GET_INFO( get_sc0_tile_info )
{
	int code = sc0_vram[tile_index];
	SET_TILE_INFO(
			1,
			code & 0xfff,
			code >> 12,
			0);
}

static TILE_GET_INFO( get_sc3_tile_info )
{
	int code = sc3_vram[tile_index];
	SET_TILE_INFO(
			0,
			code & 0xfff,
			code >> 12,
			0);
}

static VIDEO_START( ddealer )
{
	sc3_tilemap = tilemap_create(get_sc3_tile_info,tilemap_scan_cols,8,8,64,32);
	sc0_tilemap = tilemap_create(get_sc0_tile_info,bg_scan,16,16,256,32);

	tilemap_set_transparent_pen(sc0_tilemap,15);
}

#if 0
static void ddealer_protection(running_machine *machine)
{

	if(prot==0)
	{
		shared_ram[0xe000/2]=0x4ef9;
		shared_ram[0xe002/2]=0x0000;
		shared_ram[0xe004/2]=0x0796;
	}

	shared_ram[0xe008/2]=0x0001;
	shared_ram[0xe00a/2]=0x0001;
	shared_ram[0xe00c/2]=0x0001;
	shared_ram[0xe00e/2]=0x0001;

	shared_ram[0xe010/2]=mame_rand(machine) & 0xffff;
	shared_ram[0xe012/2]=mame_rand(machine) & 0xffff;
	shared_ram[0xe014/2]=mame_rand(machine) & 0xffff;
	shared_ram[0xe016/2]=mame_rand(machine) & 0xffff;

}
#endif

static VIDEO_UPDATE( ddealer )
{
	tilemap_set_scrollx( sc3_tilemap, 0, -64);

	tilemap_draw(bitmap,cliprect,sc3_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,sc0_tilemap,0,0);

	/*temporary*/
	if(input_code_pressed_once(KEYCODE_Z))
		mcu_shared_ram[0x000/2]++;
	return 0;
}

WRITE16_HANDLER( sc3_vram_w )
{
	int oldword = sc3_vram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		sc3_vram[offset] = newword;
		tilemap_mark_tile_dirty(sc3_tilemap,offset);
	}
}


WRITE16_HANDLER( sc0_vram_w )
{
	int oldword = sc0_vram[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		sc0_vram[offset] = newword;
		tilemap_mark_tile_dirty(sc0_tilemap,offset);
	}
}

WRITE16_HANDLER( ddealer_vregs_w )
{
	COMBINE_DATA(&ddealer_vregs[offset]);
}

#define PROT_JSR(_offs_,_protvalue_,_pc_) \
	if(mcu_shared_ram[(_offs_)/2] == _protvalue_) \
	{ \
		mcu_shared_ram[(_offs_)/2] = 0xffff;  /*(MCU job done)*/ \
		mcu_shared_ram[(_offs_+2-0x10)/2] = 0x4ef9;/*JMP*/\
		mcu_shared_ram[(_offs_+4-0x10)/2] = 0x0000;/*HI-DWORD*/\
		mcu_shared_ram[(_offs_+6-0x10)/2] = _pc_;  /*LO-DWORD*/\
	} \

#define PROT_INPUT(_offs_,_protvalue_,_protinput_,_input_) \
	if(mcu_shared_ram[_offs_] == _protvalue_) \
	{\
		mcu_shared_ram[_protinput_] = ((_input_ & 0xffff0000)>>16);\
		mcu_shared_ram[_protinput_+1] = (_input_ & 0x0000ffff);\
	}

static WRITE16_HANDLER( ddealer_mcu_shared_w )
{
	COMBINE_DATA(&mcu_shared_ram[offset]);

	switch(offset)
	{
		/*
            shared_ram[0xe490/2]=0x4ef9;
            shared_ram[0xe492/2]=0x0000;
            shared_ram[0xe494/2]=0x9696;// write to tx tilemap
            shared_ram[0xe4a0/2]=0x4ef9;
            shared_ram[0xe4a2/2]=0x0000;
            shared_ram[0xe4a4/2]=0x95fe;// write to bg tilemap,might be 0x9634
            shared_ram[0xe4c0/2]=0x4ef9;
            shared_ram[0xe4c2/2]=0x0000;
            shared_ram[0xe4c4/2]=0x9634;// unsure about this,tx tilemap
            shared_ram[0xe4e0/2]=0x4ef9;
            shared_ram[0xe4e2/2]=0x0000;
            shared_ram[0xe4e4/2]=0x5ac6;//palette ram buffer
        */
		case 0x086/2: PROT_INPUT(0x086/2,0x1234,0x100/2,0x80000); break;
		case 0x164/2: PROT_INPUT(0x164/2,0x5678,0x104/2,0x80002); break;
		case 0x62e/2: PROT_INPUT(0x62e/2,0x9ca3,0x108/2,0x80008); break;
		case 0x734/2: PROT_INPUT(0x734/2,0xaba2,0x10c/2,0x8000a); break;
 //00054C: 33FC B891 000F E828        move.w  #$b891, $fe828.l
 //000554: 33FC C760 000F E950        move.w  #$c760, $fe950.l
 //00055C: 33FC D45F 000F EA7C        move.w  #$d45f, $fea7c.l
 //000564: 33FC E32E 000F ED4A        move.w  #$e32e, $fed4a.l
//006992-7348-7518
/*
fe400->score sub-routine
fe410-><unused>
fe420->called before you enter into the gameplay,cards initialize? a39a will break lots of things
fe430->called in some circumstances after 420 (ASM-wise)
fe440->called during attract

fe470->called when it should update the graphics
A0 = f3072
A2 = 9cf4c
A3 = 9ce9c

6176 is the player 2

work ram addresses
f031e->
f3010->mirror for inputs player 1
f5010->mirror for inputs player 2
*/

		case 0x40e/2: PROT_JSR(0x40e,0x8011,0x6992);//0x89e); break;//0x6992); break;
		case 0x41e/2: break;//unused
		case 0x42e/2: PROT_JSR(0x42e,0x8007,0x6004); break;//0x89e); break;//0xa39a); break;
		case 0x43e/2: PROT_JSR(0x43e,0x801d,0x89e);  break;//0x62f2); break;//0x6176); break;//0x6ebe); break;//0x9b6);  break;//801d
		case 0x44e/2: PROT_JSR(0x44e,0x8028,0x68f6); break;
		case 0x45e/2: PROT_JSR(0x45e,0x803e,0x6f90); break;
		case 0x46e/2: PROT_JSR(0x46e,0x8033,0x93c2); break;
		case 0x47e/2: PROT_JSR(0x47e,0x8026,0x89e);  break;//0x6786); break; //wrong,just to let pass the check
		case 0x48e/2: PROT_JSR(0x48e,0x8012,0x6176); break;
/**/	case 0x49e/2: PROT_JSR(0x49e,0x8004,0x9696); break;
		case 0x4ae/2: PROT_JSR(0x4ae,0x8035,0x95fe); break;
		case 0x4be/2: PROT_JSR(0x4be,0x8009,0x89e);  break;//0xa74);  break; //wrong
/**/	case 0x4ce/2: PROT_JSR(0x4ce,0x802a,0x9656); break;//z
		case 0x4de/2: PROT_JSR(0x4de,0x803b,0x96c2); break;
/**/	case 0x4ee/2: PROT_JSR(0x4ee,0x800c,0x5ca4); break;//correct
		case 0x4fe/2: PROT_JSR(0x4fe,0x8018,0x6004); break;//0x9818); break;//6004?
		case 0x000/2:
			if(mcu_shared_ram[0x000/2] == 0x60fe)
			{
				mcu_shared_ram[0x000/2] = 0x0000;
				mcu_shared_ram[0x002/2] = 0x0000;
				mcu_shared_ram[0x004/2] = 0x4ef9;
//              mcu_shared_ram[0x006/2] = 0x0000;
//              mcu_shared_ram[0x008/2] = 0x0000;
//              mcu_shared_ram[0x00a/2] = 0x0000;
//              mcu_shared_ram[0x00c/2] = 0x0000;
//              mcu_shared_ram[0x00e/2] = 0x0000;
			}
			break;
		case 0x002/2:
		case 0x004/2:
			if(mcu_shared_ram[0x002/2] == 0x0000 && mcu_shared_ram[0x004/2] == 0x0214) //<- he believes to be astute...
				mcu_shared_ram[0x004/2] = 0x4ef9;//0604
			break;
		case 0x008/2:
			if(mcu_shared_ram[0x008/2] == 0x000f)
				mcu_shared_ram[0x008/2] = 0x0604;
			break;
		case 0x00c/2:
			if(mcu_shared_ram[0x00c/2] == 0x000f)
				mcu_shared_ram[0x00c/2] = 0x0000;
	}
}

static ADDRESS_MAP_START( ddealer, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ_PORT("IN0")
	AM_RANGE(0x080002, 0x080003) AM_READ_PORT("IN1")
	AM_RANGE(0x080008, 0x080009) AM_READ_PORT("DSW1")
	AM_RANGE(0x08000a, 0x08000b) AM_READ_PORT("UNK")
	AM_RANGE(0x084000, 0x084003) AM_RAM // ym ?
	AM_RANGE(0x088000, 0x0887ff) AM_RAM_WRITE(paletteram16_RRRRGGGGBBBBRGBx_word_w) AM_BASE(&paletteram16) // palette ram
	AM_RANGE(0x08c000, 0x08cfff) AM_RAM_WRITE(ddealer_vregs_w) AM_BASE(&ddealer_vregs) // palette ram
	AM_RANGE(0x090000, 0x093fff) AM_RAM_WRITE(sc0_vram_w) AM_BASE(&sc0_vram) // bg tilemap
	AM_RANGE(0x09c000, 0x09ffff) AM_RAM_WRITE(sc3_vram_w) AM_BASE(&sc3_vram) // fg tilemap
	AM_RANGE(0x0f0000, 0x0fdfff) AM_RAM
	AM_RANGE(0x0fe000, 0x0fefff) AM_RAM_WRITE(ddealer_mcu_shared_w) AM_BASE(&mcu_shared_ram)
	AM_RANGE(0x0ff000, 0x0fffff) AM_RAM
ADDRESS_MAP_END

/*copied from hachamf*/
static INPUT_PORTS_START( ddealer )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) //bryan:  test mode in some games?

	PORT_START("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START("DSW1")
	PORT_DIPNAME( 0x0700, 0x0700, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0100, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0200, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0300, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0700, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0600, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0500, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0400, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x3800, 0x3800, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0800, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x1000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x3800, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x3000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x2800, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x2000, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Japanese ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
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

	PORT_START("UNK")
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static const gfx_layout tilelayout =
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

static GFXDECODE_START( ddealer )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout, 0, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, tilelayout, 0x100, 16 )
GFXDECODE_END

static MACHINE_RESET (ddealer)
{
	respcount = 0;
}

static INTERRUPT_GEN( ddealer_interrupt )
{
	cpunum_set_input_line(machine, 0, 4, HOLD_LINE);
}

static MACHINE_DRIVER_START( ddealer )
	MDRV_CPU_ADD("main" , M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(ddealer,0)
	MDRV_CPU_VBLANK_INT("main", ddealer_interrupt)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold, 112)

	MDRV_GFXDECODE(ddealer)


	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 48*8-1, 2*8, 30*8-1)

	MDRV_PALETTE_LENGTH(0x400)
	MDRV_MACHINE_RESET(ddealer)

	MDRV_VIDEO_START(ddealer)
	MDRV_VIDEO_UPDATE(ddealer)

	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END



static READ16_HANDLER( ddealer_mcu_r )
{
	static const int resp[] =
	{
		0x93, 0xc7, 0x00, 0x8000,
		0x2d, 0x6d, 0x00, 0x8000,
		0x99, 0xc7, 0x00, 0x8000,
		0x2a, 0x6a, 0x00, 0x8000,
		0x8e, 0xc7, 0x00, 0x8000,
	-1};

	int res;

	res = resp[respcount++];
	if (resp[respcount]<0)
	{
		 respcount = 0;
		// ddealer_protection(machine);

	}
	return res;
}

/*static WRITE16_HANDLER( ddealer_mcu_w )
{
    if(data==1)
    {
        prot=1;
        shared_ram[0xe000/2]=0;
        shared_ram[0xe002/2]=0;
        shared_ram[0xe004/2]=0;
        shared_ram[0xe006/2]=0;
    }
}
*/
static DRIVER_INIT( ddealer )
{
	memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfe01c, 0xfe01d, 0, 0, ddealer_mcu_r );
//  memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfe01e, 0xfe01f, 0, 0, ddealer_mcu_w );
}

ROM_START( ddealer )
	ROM_REGION( 0x40000, "main", 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "1.ic6", 0x00001, 0x20000, CRC(ce0dff50) SHA1(2d7a03f6b9609aea7511a4dc49560a901b0b9f19) )
	ROM_LOAD16_BYTE( "2.ic28", 0x00000, 0x20000, CRC(f00c346f) SHA1(bd73efb19d5f9efc88210d92a82a3f4595b41097) )

	ROM_REGION( 0x20000, "gfx1", 0 ) /* BG0 */
	ROM_LOAD( "4.ic65", 0x00000, 0x20000, CRC(4939ff1b) SHA1(af2f2feeef5520d775731a58cbfc8fcc913b7348) )

	ROM_REGION( 0x80000, "gfx2", 0 ) /* BG1 */
	ROM_LOAD( "3.ic64", 0x00000, 0x80000, CRC(660e367c) SHA1(54827a8998c58c578c594126d5efc18a92363eaa))

	ROM_REGION( 0x200, "user1", 0 ) /* Proms */
	ROM_LOAD( "5.ic67", 0x000, 0x100, NO_DUMP )
	ROM_LOAD( "6.ic86", 0x100, 0x100, NO_DUMP )
ROM_END

GAME( 1991, ddealer,  0, ddealer, ddealer, ddealer,  ROT0, "NMK", "Double Dealer", GAME_NOT_WORKING | GAME_NO_SOUND)

