#include "driver.h"
#include "deprecat.h"
#include "video/ppu2c0x.h"
#include "includes/vsnes.h"


PALETTE_INIT( vsnes )
{
	ppu2c0x_init_palette(machine, 0 );
}

PALETTE_INIT( vsdual )
{
	ppu2c0x_init_palette(machine, 0 );
	ppu2c0x_init_palette(machine, 8*4*16 );
}

static void ppu_irq( int num, int *ppu_regs )
{
	cpunum_set_input_line(Machine, num, INPUT_LINE_NMI, PULSE_LINE );
}

/* our ppu interface                                            */
static const ppu2c0x_interface ppu_interface =
{
	PPU_2C04,				/* type */
	1,						/* num */
	{ "gfx1" },				/* vrom gfx region */
	{ 0 },					/* gfxlayout num */
	{ 0 },					/* color base */
	{ PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq }				/* irq */
};

/* our ppu interface for dual games                             */
static const ppu2c0x_interface ppu_dual_interface =
{
	PPU_2C04,								/* type */
	2,										/* num */
	{ "gfx1", "gfx2" },						/* vrom gfx region */
	{ 0, 1 },								/* gfxlayout num */
	{ 0, 64 },								/* color base */
	{ PPU_MIRROR_NONE, PPU_MIRROR_NONE },	/* mirroring */
	{ ppu_irq, ppu_irq }					/* irq */
};

VIDEO_START( vsnes )
{
	ppu2c0x_init(machine, &ppu_interface );
}

VIDEO_START( vsdual )
{
	ppu2c0x_init(machine, &ppu_dual_interface );
}

/***************************************************************************

  Display refresh

***************************************************************************/
VIDEO_UPDATE( vsnes )
{
	/* render the ppu */
	ppu2c0x_render( 0, bitmap, 0, 0, 0, 0 );
	return 0;
}


VIDEO_UPDATE( vsdual )
{
	const device_config *top_screen = device_list_find_by_tag(screen->machine->config->devicelist, VIDEO_SCREEN, "top");
	const device_config *bottom_screen = device_list_find_by_tag(screen->machine->config->devicelist, VIDEO_SCREEN, "bottom");

	/* render the ppu's */
	if (screen == top_screen)
		ppu2c0x_render(0, bitmap, 0, 0, 0, 0);
	else if (screen == bottom_screen)
		ppu2c0x_render(1, bitmap, 0, 0, 0, 0);

	return 0;
}
