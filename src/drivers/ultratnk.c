/***************************************************************************

	Atari/Kee Ultra Tank hardware

	Games supported:
		* Ultra Tank

	Known issues:
		- sound samples needed
		- colors are probably correct, but should be verified
		- invisible tanks option doesn't work
		- coin counters aren't mapped
		- hardware collision detection is not emulated. However, the game is fully playable,
		  since the game software uses it only as a hint to check for tanks bumping into
		  walls/mines.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int ultratnk_controls;
static data8_t *mirror_ram;



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x02, 0x01,
	0x02, 0x00,
	0x02, 0x00,
	0x02, 0x01
};

static PALETTE_INIT( ultratnk )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */
	palette_set_color(2,0x80,0x80,0x80); /* LT GREY */
	palette_set_color(3,0x55,0x55,0x55); /* DK GREY */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Sprite rendering
 *
 *************************************/

static void draw_sprites( struct mame_bitmap *bitmap )
{
	const data8_t *pMem = memory_region( REGION_CPU1 );

	if( (pMem[0x93]&0x80)==0 )
	/*	Probably wrong; game description indicates that one or both tanks can
		be invisible; in this game mode, tanks are visible when hit, bumping
		into a wall, or firing
	*/
	{
		drawgfx( bitmap, Machine->gfx[1], /* tank */
			pMem[0x99]>>3,
			0,
			0,0, /* no flip */
			pMem[0x90]-16,pMem[0x98]-16,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0 );

		drawgfx( bitmap, Machine->gfx[1], /* tank */
			pMem[0x9b]>>3,
			1,
			0,0, /* no flip */
			pMem[0x92]-16,pMem[0x9a]-16,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0 );
	}

	drawgfx( bitmap, Machine->gfx[1], /* bullet */
		(pMem[0x9f]>>3)|0x20,
		0,
		0,0, /* no flip */
		pMem[0x96]-16,pMem[0x9e]-16,
		&Machine->visible_area,
		TRANSPARENCY_PEN, 0 );

	drawgfx( bitmap, Machine->gfx[1], /* bullet */
		(pMem[0x9d]>>3)|0x20,
		1,
		0,0, /* no flip */
		pMem[0x94]-16,pMem[0x9c]-16,
		&Machine->visible_area,
		TRANSPARENCY_PEN, 0 );
}



/*************************************
 *
 *	Video update
 *
 *************************************/

VIDEO_UPDATE( ultratnk )
{
	int code;
	int sx,sy;
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for( offs = 0; offs<0x400; offs++ )
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs]=0;
			code = videoram[offs];
			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);
			drawgfx( tmpbitmap,
				Machine->gfx[0],
				code&0x3F, code>>6,
				0,0,
				sx,sy,
				0,TRANSPARENCY_NONE,0);
		}
	}
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	draw_sprites( bitmap );

	/* Weird, but we have to update our sound registers here. */
	discrete_sound_w(2, mirror_ram[0x88] % 16);
	discrete_sound_w(3, mirror_ram[0x8A] % 16);
}



/*************************************
 *
 *	Control reading
 *
 *************************************/

static WRITE_HANDLER( da_latch_w )
{
	int joybits = readinputport(4);
	ultratnk_controls = readinputport(3); /* start and fire buttons */

	switch( data )
	{
	case 0x0a:
		if( joybits&0x08 ) ultratnk_controls &= ~0x40;
		if( joybits&0x04 ) ultratnk_controls &= ~0x04;

		if( joybits&0x80 ) ultratnk_controls &= ~0x10;
		if( joybits&0x40 ) ultratnk_controls &= ~0x01;
		break;

	case 0x05:
		if( joybits&0x02 ) ultratnk_controls &= ~0x40;
		if( joybits&0x01 ) ultratnk_controls &= ~0x04;

		if( joybits&0x20 ) ultratnk_controls &= ~0x10;
		if( joybits&0x10 ) ultratnk_controls &= ~0x01;
		break;
	}
}


static READ_HANDLER( ultratnk_controls_r )
{
	return (ultratnk_controls << offset) & 0x80;
}


