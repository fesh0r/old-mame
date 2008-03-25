/***************************************************************************
	microbee.c

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

****************************************************************************/

#include "driver.h"
#include "includes/mbee.h"


typedef struct {		 // CRTC 6545
	UINT8 horizontal_total;
    UINT8 horizontal_displayed;
    UINT8 horizontal_sync_pos;
    UINT8 horizontal_length;
    UINT8 vertical_total;
    UINT8 vertical_adjust;
    UINT8 vertical_displayed;
    UINT8 vertical_sync_pos;
    UINT8 crt_mode;
    UINT8 scan_lines;
    UINT8 cursor_top;
    UINT8 cursor_bottom;
    UINT8 screen_address_hi;
    UINT8 screen_address_lo;
    UINT8 cursor_address_hi;
    UINT8 cursor_address_lo;
	UINT8 lpen_hi;
	UINT8 lpen_lo;
	UINT8 transp_hi;
	UINT8 transp_lo;
    UINT8 idx;
	UINT8 cursor_visible;
	UINT8 cursor_phase;
	UINT8 lpen_strobe;
	UINT8 update_strobe;
} CRTC6545;

static CRTC6545 crt;
static int off_x = 0;
static int off_y = 0;
static int framecnt = 0;
static int m6545_color_bank = 0;
static int m6545_video_bank = 0;
static int mbee_pcg_color_latch = 0;

int mbee_frame_counter;


UINT8 *pcgram;


WRITE8_HANDLER ( mbee_pcg_color_latch_w )
{
	mbee_pcg_color_latch = data;
}

 READ8_HANDLER ( mbee_pcg_color_latch_r )
{
	return mbee_pcg_color_latch;
}

WRITE8_HANDLER ( mbee_videoram_w )
{
	videoram[offset] = data;
}

 READ8_HANDLER ( mbee_videoram_r )
{
	if( m6545_video_bank & 0x01 )
		return pcgram[offset];
	else
		return videoram[offset];
}

WRITE8_HANDLER ( mbee_pcg_w )
{
	if( pcgram[0x0800+offset] != data )
       	{
		int chr = 0x80 + offset / 16;
		pcgram[0x0800+offset] = data;
		/* decode character graphics again */
		decodechar(machine->gfx[0], chr, pcgram);
	}
}

 READ8_HANDLER ( mbee_pcg_r )
{
	return pcgram[0x0800+offset];
}

WRITE8_HANDLER ( mbee_pcg_color_w )
{
	if( (m6545_video_bank & 0x01) || (mbee_pcg_color_latch & 0x40) == 0 )
	{
		if( pcgram[0x0800+offset] != data )
        	{
			int chr = 0x80 + offset / 16;
			pcgram[0x0800+offset] = data;
			/* decode character graphics again */
			decodechar(machine->gfx[0], chr, pcgram);
		}
	}
	else
		colorram[offset] = data;
}

 READ8_HANDLER ( mbee_pcg_color_r )
{

	if ( mbee_pcg_color_latch & 0x40 )
		return colorram[offset];
	else
		return pcgram[0x0800+offset];
}

static int keyboard_matrix_r(int offs)
{
	int port = (offs >> 7) & 7;
	int bit = (offs >> 4) & 7;
	int data = (readinputport(port) >> bit) & 1;
	int extra = readinputport(8);

	if( extra & 0x01 )	/* extra: cursor up */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 5 ) data = 1;	/* E */
	}
	if( extra & 0x02 )	/* extra: cursor down */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 3 && bit == 0 ) data = 1;	/* X */
	}
	if( extra & 0x04 )	/* extra: cursor left */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 3 ) data = 1;	/* S */
	}
	if( extra & 0x08 )	/* extra: cursor right */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 0 && bit == 4 ) data = 1;	/* D */
	}
	if( extra & 0x10 )	/* extra: insert */
	{
		if( port == 7 && bit == 1 ) data = 1;	/* Control */
		if( port == 2 && bit == 6 ) data = 1;	/* V */
	}
	if( data )
	{
		crt.lpen_lo = offs & 0xff;
		crt.lpen_hi = (offs >> 8) & 0x03;
		crt.lpen_strobe = 1;
//		logerror("mbee keyboard_matrix_r $%03X (port:%d bit:%d) = %d\n", offs, port, bit, data);
	}
	return data;
}

static void m6545_offset_xy(void)
{
	if( crt.horizontal_sync_pos )
		off_x = crt.horizontal_total - crt.horizontal_sync_pos - 23;
	else
		off_x = -24;

	off_y = (crt.vertical_total - crt.vertical_sync_pos) *
		(crt.scan_lines + 1) + crt.vertical_adjust;

	if( off_y < 0 )
		off_y = 0;

	if( off_y > 128 )
		off_y = 128;
}

 READ8_HANDLER ( mbee_color_bank_r )
{
	return m6545_color_bank;
}

WRITE8_HANDLER ( mbee_color_bank_w )
{
	m6545_color_bank = data;
}

 READ8_HANDLER ( mbee_video_bank_r )
{
	return m6545_video_bank;
}

