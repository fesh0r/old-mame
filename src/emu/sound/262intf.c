/***************************************************************************

  262intf.c

  MAME interface for YMF262 (OPL3) emulator

***************************************************************************/
#include "emu.h"
#include "262intf.h"
#include "ymf262.h"


struct ymf262_state
{
	sound_stream *  stream;
	emu_timer *     timer[2];
	void *          chip;
	const ymf262_interface *intf;
	device_t *device;
	devcb_resolved_write_line irqhandler;
};


INLINE ymf262_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == YMF262);
	return (ymf262_state *)downcast<ymf262_device *>(device)->token();
}




static void IRQHandler_262(void *param,int irq)
{
	ymf262_state *info = (ymf262_state *)param;
	if (!info->irqhandler.isnull())
		info->irqhandler(irq);
}

static TIMER_CALLBACK( timer_callback_262_0 )
{
	ymf262_state *info = (ymf262_state *)ptr;
	ymf262_timer_over(info->chip, 0);
}

static TIMER_CALLBACK( timer_callback_262_1 )
{
	ymf262_state *info = (ymf262_state *)ptr;
	ymf262_timer_over(info->chip, 1);
}

static void timer_handler_262(void *param,int timer, attotime period)
{
	ymf262_state *info = (ymf262_state *)param;
	if( period == attotime::zero )
	{   /* Reset FM Timer */
		info->timer[timer]->enable(false);
	}
	else
	{   /* Start FM Timer */
		info->timer[timer]->adjust(period);
	}
}

static STREAM_UPDATE( ymf262_stream_update )
{
	ymf262_state *info = (ymf262_state *)param;
	ymf262_update_one(info->chip, outputs, samples);
}

static void _stream_update(void *param, int interval)
{
	ymf262_state *info = (ymf262_state *)param;
	info->stream->update();
}


static DEVICE_START( ymf262 )
{
	static const ymf262_interface dummy = { DEVCB_NULL };
	ymf262_state *info = get_safe_token(device);
	int rate = device->clock()/288;

	info->intf = device->static_config() ? (const ymf262_interface *)device->static_config() : &dummy;
	info->device = device;
	info->irqhandler.resolve(info->intf->irqhandler, *device);

	/* stream system initialize */
	info->chip = ymf262_init(device,device->clock(),rate);
	assert_always(info->chip != NULL, "Error creating YMF262 chip");

	info->stream = device->machine().sound().stream_alloc(*device,0,4,rate,info,ymf262_stream_update);

	/* YMF262 setup */
	ymf262_set_timer_handler (info->chip, timer_handler_262, info);
	ymf262_set_irq_handler   (info->chip, IRQHandler_262, info);
	ymf262_set_update_handler(info->chip, _stream_update, info);

	info->timer[0] = device->machine().scheduler().timer_alloc(FUNC(timer_callback_262_0), info);
	info->timer[1] = device->machine().scheduler().timer_alloc(FUNC(timer_callback_262_1), info);
}

static DEVICE_STOP( ymf262 )
{
	ymf262_state *info = get_safe_token(device);
	ymf262_shutdown(info->chip);
}

/* reset */
static DEVICE_RESET( ymf262 )
{
	ymf262_state *info = get_safe_token(device);
	ymf262_reset_chip(info->chip);
}


READ8_DEVICE_HANDLER( ymf262_r )
{
	ymf262_state *info = get_safe_token(device);
	return ymf262_read(info->chip, offset & 3);
}

WRITE8_DEVICE_HANDLER( ymf262_w )
{
	ymf262_state *info = get_safe_token(device);
	ymf262_write(info->chip, offset & 3, data);
}

READ8_DEVICE_HANDLER ( ymf262_status_r ) { return ymf262_r(device, space, 0); }
WRITE8_DEVICE_HANDLER( ymf262_register_a_w ) { ymf262_w(device, space, 0, data); }
WRITE8_DEVICE_HANDLER( ymf262_register_b_w ) { ymf262_w(device, space, 2, data); }
WRITE8_DEVICE_HANDLER( ymf262_data_a_w ) { ymf262_w(device, space, 1, data); }
WRITE8_DEVICE_HANDLER( ymf262_data_b_w ) { ymf262_w(device, space, 3, data); }

const device_type YMF262 = &device_creator<ymf262_device>;

ymf262_device::ymf262_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, YMF262, "YMF262", tag, owner, clock),
		device_sound_interface(mconfig, *this)
{
	m_token = global_alloc_clear(ymf262_state);
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ymf262_device::device_config_complete()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ymf262_device::device_start()
{
	DEVICE_START_NAME( ymf262 )(this);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void ymf262_device::device_reset()
{
	DEVICE_RESET_NAME( ymf262 )(this);
}

//-------------------------------------------------
//  device_stop - device-specific stop
//-------------------------------------------------

void ymf262_device::device_stop()
{
	DEVICE_STOP_NAME( ymf262 )(this);
}

//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void ymf262_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	// should never get here
	fatalerror("sound_stream_update called; not applicable to legacy sound devices\n");
}
