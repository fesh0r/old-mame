/***************************************************************************

Nintendo VS UniSystem and DualSystem - (c) 1984 Nintendo of America

	Portions of this code are heavily based on
	Brad Oliver's MESS implementation of the NES.

RP2C04-001:
- Baseball
- Gradius
- Hogan's Alley
- Mach Rider (Japan, Fighting Course)
- Pinball
- Platoon
- Super Xevious

RP2C04-002:
- Castlevania
- Ladies golf
- Mach Rider (Endurance Course)
- Raid on Bungeling Bay (Japan)
- Slalom
- Stroke N' Match Golf
- Wrecking Crew

RP2C04-003:
- Dr mario
- Excite Bike
- Goonies
- Soccer
- TKO Boxing

RP2c05-004:
- Clu Clu Land
- Excite Bike (Japan)
- Ice Climber
- Ice Climber Dual (Japan)
- Super Mario Bros.

Rcp2c03b:
- Battle City
- Duck Hunt
- Mahjang
- Pinball (Japan)
- Rbi Baseball
- Star Luster
- Stroke and Match Golf (Japan)
- Super Skykid
- Tennis
- Tetris

RC2C05-01:
- Ninja Jajamaru Kun (Japan)

RC2C05-02:
- Mighty Bomb Jack (Japan)

RC2C05-03:
- Gumshoe

RC2C05-04:
- Top Gun

Needed roms:
- Babel no Tou								(by Namco, 198?)
- Family Boxing								(by Namco/Woodplace, 198?)
- Family Stadium '87						(by Namco, 1987)
- Family Stadium '88						(by Namco, 1988)
- Family Tennis								(by Namco, 198?)
- Japanese version of Vs. Tennis			(1984)
- Japanese version of Vs. Soccer			(1985)
- Japanese version of Vs. Super Mario Bros. (1986)
- Madoula no Tsubasa						(by Sunsoft, 198?)
- Pro Yakyuu Family Stadium					(by Namco, 1986?)
- Quest of Ki								(by Namco/Game Studio, 198?)
- Skate Kids								(by Two-Bit Score, 198?; hack of Vs. Super Mario Bros.)
- Sky Kid									(by Namco, 1985; marquee at Klov.com)
- Super Chinese								(by Namco/Culture Brain, 1988)
- Trojan									(by Capcom, 1987)
- Urban Champion							(1984)
- Volleyball								(1986)
- Walkure no Bouken							(by Namco, 198?)
- Wild Gunman								(1984, light gun game)
- Xevious


TO DO:
	- Fix some mirroring in games with ppuRC2C05_protection (jajamaru, topgun, ...)
	- Add unknown dip-switches, some of them could be Bonus or Difficulty
	- Test cstlevna bonus dip, iceclimb bear dip
	- Check others bits in coin counter
	- Check other values in bnglngby irq
	- Top Gun: cpu #0 (PC=00008016): unmapped memory byte read from 00007FFF ???

Changes:

  24/12/2002 Pierpaolo Prazzoli

  - Added
		- Vs. Mighty Bomb Jack (Japan)
		- Vs. Ninja Jajamaru Kun (Japan)
		- Vs. Raid on Bungeling Bay (Japan)
		- Vs. Top Gun
		- Vs. Mach Rider (Japan, Fighting Course Version)
		- Vs. Ice Climber (Japan)
		- Vs. Gumshoe (partially working)
		- Vs. Freedom Force (not working)
		- Vs. Stroke and Match Golf (Men set 2) (not working)
		- Vs. BaseBall (Japan set 3) (not working)
  - Added coin counter
  - Added Extra Ram in vstetris
  - Added Demo Sound in vsmahjng
  - Fixed vsskykid inputs
  - Fixed protection in Super Xevious (?)
  - Corrected or checked dip-switches in Castlevania, Duck Hunt, Excitebike,
	Gradius, Hogan's Alley, Ice Climber, R.B.I. Baseball, Slalom, Soccer,
	Super Mario Bros., Top Gun, BaseBall, Tennis, Stroke and Match Golf

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/ppu2c03b.h"
#include "machine/rp5h01.h"

/* clock frequency */
#define N2A03_DEFAULTCLOCK ( 21477272.724 / 12 )

#define DUAL_RBI 1

/* from vidhrdw */
extern VIDEO_START( vsnes );
extern PALETTE_INIT( vsnes );
extern VIDEO_UPDATE( vsnes );
extern VIDEO_START( vsdual );
extern VIDEO_UPDATE( vsdual );
extern PALETTE_INIT( vsdual );

/* from machine */
extern MACHINE_INIT( vsnes );
extern MACHINE_INIT( vsdual );
extern DRIVER_INIT( vsnes );
extern DRIVER_INIT( suprmrio );
extern DRIVER_INIT( excitebk );
extern DRIVER_INIT( excitbkj );
extern DRIVER_INIT( vsnormal );
extern DRIVER_INIT( duckhunt );
extern DRIVER_INIT( hogalley );
extern DRIVER_INIT( goonies );
extern DRIVER_INIT( machridr );
extern DRIVER_INIT( vsslalom );
extern DRIVER_INIT( cstlevna );
extern DRIVER_INIT( drmario );
extern DRIVER_INIT( rbibb );
extern DRIVER_INIT( tkoboxng );
extern DRIVER_INIT( topgun );
extern DRIVER_INIT( vsgradus );
extern DRIVER_INIT( vspinbal );
extern DRIVER_INIT( vsskykid );
extern DRIVER_INIT( platoon );
extern DRIVER_INIT( vstennis );
extern DRIVER_INIT( wrecking );
extern DRIVER_INIT( balonfgt );
extern DRIVER_INIT( vsbball );
extern DRIVER_INIT( iceclmrj );
extern DRIVER_INIT( xevious );
extern DRIVER_INIT( btlecity );
extern DRIVER_INIT( vstetris );
extern DRIVER_INIT( bnglngby );
extern DRIVER_INIT( jajamaru);
extern DRIVER_INIT( vsgshoe );
extern DRIVER_INIT( vsfdf );
extern DRIVER_INIT( mightybj);

extern READ_HANDLER( vsnes_in0_r );
extern READ_HANDLER( vsnes_in1_r );
extern READ_HANDLER( vsnes_in0_1_r );
extern READ_HANDLER( vsnes_in1_1_r );
extern WRITE_HANDLER( vsnes_in0_w );
extern WRITE_HANDLER( vsnes_in0_1_w );

/******************************************************************************/

/* local stuff */
static UINT8 *work_ram, *work_ram_1;
static int coin;

static READ_HANDLER( mirror_ram_r )
{
	return work_ram[ offset & 0x7ff ];
}

static READ_HANDLER( mirror_ram_1_r )
{
	return work_ram[ offset & 0x7ff ];
}

static WRITE_HANDLER( mirror_ram_w )
{
	work_ram[ offset & 0x7ff ] = data;
}

static WRITE_HANDLER( mirror_ram_1_w )
{
	work_ram[ offset & 0x7ff ] = data;
}

static WRITE_HANDLER( sprite_dma_w )
{
	int source = ( data & 7 ) * 0x100;

	ppu2c03b_spriteram_dma( 0, &work_ram[source] );
}

static WRITE_HANDLER( sprite_dma_1_w )
{
	int source = ( data & 7 ) * 0x100;

	ppu2c03b_spriteram_dma( 1, &work_ram_1[source] );
}

static WRITE_HANDLER( vsnes_coin_counter_w )
{
	coin_counter_w( 0, data & 0x01 );
	coin = data;
	if( data & 0xfe ) //"bnglngby" and "cluclu"
	{
		//do something?
		logerror("vsnes_coin_counter_w: pc = 0x%04x - data = 0x%02x\n", activecpu_get_pc(), data);
	}
}

static READ_HANDLER( vsnes_coin_counter_r )
{
	//only for platoon
	return coin;
}

static WRITE_HANDLER( vsnes_coin_counter_1_w )
{
	coin_counter_w( 1, data & 0x01 );
	if( data & 0xfe ) //vsbball service mode
	{
		//do something?
		logerror("vsnes_coin_counter_1_w: pc = 0x%04x - data = 0x%02x\n", activecpu_get_pc(), data);
	}

}
/******************************************************************************/

