#include "video/bufsprite.h"
#include "sound/upd7759.h"
#include "sound/k007232.h"

class twin16_state : public driver_device
{
public:
	twin16_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_spriteram(*this, "spriteram") ,
		m_text_ram(*this, "text_ram"),
		m_videoram(*this, "videoram"),
		m_tile_gfx_ram(*this, "tile_gfx_ram"),
		m_sprite_gfx_ram(*this, "sprite_gfx_ram"),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_subcpu(*this, "sub"),
		m_k007232(*this, "k007232"),
		m_upd7759(*this, "upd") { }

	required_device<buffered_spriteram16_device> m_spriteram;
	required_shared_ptr<UINT16> m_text_ram;
	required_shared_ptr<UINT16> m_videoram;
	optional_shared_ptr<UINT16> m_tile_gfx_ram;
	optional_shared_ptr<UINT16> m_sprite_gfx_ram;

	UINT16 m_CPUA_register;
	UINT16 m_CPUB_register;
	UINT16 m_sound_command;
	int m_cuebrickj_nvram_bank;
	UINT16 m_cuebrickj_nvram[0x400*0x20];
	UINT16 m_custom_video;
	UINT16 *m_gfx_rom;
	UINT16 m_sprite_buffer[0x800];
	emu_timer *m_sprite_timer;
	int m_sprite_busy;
	int m_need_process_spriteram;
	UINT16 m_gfx_bank;
	UINT16 m_scrollx[3];
	UINT16 m_scrolly[3];
	UINT16 m_video_register;
	tilemap_t *m_text_tilemap;
	DECLARE_READ16_MEMBER(videoram16_r);
	DECLARE_WRITE16_MEMBER(videoram16_w);
	DECLARE_READ16_MEMBER(extra_rom_r);
	DECLARE_READ16_MEMBER(twin16_gfx_rom1_r);
	DECLARE_READ16_MEMBER(twin16_gfx_rom2_r);
	DECLARE_WRITE16_MEMBER(sound_command_w);
	DECLARE_WRITE16_MEMBER(twin16_CPUA_register_w);
	DECLARE_WRITE16_MEMBER(twin16_CPUB_register_w);
	DECLARE_WRITE16_MEMBER(fround_CPU_register_w);
	DECLARE_READ16_MEMBER(twin16_input_r);
	DECLARE_READ16_MEMBER(cuebrickj_nvram_r);
	DECLARE_WRITE16_MEMBER(cuebrickj_nvram_w);
	DECLARE_WRITE16_MEMBER(cuebrickj_nvram_bank_w);
	DECLARE_WRITE16_MEMBER(twin16_text_ram_w);
	DECLARE_WRITE16_MEMBER(twin16_paletteram_word_w);
	DECLARE_WRITE16_MEMBER(fround_gfx_bank_w);
	DECLARE_WRITE16_MEMBER(twin16_video_register_w);
	DECLARE_READ16_MEMBER(twin16_sprite_status_r);
	DECLARE_READ8_MEMBER(twin16_upd_busy_r);
	DECLARE_WRITE8_MEMBER(twin16_upd_reset_w);
	DECLARE_WRITE8_MEMBER(twin16_upd_start_w);
	DECLARE_DRIVER_INIT(fround);
	DECLARE_DRIVER_INIT(twin16);
	DECLARE_DRIVER_INIT(cuebrickj);
	TILE_GET_INFO_MEMBER(get_text_tile_info);
	DECLARE_MACHINE_START(twin16);
	DECLARE_MACHINE_RESET(twin16);
	DECLARE_VIDEO_START(twin16);
	UINT32 screen_update_twin16(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void screen_eof_twin16(screen_device &screen, bool state);
	INTERRUPT_GEN_MEMBER(CPUA_interrupt);
	INTERRUPT_GEN_MEMBER(CPUB_interrupt);
	TIMER_CALLBACK_MEMBER(twin16_sprite_tick);
	int twin16_set_sprite_timer(  );
	void twin16_spriteram_process(  );
	void draw_sprites( screen_device &screen, bitmap_ind16 &bitmap );
	void draw_layer( screen_device &screen, bitmap_ind16 &bitmap, int opaque );
	int twin16_spriteram_process_enable(  );
	void gfx_untangle(  );
	DECLARE_WRITE8_MEMBER(volume_callback);
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	optional_device<cpu_device> m_subcpu;
	required_device<k007232_device> m_k007232;
	required_device<upd7759_device> m_upd7759;
};
