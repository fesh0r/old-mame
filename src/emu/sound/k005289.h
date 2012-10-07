#pragma once

#ifndef __K005289_H__
#define __K005289_H__

#include "devlegcy.h"

DECLARE_WRITE8_DEVICE_HANDLER( k005289_control_A_w );
DECLARE_WRITE8_DEVICE_HANDLER( k005289_control_B_w );
DECLARE_WRITE8_DEVICE_HANDLER( k005289_pitch_A_w );
DECLARE_WRITE8_DEVICE_HANDLER( k005289_pitch_B_w );
DECLARE_WRITE8_DEVICE_HANDLER( k005289_keylatch_A_w );
DECLARE_WRITE8_DEVICE_HANDLER( k005289_keylatch_B_w );

class k005289_device : public device_t,
                                  public device_sound_interface
{
public:
	k005289_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~k005289_device() { global_free(m_token); }

	// access to legacy token
	void *token() const { assert(m_token != NULL); return m_token; }
protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
private:
	// internal state
	void *m_token;
};

extern const device_type K005289;


#endif /* __K005289_H__ */