static MEMORY_READ_START (readmem)
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x1fff, mirror_ram_r },
	{ 0x2000, 0x3fff, ppu2c03b_0_r },
	{ 0x4000, 0x4015, NESPSG_0_r },
	{ 0x4016, 0x4016, vsnes_in0_r },
	{ 0x4017, 0x4017, vsnes_in1_r },
	{ 0x4020, 0x4020, vsnes_coin_counter_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START (writemem)
	{ 0x0000, 0x07ff, MWA_RAM, &work_ram },
	{ 0x0800, 0x1fff, mirror_ram_w },
	{ 0x2000, 0x3fff, ppu2c03b_0_w },
	{ 0x4011, 0x4011, DAC_0_data_w },
	{ 0x4014, 0x4014, sprite_dma_w },
	{ 0x4000, 0x4015, NESPSG_0_w },
	{ 0x4016, 0x4016, vsnes_in0_w },
	{ 0x4017, 0x4017, MWA_NOP }, /* in 1 writes ignored */
	{ 0x4020, 0x4020, vsnes_coin_counter_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START (readmem_1)
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x1fff, mirror_ram_1_r },
	{ 0x2000, 0x3fff, ppu2c03b_1_r },
	{ 0x4000, 0x4015, NESPSG_0_r },
	{ 0x4016, 0x4016, vsnes_in0_1_r },
	{ 0x4017, 0x4017, vsnes_in1_1_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START (writemem_1)
	{ 0x0000, 0x07ff, MWA_RAM, &work_ram_1 },
	{ 0x0800, 0x1fff, mirror_ram_1_w },
	{ 0x2000, 0x3fff, ppu2c03b_1_w },
	{ 0x4011, 0x4011, DAC_1_data_w },
	{ 0x4014, 0x4014, sprite_dma_1_w },
	{ 0x4000, 0x4015, NESPSG_1_w },
	{ 0x4016, 0x4016, vsnes_in0_1_w },
	{ 0x4017, 0x4017, MWA_NOP }, /* in 1 writes ignored */
	{ 0x4020, 0x4020, vsnes_coin_counter_1_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

/******************************************************************************/

#define VS_CONTROLS( SELECT_IN0, START_IN0, SELECT_IN1, START_IN1 ) \
	PORT_START	/* IN0 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, SELECT_IN0 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, START_IN0 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) \
\
	PORT_START	/* IN1 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, SELECT_IN1 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, START_IN1 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
\
	PORT_START	/* IN2 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */ \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define VS_CONTROLS_REVERSE( SELECT_IN0, START_IN0, SELECT_IN1, START_IN1 ) \
	PORT_START	/* IN0 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, SELECT_IN0 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, START_IN0 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
 	\
	PORT_START	/* IN1 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, SELECT_IN1 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, START_IN1 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) \
 	\
	PORT_START	/* IN2 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */ \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define VS_ZAPPER \
PORT_START	/* IN0 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )	/* sprite hit */ \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* gun trigger */ \
	\
	PORT_START	/* IN1 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	\
	PORT_START	/* IN2 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */ \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

#define VS_DUAL_CONTROLS_L \
	PORT_START	/* IN0 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START3 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) \
 	\
	PORT_START	/* IN1 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START2 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START4 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
 	\
	PORT_START	/* IN2 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */ \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* this bit masks irqs - dont change */

#define VS_DUAL_CONTROLS_R \
	PORT_START	/* IN3 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )	/* BUTTON B on a nes */ \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "2nd Side 1 Player Start", KEYCODE_MINUS, IP_JOY_NONE )	/* SELECT on a nes */ \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "2nd Side 3 Player Start", KEYCODE_BACKSLASH, IP_JOY_NONE )	/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER3 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 ) \
 	\
	PORT_START	/* IN4 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )	/* BUTTON B on a nes */ \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "2nd Side 2 Player Start", KEYCODE_EQUALS, IP_JOY_NONE ) /* SELECT on a nes */ \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "2nd Side 4 Player Start", KEYCODE_BACKSPACE, IP_JOY_NONE ) /* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 ) \
	\
	PORT_START	/* IN5 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN4 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* this bit masks irqs - dont change */ \

#define VS_DUAL_CONTROLS_REVERSE_L \
	PORT_START	/* IN0 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START3 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 ) \
 	\
	PORT_START	/* IN1 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */ \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START2 )				/* SELECT on a nes */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START4 )				/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 ) \
 	\
	PORT_START	/* IN2 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */ \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* this bit masks irqs - dont change */

#define VS_DUAL_CONTROLS_REVERSE_R \
	PORT_START	/* IN3 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER4 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER4 )	/* BUTTON B on a nes */ \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "2nd Side 1 Player Start", KEYCODE_MINUS, IP_JOY_NONE )	/* SELECT on a nes */ \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "2nd Side 3 Player Start", KEYCODE_BACKSLASH, IP_JOY_NONE )	/* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 ) \
 	\
	PORT_START	/* IN4 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER3 )	/* BUTTON A on a nes */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER3 )	/* BUTTON B on a nes */ \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0, "2nd Side 2 Player Start", KEYCODE_EQUALS, IP_JOY_NONE ) /* SELECT on a nes */ \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "2nd Side 4 Player Start", KEYCODE_BACKSPACE, IP_JOY_NONE ) /* START on a nes */ \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER3 ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 ) \
	\
	PORT_START	/* IN5 */ \
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */ \
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE2 ) /* service credit? */ \
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) \
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 ) \
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN4 ) \
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* this bit masks irqs - dont change */ \

