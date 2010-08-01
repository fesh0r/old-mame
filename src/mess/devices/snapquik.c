/*********************************************************************

    snapquik.h

    Snapshots and quickloads

*********************************************************************/

#include "emu.h"
#include "snapquik.h"
#include "z80bin.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _snapquick_token snapquick_token;
struct _snapquick_token
{
	emu_timer *timer;
	snapquick_load_func load;
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    assert_is_snapshot_or_quickload - asserts/confirms
    that a given device is a snapshot or quickload
-------------------------------------------------*/

INLINE void assert_is_snapshot_or_quickload(running_device *device)
{
	assert(device != NULL);
	assert(downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config() != NULL);
	assert((device->type() == SNAPSHOT) || (device->type() == QUICKLOAD)
		|| (device->type() == Z80BIN));
}



/*-------------------------------------------------
    get_token - safely gets the snapshot/quickload data
-------------------------------------------------*/

INLINE snapquick_token *get_token(running_device *device)
{
	assert_is_snapshot_or_quickload(device);
	return (snapquick_token *) downcast<legacy_device_base *>(device)->token();
}



/*-------------------------------------------------
    get_config - safely gets the quickload config
-------------------------------------------------*/

INLINE const snapquick_config *get_config(running_device *device)
{
	assert_is_snapshot_or_quickload(device);
	return (const snapquick_config *) downcast<const legacy_device_config_base &>(device->baseconfig()).inline_config();
}

INLINE const snapquick_config *get_config_dev(const device_config *device)
{
	assert(device != NULL);
	assert((device->type() == SNAPSHOT) || (device->type() == QUICKLOAD)
		|| (device->type() == Z80BIN));
	return (const snapquick_config *) downcast<const legacy_device_config_base *>(device)->inline_config();
}

/*-------------------------------------------------
    log_quickload - logs and displays useful
    data for the end user
-------------------------------------------------*/

void log_quickload(const char *type, UINT32 start, UINT32 length, UINT32 exec, const char *exec_format)
{
    astring tempstring;

    logerror("Loading %04X bytes of RAM at %04X\n", length, start);

    tempstring.catprintf("Quickload type: %s   Length: %d bytes\n", type, length);
    tempstring.catprintf("Start: 0x%04X   End: 0x%04X   Exec: ", start, start + length - 1);

    logerror("Quickload loaded.\n");
    if (!mame_stricmp(exec_format, EXEC_NA))
        tempstring.cat("N/A");
    else
    {
        logerror("Execution can resume with ");
        logerror(exec_format, exec);
        logerror("\n");
        tempstring.catprintf(exec_format, exec);
    }

    ui_popup_time(10, "%s", tempstring.cstr());
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK(process_snapshot_or_quickload)
-------------------------------------------------*/

static TIMER_CALLBACK(process_snapshot_or_quickload)
{
	device_image_interface *image = (device_image_interface *) ptr;
	snapquick_token *token = get_token(&image->device());

	/* invoke the load */
	(*token->load)(*image,
		image->filetype(),
		image->length());

	/* unload the device */
	image->unload();
}



/*-------------------------------------------------
    DEVICE_START( snapquick )
-------------------------------------------------*/

static DEVICE_START( snapquick )
{
	snapquick_token *token = get_token(device);

	/* allocate a timer */
	token->timer = timer_alloc(device->machine, process_snapshot_or_quickload, (void *) dynamic_cast<device_image_interface *>(device));

}



/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( snapquick )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( snapquick )
{
	const snapquick_config *config = get_config(image);
	snapquick_token *token = get_token(image);

	/* locate the load function */
	token->load = (snapquick_load_func) reinterpret_cast<snapquick_load_func>(image.get_device_specific_call());

	/* adjust the timer */
	timer_adjust_oneshot(
		token->timer,
		attotime_make(config->delay_seconds, config->delay_attoseconds),
		0);

	return IMAGE_INIT_PASS;
}



/*-------------------------------------------------
    DEVICE_GET_INFO(snapquick) - device getinfo
    function
-------------------------------------------------*/

static DEVICE_GET_INFO(snapquick)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(snapquick_token); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(snapquick_config); break;
		case DEVINFO_INT_IMAGE_READABLE:				info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:				info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATABLE:				info->i = 0; break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(snapquick); break;
		case DEVINFO_FCT_IMAGE_LOAD:					info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(snapquick); break;
		case DEVINFO_FCT_SNAPSHOT_QUICKLOAD_LOAD:		info->f = (genf *) get_config_dev(device)->load; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:			strcpy(info->s, get_config_dev(device)->file_extensions); break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(snapshot) - device getinfo
    function
-------------------------------------------------*/

DEVICE_GET_INFO(snapshot)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_SNAPSHOT; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Snapshot"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Snapshot"); break;

		default: DEVICE_GET_INFO_CALL(snapquick); break;
	}
}



/*-------------------------------------------------
    DEVICE_GET_INFO(quickload) - device getinfo
    function
-------------------------------------------------*/

DEVICE_GET_INFO(quickload)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_IMAGE_TYPE:					info->i = IO_QUICKLOAD; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Quickload"); break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Quickload"); break;

		default: DEVICE_GET_INFO_CALL(snapquick); break;
	}
}

DEFINE_LEGACY_IMAGE_DEVICE(SNAPSHOT, snapshot);
DEFINE_LEGACY_IMAGE_DEVICE(QUICKLOAD, quickload);