WRITE8_HANDLER ( mbee_video_bank_w )
{
	m6545_video_bank = data;
}

static void m6545_update_strobe(int param)
{
	int data;
	data = keyboard_matrix_r(param);
	crt.update_strobe = 1;
//	if( data )
//		logerror("6545 update_strobe_cb $%04X = $%02X\n", param, data);
}

READ8_HANDLER ( m6545_status_r )
{
	const device_config *screen = video_screen_first(machine->config);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	int data = 0, y = video_screen_get_vpos(machine->primary_screen);

	if( y < visarea->min_y ||
		y > visarea->max_y )
		data |= 0x20;	/* vertical blanking */
	if( crt.lpen_strobe )
		data |= 0x40;	/* lpen register full */
	if( crt.update_strobe )
		data |= 0x80;	/* update strobe has occured */
//	logerror("6545 status_r $%02X\n", data);
	return data;
}

 READ8_HANDLER ( m6545_data_r )
{
	int addr, data = 0;

	switch( crt.idx )
	{
/* These are write only on a Rockwell 6545 */
#if 0
	case 0:
		return crt.horizontal_total;
	case 1:
		return crt.horizontal_displayed;
	case 2:
		return crt.horizontal_sync_pos;
	case 3:
		return crt.horizontal_length;
	case 4:
		return crt.vertical_total;
	case 5:
		return crt.vertical_adjust;
	case 6:
		return crt.vertical_displayed;
	case 7:
		return crt.vertical_sync_pos;
	case 8:
		return crt.crt_mode;
	case 9:
		return crt.scan_lines;
	case 10:
		return crt.cursor_top;
	case 11:
		return crt.cursor_bottom;
	case 12:
		return crt.screen_address_hi;
	case 13:
		return crt.screen_address_lo;
#endif
	case 14:
		data = crt.cursor_address_hi;
		break;
	case 15:
		data = crt.cursor_address_lo;
		break;
	case 16:
//		logerror("6545 lpen_hi_r $%02X (lpen:%d upd:%d)\n", crt.lpen_hi, crt.lpen_strobe, crt.update_strobe);
		crt.lpen_strobe = 0;
		crt.update_strobe = 0;
		data = crt.lpen_hi;
		break;
	case 17:
//		logerror("6545 lpen_lo_r $%02X (lpen:%d upd:%d)\n", crt.lpen_lo, crt.lpen_strobe, crt.update_strobe);
		crt.lpen_strobe = 0;
		crt.update_strobe = 0;
		data = crt.lpen_lo;
		break;
	case 18:
//		logerror("6545 transp_hi_r $%02X\n", crt.transp_hi);
		data = crt.transp_hi;
		break;
	case 19:
//		logerror("6545 transp_lo_r $%02X\n", crt.transp_lo);
		data = crt.transp_lo;
		break;
	case 31:
		/* shared memory latch */
		addr = (crt.transp_hi << 8) | crt.transp_lo;
//		logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(addr);
		break;
	default:
		logerror("6545 read unmapped port $%X\n", crt.idx);
	}
	return data;
}

WRITE8_HANDLER ( m6545_index_w )
{
	crt.idx = data & 0x1f;
}

WRITE8_HANDLER ( m6545_data_w )
{
	int addr, i;

	switch( crt.idx )
	{
	case 0:
		if( crt.horizontal_total == data )
			break;
		crt.horizontal_total = data;
		m6545_offset_xy();
		break;
	case 1:
		crt.horizontal_displayed = data;
		break;
	case 2:
		if( crt.horizontal_sync_pos == data )
			break;
		crt.horizontal_sync_pos = data;
		m6545_offset_xy();
		break;
	case 3:
		crt.horizontal_length = data;
		break;
	case 4:
		if( crt.vertical_total == data )
			break;
		crt.vertical_total = data;
		m6545_offset_xy();
		break;
	case 5:
		if( crt.vertical_adjust == data )
			break;
		crt.vertical_adjust = data;
		m6545_offset_xy();
		break;
	case 6:
		crt.vertical_displayed = data;
		break;
	case 7:
		if( crt.vertical_sync_pos == data )
			break;
		crt.vertical_sync_pos = data;
		m6545_offset_xy();
		break;
	case 8:
		crt.crt_mode = data;

		{
			logerror("6545 mode_w $%02X\n", data);
			logerror("     interlace          %d\n", data & 3);
			logerror("     addr mode          %d\n", (data >> 2) & 1);
			logerror("     refresh RAM        %s\n", ((data >> 3) & 1) ? "transparent" : "shared");
			logerror("     disp enb, skew     %d\n", (data >> 4) & 3);
			logerror("     pin 34             %s\n", ((data >> 6) & 1) ? "update strobe" : "RA4");
			logerror("     update read mode   %s\n", ((data >> 7) & 1) ? "interleaved" : "during h/v-blank");
		}
		break;
	case 9:
		data &= 15;
		if( crt.scan_lines == data )
			break;
		crt.scan_lines = data;
		m6545_offset_xy();
		break;
	case 10:
		crt.cursor_top = data;
		break;
	case 11:
		crt.cursor_bottom = data;
		break;
	case 12:
		data &= 0x3f;
		if( crt.screen_address_hi == data )
			break;
		crt.screen_address_hi = data;
		addr = 0x17000+((data & 32) << 6);
		memcpy(memory_region(REGION_CPU1)+0xf000, memory_region(REGION_CPU1)+addr, 0x800);
		for (i = 0; i < 128; i++)
				decodechar(machine->gfx[0],i, pcgram);
		break;
	case 13:
		crt.screen_address_lo = data;
		break;
	case 14:
		crt.cursor_address_hi = data & 0x3f;
		break;
	case 15:
		crt.cursor_address_lo = data;
		break;
	case 16:
		/* lpen hi is read only */
		break;
	case 17:
		/* lpen lo is read only */
		break;
	case 18:
		data &= 63;
		crt.transp_hi = data;
//		logerror("6545 transp_hi_w $%02X\n", data);
		break;
	case 19:
		crt.transp_lo = data;
//		logerror("6545 transp_lo_w $%02X\n", data);
		break;
	case 31:
		/* shared memory latch */
		addr = (crt.transp_hi << 8) | crt.transp_lo;
//		logerror("6545 transp_latch $%04X\n", addr);
		m6545_update_strobe(addr);
		break;
	default:
		logerror("6545 write unmapped port $%X <- $%02X\n", crt.idx, data);
	}
}