INPUT_PORTS_START( vsnes )
	VS_CONTROLS( IPT_START1, IPT_BUTTON3 | IPF_PLAYER1, IPT_START2, IPT_BUTTON3 | IPF_PLAYER2 )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( topgun )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Lives per Coin" )
	PORT_DIPSETTING(	0x00, "3 - 12 Max" )
	PORT_DIPSETTING(	0x08, "2 - 9 Max" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus" )
	PORT_DIPSETTING(	0x00, "30k and Every 50k" )
	PORT_DIPSETTING(	0x20, "50k and Every 100k" )
	PORT_DIPSETTING(	0x10, "100k and Every 150k" )
	PORT_DIPSETTING(	0x30, "200k and Every 200k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( platoon )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xE0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0xc0, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0xa0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x60, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0xe0, DEF_STR( Free_Play ) )
INPUT_PORTS_END

/*
Stroke Play Off On
Hole in 1 +5 +4
Double Eagle +4 +3
Eagle +3 +2
Birdie +2 +1
Par +1 0
Bogey 0 -1
Other 0 -2

Match Play OFF ON
Win Hole +1 +2
Tie 0 0
Lose Hole -1 -2
*/

INPUT_PORTS_START( golf )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Hole Size" )
	PORT_DIPSETTING(	0x00, "Large" )
	PORT_DIPSETTING(	0x08, "Small" )
	PORT_DIPNAME( 0x10, 0x00, "Points per Stroke" )
	PORT_DIPSETTING(	0x00, "Easier" )
	PORT_DIPSETTING(	0x10, "Harder" )
	PORT_DIPNAME( 0x60, 0x00, "Starting Points" )
	PORT_DIPSETTING(	0x00, "10" )
	PORT_DIPSETTING(	0x40, "13" )
	PORT_DIPSETTING(	0x20, "16" )
	PORT_DIPSETTING(	0x60, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Difficulty Vs. Computer" )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x80, "Hard" )
INPUT_PORTS_END

/* Same as 'golf', but 4 start buttons */
INPUT_PORTS_START( golf4s )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_START3, IPT_START2, IPT_START4 )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Hole Size" )
	PORT_DIPSETTING(	0x00, "Large" )
	PORT_DIPSETTING(	0x08, "Small" )
	PORT_DIPNAME( 0x10, 0x00, "Points per Stroke" )
	PORT_DIPSETTING(	0x00, "Easier" )
	PORT_DIPSETTING(	0x10, "Harder" )
	PORT_DIPNAME( 0x60, 0x00, "Starting Points" )
	PORT_DIPSETTING(	0x00, "10" )
	PORT_DIPSETTING(	0x40, "13" )
	PORT_DIPSETTING(	0x20, "16" )
	PORT_DIPSETTING(	0x60, "20" )
	PORT_DIPNAME( 0x80, 0x00, "Difficulty Vs. Computer" )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x80, "Hard" )
INPUT_PORTS_END

INPUT_PORTS_START( vstennis )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty Vs. Computer" )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x02, "Normal" )
	PORT_DIPSETTING(	0x01, "Hard" )
	PORT_DIPSETTING(	0x03, "Very Hard" )
	PORT_DIPNAME( 0x0c, 0x00, "Difficulty Vs. Player" )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x04, "Hard" )
	PORT_DIPSETTING(	0x0c, "Very Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Raquet Size" )
	PORT_DIPSETTING(	0x00, "Large" )
	PORT_DIPSETTING(	0x10, "Small" )
	PORT_DIPNAME( 0x20, 0x00, "Extra Score" )
	PORT_DIPSETTING(	0x00, "1 Set" )
	PORT_DIPSETTING(	0x20, "1 Game" )
	PORT_DIPNAME( 0x40, 0x00, "Court Color" )
	PORT_DIPSETTING(	0x00, "Green" )
	PORT_DIPSETTING(	0x40, "Blue" )
	PORT_DIPNAME( 0x80, 0x00, "Copyright" )
	PORT_DIPSETTING(	0x00, "Japan" )
	PORT_DIPSETTING(	0x80, "USA" )

	VS_DUAL_CONTROLS_R /* Right Side Controls */

	PORT_START /* DSW1 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, "Doubles 4 Player" )
	PORT_DIPSETTING(	0x00, "2 Credits" )
	PORT_DIPSETTING(	0x08, "4 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Doubles Vs CPU" )
	PORT_DIPSETTING(	0x00, "1 Credit" )
	PORT_DIPSETTING(	0x10, "2 Credits" )
	PORT_DIPNAME( 0x60, 0x00, "Rackets Per Game" )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( wrecking )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	VS_DUAL_CONTROLS_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( balonfgt )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_HIGH )

	VS_DUAL_CONTROLS_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x02, "4" )
	PORT_DIPSETTING(	0x01, "5" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( vsmahjng )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Infinite Time" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, "Time" )
	PORT_DIPSETTING(	0x30, "30" )
	PORT_DIPSETTING(	0x10, "45" )
	PORT_DIPSETTING(	0x20, "60" )
	PORT_DIPSETTING(	0x00, "90" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	VS_DUAL_CONTROLS_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x20, "Starting Points" )
	PORT_DIPSETTING(	0x60, "15000" )
	PORT_DIPSETTING(	0x20, "20000" )
	PORT_DIPSETTING(	0x40, "25000" )
	PORT_DIPSETTING(	0x00, "30000" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( vsbball )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x00, "Starting Points" )
	PORT_DIPSETTING(    0x00, "80 Pts" )
	PORT_DIPSETTING(    0x20, "100 Pts" )
	PORT_DIPSETTING(    0x10, "150 Pts" )
	PORT_DIPSETTING(    0x30, "200 Pts" )
	PORT_DIPSETTING(    0x08, "250 Pts" )
	PORT_DIPSETTING(    0x28, "300 Pts" )
	PORT_DIPSETTING(    0x18, "350 Pts" )
	PORT_DIPSETTING(    0x38, "400 Pts" )
	PORT_DIPNAME( 0x40, 0x00, "Bonus Play" ) //?
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	VS_DUAL_CONTROLS_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x02, "Player Defense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x02, "Normal" )
	PORT_DIPSETTING(	0x01, "Strong" )
	PORT_DIPSETTING(	0x03, "Very Strong" )
	PORT_DIPNAME( 0x0c, 0x08, "Player Offense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x04, "Strong" )
	PORT_DIPSETTING(	0x0c, "Very Strong" )
	PORT_DIPNAME( 0x30, 0x20, "Computer Defense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x20, "Normal" )
	PORT_DIPSETTING(	0x10, "Strong" )
	PORT_DIPSETTING(	0x30, "Very Strong" )
	PORT_DIPNAME( 0xc0, 0x80, "Computer Offense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x80, "Normal" )
	PORT_DIPSETTING(	0x40, "Strong" )
	PORT_DIPSETTING(	0xc0, "Very Strong" )
INPUT_PORTS_END

INPUT_PORTS_START( vsbballj )
	VS_DUAL_CONTROLS_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x02, "Player Defense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x02, "Normal" )
	PORT_DIPSETTING(	0x01, "Strong" )
	PORT_DIPSETTING(	0x03, "Very Strong" )
	PORT_DIPNAME( 0x0c, 0x08, "Player Offense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x04, "Strong" )
	PORT_DIPSETTING(	0x0c, "Very Strong" )
	PORT_DIPNAME( 0x30, 0x20, "Computer Defense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x20, "Normal" )
	PORT_DIPSETTING(	0x10, "Strong" )
	PORT_DIPSETTING(	0x30, "Very Strong" )
	PORT_DIPNAME( 0xc0, 0x80, "Computer Offense Strenght" )
	PORT_DIPSETTING(	0x00, "Weak" )
	PORT_DIPSETTING(	0x80, "Normal" )
	PORT_DIPSETTING(	0x40, "Strong" )
	PORT_DIPSETTING(	0xc0, "Very Strong" )

	VS_DUAL_CONTROLS_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x06, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x00, "Starting Points" )
	PORT_DIPSETTING(    0x00, "80 Pts" )
	PORT_DIPSETTING(    0x20, "100 Pts" )
	PORT_DIPSETTING(    0x10, "150 Pts" )
	PORT_DIPSETTING(    0x30, "200 Pts" )
	PORT_DIPSETTING(    0x08, "250 Pts" )
	PORT_DIPSETTING(    0x28, "300 Pts" )
	PORT_DIPSETTING(    0x18, "350 Pts" )
	PORT_DIPSETTING(    0x38, "400 Pts" )
	PORT_DIPNAME( 0x40, 0x00, "Bonus Play" ) //?
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( iceclmrj )
	VS_DUAL_CONTROLS_REVERSE_L /* left side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, "Coinage (Left Side)" )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, "Lives (Left Side)" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPSETTING(	0x18, "7" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode (Left Side)", KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	VS_DUAL_CONTROLS_REVERSE_R /* right side controls */

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, "Coinage (Right Side)" )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, "Lives (Right Side)" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPSETTING(	0x18, "7" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode (Right Side)", KEYCODE_F1, IP_JOY_NONE )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( drmario )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, "Drop Rate Increases After" )
	PORT_DIPSETTING(	0x00, "7 Pills" )
	PORT_DIPSETTING(	0x01, "8 Pills" )
	PORT_DIPSETTING(	0x02, "9 Pills" )
	PORT_DIPSETTING(	0x03, "10 Pills" )
	PORT_DIPNAME( 0x0c, 0x00, "Virus Level" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPSETTING(	0x0c, "7" )
	PORT_DIPNAME( 0x30, 0x00, "Drop Speed Up" )
	PORT_DIPSETTING(	0x00, "Slow" )
	PORT_DIPSETTING(	0x10, "Medium" )
	PORT_DIPSETTING(	0x20, "Fast" )
	PORT_DIPSETTING(	0x30, "Fastest" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( rbibb )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x0c, 0x0c, "Max. 1p/in, 2p/in, Min" )
	PORT_DIPSETTING(	0x04, "2, 1, 3" )
	PORT_DIPSETTING(	0x0c, "2, 2, 4" )
	PORT_DIPSETTING(	0x00, "3, 2, 6" )
	PORT_DIPSETTING(	0x08, "4, 3, 7" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x80, "Color Palette" )
	PORT_DIPSETTING(	0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Wrong 1" )
	PORT_DIPSETTING(    0x40, "Wrong 2" )
	PORT_DIPSETTING(    0x20, "Wrong 3" )
	PORT_DIPSETTING(    0xc0, "Wrong 4" )
	/* 0x60,0xa0,0xe0:again "Wrong 3" */
INPUT_PORTS_END

INPUT_PORTS_START( btlecity )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x01, "Credits for 2 Players" )
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x01, "2" )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x80, "Color Palette" )
	PORT_DIPSETTING(	0x80, "Normal" )
	PORT_DIPSETTING(	0x00, "Wrong 1" )
	PORT_DIPSETTING(	0x40, "Wrong 2" )
	PORT_DIPSETTING(	0xc0, "Wrong 3" )
INPUT_PORTS_END

