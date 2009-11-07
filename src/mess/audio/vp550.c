/**********************************************************************

    RCA VP550 - VIP Super Sound System emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

/*

    TODO:

    - tempo control
    - mono/stereo mode
    - VP551 memory map

*/

#include "driver.h"
#include "vp550.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1863.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define MAX_CHANNELS	4

#define CDP1863_A_TAG	"u1"
#define CDP1863_B_TAG	"u2"
#define CDP1863_C_TAG	"cdp1863c"
#define CDP1863_D_TAG	"cdp1863d"

enum
{
	CHANNEL_A = 0,
	CHANNEL_B,
	CHANNEL_C,
	CHANNEL_D
};

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _vp550_t vp550_t;
struct _vp550_t
{
	int channels;						/* number of channels */

	/* devices */
	const device_config *cdp1863[MAX_CHANNELS];
	const device_config *sync_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE vp550_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == VP550) || (device->type == VP551));
	return (vp550_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    vp550_q_w - Q line write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vp550_q_w )
{
	vp550_t *vp550 = get_safe_token(device);

	int channel;

	for (channel = CHANNEL_A; channel < vp550->channels; channel++)
	{
		cdp1863_oe_w(vp550->cdp1863[channel], state);
	}
}

/*-------------------------------------------------
    vp550_sc1_w - SC1 line write
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( vp550_sc1_w )
{
	if (state)
	{
		cpu_set_input_line(device, CDP1802_INPUT_LINE_INT, CLEAR_LINE);

		if (LOG) logerror("VP550 Clear Interrupt\n");
	}
}

/*-------------------------------------------------
    vp550_octave_w - octave select write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_octave_w )
{
	vp550_t *vp550 = get_safe_token(device);

	int channel = (data >> 2) & 0x03;
	int clock = 0;

	if (data & 0x10)
	{
		switch (data & 0x03)
		{
		case 0: clock = device->clock / 8; break;
		case 1: clock = device->clock / 4; break;
		case 2: clock = device->clock / 2; break;
		case 3: clock = device->clock;	   break;
		}
	}

	if (vp550->cdp1863[channel]) cdp1863_set_clk2(vp550->cdp1863[channel], clock);

	if (LOG) logerror("VP550 Clock %c: %u Hz\n", 'A' + channel, clock);
}

/*-------------------------------------------------
    vp550_vlmn_w - amplitude write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_vlmn_w )
{
	//float gain = 0.625f * (data & 0x0f);

//  sound_set_output_gain(device, 0, gain);

	if (LOG) logerror("VP550 '%s' Volume: %u\n", device->tag, data & 0x0f);
}

/*-------------------------------------------------
    vp550_sync_w - interrupt enable write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( vp550_sync_w )
{
	timer_device_enable(device, BIT(data, 0));

	if (LOG) logerror("VP550 Interrupt Enable: %u\n", BIT(data, 0));
}

/*-------------------------------------------------
    vp550_install_readwrite_handler - install
    or uninstall write handlers
-------------------------------------------------*/

