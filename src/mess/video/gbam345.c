 /***************************************************************************

  gbam345.c

  Handles GBA rendering for modes 3, 4, and 5.

  By R. Belmont & Harmony

***************************************************************************/

static void draw_roz_bitmap_mode_scanline(running_machine *machine, gba_state *gba_state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)gba_state->gba_pram)[0] | 0x30000000;

	if(bpp == 4)
	{
		fatalerror( "Screen mode 5 is not fully supported yet, contact MESSdev!\n" );
	}

	draw_roz_bitmap_scanline(gba_state, line2, y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2X, gba_state->BG2Y, gba_state->BG2PA, gba_state->BG2PB, gba_state->BG2PC, gba_state->BG2PD, &gba_state->gfxBG2X, &gba_state->gfxBG2Y, gba_state->gfxBG2Changed, bpp);
	draw_gba_oam(gba_state, machine, lineOBJ, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;

		if(line2[x] < color)
		{
			color = line2[x];
			top = 0x04;
		}

		if((UINT8)(lineOBJ[x] >> 24) < (UINT8)(color >> 24))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if(top == 0x10 && (color & 0x00010000) != 0)
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if(line2[x] < back)
			{
				back = line2[x];
				top2 = 0x04;
			}

			if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(gba_state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & gba_state->BLDCNT)
						{
							color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & gba_state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}

		lineMix[x] = color;
	}
	gba_state->gfxBG2Changed = 0;
}

static void draw_roz_bitmap_mode_scanline_nowindow(running_machine *machine, gba_state *gba_state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)gba_state->gba_pram)[0] | 0x30000000;
	int effect = gba_state->BLDCNT & BLDCNT_SFX;

	if(bpp == 4)
	{
		fatalerror( "Screen mode 5 is not fully supported yet, contact MESSdev!\n" );
	}

	draw_roz_bitmap_scanline(gba_state, line2, y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2X, gba_state->BG2Y, gba_state->BG2PA, gba_state->BG2PB, gba_state->BG2PC, gba_state->BG2PD, &gba_state->gfxBG2X, &gba_state->gfxBG2Y, gba_state->gfxBG2Changed, bpp);
	draw_gba_oam(gba_state, machine, lineOBJ, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;

		if(line2[x] < color)
		{
			color = line2[x];
			top = 0x04;
		}

		if((UINT8)(lineOBJ[x] >> 24) < (UINT8)(color >> 24))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if((color & 0x00010000) == 0)
		{
			switch(effect)
			{
				case BLDCNT_SFX_NONE:
					break;
				case BLDCNT_SFX_ALPHA:
					if(gba_state->BLDCNT & top)
					{
						UINT32 back = backdrop;
						UINT8 top2 = 0x20;

						if(line2[x] < back)
						{
							if(top != 0x04)
							{
								back = line2[x];
								top2 = 0x04;
							}
						}

						if((UINT8)(lineOBJ[x] >> 24) < (UINT8)(back >> 24))
						{
							if(top != 0x10)
							{
								back = lineOBJ[x];
								top2 = 0x10;
							}
						}

						if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
						{
							color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
						}
					}
					break;
				case BLDCNT_SFX_LIGHTEN:
					if(top & gba_state->BLDCNT)
					{
						color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
					}
					break;
				case BLDCNT_SFX_DARKEN:
					if(top & gba_state->BLDCNT)
					{
						color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
					}
					break;
			}
		}
		else
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if(line2[x] < back)
			{
				back = line2[x];
				top2 = 0x04;
			}

			if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(gba_state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & gba_state->BLDCNT)
						{
							color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & gba_state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}
		lineMix[x] = color;
	}
	gba_state->gfxBG2Changed = 0;
}