INPUT_PORTS_START( cluclu )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( cstlevna )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus" )
	PORT_DIPSETTING(	0x00, "100k" )
	PORT_DIPSETTING(	0x20, "200k" )
	PORT_DIPSETTING(	0x10, "300k" )
	PORT_DIPSETTING(	0x30, "400k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )	// Damage taken
	PORT_DIPSETTING(	0x00, "Normal" )				// Normal
	PORT_DIPSETTING(	0x40, "Hard" )					// Double
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( iceclimb )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPSETTING(	0x18, "7" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x00, "Time before bear appears" ) //?
	PORT_DIPSETTING(	0x00, "Long" )
	PORT_DIPSETTING(	0x40, "Short" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

/* Same as 'iceclimb', but different buttons mapping and input protection */
INPUT_PORTS_START( iceclmbj )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_SPECIAL ) // protection /* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START2 )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_SPECIAL ) // protection /* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPSETTING(	0x18, "7" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x00, "Time before bear appears ?" )
	PORT_DIPSETTING(	0x00, "Long" )
	PORT_DIPSETTING(	0x40, "Short" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( excitebk )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, "Bonus" )
	PORT_DIPSETTING(	0x00, "100k and Every 50k" )
	PORT_DIPSETTING(	0x10, "Every 100k" )
	PORT_DIPSETTING(	0x08, "100k Only" )
	PORT_DIPSETTING(	0x18, "None" )
	PORT_DIPNAME( 0x20, 0x00, "1st Half Qualifying Time" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x00, "2nd Half Qualifying Time" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( jajamaru )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				 /* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_START2 )				/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )				/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x10, "4" )
	PORT_DIPSETTING(	0x08, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( machridr )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, "Time" )
	PORT_DIPSETTING(	0x00, "280" )
	PORT_DIPSETTING(	0x10, "250" )
	PORT_DIPSETTING(	0x08, "220" )
	PORT_DIPSETTING(	0x18, "200" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( machridj )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )				/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )				/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x60, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, "Km 1st Race" ) //?
	PORT_DIPSETTING(	0x00, "12" )
	PORT_DIPSETTING(	0x10, "15" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( suprmrio )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "100" )
	PORT_DIPSETTING(	0x20, "150" )
	PORT_DIPSETTING(	0x10, "200" )
	PORT_DIPSETTING(	0x30, "250" )
	PORT_DIPNAME( 0x40, 0x00, "Timer" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Continue Lives" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0x00, "4" )
INPUT_PORTS_END

INPUT_PORTS_START( duckhunt )
	VS_ZAPPER

	PORT_START /* IN3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0X03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0X01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0X00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0X02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x10, "Hard" )
	PORT_DIPSETTING(	0x18, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Misses per game" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000" )
	PORT_DIPSETTING(	0x40, "50000" )
	PORT_DIPSETTING(	0x80, "80000" )
	PORT_DIPSETTING(	0xc0, "100000" )

	PORT_START	/* IN4 - FAKE - Gun X pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 70, 30, 0, 255 )

	PORT_START	/* IN5 - FAKE - Gun Y pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 50, 30, 0, 255 )
INPUT_PORTS_END

INPUT_PORTS_START( hogalley )
	VS_ZAPPER

	PORT_START /* IN3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x10, "Hard" )
	PORT_DIPSETTING(	0x18, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Misses per game" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(	0x00, "30000" )
	PORT_DIPSETTING(	0x40, "50000" )
	PORT_DIPSETTING(	0x80, "80000" )
	PORT_DIPSETTING(	0xc0, "100000" )

	PORT_START	/* IN4 - FAKE - Gun X pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 70, 30, 0, 255 )

	PORT_START	/* IN5 - FAKE - Gun Y pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 50, 30, 0, 255 )
INPUT_PORTS_END

INPUT_PORTS_START( vsgshoe )
	VS_ZAPPER

	PORT_START /* IN3 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0X03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0X01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0X00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0X02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x08, "Normal" )
	PORT_DIPSETTING(	0x10, "Hard" )
	PORT_DIPSETTING(	0x18, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPSETTING(	0x20, "3" )
	PORT_DIPNAME( 0x40, 0x00, "Bullets per Balloon" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPNAME( 0x80, 0x00, "1 Bonus Man Awarded at 50k" )
	PORT_DIPSETTING(	0x00, "80000" )
	PORT_DIPSETTING(	0x80, "100000" )

	PORT_START	/* IN4 - FAKE - Gun X pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 70, 30, 0, 255 )

	PORT_START	/* IN5 - FAKE - Gun Y pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 50, 30, 0, 255 )
INPUT_PORTS_END

INPUT_PORTS_START( vsfdf )
	VS_ZAPPER

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )

	PORT_START	/* IN4 - FAKE - Gun X pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_X, 70, 30, 0, 255 )

	PORT_START	/* IN5 - FAKE - Gun Y pos */
	PORT_ANALOG( 0xff, 0x80, IPT_LIGHTGUN_Y, 50, 30, 0, 255 )
INPUT_PORTS_END

INPUT_PORTS_START( vstetris )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x04, "Normal" )
	PORT_DIPSETTING(	0x08, "Hard" )
	PORT_DIPSETTING(	0x0c, "Very Hard" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0xe0, 0x80, "Color Palette" )
	PORT_DIPSETTING(	0x80, "Normal" )
	PORT_DIPSETTING(  0x00, "Wrong 1" )
	PORT_DIPSETTING(	0x40, "Wrong 2" )
	PORT_DIPSETTING(	0x20, "Wrong 3" )
	PORT_DIPSETTING(  0xc0, "Wrong 4" )
	/* 0x60,0xa0,0xe0:again "Wrong 3" */
INPUT_PORTS_END

INPUT_PORTS_START( vsskykid )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_START2, IPT_UNKNOWN, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x04, "3" )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xe0, 0x20, "Color Palette" )
	PORT_DIPSETTING(	0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Wrong 1" )
	PORT_DIPSETTING(	0x40, "Wrong 2" )
	PORT_DIPSETTING(	0x80, "Wrong 3" )
	PORT_DIPSETTING(    0xc0, "Wrong 4" )
	/* 0x60,0xa0,0xe0:again "Normal" */
INPUT_PORTS_END

INPUT_PORTS_START( vspinbal )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Balls" )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x80, 0x00, "Ball speed" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x80, "Fast" )
INPUT_PORTS_END

/* Same as 'vspinbal', but different buttons mapping */
INPUT_PORTS_START( vspinblj )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )		/* Right flipper */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )		/* Right flipper */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )					/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )					/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )		/* Left flipper */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )		/* Left flipper */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START2 ) 					/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )				 	/* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x00, "Balls" )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPSETTING(	0x20, "5" )
	PORT_DIPNAME( 0x80, 0x00, "Ball speed" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x80, "Fast" )
INPUT_PORTS_END

INPUT_PORTS_START( goonies )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x08, "2" )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME(0x40,  0x00, "Timer" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( vssoccer )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x08, "Points Timer" )
	PORT_DIPSETTING(	0x00, "600 Pts" )
	PORT_DIPSETTING(	0x10, "800 Pts" )
	PORT_DIPSETTING(	0x08, "1000 Pts" )
	PORT_DIPSETTING(	0x18, "1200 Pts" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x40, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPSETTING(	0x60, "Very Hard" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( vsgradus )
	VS_CONTROLS_REVERSE( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPSETTING(	0x00, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus" )
	PORT_DIPSETTING(	0x00, "100k" )
	PORT_DIPSETTING(	0x20, "200k" )
	PORT_DIPSETTING(	0x10, "300k" )
	PORT_DIPSETTING(	0x30, "400k" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( vsslalom )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x00, "Freestyle Points ?" )
	PORT_DIPSETTING(	0x00, "Left / Right" )
	PORT_DIPSETTING(	0x08, "Hold Time" )
	PORT_DIPNAME( 0x30, 0x10, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(	0x00, "Easy" )
	PORT_DIPSETTING(	0x10, "Normal" )
	PORT_DIPSETTING(	0x20, "Hard" )
	PORT_DIPSETTING(	0x30, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue" )
	PORT_DIPSETTING(	0x40, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, "Inverted input" )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( starlstr )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x40, "Palette Color" )
	PORT_DIPSETTING(	0x40, "Black" )
	PORT_DIPSETTING(	0x20, "Green" )
	PORT_DIPSETTING(	0x60, "Grey" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( tkoboxng )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "Palette Color" )
	PORT_DIPSETTING(	0x00, "Black" )
	PORT_DIPSETTING(	0x20, "White" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bnglngby )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START1 )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_SPECIAL ) // protection /* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START	/* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	/* BUTTON A on a nes */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	/* BUTTON B on a nes */
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_START2 )				/* SELECT on a nes */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW,  IPT_SPECIAL ) // protection /* START on a nes */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED ) /* serial pin from controller */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 ) /* service credit? */
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 0 of dsw goes here */
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* bit 1 of dsw goes here */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW,  IPT_SPECIAL )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mightybj )
	VS_CONTROLS( IPT_START1, IPT_UNKNOWN, IPT_START2, IPT_UNKNOWN )

	PORT_START /* DSW0 - bit 0 and 1 read from bit 3 and 4 on $4016, rest of the bits read on $4017 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x07, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(	0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x06, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x18, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(	0x10, "2" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x08, "4" )
	PORT_DIPSETTING(	0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxDecodeInfo nes_gfxdecodeinfo[] =
{
	/* none, the ppu generates one */
	{ -1 } /* end of array */
};

static struct NESinterface nes_interface =
{
	1,
	{ REGION_CPU1 },
	{ 50 },
};

static struct DACinterface nes_dac_interface =
{
	1,
	{ 50 },
};

static struct NESinterface nes_dual_interface =
{
	2,
	{ REGION_CPU1, REGION_CPU2 },
	{ 25, 25 },
};

static struct DACinterface nes_dual_dac_interface =
{
	2,
	{ 25, 25 },
};

static MACHINE_DRIVER_START( vsnes )

	/* basic machine hardware */
	MDRV_CPU_ADD(N2A03,N2A03_DEFAULTCLOCK)
	MDRV_CPU_MEMORY(readmem,writemem)
								/* some carts also trigger IRQs */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(( ( ( 1.0 / 60.0 ) * 1000000.0 ) / 262 ) * ( 262 - 239 ))

	MDRV_MACHINE_INIT(vsnes)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(nes_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4*16)
	MDRV_COLORTABLE_LENGTH(4*8)

	MDRV_PALETTE_INIT(vsnes)
	MDRV_VIDEO_START(vsnes)
	MDRV_VIDEO_UPDATE(vsnes)

	/* sound hardware */
	MDRV_SOUND_ADD(NES, nes_interface)
	MDRV_SOUND_ADD(DAC, nes_dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( vsdual )

	/* basic machine hardware */
	MDRV_CPU_ADD(N2A03,N2A03_DEFAULTCLOCK)
	MDRV_CPU_MEMORY(readmem,writemem)
								/* some carts also trigger IRQs */
	MDRV_CPU_ADD(N2A03,N2A03_DEFAULTCLOCK)
	MDRV_CPU_MEMORY(readmem_1,writemem_1)
								/* some carts also trigger IRQs */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(( ( ( 1.0 / 60.0 ) * 1000000.0 ) / 262 ) * ( 262 - 239 ))

	MDRV_MACHINE_INIT(vsdual)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_DUAL_MONITOR)
	MDRV_ASPECT_RATIO(8,3)
	MDRV_SCREEN_SIZE(32*8*2, 30*8)
	MDRV_VISIBLE_AREA(0*8, 32*8*2-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(nes_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2*4*16)
	MDRV_COLORTABLE_LENGTH(2*4*16)

	MDRV_PALETTE_INIT(vsdual)
	MDRV_VIDEO_START(vsdual)
	MDRV_VIDEO_UPDATE(vsdual)

	/* sound hardware */
	MDRV_SOUND_ADD(NES, nes_dual_interface)
	MDRV_SOUND_ADD(DAC, nes_dual_dac_interface)
MACHINE_DRIVER_END


/******************************************************************************/


ROM_START( suprmrio)
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "1d",  0x8000, 0x2000, 0xbe4d5436 )
	ROM_LOAD( "1c",  0xa000, 0x2000, 0x0011fc5a )
	ROM_LOAD( "1b",  0xc000, 0x2000, 0xb1b87893 )
	ROM_LOAD( "1a",  0xe000, 0x2000, 0x1abf053c )

	ROM_REGION( 0x4000,REGION_GFX1, 0  ) /* PPU memory */
	ROM_LOAD( "2b",  0x0000, 0x2000, 0x42418d40 )
	ROM_LOAD( "2a",  0x2000, 0x2000, 0x15506b86 )
ROM_END