VIDEO_START( mbee )
{
	videoram = auto_malloc(0x800);
	pcgram = memory_region(REGION_CPU1)+0xf000;
}

VIDEO_START( mbeeic )
{
	videoram = auto_malloc(0x800);
	colorram = auto_malloc(0x800);
	pcgram = memory_region(REGION_CPU1)+0xf000;
}

VIDEO_UPDATE( mbee )
{
	int offs, cursor, screen_;

	for( offs = 0x000; offs < 0x380; offs += 0x10 )
		keyboard_matrix_r(offs);

	framecnt++;

	cursor = (crt.cursor_address_hi << 8) | crt.cursor_address_lo;
	screen_ = (crt.screen_address_hi << 8) | crt.screen_address_lo;

	for (offs = 0; offs < crt.horizontal_displayed * crt.vertical_displayed; offs++)
	{
		int mem = ((offs + screen_) & 0x7ff);
		int sy = off_y - 9 + (offs / crt.horizontal_displayed) * (crt.scan_lines + 1);
		int sx = (off_x + 3 + (offs % crt.horizontal_displayed)) << 3;
		int code = videoram[mem];
		drawgfx( bitmap, screen->machine->gfx[0],code,0,0,0,sx,sy,
			NULL,TRANSPARENCY_NONE,0);

		if( mem == cursor && (crt.cursor_top & 0x60) != 0x20 )
		{
			if( (crt.cursor_top & 0x60) == 0x60 || (framecnt & 16) == 0 )
			{
				int x, y;
		                for( y = (crt.cursor_top & 31); y <= (crt.cursor_bottom & 31); y++ )
				{
					if( y > crt.scan_lines )
						break;
					for( x = 0; x < 8; x++ )
						*BITMAP_ADDR16(bitmap, sy+y, sx+x) = 1;
				}
			}
		}
	}
		
	return 0;
}

VIDEO_UPDATE( mbeeic )
{
	int offs, cursor, screen_;
	UINT16 colourm = (mbee_pcg_color_latch & 0x0e) << 7;

	for( offs = 0x000; offs < 0x380; offs += 0x10 )
		keyboard_matrix_r(offs);

	framecnt++;

	cursor = (crt.cursor_address_hi << 8) | crt.cursor_address_lo;
	screen_ = (crt.screen_address_hi << 8) | crt.screen_address_lo;

	for (offs = 0; offs < crt.horizontal_displayed * crt.vertical_displayed; offs++)
	{
		int mem = ((offs + screen_) & 0x7ff);
		int sy = off_y + (offs / crt.horizontal_displayed) * (crt.scan_lines + 1);
		int sx = (off_x + (offs % crt.horizontal_displayed)) << 3;
		int code = videoram[mem];
		int color = colorram[mem] | colourm;
		drawgfx( bitmap, screen->machine->gfx[0],code,color,0,0,sx,sy,
			NULL, TRANSPARENCY_NONE, 0);

		if( mem == cursor && (crt.cursor_top & 0x60) != 0x20 )
		{
			if( (crt.cursor_top & 0x60) == 0x60 || (framecnt & 16) == 0 )
			{
				int x, y;
		                for( y = (crt.cursor_top & 31); y <= (crt.cursor_bottom & 31); y++ )
				{
					if( y > crt.scan_lines )
						break;
					for( x = 0; x < 8; x++ )
						*BITMAP_ADDR16(bitmap, sy+y, sx+x) = ((color<<1)+1); /* convert to palette entry number */
				}
			}
		}
	}
	return 0;
}
