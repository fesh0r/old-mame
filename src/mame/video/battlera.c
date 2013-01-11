/*******************************************************************************

    Battle Rangers - Bryan McPhail, mish@tendril.co.uk

    This file only implements necessary features - not all PC-Engine video
    features are used in this game (no DMA for one).

*******************************************************************************/

#include "emu.h"
#include "cpu/h6280/h6280.h"
#include "includes/battlera.h"


/******************************************************************************/

void battlera_state::video_start()
{
	m_HuC6270_vram=auto_alloc_array(machine(), UINT8, 0x20000);
	m_vram_dirty=auto_alloc_array(machine(), UINT8, 0x1000);

	memset(m_HuC6270_vram,0,0x20000);
	memset(m_vram_dirty,1,0x1000);

	m_tile_bitmap=auto_bitmap_ind16_alloc(machine(),512,512);
	m_front_bitmap=auto_bitmap_ind16_alloc(machine(),512,512);

	m_vram_ptr=0;
	m_inc_value=1;
	m_current_scanline=0;
	m_irq_enable=m_rcr_enable=m_sb_enable=m_bb_enable=0;

	machine().gfx[0]->set_source(m_HuC6270_vram);
	machine().gfx[1]->set_source(m_HuC6270_vram);
	machine().gfx[2]->set_source(m_blank_tile);
}

/******************************************************************************/

WRITE8_MEMBER(battlera_state::battlera_palette_w)
{
	int pal_word;

	m_generic_paletteram_8[offset]=data;
	if (offset%2) offset-=1;

	pal_word=m_generic_paletteram_8[offset] | (m_generic_paletteram_8[offset+1]<<8);
	palette_set_color_rgb(machine(), offset/2, pal3bit(pal_word >> 3), pal3bit(pal_word >> 6), pal3bit(pal_word >> 0));
}

/******************************************************************************/

READ8_MEMBER(battlera_state::HuC6270_debug_r)
{
	return m_HuC6270_vram[offset];
}

WRITE8_MEMBER(battlera_state::HuC6270_debug_w)
{
	m_HuC6270_vram[offset]=data;
}
READ8_MEMBER(battlera_state::HuC6270_register_r)
{
	int rr;

	if ((m_current_scanline+56)==m_HuC6270_registers[6]) rr=1; else rr=0;

	return 0        /* CR flag */
		| (0 << 1)  /* OR flag */
		| (rr << 2) /* RR flag */
		| (0 << 3)  /* DS flag */
		| (0 << 4)  /* DV flag */
		| (m_bldwolf_vblank << 5)   /* VD flag (1 when vblank else 0) */
		| (0 << 6)  /* BSY flag (1 when dma active, else 0) */
		| (0 << 7); /* Always zero */
}

WRITE8_MEMBER(battlera_state::HuC6270_register_w)
{
	switch (offset) {
	case 0: /* Select data region */
		m_VDC_register=data;
		break;
	case 1: /* Unused */
		break;
	}
}

/******************************************************************************/

#ifdef UNUSED_FUNCTION
READ8_MEMBER(battlera_state::HuC6270_data_r)
{
	int result;

	switch (offset) {
		case 0: /* LSB */
			return m_HuC6270_vram[(m_HuC6270_registers[1]<<1)|1];

		case 1:/* MSB */
			result=m_HuC6270_vram[(m_HuC6270_registers[1]<<1)|0];
			m_HuC6270_registers[1]=(m_HuC6270_registers[1]+m_inc_value)&0xffff;
			return result;
	}

	return 0;
}
#endif

