#include "video/bufsprite.h"

class lemmings_state : public driver_device
{
public:
	lemmings_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_bitmap0(2048, 256),
		  m_spriteram(*this, "spriteram"),
		  m_spriteram2(*this, "spriteram2") { }

	/* memory pointers */
	UINT16 *  m_pixel_0_data;
	UINT16 *  m_pixel_1_data;
	UINT16 *  m_vram_data;
	UINT16 *  m_control_data;
	UINT16 *  m_paletteram;

	/* video-related */
	bitmap_ind16 m_bitmap0;
	tilemap_t *m_vram_tilemap;
	UINT16 m_sprite_triple_buffer_0[0x800];
	UINT16 m_sprite_triple_buffer_1[0x800];
	UINT8 m_vram_buffer[2048 * 64];	// 64 bytes per VRAM character

	/* devices */
	device_t *m_audiocpu;
	required_device<buffered_spriteram16_device> m_spriteram;
	required_device<buffered_spriteram16_device> m_spriteram2;
};


/*----------- defined in video/lemmings.c -----------*/

WRITE16_HANDLER( lemmings_pixel_0_w );
WRITE16_HANDLER( lemmings_pixel_1_w );
WRITE16_HANDLER( lemmings_vram_w );

VIDEO_START( lemmings );
SCREEN_VBLANK( lemmings );
SCREEN_UPDATE_RGB32( lemmings );

