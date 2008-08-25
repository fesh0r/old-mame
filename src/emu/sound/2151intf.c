/***************************************************************************

  2151intf.c

  Support interface YM2151(OPM)

***************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "deprecat.h"
#include "fm.h"
#include "2151intf.h"
#include "ym2151.h"


struct ym2151_info
{
	sound_stream *	stream;
	emu_timer *	timer[2];
	void *			chip;
	const ym2151_interface *intf;
};


static void ym2151_update(void *param, stream_sample_t **inputs, stream_sample_t **buffers, int length)
{
	struct ym2151_info *info = param;
	ym2151_update_one(info->chip, buffers, length);
}


static STATE_POSTLOAD( ym2151intf_postload )
{
	struct ym2151_info *info = param;
	ym2151_postload(machine, info->chip);
}


static void *ym2151_start(const char *tag, int sndindex, int clock, const void *config)
{
	static const ym2151_interface dummy = { 0 };
	struct ym2151_info *info;
	int rate;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config ? config : &dummy;

	rate = clock/64;

	/* stream setup */
	info->stream = stream_create(0,2,rate,info,ym2151_update);

	info->chip = ym2151_init(sndindex,clock,rate);

	state_save_register_postload(Machine, ym2151intf_postload, info);

	if (info->chip != 0)
	{
		ym2151_set_irq_handler(info->chip,info->intf->irqhandler);
		ym2151_set_port_write_handler(info->chip,info->intf->portwritehandler);
		return info;
	}
	return NULL;
}


static void ym2151_stop(void *token)
{
	struct ym2151_info *info = token;
	ym2151_shutdown(info->chip);
}

static void ym2151_reset(void *token)
{
	struct ym2151_info *info = token;
	ym2151_reset_chip(info->chip);
}

static int lastreg0,lastreg1,lastreg2;

READ8_HANDLER( ym2151_status_port_0_r )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 0);
	stream_update(token->stream);
	return ym2151_read_status(token->chip);
}

READ8_HANDLER( ym2151_status_port_1_r )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 1);
	stream_update(token->stream);
	return ym2151_read_status(token->chip);
}

READ8_HANDLER( ym2151_status_port_2_r )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 2);
	stream_update(token->stream);
	return ym2151_read_status(token->chip);
}

WRITE8_HANDLER( ym2151_register_port_0_w )
{
	lastreg0 = data;
}
WRITE8_HANDLER( ym2151_register_port_1_w )
{
	lastreg1 = data;
}
WRITE8_HANDLER( ym2151_register_port_2_w )
{
	lastreg2 = data;
}

WRITE8_HANDLER( ym2151_data_port_0_w )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 0);
	stream_update(token->stream);
	ym2151_write_reg(token->chip,lastreg0,data);
}

WRITE8_HANDLER( ym2151_data_port_1_w )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 1);
	stream_update(token->stream);
	ym2151_write_reg(token->chip,lastreg1,data);
}

WRITE8_HANDLER( ym2151_data_port_2_w )
{
	struct ym2151_info *token = sndti_token(SOUND_YM2151, 2);
	stream_update(token->stream);
	ym2151_write_reg(token->chip,lastreg2,data);
}

WRITE8_HANDLER( ym2151_word_0_w )
{
	if (offset)
		ym2151_data_port_0_w(machine,0,data);
	else
		ym2151_register_port_0_w(machine,0,data);
}

WRITE8_HANDLER( ym2151_word_1_w )
{
	if (offset)
		ym2151_data_port_1_w(machine,0,data);
	else
		ym2151_register_port_1_w(machine,0,data);
}

READ16_HANDLER( ym2151_status_port_0_lsb_r )
{
	return ym2151_status_port_0_r(machine,0);
}

READ16_HANDLER( ym2151_status_port_1_lsb_r )
{
	return ym2151_status_port_1_r(machine,0);
}

READ16_HANDLER( ym2151_status_port_2_lsb_r )
{
	return ym2151_status_port_2_r(machine,0);
}


WRITE16_HANDLER( ym2151_register_port_0_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_register_port_0_w(machine, 0, data & 0xff);
}

WRITE16_HANDLER( ym2151_register_port_1_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_register_port_1_w(machine, 0, data & 0xff);
}

WRITE16_HANDLER( ym2151_register_port_2_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_register_port_2_w(machine, 0, data & 0xff);
}

WRITE16_HANDLER( ym2151_data_port_0_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_data_port_0_w(machine, 0, data & 0xff);
}

WRITE16_HANDLER( ym2151_data_port_1_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_data_port_1_w(machine, 0, data & 0xff);
}

WRITE16_HANDLER( ym2151_data_port_2_lsb_w )
{
	if (ACCESSING_BITS_0_7)
		ym2151_data_port_2_w(machine, 0, data & 0xff);
}


/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ym2151_set_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ym2151_get_info(void *token, UINT32 state, sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ym2151_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ym2151_start;				break;
		case SNDINFO_PTR_STOP:							info->stop = ym2151_stop;				break;
		case SNDINFO_PTR_RESET:							info->reset = ym2151_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YM2151";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright Nicola Salmoria and the MAME Team"; break;
	}
}