static READ_HANDLER( ultratnk_barrier_r )
{
	return readinputport(2) & 0x80;
}


static READ_HANDLER( ultratnk_coin_r )
{
	switch (offset & 0x06)
	{
		case 0x00: return (readinputport(2) << 3) & 0x80;	/* left coin */
		case 0x02: return (readinputport(2) << 4) & 0x80;	/* right coin */
		case 0x04: return (readinputport(2) << 1) & 0x80;	/* invisible tanks */
		case 0x06: return (readinputport(2) << 2) & 0x80;	/* rebounding shots */
	}

	return 0;
}


static READ_HANDLER( ultratnk_tilt_r )
{
	return (readinputport(2) << 5) & 0x80;	/* tilt */
}


static READ_HANDLER( ultratnk_dipsw_r )
{
	int dipsw = readinputport(0);
	switch( offset )
	{
		case 0x00: return ((dipsw & 0xC0) >> 6); /* language? */
		case 0x01: return ((dipsw & 0x30) >> 4); /* credits */
		case 0x02: return ((dipsw & 0x0C) >> 2); /* game time */
		case 0x03: return ((dipsw & 0x03) >> 0); /* extended time */
	}
	return 0;
}



/*************************************
 *
 *	Sound handlers
 *
 *************************************/
WRITE_HANDLER( ultratnk_fire_w )
{
	discrete_sound_w(offset/2, offset&1);
}

WRITE_HANDLER( ultratnk_attract_w )
{
	discrete_sound_w(5, (data & 1));
}

WRITE_HANDLER( ultratnk_explosion_w )
{
	discrete_sound_w(4, data % 16);
}



/*************************************
 *
 *	Interrupt generation
 *
 *************************************/

static INTERRUPT_GEN( ultratnk_interrupt )
{
	if (readinputport(1) & 0x40 )
	{
		/* only do NMI interrupt if not in TEST mode */
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	}
}



/*************************************
 *
 *	Misc memory handlers
 *
 *************************************/

static READ_HANDLER( ultratnk_collision_r )
{
	/**	Note: hardware collision detection is not emulated.
	 *	However, the game is fully playable, since the game software uses it
	 *	only as a hint to check for tanks bumping into walls/mines.
	 */
	switch( offset )
	{
		case 0x01:	return 0x80;	/* white tank = D7 */
		case 0x03:	return 0x80;	/* black tank = D7 */
	}
	return 0;
}


static WRITE_HANDLER( ultratnk_leds_w )
{
	set_led_status(offset/2,offset&1);
}


static READ_HANDLER( mirror_r )
{
	return mirror_ram[offset];
}