ROM_START( iceclimb )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ic-1d",  0x8000, 0x2000, 0x65e21765 )
	ROM_LOAD( "ic-1c",  0xa000, 0x2000, 0xa7909c51 )
	ROM_LOAD( "ic-1b",  0xc000, 0x2000, 0x7fb3cc21 )
	ROM_LOAD( "ic-1a",  0xe000, 0x2000, 0xbf196bf7 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ic-2b",  0x0000, 0x2000, 0x331460b4 )
	ROM_LOAD( "ic-2a",  0x2000, 0x2000, 0x4ec44fb3 )
ROM_END

ROM_START( iceclmbj )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ic4_46db.bin",  0x8000, 0x2000, 0x0ea5f9cb )
	ROM_LOAD( "ic4_46cb.bin",  0xa000, 0x2000, 0x51fe438e )
	ROM_LOAD( "ic446bb1.bin",  0xc000, 0x2000, 0xa8afdc62 )
	ROM_LOAD( "ic4-46ab.bin",  0xe000, 0x2000, 0x96505d4d )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ic-2b",  0x0000, 0x2000, 0x331460b4 )
	ROM_LOAD( "ic-2a",  0x2000, 0x2000, 0x4ec44fb3 )
ROM_END

/* Gun games */
ROM_START( duckhunt )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "1d",  0x8000, 0x2000, 0x3f51f0ed )
	ROM_LOAD( "1c",  0xa000, 0x2000, 0x8bc7376c )
	ROM_LOAD( "1b",  0xc000, 0x2000, 0xa042b6e1 )
	ROM_LOAD( "1a",  0xe000, 0x2000, 0x1906e3ab )

	ROM_REGION( 0x4000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "2b",  0x0000, 0x2000, 0x0c52ec28 )
	ROM_LOAD( "2a",  0x2000, 0x2000, 0x3d238df3 )
ROM_END

ROM_START( hogalley)
	ROM_REGION( 0x10000, REGION_CPU1,0  ) /* 6502 memory */
	ROM_LOAD( "1d",  0x8000, 0x2000, 0x2089e166 )
	ROM_LOAD( "1c",  0xa000, 0x2000, 0xa85934ae )
	ROM_LOAD( "1b",  0xc000, 0x2000, 0x718e25b3 )
	ROM_LOAD( "1a",  0xe000, 0x2000, 0xf9526852 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "2b",  0x0000, 0x2000, 0x7623e954 )
	ROM_LOAD( "2a",  0x2000, 0x2000, 0x78c842b6 )
ROM_END

ROM_START( vsgshoe )
	ROM_REGION( 0x20000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "mds-gm5.1d",  0x10000, 0x4000, BADCRC(0x063b342f) ) //BAD? It's length should be 0x2000?
	ROM_LOAD( "mds-gm5.1c",  0x14000, 0x2000, 0xe1b7915e )
	ROM_LOAD( "mds-gm5.1b",  0x16000, 0x2000, 0x5b73aa3c )
	ROM_LOAD( "mds-gm5.1a",  0x18000, 0x2000, 0x70e606bc )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "mds-gm5.2b",  0x0000, 0x2000, 0x192c3360 )
	ROM_LOAD( "mds-gm5.2a",  0x2000, 0x2000, 0x823dd178 )
ROM_END

ROM_START( vsfdf )
	ROM_REGION( 0x30000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "prg2", 0x10000, 0x10000, 0x3bce8f0f)
	ROM_LOAD( "prg1", 0x20000, 0x10000, 0xc74499ce)

	ROM_REGION( 0x10000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "cha2.1",  0x00000, 0x10000, 0xa2f88df0 )
ROM_END

ROM_START( goonies )
	ROM_REGION( 0x20000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "prg.u7",  0x10000, 0x10000, 0x1e438d52 )

	ROM_REGION( 0x10000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "chr.u4",  0x0000, 0x10000, 0x4c4b61b0 )
ROM_END

ROM_START( vsgradus )
	ROM_REGION( 0x20000,REGION_CPU1, 0  ) /* 6502 memory */
	ROM_LOAD( "prg.u7",  0x10000, 0x10000, 0xd99a2087 )

	ROM_REGION( 0x10000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "chr.u4",  0x0000, 0x10000, 0x23cf2fc3 )
ROM_END

ROM_START( btlecity )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "bc.1d",  0x8000, 0x2000, 0x6aa87037 )
	ROM_LOAD( "bc.1c",  0xa000, 0x2000, 0xbdb317db )
	ROM_LOAD( "bc.1b",  0xc000, 0x2000, 0x1a0088b8 )
	ROM_LOAD( "bc.1a",  0xe000, 0x2000, 0x86307c89 )

	ROM_REGION( 0x4000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "bc.2b",  0x0000, 0x2000, 0x634f68bd )
	ROM_LOAD( "bc.2a",  0x2000, 0x2000, 0xa9b49a05 )
ROM_END

ROM_START( cluclu )
	ROM_REGION( 0x10000,REGION_CPU1, 0  ) /* 6502 memory */
	ROM_LOAD( "cl.6d",  0x8000, 0x2000, 0x1e9f97c9 )
	ROM_LOAD( "cl.6c",  0xa000, 0x2000, 0xe8b843a7 )
	ROM_LOAD( "cl.6b",  0xc000, 0x2000, 0x418ee9ea )
	ROM_LOAD( "cl.6a",  0xe000, 0x2000, 0x5e8a8457 )

	ROM_REGION( 0x4000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "cl.8b",  0x0000, 0x2000, 0x960d9a6c )
	ROM_LOAD( "cl.8a",  0x2000, 0x2000, 0xe3139791 )
ROM_END

ROM_START( excitebk )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "eb-1d",  0x8000, 0x2000, 0x7e54df1d )
	ROM_LOAD( "eb-1c",  0xa000, 0x2000, 0x89baae91 )
	ROM_LOAD( "eb-1b",  0xc000, 0x2000, 0x4c0c2098 )
	ROM_LOAD( "eb-1a",  0xe000, 0x2000, 0xb9ab7110 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "eb-2b",  0x0000, 0x2000, 0x80be1f50 )
	ROM_LOAD( "eb-2a",  0x2000, 0x2000, 0xa9b49a05 )
ROM_END

ROM_START( excitbkj )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "eb4-46da.bin",  0x8000, 0x2000, 0x6aa87037 )
	ROM_LOAD( "eb4-46ca.bin",  0xa000, 0x2000, 0xbdb317db )
	ROM_LOAD( "eb4-46ba.bin",  0xc000, 0x2000, 0xd1afe2dd )
	ROM_LOAD( "eb4-46aa.bin",  0xe000, 0x2000, 0x46711d0e )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "eb4-48ba.bin",  0x0000, 0x2000, 0x62a76c52 )
//	ROM_LOAD( "eb4-48aa.bin",  0x2000, 0x2000, 0xa9b49a05 )
	ROM_LOAD( "eb-2a",         0x2000, 0x2000, 0xa9b49a05 )
ROM_END

ROM_START( jajamaru )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "10.bin", 0x8000, 0x2000, 0x16af1704)
	ROM_LOAD( "9.bin",  0xa000, 0x2000, 0xdb7d1814)
	ROM_LOAD( "8.bin",  0xc000, 0x2000, 0xce263271)
	ROM_LOAD( "7.bin",  0xe000, 0x2000, 0xa406d0e4)

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "12.bin",  0x0000, 0x2000, 0xc91d536a )
	ROM_LOAD( "11.bin",  0x2000, 0x2000, 0xf0034c04 )
ROM_END

ROM_START( ladygolf)
	ROM_REGION( 0x10000,REGION_CPU1, 0  ) /* 6502 memory */
	ROM_LOAD( "lg-1d",  0x8000, 0x2000, 0x8b2ab436 )
	ROM_LOAD( "lg-1c",  0xa000, 0x2000, 0xbda6b432 )
	ROM_LOAD( "lg-1b",  0xc000, 0x2000, 0xdcdd8220 )
	ROM_LOAD( "lg-1a",  0xe000, 0x2000, 0x26a3cb3b )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "lg-2b",  0x0000, 0x2000, 0x95618947 )
	ROM_LOAD( "lg-2a",  0x2000, 0x2000, 0xd07407b1 )
ROM_END

ROM_START( smgolfj )
	ROM_REGION( 0x10000,REGION_CPU1, 0  ) /* 6502 memory */
	ROM_LOAD( "gf3_6d_b.bin",  0x8000, 0x2000, 0x8ce375b6 )
	ROM_LOAD( "gf3_6c_b.bin",  0xa000, 0x2000, 0x50a938d3 )
	ROM_LOAD( "gf3_6b_b.bin",  0xc000, 0x2000, 0x7dc39f1f )
	ROM_LOAD( "gf3_6a_b.bin",  0xe000, 0x2000, 0x9b8a2106 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "gf3_8b_b.bin",  0x0000, 0x2000, 0x7ef68029 )
	ROM_LOAD( "gf3_8a_b.bin",  0x2000, 0x2000, 0xf2285878 )
ROM_END

ROM_START( machridr )
	ROM_REGION( 0x10000,REGION_CPU1,0 ) /* 6502 memory */
	ROM_LOAD( "mr-1d",  0x8000, 0x2000, 0x379c44b9 )
	ROM_LOAD( "mr-1c",  0xa000, 0x2000, 0xcb864802 )
	ROM_LOAD( "mr-1b",  0xc000, 0x2000, 0x5547261f )
	ROM_LOAD( "mr-1a",  0xe000, 0x2000, 0xe3e3900d )

	ROM_REGION( 0x4000,REGION_GFX1 , 0) /* PPU memory */
	ROM_LOAD( "mr-2b",  0x0000, 0x2000, 0x33a2b41a )
	ROM_LOAD( "mr-2a",  0x2000, 0x2000, 0x685899d8 )
