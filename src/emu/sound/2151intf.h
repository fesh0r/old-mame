#pragma once

#ifndef __2151INTF_H__
#define __2151INTF_H__

#include "devlegcy.h"

struct ym2151_interface
{
	devcb_write_line irqhandler;
	devcb_write8 portwritehandler;
};

DECLARE_READ8_DEVICE_HANDLER( ym2151_r );
DECLARE_WRITE8_DEVICE_HANDLER( ym2151_w );

DECLARE_READ8_DEVICE_HANDLER( ym2151_status_port_r );
DECLARE_WRITE8_DEVICE_HANDLER( ym2151_register_port_w );
DECLARE_WRITE8_DEVICE_HANDLER( ym2151_data_port_w );

class ym2151_device : public device_t,
                                  public device_sound_interface
{
public:
	ym2151_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~ym2151_device() { global_free(m_token); }

	// access to legacy token
	void *token() const { assert(m_token != NULL); return m_token; }
protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_stop();
	virtual void device_reset();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);
private:
	// internal state
	void *m_token;
};

extern const device_type YM2151;


#endif /* __2151INTF_H__ */