void vp550_install_write_handlers(const device_config *device, const address_space *program, int enabled)
{
	vp550_t *vp550 = get_safe_token(device);

	if (enabled)
	{
		memory_install_write8_device_handler(program, vp550->cdp1863[CHANNEL_A], 0x8001, 0x8001, 0, 0, cdp1863_str_w);
		memory_install_write8_device_handler(program, vp550->cdp1863[CHANNEL_B], 0x8002, 0x8002, 0, 0, cdp1863_str_w);
		memory_install_write8_device_handler(program, device, 0x8003, 0x8003, 0, 0, vp550_octave_w);
		memory_install_write8_device_handler(program, vp550->cdp1863[CHANNEL_A], 0x8010, 0x8010, 0, 0, vp550_vlmn_w);
		memory_install_write8_device_handler(program, vp550->cdp1863[CHANNEL_B], 0x8020, 0x8020, 0, 0, vp550_vlmn_w);
		memory_install_write8_device_handler(program, vp550->sync_timer, 0x8030, 0x8030, 0, 0, vp550_sync_w);
	}
	else
	{
		memory_install_write8_handler(program, 0x8001, 0x8001, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8002, 0x8002, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8003, 0x8003, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8010, 0x8010, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8020, 0x8020, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(program, 0x8030, 0x8030, 0, 0, SMH_UNMAP);
	}
}

/*-------------------------------------------------
    vp551_install_readwrite_handler - install
    or uninstall write handlers
-------------------------------------------------*/

void vp551_install_write_handlers(const device_config *device, const address_space *program, int enabled)
{
}

/*-------------------------------------------------
    TIMER_DEVICE_CALLBACK( sync_tick )
-------------------------------------------------*/

static TIMER_DEVICE_CALLBACK( sync_tick )
{
	cpu_set_input_line(timer->machine->firstcpu, CDP1802_INPUT_LINE_INT, ASSERT_LINE);

	if (LOG) logerror("VP550 Interrupt\n");
}

/*-------------------------------------------------
    MACHINE_DRIVER( vp550 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( vp550 )
	MDRV_TIMER_ADD_PERIODIC("sync", sync_tick, HZ(50))

	MDRV_CDP1863_ADD(CDP1863_A_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CDP1863_ADD(CDP1863_B_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( vp551 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( vp551 )
	MDRV_IMPORT_FROM(vp550)

	MDRV_CDP1863_ADD(CDP1863_C_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_CDP1863_ADD(CDP1863_D_TAG, 0, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/*-------------------------------------------------
    DEVICE_START( vp550 )
-------------------------------------------------*/

static DEVICE_START( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* look up devices */
	vp550->cdp1863[CHANNEL_A] = devtag_get_device(device->machine, "vp550:u1");
	vp550->cdp1863[CHANNEL_B] = devtag_get_device(device->machine, "vp550:u2");
	vp550->sync_timer = devtag_get_device(device->machine, "vp550:sync");

	/* set initial values */
	vp550->channels = 2;
}

/*-------------------------------------------------
    DEVICE_START( vp551 )
-------------------------------------------------*/

static DEVICE_START( vp551 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* look up devices */
	vp550->cdp1863[CHANNEL_A] = device_find_child_by_tag(device, CDP1863_A_TAG);
	vp550->cdp1863[CHANNEL_B] = device_find_child_by_tag(device, CDP1863_B_TAG);
	vp550->cdp1863[CHANNEL_C] = device_find_child_by_tag(device, CDP1863_C_TAG);
	vp550->cdp1863[CHANNEL_D] = device_find_child_by_tag(device, CDP1863_D_TAG);
	vp550->sync_timer = device_find_child_by_tag(device, "sync");

	/* set initial values */
	vp550->channels = 4;
}

/*-------------------------------------------------
    DEVICE_RESET( vp550 )
-------------------------------------------------*/

static DEVICE_RESET( vp550 )
{
	vp550_t *vp550 = get_safe_token(device);

	/* reset chips */
	device_reset(vp550->cdp1863[CHANNEL_A]);
	device_reset(vp550->cdp1863[CHANNEL_B]);

	/* disable interrupt timer */
	timer_device_enable(vp550->sync_timer, 0);

	/* clear interrupt */
	cpu_set_input_line(device->machine->firstcpu, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
}

/*-------------------------------------------------
    DEVICE_RESET( vp551 )
-------------------------------------------------*/

static DEVICE_RESET( vp551 )
{
	vp550_t *vp550 = get_safe_token(device);

	DEVICE_RESET_CALL(vp550);

	/* reset chips */
	device_reset(vp550->cdp1863[CHANNEL_C]);
	device_reset(vp550->cdp1863[CHANNEL_D]);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vp550 )
-------------------------------------------------*/

DEVICE_GET_INFO( vp550 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vp550_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME( vp550 );	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp550);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vp550);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP550");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( vp551 )
-------------------------------------------------*/

DEVICE_GET_INFO( vp551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(vp550_t);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;			break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME( vp551 );	break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(vp551);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(vp551);		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "RCA VP551");				break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "RCA VIP");					break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");						break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);					break;
		case DEVINFO_STR_CREDITS:						/* Nothing */								break;
	}
}
