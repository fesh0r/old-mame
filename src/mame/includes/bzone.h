/*************************************************************************

    Atari Battle Zone hardware

*************************************************************************/

#include "sound/discrete.h"

#define BZONE_MASTER_CLOCK (XTAL_12_096MHz)
#define BZONE_CLOCK_3KHZ  (MASTER_CLOCK / 4096)

class bzone_state : public driver_device
{
public:
	bzone_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_discrete(*this, "discrete") ,
		m_maincpu(*this, "maincpu") { }

	optional_device<discrete_device> m_discrete;

	UINT8 m_analog_data;
	UINT8 m_rb_input_select;
	DECLARE_WRITE8_MEMBER(bzone_coin_counter_w);
	DECLARE_READ8_MEMBER(analog_data_r);
	DECLARE_WRITE8_MEMBER(analog_select_w);
	DECLARE_CUSTOM_INPUT_MEMBER(clock_r);
	DECLARE_READ8_MEMBER(redbaron_joy_r);
	DECLARE_WRITE8_MEMBER(redbaron_joysound_w);
	DECLARE_DRIVER_INIT(bradley);
	virtual void machine_start();
	DECLARE_MACHINE_START(redbaron);
	INTERRUPT_GEN_MEMBER(bzone_interrupt);
	DECLARE_WRITE8_MEMBER(bzone_sounds_w);
	required_device<cpu_device> m_maincpu;
};


/*----------- defined in audio/bzone.c -----------*/
MACHINE_CONFIG_EXTERN( bzone_audio );


/*----------- defined in audio/redbaron.c -----------*/

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> redbaron_sound_device

class redbaron_sound_device : public device_t,
								public device_sound_interface
{
public:
	redbaron_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~redbaron_sound_device() { }

protected:
	// device-level overrides
	virtual void device_start();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

public:
	DECLARE_WRITE8_MEMBER( redbaron_sounds_w );

private:
	INT16 *m_vol_lookup;

	INT16 m_vol_crash[16];

	sound_stream *m_channel;
	int m_latch;
	int m_poly_counter;
	int m_poly_shift;

	int m_filter_counter;

	int m_crash_amp;
	int m_shot_amp;
	int m_shot_amp_counter;

	int m_squeal_amp;
	int m_squeal_amp_counter;
	int m_squeal_off_counter;
	int m_squeal_on_counter;
	int m_squeal_out;
};

extern const device_type REDBARON;