static void draw_roz_bitmap_mode_scanline_all(running_machine *machine, gba_state *gba_state, int y, UINT32* line0, UINT32* line1, UINT32* line2, UINT32* line3, UINT32* lineOBJ, UINT32* lineOBJWin, UINT32* lineMix, int bpp)
{
	int x = 0;
	UINT32 backdrop = ((UINT16*)gba_state->gba_pram)[0] | 0x30000000;
	int inWindow0 = 0;
	int inWindow1 = 0;
	UINT8 inWin0Mask = gba_state->WININ & 0x00ff;
	UINT8 inWin1Mask = gba_state->WININ >> 8;
	UINT8 outMask = gba_state->WINOUT & 0x00ff;

	if(bpp == 4)
	{
		fatalerror( "Screen mode 5 is not fully supported yet, contact MESSdev!\n" );
	}

	if(gba_state->DISPCNT & DISPCNT_WIN0_EN)
	{
		UINT8 v0 = gba_state->WIN0V >> 8;
		UINT8 v1 = gba_state->WIN0V & 0x00ff;
		inWindow0 = ((v0 == v1) && (v0 >= 0xe8)) ? 1 : 0;
		if(v1 >= v0)
		{
			inWindow0 |= (y >= v0 && y < v1) ? 1 : 0;
		}
		else
		{
			inWindow0 |= (y >= v0 || y < v1) ? 1 : 0;
		}
	}

	if(gba_state->DISPCNT & DISPCNT_WIN1_EN)
	{
		UINT8 v0 = gba_state->WIN1V >> 8;
		UINT8 v1 = gba_state->WIN1V & 0x00ff;
		inWindow1 = ((v0 == v1) && (v0 >= 0xe8)) ? 1 : 0;
		if(v1 >= v0)
		{
			inWindow1 |= (y >= v0 && y < v1) ? 1 : 0;
		}
		else
		{
			inWindow1 |= (y >= v0 || y < v1) ? 1 : 0;
		}
	}

	draw_roz_bitmap_scanline(gba_state, line2, y, DISPCNT_BG2_EN, gba_state->BG2CNT, gba_state->BG2X, gba_state->BG2Y, gba_state->BG2PA, gba_state->BG2PB, gba_state->BG2PC, gba_state->BG2PD, &gba_state->gfxBG2X, &gba_state->gfxBG2Y, gba_state->gfxBG2Changed, bpp);
	draw_gba_oam(gba_state, machine, lineOBJ, y);
	draw_gba_oam_window(gba_state, machine, lineOBJWin, y);

	for(x = 0; x < 240; x++)
	{
		UINT32 color = backdrop;
		UINT8 top = 0x20;
		UINT8 mask = outMask;

		if((lineOBJWin[x] & 0x80000000) == 0)
		{
			mask = gba_state->WINOUT >> 8;
		}

		if(inWindow1)
		{
			if(is_in_window(gba_state, x, 1))
			{
				mask = inWin1Mask;
			}
		}

		if(inWindow0)
		{
			if(is_in_window(gba_state, x, 0))
			{
				mask = inWin0Mask;
			}
		}

		if((mask & 0x04) != 0 && (line2[x] < color))
		{
			color = line2[x];
			top = 0x04;
		}

		if((mask & 0x10) != 0 && (UINT8)(lineOBJ[x] >> 24) < (UINT8)(color >> 24))
		{
			color = lineOBJ[x];
			top = 0x10;
		}

		if((mask & 0x20) != 0)
		{
			if((color & 0x00010000) == 0)
			{
				switch(gba_state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_NONE:
						break;
					case BLDCNT_SFX_ALPHA:
					{
						if(top & gba_state->BLDCNT)
						{
							UINT32 back = backdrop;
							UINT8 top2 = 0x20;
							if((mask & 0x04) != 0 && (line2[x] < back))
							{
								if(top != 0x04)
								{
									back = line2[x];
									top2 = 0x04;
								}
							}

							if((mask & 0x10) != 0 && ((UINT8)(lineOBJ[x] >> 24) < (UINT8)(back >> 24)))
							{
								if(top != 0x10)
								{
									back = lineOBJ[x];
									top2 = 0x10;
								}
							}

							if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
							{
								color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
							}
						}
						break;
					}
					case BLDCNT_SFX_LIGHTEN:
						if(top & gba_state->BLDCNT)
						{
							color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & gba_state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
				}
			}
			else
			{
				UINT32 back = backdrop;
				UINT8 top2 = 0x20;

				if((mask & 0x04) != 0 && (line2[x] < back))
				{
					back = line2[x];
					top2 = 0x04;
				}

				if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
				{
					color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
				}
				else
				{
					switch(gba_state->BLDCNT & BLDCNT_SFX)
					{
						case BLDCNT_SFX_LIGHTEN:
							if(top & gba_state->BLDCNT)
							{
								color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
							}
							break;
						case BLDCNT_SFX_DARKEN:
							if(top & gba_state->BLDCNT)
							{
								color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
							}
							break;
					}
				}
			}
		}
		else if(color & 0x00010000)
		{
			UINT32 back = backdrop;
			UINT8 top2 = 0x20;

			if((mask & 0x04) != 0 && (line2[x] < back))
			{
				back = line2[x];
				top2 = 0x04;
			}

			if(top2 & (gba_state->BLDCNT >> BLDCNT_TP2_SHIFT))
			{
				color = alpha_blend_pixel(color, back, coeff[gba_state->BLDALPHA & 0x1f], coeff[(gba_state->BLDALPHA >> 8) & 0x1f]);
			}
			else
			{
				switch(gba_state->BLDCNT & BLDCNT_SFX)
				{
					case BLDCNT_SFX_LIGHTEN:
						if(top & gba_state->BLDCNT)
						{
							color = increase_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
					case BLDCNT_SFX_DARKEN:
						if(top & gba_state->BLDCNT)
						{
							color = decrease_brightness(color, coeff[gba_state->BLDY & 0x1f]);
						}
						break;
				}
			}
		}
		lineMix[x] = color;
	}
	gba_state->gfxBG2Changed = 0;
}