ROM_END

ROM_START( machridj )
	ROM_REGION( 0x10000,REGION_CPU1,0 ) /* 6502 memory */
	ROM_LOAD( "mr4-11da.bin",  0x8000, 0x2000, 0xab7e0594 )
	ROM_LOAD( "mr4-11ca.bin",  0xa000, 0x2000, 0xd4a341c3 )
	ROM_LOAD( "mr4-11ba.bin",  0xc000, 0x2000, 0xcbdcfece )
	ROM_LOAD( "mr4-11aa.bin",  0xe000, 0x2000, 0xe5b1e350 )

	ROM_REGION( 0x4000,REGION_GFX1 , 0) /* PPU memory */
	ROM_LOAD( "mr4-12ba.bin",  0x0000, 0x2000, 0x59867e36 )
	ROM_LOAD( "mr4-12aa.bin",  0x2000, 0x2000, 0xccfedc5a )
ROM_END

ROM_START(smgolf)
	ROM_REGION( 0x10000,REGION_CPU1,0 ) /* 6502 memory */
	ROM_LOAD( "golf-1d",  0x8000, 0x2000, 0xa3e286d3 )
	ROM_LOAD( "golf-1c",  0xa000, 0x2000, 0xe477e48b )
	ROM_LOAD( "golf-1b",  0xc000, 0x2000, 0x7d80b511 )
	ROM_LOAD( "golf-1a",  0xe000, 0x2000, 0x7b767da6 )

	ROM_REGION( 0x4000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "golf-2b",  0x0000, 0x2000, 0x2782a3e5 )
	ROM_LOAD( "golf-2a",  0x2000, 0x2000, 0x6e93fdef )
ROM_END

ROM_START( smgolfb )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "gf4-2.1df",	0x8000, 0x2000, 0x4a723087)
	ROM_LOAD( "gf4-2.1cf",  0xa000, 0x2000, 0x2debda63)
	ROM_LOAD( "gf4-2.1bf",  0xc000, 0x2000, 0x6783652f)
	ROM_LOAD( "gf4-2.1af",  0xe000, 0x2000, 0xce788209)

	ROM_REGION( 0x2000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "gf4-2.2af",  0x0000, 0x2000, 0x47e9b8c6 )
ROM_END

ROM_START( vspinbal )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "pb-6d",  0x8000, 0x2000, 0x69fc575e )
	ROM_LOAD( "pb-6c",  0xa000, 0x2000, 0xfa9472d2 )
	ROM_LOAD( "pb-6b",  0xc000, 0x2000, 0xf57d89c5 )
	ROM_LOAD( "pb-6a",  0xe000, 0x2000, 0x640c4741 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "pb-8b",  0x0000, 0x2000, 0x8822ee9e )
	ROM_LOAD( "pb-8a",  0x2000, 0x2000, 0xcbe98a28 )
ROM_END

ROM_START( vspinblj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "pn3_6d_b.bin",  0x8000, 0x2000, 0xfd50c42e  )
	ROM_LOAD( "pn3_6c_b.bin",  0xa000, 0x2000, 0x59beb9e5  )
	ROM_LOAD( "pn3_6b_b.bin",  0xc000, 0x2000, 0xce7f47ce  )
	ROM_LOAD( "pn3_6a_b.bin",  0xe000, 0x2000, 0x5685e2ee  )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "pn3_8b_b.bin",  0x0000, 0x2000, 0x1e3fec3e )
	ROM_LOAD( "pn3_8a_b.bin",  0x2000, 0x2000, 0x6f963a65 )
ROM_END

ROM_START( vsslalom )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "slalom.1d",  0x8000, 0x2000, 0x6240a07d )
	ROM_LOAD( "slalom.1c",  0xa000, 0x2000, 0x27c355e4 )
	ROM_LOAD( "slalom.1b",  0xc000, 0x2000, 0xd4825fbf )
	ROM_LOAD( "slalom.1a",  0xe000, 0x2000, 0x82333f80 )

	ROM_REGION( 0x2000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "slalom.2a",  0x0000, 0x2000, 0x977bb126 )
ROM_END

ROM_START( vssoccer )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "soccer1d",  0x8000, 0x2000, 0x0ac52145 )
	ROM_LOAD( "soccer1c",  0xa000, 0x2000, 0xf132e794 )
	ROM_LOAD( "soccer1b",  0xc000, 0x2000, 0x26bb7325 )
	ROM_LOAD( "soccer1a",  0xe000, 0x2000, 0xe731635a )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "soccer2b",  0x0000, 0x2000, 0x307b19ab )
	ROM_LOAD( "soccer2a",  0x2000, 0x2000, 0x7263613a )
ROM_END

ROM_START( starlstr )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "sl_04.1d",  0x8000, 0x2000, 0x4fd5b385 )
	ROM_LOAD( "sl_03.1c",  0xa000, 0x2000, 0xf26cd7ca )
	ROM_LOAD( "sl_02.1b",  0xc000, 0x2000, 0x9308f34e )
	ROM_LOAD( "sl_01.1a",  0xe000, 0x2000, 0xd87296e4 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "sl_06.2b",  0x0000, 0x2000, 0x25f0e027 )
	ROM_LOAD( "sl_05.2a",  0x2000, 0x2000, 0x2bbb45fd )
ROM_END

ROM_START( vstetris )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "a000.6c",  0xa000, 0x2000, 0x92a1cf10)
	ROM_LOAD( "c000.6b",  0xc000, 0x2000, 0x9e9cda9d)
	ROM_LOAD( "e000.6a",  0xe000, 0x2000, 0xbfeaf6c1)

	ROM_REGION( 0x2000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "char.8b",  0x0000, 0x2000, 0x51e8d403 )
ROM_END

ROM_START( drmario )
	ROM_REGION( 0x20000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "dm-uiprg",  0x10000, 0x10000, 0xd5d7eac4 )

	ROM_REGION( 0x8000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "dm-u3chr",  0x0000, 0x8000, 0x91871aa5 )
ROM_END

ROM_START( cstlevna )
	ROM_REGION( 0x30000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "mds-cv.prg",  0x10000, 0x20000, 0xffbef374 )

	/* No cart gfx - uses vram */
ROM_END

ROM_START( topgun )
	ROM_REGION( 0x30000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "rc-003",  0x10000, 0x20000, 0x8c0c2df5 )

	/* No cart gfx - uses vram */
ROM_END

ROM_START( tkoboxng )
	ROM_REGION( 0x20000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "tkoprg.bin",  0x10000, 0x10000, 0xeb2dba63 )

	ROM_REGION( 0x10000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "tkochr.bin",  0x0000, 0x10000, 0x21275ba5 )
ROM_END

ROM_START( rbibb )
	ROM_REGION( 0x20000,REGION_CPU1,0 ) /* 6502 memory */
	ROM_LOAD( "rbi-prg",  0x10000, 0x10000, 0x135adf7c )

	ROM_REGION( 0x8000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "rbi-cha",  0x0000, 0x8000, 0xa3c14889 )
ROM_END

ROM_START( vsskykid )
	ROM_REGION( 0x10000,REGION_CPU1,0 ) /* 6502 memory */
	ROM_LOAD( "sk-prg1",  0x08000, 0x08000, 0xcf36261e )

	ROM_REGION( 0x8000,REGION_GFX1 , 0) /* PPU memory */
	ROM_LOAD( "sk-cha",  0x0000, 0x8000, 0x9bd44dad )
ROM_END

ROM_START( platoon )
	ROM_REGION( 0x30000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "prgver0.ic4",  0x10000, 0x20000, 0xe2c0a2be)

	ROM_REGION( 0x20000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "chrver0.ic6",  0x00000, 0x20000, 0x689df57d )
ROM_END

ROM_START( bnglngby )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "rb4-26db.bin", 0x8000, 0x2000, 0xd152d8c2 )
	ROM_LOAD( "rb4-26cb.bin", 0xa000, 0x2000, 0xc3383935 )
	ROM_LOAD( "rb4-26bb.bin", 0xc000, 0x2000, 0xe2a24af8 )
	ROM_LOAD( "rb4-26ab.bin", 0xe000, 0x2000, 0x024ad874 )

	ROM_REGION( 0x4000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "rb4-28bb.bin", 0x0000, 0x2000, 0xd3d946ab )
	ROM_LOAD( "rb4-28ab.bin", 0x2000, 0x2000, 0xca08126a )

	ROM_REGION( 0x2000, REGION_USER1, 0 ) /* unknown */
	ROM_LOAD( "rb4-21ab.bin", 0x0000, 0x2000, 0xb49939ad ) /* Unknown, maps at 0xe000, maybe from another set, but we have other roms? */
ROM_END

ROM_START( supxevs )
	ROM_REGION( 0x30000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "prg2",  0x10000, 0x10000, 0x645669f0 )
	ROM_LOAD( "prg1",  0x20000, 0x10000, 0xff762ceb )

	ROM_REGION( 0x8000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "cha",  0x00000, 0x8000, 0xe27c7434 )
ROM_END

