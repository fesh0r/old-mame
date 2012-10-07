/***************************************************************************

Functions to emulate the video hardware of the machine.

***************************************************************************/


#include "emu.h"
#include "includes/srmp2.h"
#include "video/seta001.h"

PALETTE_INIT_MEMBER(srmp2_state,srmp2)
{
	const UINT8 *color_prom = machine().root_device().memregion("proms")->base();
	int i;

	for (i = 0; i < machine().total_colors(); i++)
	{
		int col;

		col = (color_prom[i] << 8) + color_prom[i + machine().total_colors()];
		palette_set_color_rgb(machine(),i ^ 0x0f,pal5bit(col >> 10),pal5bit(col >> 5),pal5bit(col >> 0));
	}
}


PALETTE_INIT_MEMBER(srmp2_state,srmp3)
{
	const UINT8 *color_prom = machine().root_device().memregion("proms")->base();
	int i;

	for (i = 0; i < machine().total_colors(); i++)
	{
		int col;

		col = (color_prom[i] << 8) + color_prom[i + machine().total_colors()];
		palette_set_color_rgb(machine(),i,pal5bit(col >> 10),pal5bit(col >> 5),pal5bit(col >> 0));
	}
}

int srmp3_gfxbank_callback( running_machine &machine, UINT16 code, UINT8 color )
{
	srmp2_state *state = machine.driver_data<srmp2_state>();

	if (code & 0x2000)
	{
		code = (code & 0x1fff);
		code += ((state->m_gfx_bank + 1) * 0x2000);
	}

	return code;
}


UINT32 srmp2_state::screen_update_srmp2(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0x1ff, cliprect);

	machine().device<seta001_device>("spritegen")->set_transpen(15);

	machine().device<seta001_device>("spritegen")->set_colorbase((m_color_bank)?0x20:0x00);

	machine().device<seta001_device>("spritegen")->set_fg_xoffsets( 0x10, 0x10 );
	machine().device<seta001_device>("spritegen")->set_fg_yoffsets( 0x05, 0x07 );
	machine().device<seta001_device>("spritegen")->set_bg_xoffsets( 0x00, 0x00 ); // bg not used?
	machine().device<seta001_device>("spritegen")->set_bg_yoffsets( 0x00, 0x00 ); // bg not used?

	machine().device<seta001_device>("spritegen")->seta001_draw_sprites(machine(),bitmap,cliprect,0x1000, 1);
	return 0;
}

UINT32 srmp2_state::screen_update_srmp3(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0x1f0, cliprect);

	machine().device<seta001_device>("spritegen")->set_fg_xoffsets( 0x10, 0x10 );
	machine().device<seta001_device>("spritegen")->set_fg_yoffsets( 0x06, 0x06 );
	machine().device<seta001_device>("spritegen")->set_bg_xoffsets( -0x01, 0x10 );
	machine().device<seta001_device>("spritegen")->set_bg_yoffsets( -0x06, 0x06 );

	machine().device<seta001_device>("spritegen")->set_gfxbank_callback( srmp3_gfxbank_callback );

	machine().device<seta001_device>("spritegen")->seta001_draw_sprites(machine(),bitmap,cliprect,0x1000, 1);
	return 0;
}

UINT32 srmp2_state::screen_update_mjyuugi(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(0x1f0, cliprect);

	machine().device<seta001_device>("spritegen")->set_fg_xoffsets( 0x10, 0x10 );
	machine().device<seta001_device>("spritegen")->set_fg_yoffsets( 0x06, 0x06 );
	machine().device<seta001_device>("spritegen")->set_bg_yoffsets( 0x09, 0x07 );

	machine().device<seta001_device>("spritegen")->set_spritelimit( 0x1ff-6 );

	machine().device<seta001_device>("spritegen")->set_gfxbank_callback( srmp3_gfxbank_callback );

	machine().device<seta001_device>("spritegen")->seta001_draw_sprites(machine(),bitmap,cliprect,0x1000, 1);
	return 0;
}