static WRITE_HANDLER( mirror_w )
{
	mirror_ram[offset] = data;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x00ff, MRA_RAM },
	{ 0x0100, 0x01ff, mirror_r },
	{ 0x0800, 0x0bff, MRA_RAM },
	{ 0x0c00, 0x0cff, MRA_RAM },
	{ 0x1000, 0x1000, input_port_1_r }, /* self test, vblank */
	{ 0x1800, 0x1800, ultratnk_barrier_r }, /* barrier */
	{ 0x2000, 0x2007, ultratnk_controls_r },
	{ 0x2020, 0x2026, ultratnk_coin_r },
	{ 0x2040, 0x2043, ultratnk_collision_r },
	{ 0x2046, 0x2046, ultratnk_tilt_r },
	{ 0x2060, 0x2063, ultratnk_dipsw_r },
	{ 0x2800, 0x2fff, MRA_NOP }, /* diagnostic ROM (see code at B1F3) */
	{ 0xb000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x00ff, MWA_RAM, &mirror_ram },
	{ 0x0100, 0x01ff, mirror_w },
	{ 0x0800, 0x0bff, videoram_w, &videoram, &videoram_size },
	{ 0x0c00, 0x0cff, MWA_RAM }, /* ? */
	{ 0x2000, 0x201f, ultratnk_attract_w }, /* attract */
	{ 0x2020, 0x203f, MWA_NOP }, /* collision reset 1-4, 2020-21=cr1, 22-23=cr2, 24-25=cr3, 26,27=cr4 */
	{ 0x2040, 0x2041, da_latch_w }, /* D/A LATCH */
	{ 0x2042, 0x2043, ultratnk_explosion_w }, /* EXPLOSION (sound) */
	{ 0x2044, 0x2045, MWA_NOP }, /* TIMER (watchdog) RESET */
	{ 0x2066, 0x2067, MWA_NOP }, /* LOCKOUT */
	{ 0x2068, 0x206b, ultratnk_leds_w },
	{ 0x206c, 0x206f, ultratnk_fire_w }, /* fire 1/2 */
	{ 0xb000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( ultratnk )
	PORT_START
	PORT_DIPNAME( 0x03, 0x01, "Extended Play" )
	PORT_DIPSETTING(	0x01, "25 Points" )
	PORT_DIPSETTING(	0x02, "50 Points" )
	PORT_DIPSETTING(	0x03, "75 Points" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x04, "Game Length" )
	PORT_DIPSETTING(	0x00, "60 Seconds" )
	PORT_DIPSETTING(	0x04, "90 Seconds" )
	PORT_DIPSETTING(	0x08, "120 Seconds" )
	PORT_DIPSETTING(	0x0c, "150 Seconds" )
	PORT_DIPNAME( 0x30, 0x20, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x30, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x20, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0x00, "Spare" ) /* Language?  Doesn't have any effect. */
	PORT_DIPSETTING(	0x00, "A" )
	PORT_DIPSETTING(	0x40, "B" )
	PORT_DIPSETTING(	0x80, "C" )
	PORT_DIPSETTING(	0xc0, "D" )

	PORT_START
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* input#2 (arbitrarily arranged) */
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_SERVICE1 | IPF_TOGGLE, "Option 1", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_SERVICE2 | IPF_TOGGLE, "Option 2", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_SERVICE3 | IPF_TOGGLE, "Option 3", IP_KEY_DEFAULT, IP_JOY_DEFAULT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START /* input#3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) /* joystick (taken from below) */

	PORT_START /* input#4 - fake */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0a, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 )	/* note that this sets 2 bits */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x05, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 )	/* note that this sets 2 bits */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0xa0, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 )	/* note that this sets 2 bits */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x50, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 )	/* note that this sets 2 bits */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout playfield_layout =
{
	8,8,
	RGN_FRAC(1,2),
	1,
	{ 0 },
	{ 4, 5, 6, 7, 4 + RGN_FRAC(1,2), 5 + RGN_FRAC(1,2), 6 + RGN_FRAC(1,2), 7 + RGN_FRAC(1,2) },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout motion_layout =
{
	16,16,
	RGN_FRAC(1,4),
	1,
	{ 0 },
	{ 7, 6, 5, 4, 7 + RGN_FRAC(1,4), 6 + RGN_FRAC(1,4), 5 + RGN_FRAC(1,4), 4 + RGN_FRAC(1,4),
	  7 + RGN_FRAC(2,4), 6 + RGN_FRAC(2,4), 5 + RGN_FRAC(2,4), 4 + RGN_FRAC(2,4),
	  7 + RGN_FRAC(3,4), 6 + RGN_FRAC(3,4), 5 + RGN_FRAC(3,4), 4 + RGN_FRAC(3,4) },
	{ 0, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &playfield_layout, 0, 4 }, 	/* playfield graphics */
	{ REGION_GFX2, 0, &motion_layout,    0, 4 }, 	/* motion graphics */
	{ -1 }
};


/************************************************************************/
/* ultratnk Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc ultratnk_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is not inverted */
	15			/* Output bit */
};

