class mustache_state : public driver_device
{
public:
	mustache_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
	emu_timer *clear_irq_timer;
	tilemap_t *bg_tilemap;
	int control_byte;
	UINT8 *spriteram;
	size_t spriteram_size;
};


/*----------- defined in video/mustache.c -----------*/

WRITE8_HANDLER( mustache_videoram_w );
WRITE8_HANDLER( mustache_scroll_w );
WRITE8_HANDLER( mustache_video_control_w );
VIDEO_START( mustache );
SCREEN_UPDATE( mustache );
PALETTE_INIT( mustache );
