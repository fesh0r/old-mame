/****************************************************************************

 PC-Engine / Turbo Grafx 16 driver
 by Charles Mac Donald
 E-Mail: cgfm2@hooked.net

 Thanks to David Shadoff and Brian McPhail for help with the driver.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"

static int pce_interrupt(void)
{
    int ret = 0;

    /* bump current scanline */
    vdc.curline = (vdc.curline + 1) % VDC_LPF;

    /* draw a line of the display */
    if(vdc.curline < vdc.physical_height)
    {
        pce_refresh_line(vdc.curline);
    }

    /* generate interrupt on line compare if necessary*/
    if(vdc.vdc_data[CR].w & CR_RC)
    if(vdc.curline == ((vdc.vdc_data[RCR].w & 0x03FF) - 64))
    {
        vdc.status |= VDC_RR;
        ret = H6280_INT_IRQ1;
    }

    /* handle frame events */
    if(vdc.curline == 256)
    {
        vdc.status |= VDC_VD;   /* set vblank flag */

        /* do VRAM > SATB DMA if the enable bit is set or the DVSSR reg. was written to */
        if((vdc.vdc_data[DCR].w & DCR_DSR) || vdc.dvssr_write)
        {
            if(vdc.dvssr_write) vdc.dvssr_write = 0;
            memcpy(&vdc.sprite_ram, &vdc.vram[vdc.vdc_data[DVSSR].w<<1], 512);
            vdc.status |= VDC_DS;   /* set satb done flag */

            /* generate interrupt if needed */
            if(vdc.vdc_data[DCR].w & DCR_DSC)
            {
                ret = H6280_INT_IRQ1;
            }
        }

        if(vdc.vdc_data[CR].w & CR_VR)  /* generate IRQ1 if enabled */
        {
            ret = H6280_INT_IRQ1;
        }
    }
    return (ret);
}

/* stubs for the irq/psg/timer code */

static WRITE_HANDLER ( pce_irq_w)
{
}

static READ_HANDLER ( pce_irq_r )
{
    return 0x00;
}

static WRITE_HANDLER ( pce_timer_w )
{
}

static READ_HANDLER ( pce_timer_r )
{
    return 0x00;
}

static WRITE_HANDLER ( pce_psg_w )
{
}

static READ_HANDLER ( pce_psg_r )
{
    return 0x00;
}

MEMORY_READ_START( pce_readmem )
    { 0x000000, 0x1EDFFF, MRA_ROM },
    { 0x1EE000, 0x1EFFFF, MRA_RAM },
    { 0x1F0000, 0x1F1FFF, MRA_RAM },
    { 0x1FE000, 0x1FE003, vdc_r },
    { 0x1FE400, 0x1FE407, vce_r },
    { 0x1FE800, 0x1FE80F, pce_psg_r },
    { 0x1FEC00, 0x1FEC00, pce_timer_r },
    { 0x1FF000, 0x1FF000, pce_joystick_r },
    { 0x1FF402, 0x1FF403, pce_irq_r },
MEMORY_END

MEMORY_WRITE_START( pce_writemem )
    { 0x000000, 0x1EDFFF, MWA_ROM },
    { 0x1EE000, 0x1EFFFF, MWA_RAM, &pce_save_ram },
    { 0x1F0000, 0x1F1FFF, MWA_RAM, &pce_user_ram },
    { 0x1FE000, 0x1FE003, vdc_w },
    { 0x1FE400, 0x1FE407, vce_w },
    { 0x1FE800, 0x1FE80F, pce_psg_w },
    { 0x1FEC00, 0x1FEC01, pce_timer_w },
    { 0x1FF000, 0x1FF000, pce_joystick_w },
    { 0x1FF402, 0x1FF403, pce_irq_w },
MEMORY_END

PORT_READ_START( pce_readport )
    { 0x00, 0x03, vdc_r },
PORT_END

PORT_WRITE_START( pce_writeport )
    { 0x00, 0x03, vdc_w },
PORT_END

/* todo: alternate forms of input (multitap, mouse, etc.) */
INPUT_PORTS_START( pce )

    PORT_START  /* Player 1 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* button I */
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* button II */
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* select */
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* run */
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
#if 0
    PORT_START  /* Fake dipswitches for system config */
    PORT_DIPNAME( 0x01, 0x01, "Console type")
    PORT_DIPSETTING(    0x00, "Turbo-Grafx 16")
    PORT_DIPSETTING(    0x01, "PC-Engine")

    PORT_DIPNAME( 0x01, 0x01, "Joystick type")
    PORT_DIPSETTING(    0x00, "2 Button")
    PORT_DIPSETTING(    0x01, "6 Button")
#endif
INPUT_PORTS_END


#if 0
static struct GfxLayout pce_bg_layout =
{
        8, 8,
        2048,
        4,
        {0x00*8, 0x01*8, 0x10*8, 0x11*8 },
        {0, 1, 2, 3, 4, 5, 6, 7 },
        { 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
        32*8,
};

static struct GfxLayout pce_obj_layout =
{
        16, 16,
        512,
        4,
        {0x00*8, 0x20*8, 0x40*8, 0x60*8},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16, 8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
        128*8,
};

static struct GfxDecodeInfo pce_gfxdecodeinfo[] =
{
   { 1, 0x0000, &pce_bg_layout, 0, 0x10 },
   { 1, 0x0000, &pce_obj_layout, 0x100, 0x10 },
MEMORY_END
#endif

static struct MachineDriver machine_driver_pce =
{
    {
        {
            CPU_H6280,
            7195090,
            pce_readmem, pce_writemem, pce_readport, pce_writeport,
            pce_interrupt, VDC_LPF
        },
    },
    60, DEFAULT_60HZ_VBLANK_DURATION,
    1,
    pce_init_machine,
    pce_stop_machine,

    45*8, 32*8, { 0*8, 45*8-1, 0*8, 32*8-1 },
    0,
    /*pce_gfxdecodeinfo,*/
    512, 512,
    0,
    VIDEO_TYPE_RASTER,
    0,		/* was... (256*2) */
    pce_vh_start,
    pce_vh_stop,
    pce_vh_screenrefresh,
    0, 0, 0, 0
};

static const struct IODevice io_pce[] = {
    {
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"pce\0",            /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
        NULL,				/* id */
		pce_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL                /* output_chunk */
	},
	{ IO_END }
};

#define rom_pce NULL

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	   YEAR  NAME	   PARENT	 MACHINE   INPUT	 INIT	   COMPANY	 FULLNAME */
CONSX( 1987, pce,	   0,		 pce,	   pce, 	 0,		   "Nippon Electronic Company", "PC Engine/TurboGrafx 16", GAME_NOT_WORKING | GAME_NO_SOUND )