WRITE8_MEMBER(battlera_state::HuC6270_data_w)
{
	switch (offset) {
		case 0: /* LSB */
			switch (m_VDC_register) {

			case 0: /* MAWR */
				m_HuC6270_registers[0]=(m_HuC6270_registers[0]&0xff00) | (data);
				return;

			case 1: /* MARR */
				m_HuC6270_registers[0]=(m_HuC6270_registers[1]&0xff00) | (data);
				return;

			case 2: /* VRAM */
				if (m_HuC6270_vram[(m_HuC6270_registers[0]<<1)|1]!=data) {
					m_HuC6270_vram[(m_HuC6270_registers[0]<<1)|1]=data;
					machine().gfx[0]->mark_dirty(m_HuC6270_registers[0]>>4);
					machine().gfx[1]->mark_dirty(m_HuC6270_registers[0]>>6);
				}
				if (m_HuC6270_registers[0]<0x1000) m_vram_dirty[m_HuC6270_registers[0]]=1;
				return;

			case 3: break; /* Unused */
			case 4: break; /* Unused */

			case 5: /* CR - Control register */
				/* Bits 0,1 unknown */
				m_rcr_enable=data&0x4; /* Raster interrupt enable */
				m_irq_enable=data&0x8; /* VBL interrupt enable */
				/* Bits 4,5 unknown (EX) */
				m_sb_enable=data&0x40; /* Sprites enable */
				m_bb_enable=data&0x80; /* Background enable */
				return;

			case 6: /* RCR - Raster counter register */
				m_HuC6270_registers[6]=(m_HuC6270_registers[6]&0xff00) | (data);
				return;

			case 7: /* BXR - X scroll */
				m_HuC6270_registers[7]=(m_HuC6270_registers[7]&0xff00) | (data);
				return;

			case 8: /* BYR - Y scroll */
				m_HuC6270_registers[8]=(m_HuC6270_registers[8]&0xff00) | (data);
				return;

			case 15: /* DMA */
			case 16:
			case 17:
			case 18:
				logerror("%04x: dma 2 %02x\n",space.device().safe_pc(),data);
				break;

			case 19: /* SATB */
				m_HuC6270_registers[19]=(m_HuC6270_registers[19]&0xff00) | (data);
				return;

			}
			break;

		/*********************************************/

		case 1: /* MSB (Autoincrement on this write) */
			switch (m_VDC_register) {

			case 0: /* MAWR - Memory Address Write Register */
				m_HuC6270_registers[0]=(m_HuC6270_registers[0]&0xff) | (data<<8);
				return;

			case 1: /* MARR */
				m_HuC6270_registers[1]=(m_HuC6270_registers[1]&0xff) | (data<<8);
				return;

			case 2: /* VWR - VRAM */
				if (m_HuC6270_vram[(m_HuC6270_registers[0]<<1)|0]!=data) {
					m_HuC6270_vram[(m_HuC6270_registers[0]<<1)|0]=data;
					machine().gfx[0]->mark_dirty(m_HuC6270_registers[0]>>4);
					machine().gfx[1]->mark_dirty(m_HuC6270_registers[0]>>6);
					if (m_HuC6270_registers[0]<0x1000) m_vram_dirty[m_HuC6270_registers[0]]=1;
				}
				m_HuC6270_registers[0]+=m_inc_value;
				m_HuC6270_registers[0]=m_HuC6270_registers[0]&0xffff;
				return;

			case 5: /* CR */
				/* IW - Auto-increment values */
				switch ((data>>3)&3) {
					case 0: m_inc_value=1; break;
					case 1: m_inc_value=32;break;
					case 2: m_inc_value=64; break;
					case 3: m_inc_value=128; break;
				}

				/* DR, TE unknown */
				return;

			case 6: /* RCR - Raster counter register */
				m_HuC6270_registers[6]=(m_HuC6270_registers[6]&0xff) | (data<<8);
				return;

			case 7: /* BXR - X scroll */
				m_HuC6270_registers[7]=(m_HuC6270_registers[7]&0xff) | (data<<8);
						return;

			case 8: /* BYR - Y scroll */
				m_HuC6270_registers[8]=(m_HuC6270_registers[8]&0xff) | (data<<8);
				return;

			case 15: /* DMA */
			case 16:
			case 17:
			case 18:
				logerror("%04x: dma 2 %02x\n",space.device().safe_pc(),data);
				break;

			case 19: /* SATB - Sprites */
				m_HuC6270_registers[19]=(m_HuC6270_registers[19]&0xff) | (data<<8);
				return;
			}
			break;
	}
	logerror("%04x: unknown write to  VDC_register %02x (%02x) at %02x\n",space.device().safe_pc(),m_VDC_register,data,offset);
}

/******************************************************************************/

