/*****************************************************************************
 *
 * includes/dai.h
 *
 ****************************************************************************/

#ifndef DAI_H_
#define DAI_H_

#include "cpu/i8085/i8085.h"
#include "machine/i8255.h"
#include "machine/pit8253.h"
#include "machine/ram.h"
#include "machine/tms5501.h"
#include "imagedev/cassette.h"
#include "sound/wave.h"


// ======================> dai_sound_device

class dai_sound_device : public device_t,
							public device_sound_interface
{
public:
	// construction/destruction
	dai_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_WRITE_LINE_MEMBER(set_input_ch0);
	DECLARE_WRITE_LINE_MEMBER(set_input_ch1);
	DECLARE_WRITE_LINE_MEMBER(set_input_ch2);
	DECLARE_WRITE8_MEMBER(set_volume);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

private:
	sound_stream *      m_mixer_channel;
	int                 m_dai_input[3];
	UINT8               m_osc_volume[3];
	UINT8               m_noise_volume;

	static const UINT16 s_osc_volume_table[];
	static const UINT16 s_noise_volume_table[];
};



class dai_state : public driver_device
{
public:
	dai_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_pit(*this, "pit8253"),
		m_tms5501(*this, "tms5501"),
		m_sound(*this, "custom"),
		m_maincpu(*this, "maincpu"),
		m_cassette(*this, "cassette"),
		m_ram(*this, RAM_TAG) { }

	required_device<pit8253_device> m_pit;
	required_device<tms5501_device> m_tms5501;
	required_device<dai_sound_device> m_sound;

	UINT8 m_paddle_select;
	UINT8 m_paddle_enable;
	UINT8 m_cassette_motor[2];
	UINT8 m_keyboard_scan_mask;
	unsigned short m_4_colours_palette[4];
	DECLARE_WRITE8_MEMBER(dai_stack_interrupt_circuit_w);
	DECLARE_READ8_MEMBER(dai_io_discrete_devices_r);
	DECLARE_WRITE8_MEMBER(dai_io_discrete_devices_w);
	DECLARE_READ8_MEMBER(dai_amd9511_r);
	DECLARE_WRITE8_MEMBER(dai_amd9511_w);
	DECLARE_READ8_MEMBER(dai_pit_r);
	DECLARE_WRITE8_MEMBER(dai_pit_w);
	DECLARE_READ8_MEMBER(dai_keyboard_r);
	DECLARE_WRITE8_MEMBER(dai_keyboard_w);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_dai(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_CALLBACK_MEMBER(dai_bootstrap_callback);
	TIMER_CALLBACK_MEMBER(dai_timer);
	required_device<cpu_device> m_maincpu;
	required_device<cassette_image_device> m_cassette;
	required_device<ram_device> m_ram;
	void dai_update_memory(int dai_rom_bank);
};


/*----------- defined in machine/dai.c -----------*/

extern const struct pit8253_config dai_pit8253_intf;
extern const i8255_interface dai_ppi82555_intf;
extern const tms5501_interface dai_tms5501_interface;


/*----------- defined in video/dai.c -----------*/

extern const unsigned char dai_palette[16*3];


/*----------- defined in audio/dai.c -----------*/

extern const device_type DAI_SOUND;

#endif /* DAI_H_ */