ROM_START( mightybj )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "1d.bin",  0x8000, 0x2000, 0x55dc8d77 )
	ROM_LOAD( "1c.bin",  0xa000, 0x2000, 0x151a6d15 )
	ROM_LOAD( "1b.bin",  0xc000, 0x2000, 0x9f9944bc )
	ROM_LOAD( "1a.bin",  0xe000, 0x2000, 0x76f49b65 )

	ROM_REGION( 0x2000, REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "2b.bin",  0x0000, 0x2000, 0x5425a4d0 )
ROM_END

/* Dual System */

ROM_START( balonfgt )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "bf.1d",  0x08000, 0x02000, 0x1248a6d6 )
	ROM_LOAD( "bf.1c",  0x0a000, 0x02000, 0x14af0e42 )
	ROM_LOAD( "bf.1b",  0x0c000, 0x02000, 0xa420babf )
	ROM_LOAD( "bf.1a",  0x0e000, 0x02000, 0x9c31f94d )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "bf.2b",  0x0000, 0x2000, 0xf27d9aa0 )
	ROM_LOAD( "bf.2a",  0x2000, 0x2000, 0x76e6bbf8 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "bf.6d",  0x08000, 0x02000, 0xef4ebff1 )
	ROM_LOAD( "bf.6c",  0x0a000, 0x02000, 0x14af0e42 )
	ROM_LOAD( "bf.6b",  0x0c000, 0x02000, 0xa420babf )
	ROM_LOAD( "bf.6a",  0x0e000, 0x02000, 0x3aa5c095 )

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "bf.8b",  0x0000, 0x2000, 0xf27d9aa0 )
	ROM_LOAD( "bf.8a",  0x2000, 0x2000, 0x76e6bbf8 )
ROM_END

ROM_START( vsmahjng )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "mj.1c",  0x0a000, 0x02000, 0xec77671f)
	ROM_LOAD( "mj.1b",  0x0c000, 0x02000, 0xac53398b)
	ROM_LOAD( "mj.1a",  0x0e000, 0x02000, 0x62f0df8e)

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "mj.2b",  0x0000, 0x2000, 0x9dae3502 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "mj.6c",  0x0a000, 0x02000, 0x3cee11e9 )
	ROM_LOAD( "mj.6b",  0x0c000, 0x02000, 0xe8341f7b )
	ROM_LOAD( "mj.6a",  0x0e000, 0x02000, 0x0ee69f25 )

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "mj.8b",  0x0000, 0x2000, 0x9dae3502)
ROM_END

ROM_START( vsbball )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "bb-1d",  0x08000, 0x02000, 0x0cc5225f)
	ROM_LOAD( "bb-1c",  0x0a000, 0x02000, 0x9856ac60)
	ROM_LOAD( "bb-1b",  0x0c000, 0x02000, 0xd1312e63)
	ROM_LOAD( "bb-1a",  0x0e000, 0x02000, 0x28199b4d)

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "bb-2b",  0x0000, 0x2000, 0x3ff8bec3 )
	ROM_LOAD( "bb-2a",  0x2000, 0x2000, 0xebb88502 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "bb-6d",  0x08000, 0x02000, 0x7ec792bc)
	ROM_LOAD( "bb-6c",  0x0a000, 0x02000, 0xb631f8aa)
	ROM_LOAD( "bb-6b",  0x0c000, 0x02000, 0xc856b45a)
	ROM_LOAD( "bb-6a",  0x0e000, 0x02000, 0x06b74c18)

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "bb-8b",  0x0000, 0x2000, 0x3ff8bec3 )
	ROM_LOAD( "bb-8a",  0x2000, 0x2000, 0x13b20cfd )
ROM_END

ROM_START( vsbballj )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ba_1d_a1.bin",  0x08000, 0x02000, 0x6dbc129b)
	ROM_LOAD( "ba_1c_a1.bin",  0x0a000, 0x02000, 0x2a684b3a)
	ROM_LOAD( "ba_1b_a1.bin",  0x0c000, 0x02000, 0x7ca0f715)
	ROM_LOAD( "ba_1a_a1.bin",  0x0e000, 0x02000, 0x926bb4fc)

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ba_2b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_2a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "ba_6d_a1.bin",  0x08000, 0x02000, 0xd534dca4)
	ROM_LOAD( "ba_6c_a1.bin",  0x0a000, 0x02000, 0x73904bbc)
	ROM_LOAD( "ba_6b_a1.bin",  0x0c000, 0x02000, 0x7c130724)
	ROM_LOAD( "ba_6a_a1.bin",  0x0e000, 0x02000, 0xd938080e)

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "ba_8b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_8a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )
ROM_END

ROM_START( vsbbalja )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ba_1d_a2.bin",  0x08000, 0x02000, 0xf3820b70)
	ROM_LOAD( "ba_1c_a2.bin",  0x0a000, 0x02000, 0x39fbbf28)
	ROM_LOAD( "ba_1b_a2.bin",  0x0c000, 0x02000, 0xb1377b12)
	ROM_LOAD( "ba_1a_a2.bin",  0x0e000, 0x02000, 0x08fab347)

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ba_2b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_2a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "ba_6d_a2.bin",  0x08000, 0x02000, 0xc69561b0)
	ROM_LOAD( "ba_6c_a2.bin",  0x0a000, 0x02000, 0x17d1ca39)
	ROM_LOAD( "ba_6b_a2.bin",  0x0c000, 0x02000, 0x37481900)
	ROM_LOAD( "ba_6a_a2.bin",  0x0e000, 0x02000, 0xa44ffc4b)

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "ba_8b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_8a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )
ROM_END

ROM_START( vsbbaljb )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ba_1d_a3.bin",  0x08000, 0x02000, 0xe234d609)
	ROM_LOAD( "ba_1c_a3.bin",  0x0a000, 0x02000, 0xca1a9591)
	ROM_LOAD( "ba_1b_a3.bin",  0x0c000, 0x02000, 0x50e1f6cf)
	ROM_LOAD( "ba_1a_a3.bin",  0x0e000, 0x02000, BADCRC(0x4312aa6d)) //FIXED BITS (xxxxxxx1)
																	 //BAD?
	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ba_2b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_2a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )

	ROM_REGION( 0x10000,REGION_CPU2,0 ) /* 6502 memory */
	ROM_LOAD( "ba_6d_a3.bin",  0x08000, 0x02000, 0x6eb9e36e)
	ROM_LOAD( "ba_6c_a3.bin",  0x0a000, 0x02000, 0xdca4dc75)
	ROM_LOAD( "ba_6b_a3.bin",  0x0c000, 0x02000, 0x46cf6f84)
	ROM_LOAD( "ba_6a_a3.bin",  0x0e000, 0x02000, 0x4cbc2cac)

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "ba_8b_a.bin",  0x0000, 0x2000, 0x919147d0 )
	ROM_LOAD( "ba_8a_a.bin",  0x2000, 0x2000, 0x3f7edb00 )
ROM_END

ROM_START( vstennis )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "vst-1d",  0x08000, 0x02000, 0xf4e9fca0 )
	ROM_LOAD( "vst-1c",  0x0a000, 0x02000, 0x7e52df58 )
	ROM_LOAD( "vst-1b",  0x0c000, 0x02000, 0x1a0d809a )
	ROM_LOAD( "vst-1a",  0x0e000, 0x02000, 0x8483a612 )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "vst-2b",  0x0000, 0x2000, 0x9de19c9c )
	ROM_LOAD( "vst-2a",  0x2000, 0x2000, 0x67a5800e )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* 6502 memory */
	ROM_LOAD( "vst-6d",  0x08000, 0x02000, 0x3131b1bf )
	ROM_LOAD( "vst-6c",  0x0a000, 0x02000, 0x27195d13 )
	ROM_LOAD( "vst-6b",  0x0c000, 0x02000, 0x4b4e26ca )
	ROM_LOAD( "vst-6a",  0x0e000, 0x02000, 0xb6bfee07 )

	ROM_REGION( 0x4000,REGION_GFX2 , 0) /* PPU memory */
	ROM_LOAD( "vst-8b",  0x0000, 0x2000, 0xc81e9260 )
	ROM_LOAD( "vst-8a",  0x2000, 0x2000, 0xd91eb295 )
ROM_END

ROM_START( wrecking )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "wr.1d",  0x08000, 0x02000, 0x8897e1b9 )
	ROM_LOAD( "wr.1c",  0x0a000, 0x02000, 0xd4dc5ebb )
	ROM_LOAD( "wr.1b",  0x0c000, 0x02000, 0x8ee4a454 )
	ROM_LOAD( "wr.1a",  0x0e000, 0x02000, 0x63d6490a )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "wr.2b",  0x0000, 0x2000, 0x455d77ac )
	ROM_LOAD( "wr.2a",  0x2000, 0x2000, 0x653350d8 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* 6502 memory */
	ROM_LOAD( "wr.6d",  0x08000, 0x02000, 0x90e49ce7 )
	ROM_LOAD( "wr.6c",  0x0a000, 0x02000, 0xa12ae745 )
	ROM_LOAD( "wr.6b",  0x0c000, 0x02000, 0x03947ca9 )
	ROM_LOAD( "wr.6a",  0x0e000, 0x02000, 0x2c0a13ac )

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "wr.8b",  0x0000, 0x2000, 0x455d77ac )
	ROM_LOAD( "wr.8a",  0x2000, 0x2000, 0x653350d8 )
ROM_END