static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap,const rectangle &clip,int pri)
{
	battlera_state *state = machine.driver_data<battlera_state>();
	int offs,my,mx,code,code2,fx,fy,cgy=0,cgx,colour,i,yinc;

	/* Draw sprites, starting at SATB, draw in _reverse_ order */
	for (offs=(state->m_HuC6270_registers[19]<<1)+0x200-8; offs>=(state->m_HuC6270_registers[19]<<1); offs-=8)
	{
		if ((state->m_HuC6270_vram[offs+7]&0x80) && !pri) continue;
		if (!(state->m_HuC6270_vram[offs+7]&0x80) && pri) continue;

		code=state->m_HuC6270_vram[offs+5] + (state->m_HuC6270_vram[offs+4]<<8);
		code=code>>1;

		my=state->m_HuC6270_vram[offs+1] + (state->m_HuC6270_vram[offs+0]<<8);
		mx=state->m_HuC6270_vram[offs+3] + (state->m_HuC6270_vram[offs+2]<<8);

		mx-=32;
		my-=57;

		fx=state->m_HuC6270_vram[offs+6]&0x8;
		fy=state->m_HuC6270_vram[offs+6]&0x80;
		cgx=state->m_HuC6270_vram[offs+6]&1;
		colour=state->m_HuC6270_vram[offs+7]&0xf;

		switch ((state->m_HuC6270_vram[offs+6]>>4)&3) {
		case 0: cgy=1; break;
		case 1: cgy=2; break;
		case 2: cgy=0; break; /* Illegal */
		case 3: cgy=4; break;
		}

		if (cgx && cgy==2) code=code&0x3fc; /* Title screen */

		if (fx && cgx) {code2=code; code++;} /* Swap tile order on X flips */
		else code2=code+1;

		yinc = 16;
		if (fy) { my += 16*(cgy-1); yinc = -16; } /* Swap tile order on Y flips */

		for (i=0; i<cgy; i++) {
			drawgfx_transpen(bitmap,clip,machine.gfx[1],
				code,
				colour,
				fx,fy,
				mx,my,0);

			if (cgx)
				drawgfx_transpen(bitmap,clip,machine.gfx[1],
						code2,
						colour,
						fx,fy,
						mx+16,my,0);
			my += yinc;
			/* if (cgx) */ /* Different from console? */
			code += 2;
			code2 += 2;
			/*else code += 1; */ /* Different from console? */
		}
	}

}

/******************************************************************************/

UINT32 battlera_state::screen_update_battlera(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int offs,code,scrollx,scrolly,mx,my;

	/* if any tiles changed, redraw the VRAM */
	if (machine().gfx[0]->dirtyseq() != m_tile_dirtyseq)
	{
		m_tile_dirtyseq = machine().gfx[0]->dirtyseq();
		memset(m_vram_dirty, 1, 0x1000);
	}

	mx=-1;
	my=0;
	for (offs = 0x0000;offs < 0x2000;offs += 2)
	{
		mx++;
		if (mx==64) {mx=0; my++;}
		code=m_HuC6270_vram[offs+1] + ((m_HuC6270_vram[offs] & 0x0f) << 8);

		/* If this tile was changed OR tilemap was changed, redraw */
		if (m_vram_dirty[offs/2]) {
			m_vram_dirty[offs/2]=0;
			drawgfx_opaque(*m_tile_bitmap,m_tile_bitmap->cliprect(),machine().gfx[0],
					code,
					m_HuC6270_vram[offs] >> 4,
					0,0,
					8*mx,8*my);
			drawgfx_opaque(*m_front_bitmap,m_tile_bitmap->cliprect(),machine().gfx[2],
					0,
					0,  /* fill the spot with pen 256 */
					0,0,
					8*mx,8*my);
			drawgfx_transmask(*m_front_bitmap,m_tile_bitmap->cliprect(),machine().gfx[0],
					code,
					m_HuC6270_vram[offs] >> 4,
					0,0,
					8*mx,8*my,0x1);
		}
	}

	/* Render bitmap */
	scrollx=-m_HuC6270_registers[7];
	scrolly=-m_HuC6270_registers[8]+cliprect.min_y-1;

	copyscrollbitmap(bitmap,*m_tile_bitmap,1,&scrollx,1,&scrolly,cliprect);

	/* Todo:  Background enable (not used anyway) */

	/* Render low priority sprites, if enabled */
	if (m_sb_enable) draw_sprites(machine(),bitmap,cliprect,0);

	/* Render background over sprites */
	copyscrollbitmap_trans(bitmap,*m_front_bitmap,1,&scrollx,1,&scrolly,cliprect,256);

	/* Render high priority sprites, if enabled */
	if (m_sb_enable) draw_sprites(machine(),bitmap,cliprect,1);

	return 0;
}

/******************************************************************************/

TIMER_DEVICE_CALLBACK_MEMBER(battlera_state::battlera_irq)
{
	m_current_scanline = param; /* 8 lines clipped at top */

	/* If raster interrupt occurs, refresh screen _up_ to this point */
	if (m_rcr_enable && (m_current_scanline+56)==m_HuC6270_registers[6]) {
		machine().primary_screen->update_partial(m_current_scanline);
		m_maincpu->set_input_line(0, HOLD_LINE); /* RCR interrupt */
	}

	/* Start of vblank */
	else if (m_current_scanline==240) {
		m_bldwolf_vblank=1;
		machine().primary_screen->update_partial(240);
		if (m_irq_enable)
			m_maincpu->set_input_line(0, HOLD_LINE); /* VBL */
	}

	/* End of vblank */
	if (m_current_scanline==254) {
		m_bldwolf_vblank=0;
	}
}