/* Nodes - Inputs */
#define ULTRATNK_MOTORSND1_DATA		NODE_01
#define ULTRATNK_MOTORSND2_DATA		NODE_02
#define ULTRATNK_FIRESND1_EN		NODE_03
#define ULTRATNK_FIRESND2_EN		NODE_04
#define ULTRATNK_EXPLOSIONSND_DATA	NODE_05
#define ULTRATNK_ATTRACT_EN		NODE_06
/* Nodes - Sounds */
#define ULTRATNK_MOTORSND1		NODE_10
#define ULTRATNK_MOTORSND2		NODE_11
#define ULTRATNK_EXPLOSIONSND		NODE_12
#define ULTRATNK_FIRESND1		NODE_13
#define ULTRATNK_FIRESND2		NODE_14
#define ULTRATNK_NOISE			NODE_15
#define ULTRATNK_FINAL_MIX		NODE_16

static DISCRETE_SOUND_START(ultratnk_sound_interface)
	/************************************************/
	/* Ultratnk sound system: 5 Sound Sources       */
	/*                     Relative Volume          */
	/*    1/2) Motor           77.72%               */
	/*      2) Explosion      100.00%               */
	/*    4/5) Fire            29.89%               */
	/* Relative volumes calculated from resitor     */
	/* network in combiner circuit                  */
	/*                                              */
	/*  Discrete sound mapping via:                 */
	/*     discrete_sound_w($register,value)        */
	/*  $00 - Fire enable Tank 1                    */
	/*  $01 - Fire enable Tank 2                    */
	/*  $02 - Motorsound frequency Tank 1           */
	/*  $03 - Motorsound frequency Tank 2           */
	/*  $04 - Explode volume                        */
	/*  $05 - Attract                               */
	/*                                              */
	/************************************************/

	/************************************************/
	/* Input register mapping for ultratnk           */
	/************************************************/
	/*                   NODE                    ADDR   MASK   GAIN    OFFSET  INIT */
	DISCRETE_INPUT (ULTRATNK_FIRESND1_EN       , 0x00, 0x000f,                  0.0)
	DISCRETE_INPUT (ULTRATNK_FIRESND2_EN       , 0x01, 0x000f,                  0.0)
	DISCRETE_INPUTX(ULTRATNK_MOTORSND1_DATA    , 0x02, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUTX(ULTRATNK_MOTORSND2_DATA    , 0x03, 0x000f, -1.0   , 15.0,   0.0)
	DISCRETE_INPUT (ULTRATNK_EXPLOSIONSND_DATA , 0x04, 0x000f,                  0.0)
	DISCRETE_INPUT (ULTRATNK_ATTRACT_EN        , 0x05, 0x000f,                  0.0)

	/************************************************/
	/* Motor sound circuit is based on a 556 VCO    */
	/* with the input frequency set by the MotorSND */
	/* latch (4 bit). This freqency is then used to */
	/* driver a modulo 12 counter, with div6, 4 & 3 */
	/* summed as the output of the circuit.         */
	/* VCO Output is Sq wave = 27-382Hz             */
	/*  F1 freq - (Div6)                            */
	/*  F2 freq = (Div4)                            */
	/*  F3 freq = (Div3) 33.3% duty, 33.3 deg phase */
	/* To generate the frequency we take the freq.  */
	/* diff. and /15 to get all the steps between   */
	/* 0 - 15.  Then add the low frequency and send */
	/* that value to a squarewave generator.        */
	/* Also as the frequency changes, it ramps due  */
	/* to a 2.2uf capacitor on the R-ladder.        */
	/* Note the VCO freq. is controlled by a 250k   */
	/* pot.  The freq. used here is for the pot set */
	/* to 125k.  The low freq is allways the same.  */
	/* This adjusts the high end.                   */
	/* 0k = 214Hz.   250k = 4416Hz                  */
	/************************************************/
	DISCRETE_RCFILTER(NODE_70, 1, ULTRATNK_MOTORSND1_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_71, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, (382.0-27.0)/12/15, DISC_LOGADJ, "Motor 1 RPM")
	DISCRETE_MULTIPLY(NODE_72, 1, NODE_70, NODE_71)

	DISCRETE_GAIN(NODE_20, NODE_72, 2)		/* F1 = /12*2 = /6 */
	DISCRETE_ADDER2(NODE_21, 1, NODE_20, 27.0/6)
	DISCRETE_SQUAREWAVE(NODE_27, 1, NODE_21, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_67, 1, NODE_27, 10000, 1e-7)

	DISCRETE_GAIN(NODE_22, NODE_72, 3)		/* F2 = /12*3 = /4 */
	DISCRETE_ADDER2(NODE_23, 1, NODE_22, 27.0/4)
	DISCRETE_SQUAREWAVE(NODE_28, 1, NODE_23, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_68, 1, NODE_28, 10000, 1e-7)

	DISCRETE_GAIN(NODE_24, NODE_72, 4)		/* F3 = /12*4 = /3 */
	DISCRETE_ADDER2(NODE_25, 1, NODE_24, 27.0/3)
	DISCRETE_SQUAREWAVE(NODE_29, 1, NODE_25, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_69, 1, NODE_29, 10000, 1e-7)

	DISCRETE_ADDER3(ULTRATNK_MOTORSND1, ULTRATNK_ATTRACT_EN, NODE_67, NODE_68, NODE_69)

	/************************************************/
	/* Tank2 motor sound is basically the same as   */
	/* Car1.  But I shifted the frequencies up for  */
	/* it to sound different from Tank1.            */
	/************************************************/
	DISCRETE_RCFILTER(NODE_73, 1, ULTRATNK_MOTORSND2_DATA, 123000, 2.2e-6)
	DISCRETE_ADJUSTMENT(NODE_74, 1, (214.0-27.0)/12/15, (4416.0-27.0)/12/15, (522.0-27.0)/12/15, DISC_LOGADJ, "Motor 2 RPM")
	DISCRETE_MULTIPLY(NODE_75, 1, NODE_73, NODE_74)

	DISCRETE_GAIN(NODE_50, NODE_75, 2)		/* F1 = /12*2 = /6 */
	DISCRETE_ADDER2(NODE_51, 1, NODE_50, 27.0/6)
	DISCRETE_SQUAREWAVE(NODE_57, 1, NODE_51, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_77, 1, NODE_57, 10000, 1e-7)

	DISCRETE_GAIN(NODE_52, NODE_75, 3)		/* F2 = /12*3 = /4 */
	DISCRETE_ADDER2(NODE_53, 1, NODE_52, 27.0/4)
	DISCRETE_SQUAREWAVE(NODE_58, 1, NODE_53, (777.2/3), 50.0, 0, 0)
	DISCRETE_RCFILTER(NODE_78, 1, NODE_58, 10000, 1e-7)

	DISCRETE_GAIN(NODE_54, NODE_75, 4)		/* F3 = /12*4 = /3 */
	DISCRETE_ADDER2(NODE_55, 1, NODE_54, 27.0/3)
	DISCRETE_SQUAREWAVE(NODE_59, 1, NODE_55, (777.2/3), 100.0/3, 0, 360.0/3)
	DISCRETE_RCFILTER(NODE_79, 1, NODE_59, 10000, 1e-7)

	DISCRETE_ADDER3(ULTRATNK_MOTORSND2, ULTRATNK_ATTRACT_EN, NODE_77, NODE_78, NODE_79)

	/************************************************/
	/* Explosion circuit is built around a noise    */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 2V signal.                */
	/* 2V = HSYNC/4                                 */
	/*    = 15750/4                                 */
	/* Output is binary weighted with 4 bits of     */
	/* crash volume.                                */
	/************************************************/
	DISCRETE_LOGIC_INVERT(NODE_37, 1, ULTRATNK_ATTRACT_EN)
	DISCRETE_LFSR_NOISE(ULTRATNK_NOISE, ULTRATNK_ATTRACT_EN, NODE_37, 15750.0/4, 1.0, 0, 0, &ultratnk_lfsr)

	DISCRETE_MULTIPLY(NODE_38, 1, ULTRATNK_NOISE, ULTRATNK_EXPLOSIONSND_DATA)
	DISCRETE_GAIN(NODE_39, NODE_38, 1000.0/15)
	DISCRETE_RCFILTER(ULTRATNK_EXPLOSIONSND, 1, NODE_39, 545, 1e-7)

	/************************************************/
	/* Fire circuits takes the noise output from    */
	/* the crash circuit and applies +ve feedback   */
	/* to cause oscillation. There is also an RC    */
	/* filter on the input to the feedback circuit. */
	/* RC is 1K & 10uF                              */
	/* Feedback cct is modelled by using the RC out */
	/* as the frequency input on a VCO,             */
	/* breadboarded freq range as:                  */
	/*  0 = 940Hz, 34% duty                         */
	/*  1 = 630Hz, 29% duty                         */
	/*  the duty variance is so small we ignore it  */
	/************************************************/
	DISCRETE_INVERT(NODE_30, ULTRATNK_NOISE)
	DISCRETE_GAIN(NODE_31, NODE_30, 940.0-630.0)
	DISCRETE_ADDER2(NODE_32, 1, NODE_31, ((940.0-630.0)/2)+630.0)
	DISCRETE_RCFILTER(NODE_33, 1, NODE_32, 1000, 1e-5)
	DISCRETE_SQUAREWAVE(NODE_34, 1, NODE_33, 407.8, 31.5, 0, 0.0)
	DISCRETE_ONOFF(ULTRATNK_FIRESND1, ULTRATNK_FIRESND1_EN, NODE_34)
	DISCRETE_ONOFF(ULTRATNK_FIRESND2, ULTRATNK_FIRESND2_EN, NODE_34)

	/************************************************/
	/* Combine all 5 sound sources.                 */
	/* Add some final gain to get to a good sound   */
	/* level.                                       */
	/************************************************/
	DISCRETE_ADDER3(NODE_90, 1, ULTRATNK_MOTORSND1, ULTRATNK_EXPLOSIONSND, ULTRATNK_FIRESND1)
	DISCRETE_ADDER3(NODE_91, 1, NODE_90, ULTRATNK_MOTORSND2, ULTRATNK_FIRESND2)
	DISCRETE_GAIN(ULTRATNK_FINAL_MIX, NODE_91, 65534.0/(777.2+777.2+1000.0+298.9+298.9))

	DISCRETE_OUTPUT(ULTRATNK_FINAL_MIX, 100)
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( ultratnk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,1500000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ultratnk_interrupt,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source)/sizeof(unsigned short))

	MDRV_PALETTE_INIT(ultratnk)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(ultratnk)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, ultratnk_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( ultratnk )
	ROM_REGION( 0x12000, REGION_CPU1, 0 )
	ROM_LOAD_NIB_LOW ( "030180.n1",	 0xb000, 0x0800, 0xb6aa6056 ) /* ROM 3 D0-D3 */
	ROM_LOAD_NIB_HIGH( "030181.k1",	 0xb000, 0x0800, 0x17145c97 ) /* ROM 3 D4-D7 */
	ROM_LOAD_NIB_LOW ( "030182.m1",	 0xb800, 0x0800, 0x034366a2 ) /* ROM 4 D0-D3 */
	ROM_RELOAD(                      0xf800, 0x0800 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "030183.l1",	 0xb800, 0x0800, 0xbe141602 ) /* ROM 4 D4-D7 */
	ROM_RELOAD(                      0xf800, 0x0800 ) /* for 6502 vectors */

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "30172-01.j6", 0x0000, 0x0200, 0x1d364b23 )
	ROM_LOAD( "30173-01.h6", 0x0200, 0x0200, 0x5c32f331 )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "30174-01.n6", 0x0000, 0x0400, 0xd0e20e73 )
	ROM_LOAD( "30175-01.m6", 0x0400, 0x0400, 0xa47459c9 )
	ROM_LOAD( "30176-01.l6", 0x0800, 0x0400, 0x1cc7c2dd )
	ROM_LOAD( "30177-01.k6", 0x0c00, 0x0400, 0x3a91b09f )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1978, ultratnk, 0, ultratnk, ultratnk, 0, 0, "Atari", "Ultra Tank" )