ROM_START( iceclmrj )
	ROM_REGION( 0x10000,REGION_CPU1, 0 ) /* 6502 memory */
	ROM_LOAD( "ic4-41da.bin",  0x08000, 0x02000, 0x94e3197d )
	ROM_LOAD( "ic4-41ca.bin",  0x0a000, 0x02000, 0xb253011e )
	ROM_LOAD( "ic441ba1.bin",  0x0c000, 0x02000, 0xf3795874 )
	ROM_LOAD( "ic4-41aa.bin",  0x0e000, 0x02000, 0x094c246c )

	ROM_REGION( 0x4000,REGION_GFX1, 0 ) /* PPU memory */
	ROM_LOAD( "ic4-42ba.bin",  0x0000, 0x2000, 0x331460b4 )
	ROM_LOAD( "ic4-42aa.bin",  0x2000, 0x2000, 0x4ec44fb3 )

	ROM_REGION( 0x10000,REGION_CPU2, 0 ) /* 6502 memory */
	ROM_LOAD( "ic4-46da.bin",  0x08000, 0x02000, 0x94e3197d )
	ROM_LOAD( "ic4-46ca.bin",  0x0a000, 0x02000, 0xb253011e )
	ROM_LOAD( "ic4-46ba.bin",  0x0c000, 0x02000, 0x2ee9c1f9 )
	ROM_LOAD( "ic4-46aa.bin",  0x0e000, 0x02000, 0x094c246c )

	ROM_REGION( 0x4000,REGION_GFX2, 0 ) /* PPU memory */
	ROM_LOAD( "ic4-48ba.bin",  0x0000, 0x2000, 0x331460b4 )
	ROM_LOAD( "ic4-48aa.bin",  0x2000, 0x2000, 0x4ec44fb3 )
ROM_END

/******************************************************************************/

/*    YEAR  NAME      PARENT    MACHINE  INPUT     INIT  	   MONITOR  */
GAME( 1985, btlecity, 0,        vsnes,   btlecity, btlecity, ROT0, "Namco",     "Vs. Battle City" )
GAME( 1985, starlstr, 0,        vsnes,   starlstr, vsnormal, ROT0, "Namco",     "Vs. Star Luster" )
GAME( 1987,	cstlevna, 0,        vsnes,   cstlevna, cstlevna, ROT0, "Konami",    "Vs. Castlevania" )
GAME( 1984, cluclu,   0,        vsnes,   cluclu,   suprmrio, ROT0, "Nintendo",  "Vs. Clu Clu Land" )
GAME( 1990,	drmario,  0,        vsnes,   drmario,  drmario,  ROT0, "Nintendo",  "Vs. Dr. Mario" )
GAME( 1985, duckhunt, 0,        vsnes,   duckhunt, duckhunt, ROT0, "Nintendo",  "Vs. Duck Hunt" )
GAME( 1984, excitebk, 0,        vsnes,   excitebk, excitebk, ROT0, "Nintendo",  "Vs. Excitebike" )
GAME( 1984, excitbkj, excitebk, vsnes,   excitebk, excitbkj, ROT0, "Nintendo",  "Vs. Excitebike (Japan)" )
GAME( 1986,	goonies,  0,        vsnes,   goonies,  goonies,  ROT0, "Konami",    "Vs. The Goonies" )
GAME( 1985, hogalley, 0,        vsnes,   hogalley, hogalley, ROT0, "Nintendo",  "Vs. Hogan's Alley" )
GAME( 1984, iceclimb, 0,        vsnes,   iceclimb, suprmrio, ROT0, "Nintendo",  "Vs. Ice Climber" )
GAME( 1984, iceclmbj, iceclimb, vsnes,   iceclmbj, suprmrio, ROT0, "Nintendo",  "Vs. Ice Climber (Japan)" )
GAME( 1984, ladygolf, 0,        vsnes,   golf,     machridr, ROT0, "Nintendo",  "Vs. Stroke and Match Golf (Ladies Version)" )
GAME( 1985, machridr, 0,        vsnes,   machridr, machridr, ROT0, "Nintendo",  "Vs. Mach Rider (Endurance Course Version)" )
GAME( 1985, machridj, machridr, vsnes,   machridj, vspinbal, ROT0, "Nintendo",  "Vs. Mach Rider (Japan, Fighting Course Version)" )
GAME( 1986, rbibb,    0,        vsnes,   rbibb,    rbibb,    ROT0, "Namco",     "Vs. Atari R.B.I. Baseball" )
GAME( 1986, suprmrio, 0,        vsnes,   suprmrio, suprmrio, ROT0, "Nintendo",  "Vs. Super Mario Bros." )
GAME( 1985, vsskykid, 0,        vsnes,   vsskykid, vsskykid, ROT0, "Namco",     "Vs. Super SkyKid"  )
GAMEX(1987, tkoboxng, 0,        vsnes,   tkoboxng, tkoboxng, ROT0, "Namco LTD.","Vs. TKO Boxing", GAME_WRONG_COLORS )
GAME( 1984, smgolf,   0,        vsnes,   golf4s,   machridr, ROT0, "Nintendo",  "Vs. Stroke and Match Golf (Men Version)" )
GAME( 1984, smgolfj,  smgolf,   vsnes,   golf,     vsnormal, ROT0, "Nintendo",  "Vs. Stroke and Match Golf (Men Version) (Japan)" )
GAME( 1984, vspinbal, 0,        vsnes,   vspinbal, vspinbal, ROT0, "Nintendo",  "Vs. Pinball" )
GAME( 1984, vspinblj, vspinbal, vsnes,   vspinblj, vsnormal, ROT0, "Nintendo",  "Vs. Pinball (Japan)" )
GAME( 1986, vsslalom, 0,        vsnes,   vsslalom, vsslalom, ROT0, "Rare LTD.", "Vs. Slalom" )
GAME( 1985, vssoccer, 0,        vsnes,   vssoccer, excitebk, ROT0, "Nintendo",  "Vs. Soccer" )
GAME( 1986, vsgradus, 0,        vsnes,   vsgradus, vsgradus, ROT0, "Konami",    "Vs. Gradius" )
GAMEX(1987, platoon,  0,        vsnes,   platoon,  platoon,  ROT0, "Ocean Software Limited", "Vs. Platoon", GAME_WRONG_COLORS )
GAMEX(1987, vstetris, 0,        vsnes,   vstetris, vstetris, ROT0, "Academysoft-Elory", "Vs. Tetris" , GAME_IMPERFECT_COLORS )
GAME( 1986, mightybj, 0,        vsnes,   mightybj, mightybj, ROT0, "Tecmo",     "Vs. Mighty Bomb Jack (Japan)" )
GAME( 1985, jajamaru, 0,        vsnes,   jajamaru, jajamaru, ROT0, "Jaleco",    "Vs. Ninja Jajamaru Kun (Japan)" )
GAME( 1987, topgun,   0,        vsnes,   topgun,   topgun,   ROT0, "Konami",    "Vs. Top Gun")
GAME( 1985, bnglngby, 0,        vsnes,   bnglngby, bnglngby, ROT0, "Nintendo / Broderbund Software Inc.",  "Vs. Raid on Bungeling Bay (Japan)" )

/* Dual games */
GAME( 1984, vstennis, 0,        vsdual,  vstennis, vstennis, ROT0, "Nintendo",  "Vs. Tennis"  )
GAME( 1984, wrecking, 0,        vsdual,  wrecking, wrecking, ROT0, "Nintendo",  "Vs. Wrecking Crew" )
GAME( 1984, balonfgt, 0,        vsdual,  balonfgt, balonfgt, ROT0, "Nintendo",  "Vs. Balloon Fight" )
GAME( 1984, vsmahjng, 0,        vsdual,  vsmahjng, vstennis, ROT0, "Nintendo",  "Vs. Mahjang (Japan)"  )
GAME( 1984, vsbball,  0,        vsdual,  vsbball,  vsbball,  ROT0, "Nintendo of America",  "Vs. BaseBall" )
GAME( 1984, vsbballj, vsbball,  vsdual,  vsbballj, vsbball,  ROT0, "Nintendo of America",  "Vs. BaseBall (Japan set 1)" )
GAME( 1984, vsbbalja, vsbball,  vsdual,  vsbballj, vsbball,  ROT0, "Nintendo of America",  "Vs. BaseBall (Japan set 2)" )
GAME( 1984, iceclmrj, 0,        vsdual,  iceclmrj, iceclmrj, ROT0, "Nintendo",  "Vs. Ice Climber Dual (Japan)"  )

/* Partially working */
GAME( 1986, vsgshoe,  0,        vsnes,   vsgshoe,  vsgshoe,  ROT0, "Nintendo",  "Vs. Gumshoe" )

/* Not Working */
GAMEX( 19??, supxevs,  0,        vsnes,   vsnes,	xevious,  ROT0, "Namco?",   "Vs. Super Xevious", GAME_NOT_WORKING )
GAMEX(1988?, vsfdf,    0,        vsnes,   vsfdf,    vsfdf   , ROT0, "Konami",   "Vs. Freedom Force", GAME_NOT_WORKING )
GAMEX( 1985, smgolfb,  smgolf,   vsnes,   golf,     machridr, ROT0, "Nintendo", "Vs. Stroke and Match Golf (Men set 2)", GAME_NOT_WORKING )
GAMEX( 1984, vsbbaljb, vsbball,  vsdual,  vsbballj, vsbball,  ROT0, "Nintendo of America",  "Vs. BaseBall (Japan set 3)", GAME_NOT_WORKING )
