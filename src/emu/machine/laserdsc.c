/*************************************************************************

    laserdsc.c

    Generic laserdisc support.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*************************************************************************/

#include "driver.h"
#include "laserdsc.h"
#include "avcomp.h"
#include "profiler.h"
#include "streams.h"
#include "sound/custom.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define PRINTF_COMMANDS				1

#define CMDPRINTF(x)				do { if (PRINTF_COMMANDS) mame_printf_debug x; } while (0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* fractional track handling */
#define FRACBITS					12
#define FRAC_ONE					(1 << FRACBITS)
#define INT_TO_FRAC(x)				((x) << FRACBITS)
#define FRAC_TO_INT(x)				((x) >> FRACBITS)

/* general laserdisc playback states */
enum _playstate
{
	LASERDISC_EJECTED,						/* no disc present */
	LASERDISC_EJECTING,						/* in the process of ejecting */
	LASERDISC_LOADED,						/* disc just loaded */
	LASERDISC_LOADING,						/* in the process of loading */
	LASERDISC_PARKED,						/* disc loaded, not playing, head on parked position */
	LASERDISC_SPINUP,						/* disc loaded, spinning up to speed */

	LASERDISC_SEARCHING_FRAME,				/* searching (seeking) to a new frame */
	LASERDISC_SEARCH_FINISHED,				/* finished searching; same as stopped */

	LASERDISC_STOPPED,						/* stopped (paused) with video visible and no sound */
	LASERDISC_AUTOSTOPPED,					/* autostopped; same as stopped */

	LASERDISC_PLAYING_FORWARD,				/* playing at 1x in the forward direction */
	LASERDISC_PLAYING_REVERSE,				/* playing at 1x in the reverse direction */
	LASERDISC_PLAYING_SLOW_FORWARD,			/* playing forward slowly */
	LASERDISC_PLAYING_SLOW_REVERSE,			/* playing backward slowly */
	LASERDISC_PLAYING_FAST_FORWARD,			/* playing fast forward */
	LASERDISC_PLAYING_FAST_REVERSE,			/* playing fast backward */
	LASERDISC_STEPPING_FORWARD,				/* stepping forward */
	LASERDISC_STEPPING_REVERSE,				/* stepping backward */
	LASERDISC_SCANNING_FORWARD,				/* scanning forward at the scan rate */
	LASERDISC_SCANNING_REVERSE				/* scanning backward at the scan rate */
};
typedef enum _playstate playstate;

/* generic states and configuration */
#define FRAMEFLAG_PREV_SAME_FRAME	0x01
#define FRAMEFLAG_NEXT_SAME_FRAME	0x02

#define AUDIO_CH1_ENABLE			0x01
#define AUDIO_CH2_ENABLE			0x02
#define AUDIO_EXPLICIT_MUTE			0x04
#define AUDIO_IMPLICIT_MUTE			0x08

#define VIDEO_ENABLE				0x01
#define VIDEO_EXPLICIT_MUTE			0x02
#define VIDEO_IMPLICIT_MUTE			0x04

#define DISPLAY_ENABLE				0x01

#define NULL_TARGET_FRAME			0					/* frame 0 indicates no target */
#define ONE_TRACK					INT_TO_FRAC(1)		/* a single track, or track #1 */
#define STOP_SPEED					INT_TO_FRAC(0)		/* no movement */
#define PLAY_SPEED					INT_TO_FRAC(1)		/* regular playback speed */

#define GENERIC_SPINUP_TIME			(attotime_make(3, 0))
#define GENERIC_LOAD_TIME			(attotime_make(10, 0))
#define GENERIC_RESET_SPEED			INT_TO_FRAC(5000)

/* Pioneer PR-7820 specific states */
#define PR7820_MODE_MANUAL			0
#define PR7820_MODE_AUTOMATIC		1
#define PR7820_MODE_PROGRAM			2

#define PR7820_SEARCH_SPEED			INT_TO_FRAC(5000)

/* Pioneer PR-8210/LD-V1100 specific states */
#define PR8210_MODE_GET_1ST			0
#define PR8210_MODE_GET_2ND			1
#define PR8210_MODE_GET_3RD			2

#define PR8210_SCAN_SPEED			(INT_TO_FRAC(2000) / 30)
#define PR8210_FAST_SPEED			(PLAY_SPEED * 3)
#define PR8210_SLOW_SPEED			(PLAY_SPEED / 5)
#define PR8210_STEP_SPEED			(PLAY_SPEED / 7)
#define PR8210_SEARCH_SPEED			INT_TO_FRAC(5000)

/* Pioneer LD-V1000 specific states */
#define LDV1000_MODE_STATUS			0
#define LDV1000_MODE_GET_FRAME		1
#define LDV1000_MODE_GET_1ST		2
#define LDV1000_MODE_GET_2ND		3
#define LDV1000_MODE_GET_RAM		4

#define LDV1000_SCAN_SPEED			(INT_TO_FRAC(2000) / 30)
#define LDV1000_SEARCH_SPEED		INT_TO_FRAC(5000)

/* Sony LDP-1450 specific states */
#define LDP1450_SCAN_SPEED			(INT_TO_FRAC(2000) / 30)
#define LDP1450_FAST_SPEED			(PLAY_SPEED * 3)
#define LDP1450_SLOW_SPEED			(PLAY_SPEED / 5)
#define LDP1450_STEP_SPEED			(PLAY_SPEED / 7)
#define LDP1450_SEARCH_SPEED		INT_TO_FRAC(5000)

/* Philips 22VP932 specific states */
#define VP932_SCAN_SPEED			(INT_TO_FRAC(2000) / 30)
#define VP932_FAST_SPEED			(PLAY_SPEED * 3)
#define VP932_SLOW_SPEED			(PLAY_SPEED / 5)
#define VP932_STEP_SPEED			(PLAY_SPEED / 7)
#define VP932_SEARCH_SPEED			INT_TO_FRAC(5000)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* PR-7820-specific data */
typedef struct _pr7820_info pr7820_info;
struct _pr7820_info
{
	UINT8				mode;					/* current mode */
	UINT32				curpc;					/* current PC for automatic execution */
	INT32				configspeed;			/* configured speed */
	UINT32				activereg;				/* active register index */
	UINT8				ram[1024];				/* RAM */
};


/* PR8210-specific data */
typedef struct _pr8210_info pr8210_info;
struct _pr8210_info
{
	UINT8				mode;					/* current mode */
	UINT16				commandtriplet[3];		/* current command triplet */
	attotime			commandtime;			/* command time */
	UINT8				commandbits;			/* command bit count */
	UINT8				seekstate;				/* state of the seek command */
};


/* LD-V1000-specific data */
typedef struct _ldv1000_info ldv1000_info;
struct _ldv1000_info
{
	UINT8				mode;					/* current mode */
	UINT32				activereg;				/* active register index */
	UINT8				ram[1024];				/* RAM */
	UINT32				readpos;				/* current read position */
	UINT32				readtotal;				/* current read position */
	UINT8				readbuf[256];			/* temporary read buffer */
	UINT8				statusmask;				/* status bits mask */
};


/* LDP-1450-specific data */
typedef struct _ldp1450_info ldp1450_info;
struct _ldp1450_info
{
	UINT32				readpos;				/* current read position */
	UINT32				readtotal;				/* current read position */
	UINT8				readbuf[256];			/* temporary read buffer */
};


/* 22VP932-specific data */
typedef struct _vp932_info vp932_info;
struct _vp932_info
{
	UINT8				incount;				/* number of pending bytes accumulated */
	UINT8				inbuffer[8];			/* input data */
	UINT8				outcount;				/* number of pending bytes to send */
	UINT8				outbuffer[8];			/* output data */
};


/* per-field metadata */
typedef struct _field_metadata field_metadata;
struct _field_metadata
{
	UINT8				version;				/* version of the data */
	UINT8				frameflags;				/* per-frame flags */
	UINT8				whiteflag;				/* white flag */
	UINT32				line16;					/* line 16 Philips code */
	UINT32				line17;					/* line 17 Philips code */
	UINT32				line18;					/* line 18 Philips code */
};


/* generic data */
typedef struct _laserdisc_state laserdisc_state;
struct _laserdisc_state
{
	/* disc parameters */
	chd_file *			disc;					/* handle to the disc itself */
	av_codec_decompress_config avconfig;		/* decompression configuration */
	UINT8				readpending;			/* true if a read is pending */
	UINT8 *				framebuffer;			/* buffer to hold one frame */
	UINT32				maxfractrack;			/* maximum track number */
	UINT32				fieldnum;				/* field number (0 or 1) */

	/* video data */
	bitmap_t *			videoframe[2];			/* currently cached frames */
	UINT8				videofields[2];			/* number of fields in each frame */
	UINT32				videoframenum[2];		/* frame number contained in each frame */
	UINT8				videoindex;				/* index of the current video buffer */
	bitmap_t *			emptyframe;				/* blank frame */

	/* audio data */
	INT16 *				audiobuffer[2];			/* buffer for audio samples */
	UINT32				audiobufsize;			/* size of buffer */
	UINT32				audiobufin;				/* input index */
	UINT32				audiobufout;			/* output index */
	int					audiocustom;			/* custom sound index */

	/* metadata */
	field_metadata		metadata[2];			/* metadata parsed from the stream, for each field */
	int					last_frame;				/* last seen frame number */
	int					last_chapter;			/* last seen chapter number */

	/* core states */
	UINT8				type;					/* laserdisc type */
	playstate			state;					/* current playback state */
	UINT8				video;					/* video state: bit 0 = on/off */
	UINT8				audio;					/* audio state: bit 0 = audio 1, bit 1 = audio 2 */
	UINT8				display;				/* display state: bit 0 = on/off */
	attotime			lastvsynctime;			/* time of the last vsync */

	/* deferred states */
	attotime			holdfinished;			/* time when current state will advance */
	UINT8				postholdstate;			/* state to switch into after holding */
	INT32				postholdfracspeed;		/* speed after the hold */

	/* input data */
	UINT8				datain;					/* current input data value */
	UINT8				linein[LASERDISC_INPUT_LINES]; /* current input line state */

	/* output data */
	UINT8				dataout;				/* current output data value */
	UINT8				lineout[LASERDISC_OUTPUT_LINES]; /* current output line state */

	/* command and parameter buffering */
	UINT8				command;				/* current command */
	INT32				parameter;				/* command parameter */

	/* playback/search/scan speeds */
	INT32				curfracspeed;			/* current speed the head is moving */
	INT32				curfractrack;			/* current track */
	INT32				targetframe;			/* target frame (0 means no target) */

	/* debugging */
	char				text[100];				/* buffer for the state */

	/* filled in by player-specific init */
	void 				(*writedata)(laserdisc_state *ld, UINT8 prev, UINT8 new); /* write callback */
	void 				(*writeline[LASERDISC_INPUT_LINES])(laserdisc_state *ld, UINT8 new); /* write line callback */
	UINT8				(*readdata)(laserdisc_state *ld);	/* status callback */
	UINT8				(*readline[LASERDISC_OUTPUT_LINES])(laserdisc_state *ld); /* read line callback */
	void				(*statechanged)(laserdisc_state *ld, UINT8 oldstate); /* state changed callback */

	/* some player-specific data */
	union
	{
		pr7820_info		pr7820;					/* PR-7820-specific info */
		pr8210_info		pr8210;					/* PR-8210-specific info */
		ldv1000_info 	ldv1000;				/* LD-V1000-specific info */
		ldp1450_info 	ldp1450;				/* LDP-1450-specific info */
		vp932_info 		vp932;					/* 22VP932-specific info */
	} u;
};


/* sound callback info */
typedef struct _sound_token sound_token;
struct _sound_token
{
	sound_stream *	stream;
	laserdisc_state *ld;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* generic helper functions */
static int update_position(laserdisc_state *ld);
static void read_track_data(laserdisc_state *ld);
static void process_track_data(laserdisc_state *ld);
static int parse_metadata(const UINT8 *rawdata, UINT32 track, UINT8 which, field_metadata *metadata);
static void *custom_start(int clock, const struct CustomSound_interface *config);
static void custom_stream_callback(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

/* Pioneer PR-7820 implementation */
static void pr7820_init(laserdisc_state *ld);
static void pr7820_soft_reset(laserdisc_state *ld);
static void pr7820_enter_w(laserdisc_state *ld, UINT8 data);
static UINT8 pr7820_ready_r(laserdisc_state *ld);
static UINT8 pr7820_status_r(laserdisc_state *ld);

/* Pioneer PR-8210 implementation */
static void pr8210_init(laserdisc_state *ld);
static void pr8210_soft_reset(laserdisc_state *ld);
static void pr8210_command(laserdisc_state *ld);
static void pr8210_control_w(laserdisc_state *ld, UINT8 data);

/* Pioneer LDV-1000 implementation */
static void ldv1000_init(laserdisc_state *ld);
static void ldv1000_soft_reset(laserdisc_state *ld);
static void ldv1000_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data);
static UINT8 ldv1000_status_strobe_r(laserdisc_state *ld);
static UINT8 ldv1000_command_strobe_r(laserdisc_state *ld);
static UINT8 ldv1000_status_r(laserdisc_state *ld);

/* Sony LDP-1450 implementation */
static void ldp1450_init(laserdisc_state *ld);
static void ldp1450_soft_reset(laserdisc_state *ld);
static void ldp1450_compute_status(laserdisc_state *ld);
static void ldp1450_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data);
static UINT8 ldp1450_data_avail_r(laserdisc_state *ld);
static UINT8 ldp1450_data_r(laserdisc_state *ld);
static void ldp1450_state_changed(laserdisc_state *ld, UINT8 oldstate);

/* Philips 22VP932 implementation */
static void vp932_init(laserdisc_state *ld);
static void vp932_soft_reset(laserdisc_state *ld);
static void vp932_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data);
static UINT8 vp932_data_avail_r(laserdisc_state *ld);
static UINT8 vp932_data_r(laserdisc_state *ld);
static void vp932_state_changed(laserdisc_state *ld, UINT8 oldstate);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

const struct CustomSound_interface laserdisc_custom_interface =
{
	custom_start
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_safe_token - makes sure that the passed
    in device is, in fact, a laserdisc device
-------------------------------------------------*/

INLINE laserdisc_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == LASERDISC);

	return (laserdisc_state *)device->token;
}


/*-------------------------------------------------
    audio_channel_active - return TRUE if the
    given audio channel should be output
-------------------------------------------------*/

INLINE int audio_channel_active(laserdisc_state *ld, int channel)
{
	int result = (ld->audio >> channel) & 1;

	/* apply muting */
	if (ld->audio & (AUDIO_EXPLICIT_MUTE | AUDIO_IMPLICIT_MUTE))
		result = 0;

	/* implicitly muted during some states */
	switch (ld->state)
	{
		case LASERDISC_EJECTED:
		case LASERDISC_EJECTING:
		case LASERDISC_LOADING:
		case LASERDISC_SPINUP:
		case LASERDISC_PARKED:
		case LASERDISC_LOADED:
		case LASERDISC_SEARCHING_FRAME:
		case LASERDISC_SEARCH_FINISHED:
		case LASERDISC_STOPPED:
		case LASERDISC_AUTOSTOPPED:
		case LASERDISC_PLAYING_SLOW_FORWARD:
		case LASERDISC_PLAYING_SLOW_REVERSE:
		case LASERDISC_PLAYING_FAST_FORWARD:
		case LASERDISC_PLAYING_FAST_REVERSE:
		case LASERDISC_SCANNING_FORWARD:
		case LASERDISC_SCANNING_REVERSE:
		case LASERDISC_STEPPING_FORWARD:
		case LASERDISC_STEPPING_REVERSE:
			result = 0;
			break;

		default:
			break;
	}
	return result;
}


/*-------------------------------------------------
    video_active - return TRUE if the video should
    be output
-------------------------------------------------*/

INLINE int video_active(laserdisc_state *ld)
{
	int result = ld->video & VIDEO_ENABLE;

	/* apply muting */
	if (ld->video & (VIDEO_EXPLICIT_MUTE | VIDEO_IMPLICIT_MUTE))
		result = 0;

	/* implicitly muted during some states */
	switch (ld->state)
	{
		case LASERDISC_EJECTED:
		case LASERDISC_EJECTING:
		case LASERDISC_LOADING:
		case LASERDISC_SPINUP:
		case LASERDISC_PARKED:
		case LASERDISC_LOADED:
		case LASERDISC_SEARCHING_FRAME:
			result = 0;
			break;

		default:
			break;
	}
	return result;
}


/*-------------------------------------------------
    laserdisc_ready - return TRUE if the
    disc is not in a transient state
-------------------------------------------------*/

INLINE int laserdisc_ready(laserdisc_state *ld)
{
	switch (ld->state)
	{
		case LASERDISC_EJECTED:
		case LASERDISC_EJECTING:
		case LASERDISC_LOADING:
		case LASERDISC_SPINUP:
		case LASERDISC_PARKED:
			return FALSE;

		case LASERDISC_LOADED:
		case LASERDISC_SEARCHING_FRAME:
		case LASERDISC_SEARCH_FINISHED:
		case LASERDISC_STOPPED:
		case LASERDISC_AUTOSTOPPED:
		case LASERDISC_PLAYING_FORWARD:
		case LASERDISC_PLAYING_REVERSE:
		case LASERDISC_PLAYING_SLOW_FORWARD:
		case LASERDISC_PLAYING_SLOW_REVERSE:
		case LASERDISC_PLAYING_FAST_FORWARD:
		case LASERDISC_PLAYING_FAST_REVERSE:
		case LASERDISC_STEPPING_FORWARD:
		case LASERDISC_STEPPING_REVERSE:
		case LASERDISC_SCANNING_FORWARD:
		case LASERDISC_SCANNING_REVERSE:
			return TRUE;
	}
	fatalerror("Unexpected state in laserdisc_ready\n");
}


/*-------------------------------------------------
    laserdisc_active - return TRUE if the
    disc is in a playing/spinning state
-------------------------------------------------*/

INLINE int laserdisc_active(laserdisc_state *ld)
{
	switch (ld->state)
	{
		case LASERDISC_EJECTED:
		case LASERDISC_EJECTING:
		case LASERDISC_LOADED:
		case LASERDISC_LOADING:
		case LASERDISC_SPINUP:
		case LASERDISC_PARKED:
			return FALSE;

		case LASERDISC_SEARCHING_FRAME:
		case LASERDISC_SEARCH_FINISHED:
		case LASERDISC_STOPPED:
		case LASERDISC_AUTOSTOPPED:
		case LASERDISC_PLAYING_FORWARD:
		case LASERDISC_PLAYING_REVERSE:
		case LASERDISC_PLAYING_SLOW_FORWARD:
		case LASERDISC_PLAYING_SLOW_REVERSE:
		case LASERDISC_PLAYING_FAST_FORWARD:
		case LASERDISC_PLAYING_FAST_REVERSE:
		case LASERDISC_STEPPING_FORWARD:
		case LASERDISC_STEPPING_REVERSE:
		case LASERDISC_SCANNING_FORWARD:
		case LASERDISC_SCANNING_REVERSE:
			return TRUE;
	}
	fatalerror("Unexpected state in laserdisc_ready\n");
}


/*-------------------------------------------------
    set_state - configure the current playback
    state
-------------------------------------------------*/

INLINE void set_state(laserdisc_state *ld, playstate state, INT32 fracspeed, INT32 targetframe)
{
	ld->holdfinished.seconds = 0;
	ld->holdfinished.attoseconds = 0;
	ld->state = state;
	ld->curfracspeed = fracspeed;
	ld->targetframe = targetframe;
}


/*-------------------------------------------------
    set_hold_state - configure a hold time and
    a playback state to follow
-------------------------------------------------*/

INLINE void set_hold_state(laserdisc_state *ld, attotime holdtime, playstate state, INT32 fracspeed)
{
	ld->holdfinished = attotime_add(timer_get_time(), holdtime);
	ld->postholdstate = state;
	ld->postholdfracspeed = fracspeed;
}


/*-------------------------------------------------
    add_to_current_track - add a value to the
    current track, stopping if we hit the min or
    max
-------------------------------------------------*/

INLINE int add_to_current_track(laserdisc_state *ld, INT32 delta)
{
	ld->curfractrack += delta;
	if (ld->curfractrack < ONE_TRACK)
	{
		ld->curfractrack = ONE_TRACK;
		return TRUE;
	}
	else if (ld->curfractrack >= ld->maxfractrack - ONE_TRACK)
	{
		ld->curfractrack = ld->maxfractrack - ONE_TRACK;
		return TRUE;
	}
	return FALSE;
}


/*-------------------------------------------------
    frame_from_metadata - return the frame number
    encoded in the metadata, if present, or -1
-------------------------------------------------*/

INLINE int frame_from_metadata(const field_metadata *metadata)
{
	UINT32 data;

	if ((metadata->line17 & 0xf80000) == 0xf80000)
		data = metadata->line17 & 0x7ffff;
	else if ((metadata->line18 & 0xf80000) == 0xf80000)
		data = metadata->line18 & 0x7ffff;
	else
		return -1;

	return (((data >> 16) & 0x0f) * 10000) + (((data >> 12) & 0x0f) * 1000) + (((data >> 8) & 0x0f) * 100) + (((data >> 4) & 0x0f) * 10) + (data & 0x0f);
}


/*-------------------------------------------------
    chapter_from_metadata - return the chapter
    number encoded in the metadata, if present,
    or -1
-------------------------------------------------*/

INLINE int chapter_from_metadata(const field_metadata *metadata)
{
	UINT32 data;

	if ((metadata->line17 & 0xf00fff) == 0x800ddd)
		data = metadata->line17 & 0x7f000;
	else if ((metadata->line18 & 0xf00fff) == 0x800ddd)
		data = metadata->line18 & 0x7f000;
	else
		return -1;

	return (((data >> 16) & 0x0f) * 10) + ((data >> 12) & 0x0f);
}


/*-------------------------------------------------
    read_16bits_from_ram_be - read 16 bits from
    player RAM in big-endian format
-------------------------------------------------*/

INLINE UINT16 read_16bits_from_ram_be(UINT8 *ram, UINT32 offset)
{
	return (ram[offset + 0] << 8) | ram[offset + 1];
}


/*-------------------------------------------------
    write_16bits_to_ram_be - write 16 bits to
    player RAM in big endian format
-------------------------------------------------*/

INLINE void write_16bits_to_ram_be(UINT8 *ram, UINT32 offset, UINT16 data)
{
	ram[offset + 0] = data >> 8;
	ram[offset + 1] = data >> 0;
}


/*-------------------------------------------------
    fillbitmap_yuy16 - fill a YUY16 bitmap with a
    given color pattern
-------------------------------------------------*/

INLINE void fillbitmap_yuy16(bitmap_t *bitmap, UINT8 yval, UINT8 cr, UINT8 cb)
{
	UINT16 color0 = (yval << 8) | cb;
	UINT16 color1 = (yval << 8) | cr;
	int x, y;

	/* write 32 bits of color (2 pixels at a time) */
	for (y = 0; y < bitmap->height; y++)
	{
		UINT16 *dest = (UINT16 *)bitmap->base + y * bitmap->rowpixels;
		for (x = 0; x < bitmap->width / 2; x++)
		{
			*dest++ = color0;
			*dest++ = color1;
		}
	}
}



/***************************************************************************
    GENERIC IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    laserdisc_vsync - call this once per field
    on the VSYNC signal
-------------------------------------------------*/

void laserdisc_vsync(const device_config *device)
{
	laserdisc_state *ld = get_safe_token(device);
	UINT8 origstate = ld->state;
	UINT8 hittarget;

	/* remember the time */
	ld->lastvsynctime = timer_get_time();

	/* if we're holding, stay in this state until finished */
	if (ld->holdfinished.seconds != 0 || ld->holdfinished.attoseconds != 0)
	{
		if (attotime_compare(ld->lastvsynctime, ld->holdfinished) < 0)
			return;
		ld->state = ld->postholdstate;
		ld->curfracspeed = ld->postholdfracspeed;
		ld->holdfinished.seconds = 0;
		ld->holdfinished.attoseconds = 0;
	}

	/* wait for previous read and decode to finish */
	process_track_data(ld);

	/* update our position for this field */
	hittarget = update_position(ld);

	/* switch off the state */
	switch (ld->state)
	{
		/* parked: do nothing */
		case LASERDISC_EJECTED:
		case LASERDISC_EJECTING:
		case LASERDISC_LOADED:
		case LASERDISC_PARKED:
			break;

		/* playing: if we hit our target, indicate so; if we hit the beginning/end, stop */
		case LASERDISC_STOPPED:
		case LASERDISC_AUTOSTOPPED:
		case LASERDISC_SEARCH_FINISHED:
		case LASERDISC_PLAYING_FORWARD:
		case LASERDISC_PLAYING_REVERSE:
		case LASERDISC_PLAYING_SLOW_FORWARD:
		case LASERDISC_PLAYING_SLOW_REVERSE:
		case LASERDISC_PLAYING_FAST_FORWARD:
		case LASERDISC_PLAYING_FAST_REVERSE:
		case LASERDISC_STEPPING_FORWARD:
		case LASERDISC_STEPPING_REVERSE:
		case LASERDISC_SCANNING_FORWARD:
		case LASERDISC_SCANNING_REVERSE:

			/* autostop if we hit the target frame */
			if (hittarget)
				set_state(ld, LASERDISC_AUTOSTOPPED, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		/* loading; keep searching until we hit the target, then go into the stopped state */
		case LASERDISC_LOADING:
		case LASERDISC_SPINUP:

			/* if we hit the target, go into search finished state */
			if (hittarget)
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		/* searching; keep seeking until we hit the target */
		case LASERDISC_SEARCHING_FRAME:

			/* if we hit the target, go into search finished state */
			if (hittarget)
				set_state(ld, LASERDISC_SEARCH_FINISHED, STOP_SPEED, NULL_TARGET_FRAME);
			break;
	}

	/* if the state changed, notify */
	if (ld->state != origstate)
	{
		/* on a state change, implicity round to the nearest fraction */
		ld->curfractrack = INT_TO_FRAC(FRAC_TO_INT(ld->curfractrack));
		if (!(ld->fieldnum & 1))
			ld->curfractrack += FRAC_ONE / 2;

		/* notify the disc handler */
		if (ld->statechanged != NULL)
			(*ld->statechanged)(ld, origstate);
	}

	/* start reading the track data for the next round */
	ld->fieldnum++;
	read_track_data(ld);
}


/*-------------------------------------------------
    laserdisc_describe_state - return a text
    string describing the current state
-------------------------------------------------*/

const char *laserdisc_describe_state(const device_config *device)
{
	static const struct
	{
		playstate state;
		const char *string;
	} state_strings[] =
	{
		{ LASERDISC_EJECTED, "Ejected" },
		{ LASERDISC_EJECTING, "Ejecting" },
		{ LASERDISC_LOADED, "Loaded" },
		{ LASERDISC_LOADING, "Loading" },
		{ LASERDISC_SPINUP, "Spinning Up" },
		{ LASERDISC_PARKED, "Parked" },
		{ LASERDISC_SEARCHING_FRAME, "Searching Frame" },
		{ LASERDISC_SEARCH_FINISHED, "Search Finished" },
		{ LASERDISC_STOPPED, "Stopped" },
		{ LASERDISC_AUTOSTOPPED, "Autostopped" },
		{ LASERDISC_PLAYING_FORWARD, "Playing Forward" },
		{ LASERDISC_PLAYING_REVERSE, "Playing Reverse" },
		{ LASERDISC_PLAYING_SLOW_FORWARD, "Playing Slow Forward x" },
		{ LASERDISC_PLAYING_SLOW_REVERSE, "Playing Slow Reverse x" },
		{ LASERDISC_PLAYING_FAST_FORWARD, "Playing Fast Forward x" },
		{ LASERDISC_PLAYING_FAST_REVERSE, "Playing Fast Reverse x" },
		{ LASERDISC_STEPPING_FORWARD, "Stepping Forward" },
		{ LASERDISC_STEPPING_REVERSE, "Stepping Reverse" },
		{ LASERDISC_SCANNING_FORWARD, "Scanning Forward" },
		{ LASERDISC_SCANNING_REVERSE, "Scanning Reverse" }
	};
	laserdisc_state *ld = get_safe_token(device);
	const char *description = "Unknown";
	int i;

	/* find the string */
	for (i = 0; i < ARRAY_LENGTH(state_strings); i++)
		if (state_strings[i].state == ld->state)
			description = state_strings[i].string;

	/* construct the string */
	if (description[strlen(description) - 1] == 'x')
		sprintf(ld->text, "%05d (%s%.1f)", FRAC_TO_INT(ld->curfractrack), description, (float)ld->curfracspeed / (float)INT_TO_FRAC(1));
	else
		sprintf(ld->text, "%05d (%s)", FRAC_TO_INT(ld->curfractrack), description);
	return ld->text;
}


/*-------------------------------------------------
    laserdisc_data_w - write data to the given
    laserdisc player
-------------------------------------------------*/

void laserdisc_data_w(const device_config *device, UINT8 data)
{
	laserdisc_state *ld = get_safe_token(device);
	UINT8 prev = ld->datain;
	ld->datain = data;

	/* call through to the player-specific write handler */
	if (ld->writedata != NULL)
		(*ld->writedata)(ld, prev, data);
}


/*-------------------------------------------------
    laserdisc_line_w - control an input line
-------------------------------------------------*/

void laserdisc_line_w(const device_config *device, UINT8 line, UINT8 newstate)
{
	laserdisc_state *ld = get_safe_token(device);

	assert(line < LASERDISC_INPUT_LINES);
	assert(newstate == ASSERT_LINE || newstate == CLEAR_LINE || newstate == PULSE_LINE);

	/* assert */
	if (newstate == ASSERT_LINE || newstate == PULSE_LINE)
	{
		if (ld->linein[line] != ASSERT_LINE)
		{
			/* call through to the player-specific line handler */
			if (ld->writeline[line] != NULL)
				(*ld->writeline[line])(ld, ASSERT_LINE);
		}
		ld->linein[line] = ASSERT_LINE;
	}

	/* deassert */
	if (newstate == CLEAR_LINE || newstate == PULSE_LINE)
	{
		if (ld->linein[line] != CLEAR_LINE)
		{
			/* call through to the player-specific line handler */
			if (ld->writeline[line] != NULL)
				(*ld->writeline[line])(ld, CLEAR_LINE);
		}
		ld->linein[line] = CLEAR_LINE;
	}
}


/*-------------------------------------------------
    laserdisc_data_r - return the current
    data byte
-------------------------------------------------*/

UINT8 laserdisc_data_r(const device_config *device)
{
	laserdisc_state *ld = get_safe_token(device);
	UINT8 result = ld->dataout;

	/* call through to the player-specific data handler */
	if (ld->readdata != NULL)
		result = (*ld->readdata)(ld);

	return result;
}


/*-------------------------------------------------
    laserdisc_line_r - return the current state
    of an output line
-------------------------------------------------*/

UINT8 laserdisc_line_r(const device_config *device, UINT8 line)
{
	laserdisc_state *ld = get_safe_token(device);
	UINT8 result;

	assert(line < LASERDISC_OUTPUT_LINES);
	result = ld->lineout[line];

	/* call through to the player-specific data handler */
	if (ld->readline[line] != NULL)
		result = (*ld->readline[line])(ld);

	return result;
}


/*-------------------------------------------------
    laserdisc_get_video - return the current
    video frame
-------------------------------------------------*/

UINT32 laserdisc_get_video(const device_config *device, bitmap_t **bitmap)
{
	laserdisc_state *ld = get_safe_token(device);
	int frameindex;

	/* determine the most recent live set of frames */
	frameindex = ld->videoindex;
	if (ld->videofields[frameindex] < 2)
		frameindex ^= 1;

	/* if no video present, return the empty frame */
	if (!video_active(ld) || ld->videofields[frameindex] < 2)
	{
		*bitmap = ld->emptyframe;
		return 0;
	}
	else
	{
		*bitmap = ld->videoframe[frameindex];
		return ld->videoframenum[frameindex];
	}
}


/*-------------------------------------------------
    laserdisc_get_field_code - return raw field
    information read from the disc
-------------------------------------------------*/

UINT32 laserdisc_get_field_code(const device_config *device, UINT8 code)
{
	laserdisc_state *ld = get_safe_token(device);
	int field = (ld->fieldnum & 1) ^ 1;

	/* if no video present, return */
	if (!video_active(ld))
		return 0;

	switch (code)
	{
		case LASERDISC_CODE_FRAME_FLAGS:
			return ld->metadata[field].frameflags;

		case LASERDISC_CODE_WHITE_FLAG:
			return ld->metadata[field].whiteflag;

		case LASERDISC_CODE_LINE16:
			return ld->metadata[field].line16;

		case LASERDISC_CODE_LINE17:
			return ld->metadata[field].line17;

		case LASERDISC_CODE_LINE18:
			return ld->metadata[field].line18;
	}

	return 0;
}



/***************************************************************************
    GENERIC HELPER FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    process_number - scan a list of bytecodes
    and treat them as single digits if we get
    a match
-------------------------------------------------*/

static int process_number(laserdisc_state *ld, UINT8 byte, const UINT8 numbers[])
{
	int value;

	/* look for a match in the list of number values; if we got one, append it to the parameter */
	for (value = 0; value < 10; value++)
		if (numbers[value] == byte)
		{
			ld->parameter = (ld->parameter == -1) ? value : (ld->parameter * 10 + value);
			return TRUE;
		}

	/* no match; return FALSE */
	return FALSE;
}


/*-------------------------------------------------
    update_position - update the head position
    for this VSYNC
-------------------------------------------------*/

static int update_position(laserdisc_state *ld)
{
	UINT32 tracknum = FRAC_TO_INT(ld->curfractrack);
	UINT32 fieldnum = ld->fieldnum & 1;
	INT32 speed = ld->curfracspeed;
	INT32 framedelta;

	/* if video isn't active, we don't need to be careful about fields and frames */
	if (!video_active(ld))
	{
		int frame = frame_from_metadata(&ld->metadata[fieldnum]);
		int direction;

		/* if we have no target, don't do anything */
		if (ld->targetframe == 0)
			return TRUE;

		/* if we didn't get any frame information this field, move onto the next */
		if (frame == -1)
		{
			/* if we're in the lead-in section, advance more aggressively */
			if (ld->metadata[fieldnum].line17 == 0x88ffff || ld->metadata[fieldnum].line18 == 0x88ffff)
				add_to_current_track(ld, 10 * ONE_TRACK);

			/* if we're on the second field of a frame, the next frame is on the next track */
			/* but don't do it if we're seeking to frame 1, since that might be the very first frame */
			else if (fieldnum == 1 && (ld->last_frame == 1 || ld->targetframe != 1))
				add_to_current_track(ld, ONE_TRACK);
			return FALSE;
		}

		/* if we hit our frame, we're done */
		if (frame == ld->targetframe)
		{
			ld->curfractrack = INT_TO_FRAC(tracknum);
			return TRUE;
		}

		/* determine the frame delta and direction */
		framedelta = ld->targetframe - frame;
		direction = (framedelta < 0) ? -1 : 1;

		/* if we're going backwards, go back at least 2 frames since we tend to move forward */
		if (framedelta == -1)
			framedelta = -2;

		/* if we're only going forward one frame, only advance if we're on the second field */
		/* otherwise, we might overshoot */
		else if (framedelta == 1 && fieldnum == 0)
			framedelta = 0;

		/* scale up to fractional bits */
		framedelta = INT_TO_FRAC(abs(framedelta));

		/* determine the stepdelta */
		add_to_current_track(ld, MIN(framedelta, speed / 2) * direction);
		return FALSE;
	}

	/* only advance on the second field */
	if (fieldnum != 1)
		return FALSE;

	/* if we're moving backwards, advance back until we hit track 1 or the target */
	if (speed < 0)
	{
		/* if we've hit the target, stop now */
		if (ld->targetframe != 0 && ld->last_frame <= ld->targetframe)
		{
			ld->curfractrack = INT_TO_FRAC(tracknum);
			return TRUE;
		}

		/* otherwise, clamp our delta so we don't overshoot */
		else
		{
			framedelta = INT_TO_FRAC((ld->targetframe == 0) ? -1000 : (ld->targetframe - ld->last_frame));
			add_to_current_track(ld, MAX(framedelta, speed));
		}
	}

	/* if we're moving forwards, advance forward until we hit the max track or the target */
	else if (speed > 0)
	{
		/* if we've hit the target, stop now */
		if (ld->targetframe != 0 && ld->last_frame >= ld->targetframe)
		{
			ld->curfractrack = INT_TO_FRAC(tracknum);
			return TRUE;
		}

		/* otherwise, clamp our delta so we don't overshoot */
		else
		{
			framedelta = INT_TO_FRAC((ld->targetframe == 0) ? 1000 : (ld->targetframe - ld->last_frame));
			add_to_current_track(ld, MIN(framedelta, speed));
		}
	}
	return FALSE;
}


/*-------------------------------------------------
    read_track_data - read and process data for
    a particular video track
-------------------------------------------------*/

static void read_track_data(laserdisc_state *ld)
{
	UINT32 tracknum = FRAC_TO_INT(ld->curfractrack);
	UINT32 fieldnum = ld->fieldnum & 1;
	UINT32 chdhunk = (tracknum - 1) * 2 + fieldnum;
	chd_error err;

	/* initialize the decompression structure */
	ld->avconfig.decode_mask = AVCOMP_DECODE_META;
	ld->avconfig.video_buffer = NULL;
	ld->avconfig.video_xor = BYTE_XOR_LE(0);
	ld->avconfig.audio_xor = BYTE_XOR_BE(0);

	/* if video is active, enable video decoding */
	if (video_active(ld))
	{
		/* if the flags on the previous field indicate that the next field
           is part of the same frame, then keep steady; otherwise, bump to
           the next videoindex */
		if (ld->videofields[ld->videoindex] > 0 && !(ld->metadata[fieldnum ^ 1].frameflags & FRAMEFLAG_NEXT_SAME_FRAME))
		{
			ld->videoindex ^= 1;
			ld->videofields[ld->videoindex] = 0;
		}

		/* enable video and configure it to read into the videoframe */
		ld->avconfig.decode_mask |= AVCOMP_DECODE_VIDEO;
		ld->avconfig.video_buffer = (UINT8 *)BITMAP_ADDR16(ld->videoframe[ld->videoindex], fieldnum, 0);
		ld->avconfig.video_stride = 4 * ld->videoframe[ld->videoindex]->rowpixels;
	}
	else
		ld->videofields[0] = ld->videofields[1] = 0;

	/* if audio is active, enable audio decoding */
	if (audio_channel_active(ld, 0))
		ld->avconfig.decode_mask |= AVCOMP_DECODE_AUDIO(0);
	if (audio_channel_active(ld, 1))
		ld->avconfig.decode_mask |= AVCOMP_DECODE_AUDIO(1);

	/* configure the codec and then read */
	if (ld->disc != NULL)
	{
		err = chd_codec_config(ld->disc, AV_CODEC_DECOMPRESS_CONFIG, &ld->avconfig);
		if (err == CHDERR_NONE)
		{
			err = chd_read_async(ld->disc, chdhunk, ld->framebuffer);
			ld->readpending = TRUE;
		}
	}
}


/*-------------------------------------------------
    process_track_data - process data from a
    track after it has been read
-------------------------------------------------*/

static void process_track_data(laserdisc_state *ld)
{
	UINT32 tracknum = FRAC_TO_INT(ld->curfractrack);
	UINT32 fieldnum = ld->fieldnum & 1;
	const UINT8 *rawdata = NULL;
	const INT16 *sampsource[2];
	int frame, chapter;
	chd_error chderr;
	int samples;

	/* wait for the async operation to complete */
	if (ld->disc != NULL && ld->readpending)
	{
		/* complete the async operation */
		chderr = chd_async_complete(ld->disc);
		if (chderr == CHDERR_NONE || chderr == CHDERR_NO_ASYNC_OPERATION)
			rawdata = ld->framebuffer;
	}
	ld->readpending = FALSE;

	/* parse the metadata */
	parse_metadata(rawdata, tracknum, fieldnum, &ld->metadata[fieldnum]);

	/* update the last seen frame and chapter */
	frame = frame_from_metadata(&ld->metadata[fieldnum]);
	if (frame != -1)
		ld->last_frame = frame;
	chapter = chapter_from_metadata(&ld->metadata[fieldnum]);
	if (chapter != -1)
		ld->last_chapter = chapter;

	/* update video ld */
	if (rawdata != NULL && (ld->avconfig.decode_mask & AVCOMP_DECODE_VIDEO))
	{
		ld->videofields[ld->videoindex]++;
		ld->videoframenum[ld->videoindex] = ld->last_frame;
	}

	/* stream the audio into our ring buffers */
	if (rawdata != NULL)
	{
		samples = (rawdata[6] << 8) + rawdata[7];
		sampsource[0] = (const INT16 *)(rawdata + 12 + rawdata[4]) + 0 * samples;
		sampsource[1] = sampsource[0] + samples;

		/* loop until all samples are copied */
		while (samples != 0)
		{
			int samples_to_copy = MIN(ld->audiobufsize - ld->audiobufin, samples);
			int channum;

			/* don't overrun the output pointer */
			if (ld->audiobufout > ld->audiobufin)
			{
				samples_to_copy = MIN(samples_to_copy, ld->audiobufout - ld->audiobufin);
				if (samples_to_copy == 0)
					break;
			}

			/* for reach channel, copy the data or clear to 0 */
			for (channum = 0; channum < 2; channum++)
			{
				if (audio_channel_active(ld, channum))
				{
					memcpy(&ld->audiobuffer[channum][ld->audiobufin], sampsource[channum], samples_to_copy * 2);
					sampsource[channum] += samples_to_copy;
				}
				else
					memset(&ld->audiobuffer[channum][ld->audiobufin], 0, samples_to_copy * 2);
			}
			samples -= samples_to_copy;

			/* point past the data */
			ld->audiobufin += samples_to_copy;
			if (ld->audiobufin >= ld->audiobufsize)
				ld->audiobufin = 0;
		}
	}
}


/*-------------------------------------------------
    parse_metadata - parse raw metadata into
    something more useful
-------------------------------------------------*/

static int parse_metadata(const UINT8 *rawdata, UINT32 track, UINT8 which, field_metadata *metadata)
{
	/* verify we have data to parse */
	if (rawdata == NULL || rawdata[4] < 1)
		goto fakeit;

	/* first byte is the version */
	metadata->version = rawdata[12+0];

	/* version 1 data */
	if (metadata->version == 2 && rawdata[4] == 12)
	{
		/*
            1 byte = version
            1 byte = internal flags
               D0 = prev field is same frame
               D1 = next field is same frame
            1 byte = white flag
            3 bytes = line 16 Philips code
            3 bytes = line 17 Philips code
            3 bytes = line 18 Philips code
        */
		metadata->frameflags = rawdata[12+1];
		metadata->whiteflag = rawdata[12+2];
		metadata->line16 = (rawdata[12+3] << 16) | (rawdata[12+4] << 8) | rawdata[12+5];
		metadata->line17 = (rawdata[12+6] << 16) | (rawdata[12+7] << 8) | rawdata[12+8];
		metadata->line18 = (rawdata[12+9] << 16) | (rawdata[12+10] << 8) | rawdata[12+11];
		return TRUE;
	}

fakeit:
	metadata->frameflags = (which == 0) ? 0x02 : 0x01;
	metadata->whiteflag = (which == 0) ? 1 : 0;
	metadata->line16 = 0x000000;
	if (which == 0)
		metadata->line17 = metadata->line18 = 0xf80000 | (((track / 10000) % 10) << 16) | (((track / 1000) % 10) << 12) | (((track / 100) % 10) << 8) | (((track / 10) % 10) << 4) | (track % 10);
	else
		metadata->line17 = metadata->line18 = 0x000000;
	return FALSE;
}


/*-------------------------------------------------
    custom_start - custom audio start
    for laserdiscs
-------------------------------------------------*/

static void *custom_start(int clock, const struct CustomSound_interface *config)
{
	sound_token *token = auto_malloc(sizeof(*token));
	token->stream = stream_create(0, 2, 44100, token, custom_stream_callback);
	token->ld = NULL;
	return token;
}


/*-------------------------------------------------
    custom_stream_callback - audio streamer
    for laserdiscs
-------------------------------------------------*/

static void custom_stream_callback(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	sound_token *token = param;
	laserdisc_state *ld = token->ld;
	stream_sample_t *dst0 = outputs[0];
	stream_sample_t *dst1 = outputs[1];
	int samples_avail = 0;

	/* see if we have enough samples to fill the buffer; if not, drop out */
	if (ld != NULL)
	{
		samples_avail = ld->audiobufin - ld->audiobufout;
		if (samples_avail < 0)
			samples_avail += ld->audiobufsize;
	}

	/* if no attached ld, just clear the buffers */
	if (samples_avail < samples)
	{
		memset(dst0, 0, samples * sizeof(dst0[0]));
		memset(dst1, 0, samples * sizeof(dst1[0]));
	}

	/* otherwise, stream from our buffer */
	else
	{
		const INT16 *buffer0 = ld->audiobuffer[0];
		const INT16 *buffer1 = ld->audiobuffer[1];
		int sampout = ld->audiobufout;

		/* copy samples */
		while (sampout != ld->audiobufin && samples-- > 0)
		{
			*dst0++ = buffer0[sampout];
			*dst1++ = buffer1[sampout];
			sampout++;
			if (sampout >= ld->audiobufsize)
				sampout = 0;
		}
		ld->audiobufout = sampout;

		/* clear out the rest of the buffer */
		if (samples > 0)
		{
			int sampout = (ld->audiobufout == 0) ? ld->audiobufsize - 1 : ld->audiobufout - 1;
			stream_sample_t fill0 = buffer0[sampout];
			stream_sample_t fill1 = buffer1[sampout];

			while (samples-- > 0)
			{
				*dst0++ = fill0;
				*dst1++ = fill1;
			}
		}
	}
}



/***************************************************************************
    DEVICE INTERFACE
***************************************************************************/

/*-------------------------------------------------
    device start callback
-------------------------------------------------*/

static DEVICE_START( laserdisc )
{
	int fps = 30, fpsfrac = 0, width = 720, height = 240, interlaced = 1, channels = 2, rate = 44100, metabytes = 0;
	const laserdisc_config *config = device->inline_config;
	UINT32 fps_times_1million, max_samples_per_track;
	laserdisc_state *ld = get_safe_token(device);
	char metadata[256];
	chd_error err;
	int sndnum;

	/* copy config data to the live state */
	ld->type = config->type;
	ld->disc = get_disk_handle(config->disknum);
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
		if (device->machine->config->sound[sndnum].tag != NULL && strcmp(device->machine->config->sound[sndnum].tag, config->soundtag) == 0)
			ld->audiocustom = sndnum;

	/* get the disc metadata and extract the ld */
	if (ld->disc != NULL)
	{
		/* require the A/V codec */
		if (chd_get_header(ld->disc)->compression != CHDCOMPRESSION_AV)
			fatalerror("Laserdisc video must be compressed with the A/V codec!");

		/* read and extract the metadata */
		err = chd_get_metadata(ld->disc, AV_METADATA_TAG, 0, metadata, sizeof(metadata), NULL, NULL);
		if (err != CHDERR_NONE)
			fatalerror("Non-A/V CHD file specified");
		if (sscanf(metadata, AV_METADATA_FORMAT, &fps, &fpsfrac, &width, &height, &interlaced, &channels, &rate, &metabytes) != 8)
			fatalerror("Invalid metadata in CHD file");

		/* require interlaced video */
		if (!interlaced)
			fatalerror("Laserdisc video must be interlaced!");

		/* determine the maximum track and allocate a frame buffer */
		ld->maxfractrack = INT_TO_FRAC(chd_get_header(ld->disc)->totalhunks / (interlaced + 1));
		ld->framebuffer = auto_malloc(chd_get_header(ld->disc)->hunkbytes);
	}
	else
		ld->maxfractrack = INT_TO_FRAC(54000);

	/* allocate video frames */
	ld->videoframe[0] = auto_bitmap_alloc(width, height * 2, BITMAP_FORMAT_YUY16);
	fillbitmap_yuy16(ld->videoframe[0], 40, 109, 240);
	ld->videoframe[1] = auto_bitmap_alloc(width, height * 2, BITMAP_FORMAT_YUY16);
	fillbitmap_yuy16(ld->videoframe[1], 40, 109, 240);
	ld->emptyframe = auto_bitmap_alloc(width, height * 2, BITMAP_FORMAT_YUY16);
	fillbitmap_yuy16(ld->emptyframe, 0, 128, 128);

	/* allocate audio buffers */
	fps_times_1million = fps * 1000000 + fpsfrac;
	max_samples_per_track = ((UINT64)rate * 1000000 + fps_times_1million - 1) / fps_times_1million;
	ld->audiobufsize = max_samples_per_track * 4;
	ld->audiobuffer[0] = auto_malloc(ld->audiobufsize * sizeof(ld->audiobuffer[0][0]));
	ld->audiobuffer[1] = auto_malloc(ld->audiobufsize * sizeof(ld->audiobuffer[1][0]));

	/* attempt to wire up the audio */
	if (sndti_exists(SOUND_CUSTOM, ld->audiocustom))
	{
		sound_token *token = custom_get_token(ld->audiocustom);
		token->ld = ld;
		stream_set_sample_rate(token->stream, rate);
	}
}


/*-------------------------------------------------
    device exit callback
-------------------------------------------------*/

static DEVICE_STOP( laserdisc )
{
	laserdisc_state *ld = get_safe_token(device);

	/* make sure all async operations have completed */
	if (ld->disc != NULL)
		chd_async_complete(ld->disc);
}


/*-------------------------------------------------
    device reset callback
-------------------------------------------------*/

static DEVICE_RESET( laserdisc )
{
	laserdisc_state *ld = get_safe_token(device);
	int i;

	/* set up the general ld */
	ld->video = VIDEO_ENABLE;
	ld->audio = AUDIO_CH1_ENABLE | AUDIO_CH2_ENABLE;
	ld->display = DISPLAY_ENABLE;

	/* seek to frame 1 to start with */
	set_state(ld, LASERDISC_LOADING, GENERIC_RESET_SPEED, 1);

	/* reset the I/O lines */
	for (i = 0; i < LASERDISC_INPUT_LINES; i++)
		ld->linein[i] = CLEAR_LINE;
	for (i = 0; i < LASERDISC_OUTPUT_LINES; i++)
		ld->lineout[i] = CLEAR_LINE;

	/* reset callbacks */
	ld->writedata = NULL;
	memset(ld->writeline, 0, sizeof(ld->writeline));
	ld->readdata = NULL;
	memset(ld->readline, 0, sizeof(ld->readline));
	ld->statechanged = NULL;

	/* each player can init */
	switch (ld->type)
	{
		case LASERDISC_TYPE_PIONEER_PR7820:
			pr7820_init(ld);
			break;

		case LASERDISC_TYPE_PIONEER_PR8210:
			pr8210_init(ld);
			break;

		case LASERDISC_TYPE_PIONEER_LDV1000:
			ldv1000_init(ld);
			break;

		case LASERDISC_TYPE_SONY_LDP1450:
			ldp1450_init(ld);
			break;

		case LASERDISC_TYPE_PHILLIPS_22VP932:
			vp932_init(ld);
			break;

		default:
			fatalerror("Invalid laserdisc player type!");
			break;
	}

	/* default to track 1 */
	ld->curfractrack = ONE_TRACK;
}


/*-------------------------------------------------
    device set info callback
-------------------------------------------------*/

static DEVICE_SET_INFO( laserdisc )
{
	laserdisc_state *ld = get_safe_token(device);
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case LDINFO_INT_TYPE:
			if (ld != NULL && ld->type != info->i)
			{
				ld->type = info->i;
				device_reset(device);
			}
			break;
	}
}


/*-------------------------------------------------
    device get info callback
-------------------------------------------------*/

DEVICE_GET_INFO( laserdisc )
{
	const laserdisc_config *config = (device != NULL) ? device->inline_config : NULL;
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(laserdisc_state);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = sizeof(laserdisc_config);					break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_VIDEO;						break;
		case LDINFO_INT_TYPE:					info->i = config->type;								break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:				info->set_info = DEVICE_SET_INFO_NAME(laserdisc); 	break;
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(laserdisc); 		break;
		case DEVINFO_FCT_STOP:					info->stop = DEVICE_STOP_NAME(laserdisc); 			break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(laserdisc);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:
			switch (config->type)
			{
				default:
				case LASERDISC_TYPE_PIONEER_PR7820:		info->s = "Pioneer PR-7820";		break;
				case LASERDISC_TYPE_PIONEER_PR8210:		info->s = "Pioneer PR-8210";		break;
				case LASERDISC_TYPE_PIONEER_LDV1000:	info->s = "Pioneer LD-V1000";		break;
				case LASERDISC_TYPE_PHILLIPS_22VP932:	info->s = "Philips 22VP932";		break;
				case LASERDISC_TYPE_SONY_LDP1450:		info->s = "Sony LDP-1450";			break;
			}
			break;
		case DEVINFO_STR_FAMILY:				info->s = "Laserdisc Player";			break;
		case DEVINFO_STR_VERSION:				info->s = "1.0";						break;
		case DEVINFO_STR_SOURCE_FILE:			info->s = __FILE__;						break;
		case DEVINFO_STR_CREDITS:				info->s = "Copyright Nicola Salmoria and the MAME Team"; break;
	}
}



/***************************************************************************
    PIONEER PR-7820 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    pr7820_init - Pioneer PR-7820-specific
    initialization
-------------------------------------------------*/

static void pr7820_init(laserdisc_state *ld)
{
	pr7820_info *pr7820 = &ld->u.pr7820;

	/* set up the write callbacks */
	ld->writeline[LASERDISC_LINE_ENTER] = pr7820_enter_w;

	/* set up the read callbacks */
	ld->readdata = pr7820_status_r;
	ld->readline[LASERDISC_LINE_READY] = pr7820_ready_r;

	/* do a soft reset */
	pr7820->configspeed = PLAY_SPEED / 2;
	pr7820_soft_reset(ld);
}


/*-------------------------------------------------
    pr7820_soft_reset - Pioneer PR-7820-specific
    soft reset
-------------------------------------------------*/

static void pr7820_soft_reset(laserdisc_state *ld)
{
	pr7820_info *pr7820 = &ld->u.pr7820;

	ld->audio = AUDIO_CH1_ENABLE  | AUDIO_CH2_ENABLE;
	ld->display = 0;
	pr7820->mode = PR7820_MODE_MANUAL;
	pr7820->activereg = 0;
	write_16bits_to_ram_be(pr7820->ram, 0, 1);
}


/*-------------------------------------------------
    pr7820_enter_w - write callback when the
    ENTER state is asserted
-------------------------------------------------*/

static void pr7820_enter_w(laserdisc_state *ld, UINT8 newstate)
{
	static const UINT8 numbers[10] = { 0x3f, 0x0f, 0x8f, 0x4f, 0x2f, 0xaf, 0x6f, 0x1f, 0x9f, 0x5f };
	pr7820_info *pr7820 = &ld->u.pr7820;
	UINT8 data = (pr7820->mode == PR7820_MODE_AUTOMATIC) ? pr7820->ram[pr7820->curpc++ % 1024] : ld->datain;

	/* we only care about assertions */
	if (newstate != ASSERT_LINE)
		return;

	/* if we're in program mode, just write data */
	if (pr7820->mode == PR7820_MODE_PROGRAM && data != 0xef)
	{
		pr7820->ram[ld->parameter++ % 1024] = data;
		return;
	}

	/* look for and process numbers */
	if (process_number(ld, data, numbers))
		return;

	/* handle commands */
	switch (data)
	{
		case 0x7f:  CMDPRINTF(("pr7820: %d Recall\n", ld->parameter));
			/* set the active register */
			pr7820->activereg = ld->parameter;
			break;

		case 0xa0:
		case 0xa1:
		case 0xa2:
		case 0xa3:	CMDPRINTF(("pr7820: Direct audio control %d\n", data & 0x03));
			/* control both channels directly */
			ld->audio = 0;
			if (data & 0x01)
				ld->audio |= AUDIO_CH1_ENABLE;
			if (data & 0x02)
				ld->audio |= AUDIO_CH2_ENABLE;
			break;

		case 0xbf:	CMDPRINTF(("pr7820: %d Halt\n", ld->parameter));
			/* stop automatic mode */
			pr7820->mode = PR7820_MODE_MANUAL;
			break;

		case 0xcc:	CMDPRINTF(("pr7820: Load\n"));
			/* load program from disc -- not implemented */
			break;

		case 0xcf:	CMDPRINTF(("pr7820: %d Branch\n", ld->parameter));
			/* branch to a new PC */
			if (pr7820->mode == PR7820_MODE_AUTOMATIC)
				pr7820->curpc = (ld->parameter == -1) ? 0 : ld->parameter;
			break;

		case 0xdf:	CMDPRINTF(("pr7820: %d Write program\n", ld->parameter));
			/* enter program mode */
			pr7820->mode = PR7820_MODE_PROGRAM;
			break;

		case 0xe1:  CMDPRINTF(("pr7820: Soft Reset\n"));
			/* soft reset */
			pr7820_soft_reset(ld);
			break;

		case 0xe3:  CMDPRINTF(("pr7820: Display off\n"));
			/* turn off frame display */
			ld->display = 0x00;
			break;

		case 0xe4:  CMDPRINTF(("pr7820: Display on\n"));
			/* turn on frame display */
			ld->display = 0x01;
			break;

		case 0xe5:  CMDPRINTF(("pr7820: Audio 2 off\n"));
			/* turn off audio channel 2 */
			ld->audio &= ~AUDIO_CH2_ENABLE;
			break;

		case 0xe6:  CMDPRINTF(("pr7820: Audio 2 on\n"));
			/* turn on audio channel 2 */
			ld->audio |= AUDIO_CH2_ENABLE;
			break;

		case 0xe7:  CMDPRINTF(("pr7820: Audio 1 off\n"));
			/* turn off audio channel 1 */
			ld->audio &= ~AUDIO_CH1_ENABLE;
			break;

		case 0xe8:  CMDPRINTF(("pr7820: Audio 1 on\n"));
			/* turn on audio channel 1 */
			ld->audio |= AUDIO_CH1_ENABLE;
			break;

		case 0xe9:	CMDPRINTF(("pr7820: %d Dump RAM\n", ld->parameter));
			/* not implemented */
			break;

		case 0xea:	CMDPRINTF(("pr7820: %d Dump frame\n", ld->parameter));
			/* not implemented */
			break;

		case 0xeb:	CMDPRINTF(("pr7820: %d Dump player status\n", ld->parameter));
			/* not implemented */
			break;

		case 0xef:	CMDPRINTF(("pr7820: %d End program\n", ld->parameter));
			/* exit programming mode */
			pr7820->mode = PR7820_MODE_MANUAL;
			break;

		case 0xf0:	CMDPRINTF(("pr7820: %d Decrement reg\n", ld->parameter));
			/* decrement register; if we hit 0, skip past the next branch statement */
			if (pr7820->mode == PR7820_MODE_AUTOMATIC)
			{
				UINT16 tempreg = read_16bits_from_ram_be(pr7820->ram, (ld->parameter * 2) % 1024);
				tempreg = (tempreg == 0) ? 0 : tempreg - 1;
				write_16bits_to_ram_be(pr7820->ram, (ld->parameter * 2) % 1024, tempreg);
				if (tempreg == 0)
					while (pr7820->ram[pr7820->curpc++ % 1024] != 0xcf) ;
			}
			break;

		case 0xf1:  CMDPRINTF(("pr7820: %d Display\n", ld->parameter));
			/* toggle or set the frame display */
			ld->display = (ld->parameter == -1) ? !ld->display : (ld->parameter & 1);
			break;

		case 0xf2:	CMDPRINTF(("pr7820: %d Slow forward\n", ld->parameter));
			/* play forward at slow speed (controlled by lever on the front of the player) */
			if (laserdisc_ready(ld))
			{
				if (ld->parameter != -1)
					set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, pr7820->configspeed, ld->parameter);
				else
					set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, pr7820->configspeed, NULL_TARGET_FRAME);
			}
			break;

		case 0xf3:  CMDPRINTF(("pr7820: %d Autostop\n", ld->parameter));
			/* play to a particular location and stop there */
			if (laserdisc_ready(ld))
			{
				INT32 targetframe = ld->parameter;

				if (targetframe == -1)
					targetframe = read_16bits_from_ram_be(pr7820->ram, (pr7820->activereg * 2) % 1024);
				pr7820->activereg++;

				if (targetframe > ld->last_frame)
					set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, targetframe);
				else
					set_state(ld, LASERDISC_SEARCHING_FRAME, PR7820_SEARCH_SPEED, targetframe);
			}
			break;

		case 0xf4:  CMDPRINTF(("pr7820: %d Audio track 1\n", ld->parameter));
			/* toggle or set the state of audio channel 1 */
			if (ld->parameter == -1)
				ld->audio ^= AUDIO_CH1_ENABLE;
			else
				ld->audio = (ld->audio & ~AUDIO_CH1_ENABLE) | ((ld->parameter & 1) ? AUDIO_CH1_ENABLE : 0);
			break;

		case 0xf5:	CMDPRINTF(("pr7820: %d Store\n", ld->parameter));
			/* store either the current frame number or an explicit value into the active register */
			if (ld->parameter == -1)
				write_16bits_to_ram_be(pr7820->ram, (pr7820->activereg * 2) % 1024, ld->last_frame);
			else
				write_16bits_to_ram_be(pr7820->ram, (pr7820->activereg * 2) % 1024, ld->parameter);
			pr7820->activereg++;
			break;

		case 0xf6:	CMDPRINTF(("pr7820: Step forward\n"));
			/* step forward one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_FORWARD, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, ONE_TRACK);
			}
			break;

		case 0xf7:  CMDPRINTF(("pr7820: %d Search\n", ld->parameter));
			/* search to a particular frame number */
			if (laserdisc_ready(ld))
			{
				INT32 targetframe = ld->parameter;

				if (targetframe == -1)
					targetframe = read_16bits_from_ram_be(pr7820->ram, (pr7820->activereg * 2) % 1024);
				pr7820->activereg++;

				if (targetframe == 0)
					targetframe = 1;
				set_state(ld, LASERDISC_SEARCHING_FRAME, PR7820_SEARCH_SPEED, targetframe);
			}
			break;

		case 0xf8:	CMDPRINTF(("pr7820: %d Input\n", ld->parameter));
			/* wait for user input -- not implemented */
			break;

		case 0xf9:	CMDPRINTF(("pr7820: Reject\n"));
			/* eject the disc */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_EJECTING, STOP_SPEED, NULL_TARGET_FRAME);
				set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_EJECTED, STOP_SPEED);
			}
			break;

		case 0xfa:	CMDPRINTF(("pr7820: %d Slow reverse\n", ld->parameter));
			/* play backwards at slow speed (controlled by lever on the front of the player) */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, -pr7820->configspeed, (ld->parameter != -1) ? ld->parameter : NULL_TARGET_FRAME);
			break;

		case 0xfb:	CMDPRINTF(("pr7820: %d Stop/Wait\n", ld->parameter));
			/* pause at the current location for a fixed amount of time (in 1/10ths of a second) */
			if (laserdisc_ready(ld))
			{
				playstate prevstate = ld->state;
				INT32 prevspeed = ld->curfracspeed;
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
				if (ld->parameter != -1)
					set_hold_state(ld, double_to_attotime(ld->parameter * 0.1), prevstate, prevspeed);
			}
			break;

		case 0xfc:  CMDPRINTF(("pr7820: %d Audio track 2\n", ld->parameter));
			/* toggle or set the state of audio channel 2 */
			if (ld->parameter == -1)
				ld->audio ^= AUDIO_CH2_ENABLE;
			else
				ld->audio = (ld->audio & ~AUDIO_CH2_ENABLE) | ((ld->parameter & 1) ? AUDIO_CH2_ENABLE : 0);
			break;

		case 0xfd:  CMDPRINTF(("pr7820: Play\n"));
			/* begin playing at regular speed, or load the disc if it is parked */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, NULL_TARGET_FRAME);
			else
			{
				if (ld->state == LASERDISC_PARKED)
				{
					set_state(ld, LASERDISC_SPINUP, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_SPINUP_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}
				else
				{
					set_state(ld, LASERDISC_LOADING, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}

				ld->curfractrack = ONE_TRACK;
			}
			break;

		case 0xfe:	CMDPRINTF(("pr7820: Step reverse\n"));
			/* step backwards one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_REVERSE, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, -ONE_TRACK);
			}
			break;

		default:	CMDPRINTF(("pr7820: %d Unknown command %02X\n", ld->parameter, data));
			/* unknown command */
			break;
	}

	/* reset the parameter after executing a command */
	ld->parameter = -1;
}


/*-------------------------------------------------
    pr7820_ready_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 pr7820_ready_r(laserdisc_state *ld)
{
	return (ld->state != LASERDISC_SEARCHING_FRAME) ? ASSERT_LINE : CLEAR_LINE;
}


/*-------------------------------------------------
    pr7820_status_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 pr7820_status_r(laserdisc_state *ld)
{
	pr7820_info *pr7820 = &ld->u.pr7820;

	/* top 3 bits reflect audio and display states */
	UINT8 status = (ld->audio << 6) | (ld->display << 5);

	/* low 5 bits reflect player states */

	/* handle program mode */
	if (pr7820->mode == PR7820_MODE_PROGRAM)
		status |= 0x0c;
	else
	{
		/* convert generic status into specific state equivalents */
		switch (ld->state)
		{
			case LASERDISC_EJECTED:					status |= 0x0d;		break;
			case LASERDISC_EJECTING:				status |= 0x0d;		break;
			case LASERDISC_LOADED:					status |= 0x01;		break;
			case LASERDISC_SPINUP:					status |= 0x01;		break;
			case LASERDISC_LOADING:					status |= 0x01;		break;
			case LASERDISC_PARKED:					status |= 0x01;		break;

			case LASERDISC_SEARCHING_FRAME:			status |= 0x06;		break;
			case LASERDISC_SEARCH_FINISHED:			status |= 0x07;		break;

			case LASERDISC_STOPPED:					status |= 0x03;		break;
			case LASERDISC_AUTOSTOPPED:				status |= 0x09;		break;

			case LASERDISC_PLAYING_FORWARD:			status |= 0x02;		break;
			case LASERDISC_PLAYING_REVERSE:			status |= 0x02;		break;
			case LASERDISC_PLAYING_SLOW_FORWARD:	status |= 0x04;		break;
			case LASERDISC_PLAYING_SLOW_REVERSE:	status |= 0x05;		break;
			case LASERDISC_PLAYING_FAST_FORWARD:	status |= 0x02;		break;
			case LASERDISC_PLAYING_FAST_REVERSE:	status |= 0x02;		break;
			case LASERDISC_STEPPING_FORWARD:		status |= 0x03;		break;
			case LASERDISC_STEPPING_REVERSE:		status |= 0x03;		break;
			case LASERDISC_SCANNING_FORWARD:		status |= 0x02;		break;
			case LASERDISC_SCANNING_REVERSE:		status |= 0x02;		break;
			default:
				fatalerror("Unexpected disc state in pr7820_status_r\n");
				break;
		}
	}
	return status;
}


/*-------------------------------------------------
    pr7820_set_slow_speed - set the speed of
    "slow" playback, which is controlled by a
    slider on the device
-------------------------------------------------*/

void pr7820_set_slow_speed(const device_config *device, double frame_rate_scaler)
{
	laserdisc_state *ld = get_safe_token(device);
	pr7820_info *pr7820 = &ld->u.pr7820;
	pr7820->configspeed = PLAY_SPEED * frame_rate_scaler;
}



/***************************************************************************
    PIONEER PR-8210 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    pr8210_init - Pioneer PR-8210-specific
    initialization
-------------------------------------------------*/

static void pr8210_init(laserdisc_state *ld)
{
	/* set up the write callbacks */
	ld->writeline[LASERDISC_LINE_CONTROL] = pr8210_control_w;	/* serial access */

	/* do a soft reset */
	pr8210_soft_reset(ld);
}


/*-------------------------------------------------
    pr8210_soft_reset - Pioneer PR-8210-specific
    soft reset
-------------------------------------------------*/

static void pr8210_soft_reset(laserdisc_state *ld)
{
	pr8210_info *pr8210 = &ld->u.pr8210;

	ld->audio = AUDIO_CH1_ENABLE  | AUDIO_CH2_ENABLE;
	ld->display = 0;
	pr8210->mode = PR8210_MODE_GET_1ST;
	pr8210->commandtime = timer_get_time();
	pr8210->commandbits = 0;
	memset( pr8210->commandtriplet, 0, 3*sizeof(UINT16) );
	pr8210->seekstate = 0;
}


/*-------------------------------------------------
    pr8210_command - Pioneer PR-8210-specific
    command processing
-------------------------------------------------*/

static void pr8210_command(laserdisc_state *ld)
{
	pr8210_info *pr8210 = &ld->u.pr8210;

	/* if we don't have the entire command triplet yet, keep going */
	if ( pr8210->mode < PR8210_MODE_GET_3RD )
	{
		/* do some sanity checks on the command data */
		if ( pr8210->mode == PR8210_MODE_GET_2ND )
		{
			/* if the commands don't match, then reassign the new command to the first word, then keep fetching */
			if ( pr8210->commandtriplet[PR8210_MODE_GET_1ST] != pr8210->commandtriplet[PR8210_MODE_GET_2ND] )
			{
				pr8210->commandtriplet[PR8210_MODE_GET_1ST] = pr8210->commandtriplet[PR8210_MODE_GET_2ND];
				pr8210->mode = PR8210_MODE_GET_2ND;
				return;
			}
		}

		pr8210->mode++;
	}
	else /* we're ready to process the command */
	{
		/* do some sanity checks on the command data */
		static const UINT8 numbers[10] = { 0x01,0x11,0x09,0x19,0x05,0x15,0x0d,0x1d,0x03,0x13 };
		UINT16	cmd = pr8210->commandtriplet[PR8210_MODE_GET_1ST];

		/* more sanity checking: bit 7 must be set for a valid command */
		if ( cmd & 0x80 )
		{
			/* extract the actual command number */
			cmd = ( cmd >> 2 ) & 0x1f;

			/* look for and process numbers */
			if (!process_number(ld, cmd, numbers))
			{
				switch( cmd )
				{
					case 0x00:	CMDPRINTF(("pr8210: EOC\n"));
						/* EOC marker - can be safely ignored */
						break;

					case 0x02:	CMDPRINTF(("pr8210: Slow reverse\n"));
						/* slow reverse */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, -PR8210_SLOW_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x04:	CMDPRINTF(("pr8210: Step forward\n"));
						/* step forward */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_STEPPING_FORWARD, PR8210_STEP_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x06 :	CMDPRINTF(("pr8210: Chapter\n"));
						/* chapter -- not implemented */
						break;

					case 0x08:	CMDPRINTF(("pr8210: Scan forward\n"));
						/* scan forward */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_SCANNING_FORWARD, PR8210_SCAN_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x0a:	CMDPRINTF(("pr8210: Pause\n"));
						/* still picture */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x0b :	CMDPRINTF(("pr8210: Frame\n"));
						/* frame -- not implemented */
						break;

					case 0x0c:	CMDPRINTF(("pr8210: Fast reverse\n"));
						/* play reverse fast speed */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_PLAYING_FAST_REVERSE, -PR8210_FAST_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x0e:	CMDPRINTF(("pr8210: Ch1 toggle\n"));
						/* channel 1 audio toggle */
						ld->audio ^= AUDIO_CH1_ENABLE;
						break;

					case 0x10:	CMDPRINTF(("pr8210: Fast forward\n"));
						/* play forward fast speed */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_PLAYING_FAST_FORWARD, PR8210_FAST_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x12:	CMDPRINTF(("pr8210: Step reverse\n"));
						/* step backwards one frame */
						if (laserdisc_ready(ld))
						{
							set_state(ld, LASERDISC_STEPPING_REVERSE, STOP_SPEED, NULL_TARGET_FRAME);
							add_to_current_track(ld, -ONE_TRACK);
						}
						break;

					case 0x14:  CMDPRINTF(("pr8210: Play\n"));
						/* begin playing at regular speed, or load the disc if it is parked */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, NULL_TARGET_FRAME);
						else
						{
							/* if we're already spinning up or loading, ignore */
							if (ld->state != LASERDISC_SPINUP && ld->state != LASERDISC_LOADING)
							{
								if (ld->state == LASERDISC_PARKED)
								{
									set_state(ld, LASERDISC_SPINUP, STOP_SPEED, NULL_TARGET_FRAME);
									set_hold_state(ld, GENERIC_SPINUP_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
								}
								else
								{
									set_state(ld, LASERDISC_LOADING, STOP_SPEED, NULL_TARGET_FRAME);
									set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
								}

								ld->curfractrack = ONE_TRACK;
							}
						}
						break;

					case 0x16:	CMDPRINTF(("pr8210: Ch2 toggle\n"));
						/* channel 1 audio toggle */
						ld->audio ^= AUDIO_CH2_ENABLE;
						break;

					case 0x18:	CMDPRINTF(("pr8210: Slow forward\n"));
						/* slow forward */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, PR8210_SLOW_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x1a:	CMDPRINTF(("pr8210: Seek\n"));
						/* seek */
						if ( pr8210->seekstate )
						{
							CMDPRINTF(("pr8210: Seeking to frame:%d\n", ld->parameter));
							/* we're ready to seek */
							set_state(ld, LASERDISC_SEARCHING_FRAME, PR8210_SEARCH_SPEED, ld->parameter);
						}
						else
						{
							/* waiting for digits indicating position */
							ld->parameter = 0;
						}
						pr8210->seekstate ^=1 ;
						break;

					case 0x1c:	CMDPRINTF(("pr8210: Scan reverse\n"));
						/* scan reverse */
						if (laserdisc_ready(ld))
							set_state(ld, LASERDISC_SCANNING_REVERSE, -PR8210_SCAN_SPEED, NULL_TARGET_FRAME);
						break;

					case 0x1e:	CMDPRINTF(("pr8210: Reject\n"));
						/* eject the disc */
						if (laserdisc_ready(ld))
						{
							set_state(ld, LASERDISC_EJECTING, STOP_SPEED, NULL_TARGET_FRAME);
							set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_EJECTED, STOP_SPEED);
						}
						break;

					default:	CMDPRINTF(("pr8210: Unknown command %02X\n", cmd));
						/* unknown command */
						break;
				}
			}
		}

		/* reset our command data */
		memset( pr8210->commandtriplet, 0, 3*sizeof(UINT16) );
		pr8210->mode = PR8210_MODE_GET_1ST;
	}
}


/*-------------------------------------------------
    pr8210_command_w - write callback when the
    CONTROL line is toggled
-------------------------------------------------*/

static void pr8210_control_w(laserdisc_state *ld, UINT8 data)
{
	pr8210_info *pr8210 = &ld->u.pr8210;

	if ( data == ASSERT_LINE )
	{
		/* get the time difference from the last assert */
		attotime delta = attotime_sub(timer_get_time(), pr8210->commandtime);

		/* and update our internal command time */
		pr8210->commandtime = timer_get_time();

#if 0
		{
			int usecdiff = (int)(delta.attoseconds / ATTOSECONDS_IN_USEC(1));

			printf( "bitdelta = %d\n", usecdiff );
		}
#endif

		/* if the delay is less than 3 msec, we're receiving data */
		if ( delta.attoseconds < ATTOTIME_IN_MSEC(3).attoseconds )
		{
			/* 0 bit delta is 1.05 msec, 1 bit delta is 2.11 msec */
			int longpulse = ( delta.attoseconds < ATTOTIME_IN_USEC(1500).attoseconds ) ? 0 : 1;
			pr8210->commandtriplet[pr8210->mode] <<= 1;
			pr8210->commandtriplet[pr8210->mode] |= longpulse;

			/* if we received 10 bits, see what we need to do */
			if ( ++pr8210->commandbits >= 10 )
			{
				/* reset bit shift count */
				pr8210->commandbits = 0;

				/* mask just the 10 bits */
				pr8210->commandtriplet[pr8210->mode] &= 0x3ff;

				/* execute command */
				pr8210_command( ld );
			}
		}
	}
}



/***************************************************************************
    PIONEER LD-V1000 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ldv1000_init - Pioneer LDV-1000-specific
    initialization
-------------------------------------------------*/

static void ldv1000_init(laserdisc_state *ld)
{
	/* set up the write callbacks */
	ld->writedata = ldv1000_data_w;

	/* set up the read callbacks */
	ld->readdata = ldv1000_status_r;
	ld->readline[LASERDISC_LINE_STATUS] = ldv1000_status_strobe_r;
	ld->readline[LASERDISC_LINE_COMMAND] = ldv1000_command_strobe_r;

	/* do a soft reset */
	ldv1000_soft_reset(ld);
}


/*-------------------------------------------------
    ldv1000_soft_reset - Pioneer LDV-1000-specific
    soft reset
-------------------------------------------------*/

static void ldv1000_soft_reset(laserdisc_state *ld)
{
	ldv1000_info *ldv1000 = &ld->u.ldv1000;

	ld->audio = AUDIO_CH1_ENABLE | AUDIO_CH2_ENABLE;
	ld->display = FALSE;
	ldv1000->mode = LDV1000_MODE_STATUS;
	ldv1000->activereg = 0;
	ldv1000->statusmask = 0xff;
	write_16bits_to_ram_be(ldv1000->ram, 0, 1);
}


/*-------------------------------------------------
    ldv1000_data_w - write callback when the
    ENTER state is written
-------------------------------------------------*/

static void ldv1000_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data)
{
	static const UINT8 numbers[10] = { 0x3f, 0x0f, 0x8f, 0x4f, 0x2f, 0xaf, 0x6f, 0x1f, 0x9f, 0x5f };
	ldv1000_info *ldv1000 = &ld->u.ldv1000;

	/* 0xFF bytes are used for synchronization */
	if (data == 0xff)
	{
		ldv1000->statusmask = 0xff;	/* EHC 012207: reset our status mask */
		return;
	}

	/* if we're not ready, return, otherwise signal ourselves busy */
	if (ldv1000->statusmask == 0x7f)
		return;
	ldv1000->statusmask = 0x7f;

	/* look for and process numbers */
	if (process_number(ld, data, numbers))
		return;

	/* handle commands */
	switch (data)
	{
		case 0x7f:  CMDPRINTF(("ldv1000: %d Recall\n", ld->parameter));
			/* set the active register */
			ldv1000->activereg = ld->parameter;
			/* should also display the register value */
			break;

		case 0x20:  CMDPRINTF(("ldv1000: x0 reverse (stop) - Badlands special\n"));
			/* play reverse at 0 speed (stop) */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x21:  CMDPRINTF(("ldv1000: x1/4 reverse - Badlands special\n"));
			/* play reverse at 1/4 speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, -PLAY_SPEED / 4, NULL_TARGET_FRAME);
			break;

		case 0x22:  CMDPRINTF(("ldv1000: x1/2 reverse - Badlands special\n"));
			/* play reverse at 1/2 speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, -PLAY_SPEED / 2, NULL_TARGET_FRAME);
			break;

		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:  CMDPRINTF(("ldv1000: x%d reverse - Badlands special\n", (data & 0x07) - 2));
			/* play reverse at 1-5x speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FAST_REVERSE, -PLAY_SPEED * ((data & 0x07) - 2), NULL_TARGET_FRAME);
			break;

		case 0xa0:  CMDPRINTF(("ldv1000: x0 forward (stop)\n"));
			/* play forward at 0 speed (stop) */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		case 0xa1:  CMDPRINTF(("ldv1000: x1/4 forward\n"));
			/* play forward at 1/4 speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, PLAY_SPEED / 4, NULL_TARGET_FRAME);
			break;

		case 0xa2:  CMDPRINTF(("ldv1000: x1/2 forward\n"));
			/* play forward at 1/2 speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, PLAY_SPEED / 2, NULL_TARGET_FRAME);
			break;

		case 0xa3:
		case 0xa4:
		case 0xa5:
		case 0xa6:
		case 0xa7:  CMDPRINTF(("ldv1000: x%d forward\n", (data & 0x07) - 2));
			/* play forward at 1-5x speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FAST_FORWARD, PLAY_SPEED * ((data & 0x07) - 2), NULL_TARGET_FRAME);
			break;

		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7:
		case 0xb8:
		case 0xb9:
		case 0xba:  CMDPRINTF(("ldv1000: Skip forward %d0\n", data & 0x0f));
			/* skip forward */
			if (laserdisc_active(ld))
			{
				/* note that this skips tracks, not frames; the track->frame count is not 1:1 */
				/* in the case of 3:2 pulldown or other effects; for now, we just ignore the diff */
				add_to_current_track(ld, INT_TO_FRAC(10 * (data & 0x0f)));
			}
			break;

		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3a:  CMDPRINTF(("ldv1000: Skip backwards %d0 - Badlands special\n", data & 0x0f));
			/* skip backward */
			if (laserdisc_active(ld))
			{
				/* note that this skips tracks, not frames; the track->frame count is not 1:1 */
				/* in the case of 3:2 pulldown or other effects; for now, we just ignore the diff */
				add_to_current_track(ld, -INT_TO_FRAC(10 * (data & 0x0f)));
			}
			break;

		case 0xbf:	CMDPRINTF(("ldv1000: %d Clear\n", ld->parameter));
			/* clears register display and removes pending arguments */
			break;

		case 0xc2:	CMDPRINTF(("ldv1000: Get frame no.\n"));
			/* returns the current frame number */
			ldv1000->mode = LDV1000_MODE_GET_FRAME;
			ldv1000->readpos = 0;
			ldv1000->readtotal = 5;
			sprintf((char *)ldv1000->readbuf, "%05d", ld->last_frame);
			break;

		case 0xc3:	CMDPRINTF(("ldv1000: Get 2nd display\n"));
			/* returns the data from the 2nd display line */
			ldv1000->mode = LDV1000_MODE_GET_2ND;
			ldv1000->readpos = 0;
			ldv1000->readtotal = 8;
			sprintf((char *)ldv1000->readbuf, "	 \x1c\x1c\x1c");
			break;

		case 0xc4:	CMDPRINTF(("ldv1000: Get 1st display\n"));
			/* returns the data from the 1st display line */
			ldv1000->mode = LDV1000_MODE_GET_1ST;
			ldv1000->readpos = 0;
			ldv1000->readtotal = 8;
			sprintf((char *)ldv1000->readbuf, "	 \x1c\x1c\x1c");
			break;

		case 0xc8:	CMDPRINTF(("ldv1000: Transfer memory\n"));
			/* returns the data from the 1st display line */
			ldv1000->mode = LDV1000_MODE_GET_RAM;
			ldv1000->readpos = 0;
			ldv1000->readtotal = 1024;
			break;

		case 0xcc:	CMDPRINTF(("ldv1000: Load\n"));
			/* load program from disc -- not implemented */
			break;

		case 0xcd:	CMDPRINTF(("ldv1000: Display disable\n"));
			/* disables the display of current command -- not implemented */
			break;

		case 0xce:	CMDPRINTF(("ldv1000: Display enable\n"));
			/* enables the display of current command -- not implemented */
			break;

		case 0xf0:	CMDPRINTF(("ldv1000: Scan forward\n"));
			/* scan forward */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_SCANNING_FORWARD, LDV1000_SCAN_SPEED, NULL_TARGET_FRAME);
			break;

		case 0xf1:  CMDPRINTF(("ldv1000: %d Display\n", ld->parameter));
			/* toggle or set the frame display */
			ld->display = (ld->parameter == -1) ? !ld->display : (ld->parameter & 1);
			break;

		case 0xf3:  CMDPRINTF(("ldv1000: %d Autostop\n", ld->parameter));
			/* play to a particular location and stop there */
			if (laserdisc_ready(ld))
			{
				INT32 targetframe = ld->parameter;

				if (targetframe == -1)
					targetframe = read_16bits_from_ram_be(ldv1000->ram, (ldv1000->activereg++ * 2) % 1024);

				if (targetframe > ld->last_frame)
					set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, targetframe);
				else
					set_state(ld, LASERDISC_SEARCHING_FRAME, PR7820_SEARCH_SPEED, targetframe);
	   		}
			break;

		case 0xf4:  CMDPRINTF(("ldv1000: %d Audio track 1\n", ld->parameter));
			/* toggle or set the state of audio channel 1 */
			if (ld->parameter == -1)
				ld->audio ^= AUDIO_CH1_ENABLE;
			else
				ld->audio = (ld->audio & ~AUDIO_CH1_ENABLE) | ((ld->parameter & 1) ? AUDIO_CH1_ENABLE : 0);
			break;

		case 0xf5:	CMDPRINTF(("ldv1000: %d Store\n", ld->parameter));
			/* store either the current frame number or an explicit value into the active register */
			if (ld->parameter == -1)
				write_16bits_to_ram_be(ldv1000->ram, (ldv1000->activereg * 2) % 1024, ld->last_frame);
			else
				write_16bits_to_ram_be(ldv1000->ram, (ldv1000->activereg * 2) % 1024, ld->parameter);
			ldv1000->activereg++;
			break;

		case 0xf6:	CMDPRINTF(("ldv1000: Step forward\n"));
			/* step forward one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_FORWARD, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, ONE_TRACK);
			}
			break;

		case 0xf7:  CMDPRINTF(("ldv1000: %d Search\n", ld->parameter));
			/* search to a particular frame number */
			if (laserdisc_ready(ld))
			{
				INT32 targetframe = ld->parameter;

				if (targetframe == -1)
					targetframe = read_16bits_from_ram_be(ldv1000->ram, (ldv1000->activereg++ * 2) % 1024);
				ldv1000->activereg++;

				set_state(ld, LASERDISC_SEARCHING_FRAME, LDV1000_SEARCH_SPEED, targetframe);
			}
			break;

		case 0xf8:	CMDPRINTF(("ldv1000: Scan reverse\n"));
			/* scan reverse */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_SCANNING_REVERSE, -LDV1000_SCAN_SPEED, NULL_TARGET_FRAME);
			break;

		case 0xf9:	CMDPRINTF(("ldv1000: Reject\n"));
			/* move the head to parked position, and stop rotation */
			set_state(ld, LASERDISC_PARKED, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		case 0xfb:	CMDPRINTF(("ldv1000: %d Stop/Wait\n", ld->parameter));
			/* pause at the current location for a fixed amount of time (in 1/10ths of a second) */
			if (laserdisc_ready(ld))
			{
				playstate prevstate = ld->state;
				INT32 prevspeed = ld->curfracspeed;
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
				if (ld->parameter != -1)
					set_hold_state(ld, double_to_attotime(ld->parameter * 0.1), prevstate, prevspeed);
			}
			break;

		case 0xfc:  CMDPRINTF(("ldv1000: %d Audio track 2\n", ld->parameter));
			/* toggle or set the state of audio channel 2 */
			if (ld->parameter == -1)
				ld->audio ^= AUDIO_CH2_ENABLE;
			else
				ld->audio = (ld->audio & ~AUDIO_CH2_ENABLE) | ((ld->parameter & 1) ? AUDIO_CH2_ENABLE : 0);
			break;

		case 0xfd:  CMDPRINTF(("ldv1000: Play\n"));
			/* begin playing at regular speed, or load the disc if it is parked */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, NULL_TARGET_FRAME);
			else
			{
				if (ld->state == LASERDISC_PARKED)
				{
					set_state(ld, LASERDISC_SPINUP, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_SPINUP_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}
				else
				{
					set_state(ld, LASERDISC_LOADING, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}

				ld->curfractrack = ONE_TRACK;
			}
			break;

		case 0xfe:	CMDPRINTF(("ldv1000: Step reverse\n"));
			/* step backwards one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_REVERSE, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, -ONE_TRACK);
			}
			break;

		default:	CMDPRINTF(("ldv1000: %d Unknown command %02X\n", ld->parameter, data));
			/* unknown command */
			break;
	}

	/* reset the parameter after executing a command */
	ld->parameter = -1;
}


/*-------------------------------------------------
    ldv1000_status_strobe_r - return state of the
    status strobe
-------------------------------------------------*/

static UINT8 ldv1000_status_strobe_r(laserdisc_state *ld)
{
	/* the status strobe is asserted (active low) 500-650usec after VSYNC */
	/* for a duration of 26usec; we pick 600-626usec */
	attotime delta = attotime_sub(timer_get_time(), ld->lastvsynctime);
	if (delta.attoseconds >= ATTOTIME_IN_USEC(600).attoseconds &&
		delta.attoseconds < ATTOTIME_IN_USEC(626).attoseconds)
		return ASSERT_LINE;

	return CLEAR_LINE;
}


/*-------------------------------------------------
    ldv1000_command_strobe_r - return state of the
    command strobe
-------------------------------------------------*/

static UINT8 ldv1000_command_strobe_r(laserdisc_state *ld)
{
	/* the command strobe is asserted (active low) 54 or 84usec after the status */
	/* strobe for a duration of 25usec; we pick 600+84 = 684-709usec */
	/* for a duration of 26usec; we pick 600-626usec */
	attotime delta = attotime_sub(timer_get_time(), ld->lastvsynctime);
	if (delta.attoseconds >= ATTOTIME_IN_USEC(684).attoseconds &&
		delta.attoseconds < ATTOTIME_IN_USEC(709).attoseconds)
		return ASSERT_LINE;

	return CLEAR_LINE;
}


/*-------------------------------------------------
    ldv1000_status_r - return state of the ready
    line
-------------------------------------------------*/

static UINT8 ldv1000_status_r(laserdisc_state *ld)
{
	ldv1000_info *ldv1000 = &ld->u.ldv1000;
	UINT8 status = 0xff;

	/* switch off the current mode */
	switch (ldv1000->mode)
	{
		/* reading frame number returns 5 characters */
		/* reading display lines returns 8 characters */
		case LDV1000_MODE_GET_FRAME:
		case LDV1000_MODE_GET_1ST:
		case LDV1000_MODE_GET_2ND:
			assert(ldv1000->readpos < ldv1000->readtotal);
			status = ldv1000->readbuf[ldv1000->readpos++];
			if (ldv1000->readpos == ldv1000->readtotal)
				ldv1000->mode = LDV1000_MODE_STATUS;
			break;

		/* reading RAM returns 1024 bytes */
		case LDV1000_MODE_GET_RAM:
			assert(ldv1000->readpos < ldv1000->readtotal);
			status = ldv1000->ram[1023 - ldv1000->readpos++];
			if (ldv1000->readpos == ldv1000->readtotal)
				ldv1000->mode = LDV1000_MODE_STATUS;
			break;

		/* otherwise, we just compute a status code */
		default:
		case LDV1000_MODE_STATUS:
			switch (ld->state)
			{
				case LASERDISC_EJECTED:					status = 0xe0;		break;
				case LASERDISC_EJECTING:				status = 0x60;		break;
				case LASERDISC_LOADED:					status = 0xc8;		break;
				case LASERDISC_SPINUP:
				case LASERDISC_LOADING:					status = 0x48;		break;
				case LASERDISC_PARKED:					status = 0xfc;		break;

				case LASERDISC_SEARCHING_FRAME:			status = 0x50;		break;
				case LASERDISC_SEARCH_FINISHED:			status = 0xd0;		break;

				case LASERDISC_STOPPED:					status = 0xe5;		break;
				case LASERDISC_AUTOSTOPPED:				status = 0x54;		break;

				case LASERDISC_PLAYING_FORWARD:			status = 0xe4;		break;
				case LASERDISC_PLAYING_REVERSE:			status = 0xe4;		break;
				case LASERDISC_PLAYING_SLOW_FORWARD:	status = 0xae;		break;
				case LASERDISC_PLAYING_SLOW_REVERSE:	status = 0xae;		break;
				case LASERDISC_PLAYING_FAST_FORWARD:	status = 0xae;		break;
				case LASERDISC_PLAYING_FAST_REVERSE:	status = 0xae;		break;
				case LASERDISC_STEPPING_FORWARD:		status = 0xe5;		break;
				case LASERDISC_STEPPING_REVERSE:		status = 0xe5;		break;
				case LASERDISC_SCANNING_FORWARD:		status = 0x4c;		break;
				case LASERDISC_SCANNING_REVERSE:		status = 0x4c;		break;
				default:
					fatalerror("Unexpected disc state in ldv1000_status_r\n");
					break;
			}
			break;
	}

	/* bit 7 indicates our busy status */
	return status & ldv1000->statusmask;
}



/***************************************************************************
    SONY LDP-1450 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ldp1450_init - Sony LDP-1450-specific
    initialization
-------------------------------------------------*/

static void ldp1450_init(laserdisc_state *ld)
{
	/* set up the write callbacks */
	ld->writedata = ldp1450_data_w;

	/* set up the read callbacks */
	ld->readdata = ldp1450_data_r;
	ld->readline[LASERDISC_LINE_DATA_AVAIL] = ldp1450_data_avail_r;

	/* use a state changed callback */
	ld->statechanged = ldp1450_state_changed;

	/* do a soft reset */
	ldp1450_soft_reset(ld);
}


/*-------------------------------------------------
    ldp1450_soft_reset - Sony LDP-1450-specific
    soft reset
-------------------------------------------------*/

static void ldp1450_soft_reset(laserdisc_state *ld)
{
	ld->audio = AUDIO_CH1_ENABLE | AUDIO_CH2_ENABLE;
	ld->display = FALSE;
	set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
	ld->curfractrack = ONE_TRACK;
}


/*-------------------------------------------------
    ldp1450_compute_status - compute the current
    status bytes on the LDP-1450
-------------------------------------------------*/

static void ldp1450_compute_status(laserdisc_state *ld)
{
	/*
       Byte 0: LDP ready status:
        0x80 Ready
        0x82 Disc Spinning down
        0x86 Tray closed & no disc loaded
        0x8A Disc ejecting, tray opening
        0x92 Disc loading, tray closing
        0x90 Disc spinning up
        0xC0 Busy Searching

       Byte 1: Error status:
        0x00 No error
        0x01 Focus unlock
        0x02 Communication Error

       Byte 2: Disc/motor status:
        0x01 Motor Off (disc spinning down or tray opening/ed)
        0x02 Tray closed & motor Off (no disc loaded)
        0x11 Motor On

       Byte 3: Command/argument status:
        0x00 Normal
        0x01 Relative search or Mark Set pending (Enter not sent)
        0x03 Search pending (Enter not sent)
        0x05 Repeat pending (Enter not sent) or Memory Search in progress
        0x80 FWD or REV Step

       Byte 4: Playback mode:
        0x00 Searching, Motor On (spinning up), motor Off (spinning down), tray opened/ing (no motor phase lock)
        0x01 FWD Play
        0x02 FWD Fast
        0x04 FWD Slow
        0x08 FWD Step
        0x10 FWD Scan
        0x20 Still
        0x81 REV Play
        0x82 REV Fast
        0x84 REV Slow
        0x88 REV Step
        0x90 REV Scan
    */

	ldp1450_info *ldp1450 = &ld->u.ldp1450;
	UINT32 statusbytes = 0;

	switch (ld->state)
	{
		case LASERDISC_EJECTED:					statusbytes = 0x86000200;	break;
		case LASERDISC_EJECTING:				statusbytes = 0x8a000100;	break;
		case LASERDISC_LOADED:					statusbytes = 0x80000100;	break;
		case LASERDISC_SPINUP:
		case LASERDISC_LOADING:					statusbytes = 0x92001100;	break;
		case LASERDISC_PARKED:					statusbytes = 0x80000100;	break;
		case LASERDISC_SEARCHING_FRAME:			statusbytes = 0xc0001100;	break;

		case LASERDISC_SEARCH_FINISHED:
		case LASERDISC_STOPPED:
		case LASERDISC_AUTOSTOPPED:				statusbytes = 0x80001120;	break;

		case LASERDISC_PLAYING_FORWARD:			statusbytes = 0x80001101;	break;
		case LASERDISC_PLAYING_REVERSE:			statusbytes = 0x80001181;	break;
		case LASERDISC_PLAYING_SLOW_FORWARD:	statusbytes = 0x80001104;	break;
		case LASERDISC_PLAYING_SLOW_REVERSE:	statusbytes = 0x80001184;	break;
		case LASERDISC_PLAYING_FAST_FORWARD:	statusbytes = 0x80001102;	break;
		case LASERDISC_PLAYING_FAST_REVERSE:	statusbytes = 0x80001182;	break;
		case LASERDISC_STEPPING_FORWARD:		statusbytes = 0x80001108;	break;
		case LASERDISC_STEPPING_REVERSE:		statusbytes = 0x80001188;	break;
		case LASERDISC_SCANNING_FORWARD:		statusbytes = 0x80001110;	break;
		case LASERDISC_SCANNING_REVERSE:		statusbytes = 0x80001190;	break;

		default:
			fatalerror("Unexpected disc state in ldp1450_compute_status\n");
			break;
	}

	/* copy in the result bytes */
	ldp1450->readtotal = 0;
	ldp1450->readbuf[ldp1450->readtotal++] = (statusbytes >> 24) & 0xff;
	ldp1450->readbuf[ldp1450->readtotal++] = (statusbytes >> 16) & 0xff;
	ldp1450->readbuf[ldp1450->readtotal++] = (statusbytes >> 8) & 0xff;
	switch (ld->command)
	{
		case 0x43:	ldp1450->readbuf[ldp1450->readtotal++] = 0x03;	break;
		default:	ldp1450->readbuf[ldp1450->readtotal++] = 0x00;	break;
	}
	ldp1450->readbuf[ldp1450->readtotal++] = (statusbytes >> 0) & 0xff;
}


/*-------------------------------------------------
    ldp1450_data_w - write callback when the
    ENTER state is written
-------------------------------------------------*/

static void ldp1450_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data)
{
	static const UINT8 numbers[10] = { 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39 };
	ldp1450_info *ldp1450 = &ld->u.ldp1450;

	/* by default, we return an ack on each command */
	ldp1450->readpos = ldp1450->readtotal = 0;
	ldp1450->readbuf[ldp1450->readtotal++] = 0x0a;

	/* look for and process numbers */
	if (process_number(ld, data, numbers))
	{
		CMDPRINTF(("%c", data));
		return;
	}

	/* handle commands */
	switch (data)
	{
		case 0x24:  CMDPRINTF(("ldp1450: Audio Mute On\n"));
			/* mute audio */
			ld->audio |= AUDIO_EXPLICIT_MUTE;
			break;

		case 0x25:  CMDPRINTF(("ldp1450: Audio Mute Off\n"));
			/* unmute audio */
			ld->audio &= ~AUDIO_EXPLICIT_MUTE;
			break;

		case 0x26:  CMDPRINTF(("ldp1450: Video Mute Off\n"));
			/* mute video -- not implemented */
			break;

		case 0x27:  CMDPRINTF(("ldp1450: Video Mute On\n"));
			/* unmute video -- not implemented */
			break;

		case 0x28:  CMDPRINTF(("ldp1450: PSC Enable\n"));
			/* enable Picture Stop Codes (PSC) -- not implemented */
			break;

		case 0x29:  CMDPRINTF(("ldp1450: PSC Disable\n"));
			/* disable Picture Stop Codes (PSC) -- not implemented */
			break;

		case 0x2a:	CMDPRINTF(("ldp1450: %d Eject\n", ld->parameter));
			/* eject the disc */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_EJECTING, STOP_SPEED, NULL_TARGET_FRAME);
				set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_EJECTED, STOP_SPEED);
			}
			break;

		case 0x2b:	CMDPRINTF(("ldp1450: %d Step forward\n", ld->parameter));
			/* step forward one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_FORWARD, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, ONE_TRACK);
			}
			break;

		case 0x2c:	CMDPRINTF(("ldp1450: %d Step reverse\n", ld->parameter));
			/* step backwards one frame */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_REVERSE, STOP_SPEED, NULL_TARGET_FRAME);
				add_to_current_track(ld, -ONE_TRACK);
			}
			break;

		case 0x2d:  CMDPRINTF(("ldp1450: %d Search multiple tracks forward\n", ld->parameter));
			/* enable Picture Stop Codes (PSC) -- not implemented */
			break;

		case 0x2e:  CMDPRINTF(("ldp1450: %d Search multiple tracks reverse\n", ld->parameter));
			/* disable Picture Stop Codes (PSC) -- not implemented */
			break;

		case 0x3a:  CMDPRINTF(("ldp1450: %d Play\n", ld->parameter));
			/* begin playing at regular speed, or load the disc if it is parked */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FORWARD, PLAY_SPEED, NULL_TARGET_FRAME);
			else
			{
				if (ld->state == LASERDISC_PARKED)
				{
					set_state(ld, LASERDISC_SPINUP, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_SPINUP_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}
				else
				{
					set_state(ld, LASERDISC_LOADING, STOP_SPEED, NULL_TARGET_FRAME);
					set_hold_state(ld, GENERIC_LOAD_TIME, LASERDISC_PLAYING_FORWARD, PLAY_SPEED);
				}

				ld->curfractrack = ONE_TRACK;
			}
			break;

		case 0x3b:  CMDPRINTF(("ldp1450: %d Fast forward play\n", ld->parameter));
			/* play forward fast speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FAST_FORWARD, LDP1450_FAST_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x3c:  CMDPRINTF(("ldp1450: %d Slow forward play\n", ld->parameter));
			/* play forward slow speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_FORWARD, LDP1450_SLOW_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x3d:  CMDPRINTF(("ldp1450: %d Variable forward play\n", ld->parameter));
			/* play forward variable speed */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_FORWARD, LDP1450_STEP_SPEED, NULL_TARGET_FRAME);
				ld->command = 0x3d;
			}
			break;

		case 0x3e:  CMDPRINTF(("ldp1450: %d Scan forward\n", ld->parameter));
			/* scan forward */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_SCANNING_FORWARD, LDP1450_SCAN_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x3f:	CMDPRINTF(("ldp1450: Stop\n"));
			/* pause at the current location */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x40:  CMDPRINTF((" ... Enter\n"));
			/* Enter -- execute command with parameter */
			switch (ld->command)
			{
				case 0x3d:	/* forward variable speed */
					if (ld->parameter != 0)
						ld->curfracspeed = PLAY_SPEED / ld->parameter;
					break;

				case 0x43:	/* search */
					set_state(ld, LASERDISC_SEARCHING_FRAME, LDP1450_SEARCH_SPEED, ld->parameter);
					break;

				case 0x4d:	/* reverse variable speed */
					if (ld->parameter != 0)
						ld->curfracspeed = -PLAY_SPEED / ld->parameter;
					break;

				default: CMDPRINTF(("Unknown command: %02X\n", ld->command));
					break;
			}
			break;

		case 0x41:  CMDPRINTF(("ldp1450: %d Clear entry\n", ld->parameter));
			/* Clear entry */
			break;

		case 0x42:  CMDPRINTF(("ldp1450: %d Menu\n", ld->parameter));
			/* Menu -- not implemented */
			break;

		case 0x43:  CMDPRINTF(("ldp1450: Search ... "));
			/* search to a particular frame number */
			if (laserdisc_ready(ld))
			{
				ld->command = data;

				/* Note that the disc stops as soon as the search command is issued */
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
			}
			break;

		case 0x44:  CMDPRINTF(("ldp1450: %d Repeat\n", ld->parameter));
			/* Repeat -- not implemented */
			break;

		case 0x46:  CMDPRINTF(("ldp1450: Ch1 On\n"));
			/* channel 1 audio on */
			ld->audio |= AUDIO_CH1_ENABLE;
			break;

		case 0x47:  CMDPRINTF(("ldp1450: Ch1 Off\n"));
			/* channel 1 audio off */
			ld->audio &= ~AUDIO_CH1_ENABLE;
			break;

		case 0x48:  CMDPRINTF(("ldp1450: Ch2 On\n"));
			/* channel 1 audio on */
			ld->audio |= AUDIO_CH2_ENABLE;
			break;

		case 0x49:  CMDPRINTF(("ldp1450: Ch2 Off\n"));
			/* channel 1 audio off */
			ld->audio &= ~AUDIO_CH2_ENABLE;
			break;

		case 0x4a:  CMDPRINTF(("ldp1450: %d Reverse Play\n", ld->parameter));
			/* begin playing at regular speed, or load the disc if it is parked */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_REVERSE, -PLAY_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x4b:  CMDPRINTF(("ldp1450: %d Fast reverse play\n", ld->parameter));
			/* play reverse fast speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_FAST_REVERSE, -LDP1450_FAST_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x4c:  CMDPRINTF(("ldp1450: %d Slow reverse play\n", ld->parameter));
			/* play reverse slow speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_PLAYING_SLOW_REVERSE, -LDP1450_SLOW_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x4d:  CMDPRINTF(("ldp1450: %d Variable reverse play\n", ld->parameter));
			/* play reverse variable speed */
			if (laserdisc_ready(ld))
			{
				set_state(ld, LASERDISC_STEPPING_REVERSE, -LDP1450_STEP_SPEED, NULL_TARGET_FRAME);
				ld->command = 0x4d;
			}
			break;

		case 0x4e:  CMDPRINTF(("ldp1450: %d Scan reverse\n", ld->parameter));
			/* play forward variable speed */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_SCANNING_REVERSE, -LDP1450_SCAN_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x4f:	CMDPRINTF(("ldp1450: %d Still\n", ld->parameter));
			/* still picture */
			if (laserdisc_ready(ld))
				set_state(ld, LASERDISC_STOPPED, STOP_SPEED, NULL_TARGET_FRAME);
			break;

		case 0x50:  CMDPRINTF(("ldp1450: Index On\n"));
			/* index on -- not implemented */
			break;

		case 0x51:  CMDPRINTF(("ldp1450: Index Off\n"));
			/* index off -- not implemented */
			break;

		case 0x55:  CMDPRINTF(("ldp1450: %d Set to frame number mode\n", ld->parameter));
			/* set to frame number mode -- not implemented */
			break;

		case 0x56:  CMDPRINTF(("ldp1450: %d Clear all\n", ld->parameter));
			/* clear all */
			ldp1450_soft_reset(ld);
			break;

		case 0x5a:  CMDPRINTF(("ldp1450: %d Memory\n", ld->parameter));
			/* memorize current position -- not implemented */
			break;

		case 0x5b:  CMDPRINTF(("ldp1450: %d Memory Search\n", ld->parameter));
			/* locate memorized position -- not implemented */
			break;

		case 0x60:  CMDPRINTF(("ldp1450: %d Address Inquire\n", ld->parameter));
			/* inquire for current address */
			ldp1450->readtotal = 0;
			ldp1450->readbuf[ldp1450->readtotal++] = '0' + (ld->last_frame / 10000) % 10;
			ldp1450->readbuf[ldp1450->readtotal++] = '0' + (ld->last_frame / 1000) % 10;
			ldp1450->readbuf[ldp1450->readtotal++] = '0' + (ld->last_frame / 100) % 10;
			ldp1450->readbuf[ldp1450->readtotal++] = '0' + (ld->last_frame / 10) % 10;
			ldp1450->readbuf[ldp1450->readtotal++] = '0' + (ld->last_frame / 1) % 10;
			break;

		case 0x61:  CMDPRINTF(("ldp1450: %d Continue\n", ld->parameter));
			/* resume the mode prior to still -- not implemented */
			break;

		case 0x62:  CMDPRINTF(("ldp1450: Motor On\n"));
			/* motor on -- not implemented */
			break;

		case 0x63:  CMDPRINTF(("ldp1450: Motor Off\n"));
			/* motor off -- not implemented */
			break;

		case 0x67:  CMDPRINTF(("ldp1450: %d Status Inquire\n", ld->parameter));
			/* inquire status of player */
			ldp1450_compute_status(ld);
			break;

		case 0x69:  CMDPRINTF(("ldp1450: %d Chapter Mode\n", ld->parameter));
			/* set to chapter number mode -- not implemented */
			break;

		case 0x6e:  CMDPRINTF(("ldp1450: CX On\n"));
			/* CX noise reduction on -- not implemented */
			break;

		case 0x6f:  CMDPRINTF(("ldp1450: CX Off\n"));
			/* CX noise reduction off -- not implemented */
			break;

		case 0x71:  CMDPRINTF(("ldp1450: %d Non-CF Play\n", ld->parameter));
			/* disengage color framing -- not implemented */
			break;

		case 0x72:  CMDPRINTF(("ldp1450: %d ROM Version\n", ld->parameter));
			/* inquire ROM version -- not implemented */
			break;

		case 0x73:  CMDPRINTF(("ldp1450: %d Mark Set\n", ld->parameter));
			/* set mark position -- not implemented */
			break;

		case 0x74:  CMDPRINTF(("ldp1450: Eject Enable On\n"));
			/* activate eject function -- not implemented */
			break;

		case 0x75:  CMDPRINTF(("ldp1450: Eject Disable Off\n"));
			/* deactivate eject function -- not implemented */
			break;

		case 0x76:  CMDPRINTF(("ldp1450: %d Chapter Inquire\n", ld->parameter));
			/* inquire current chapter -- not implemented */
			break;

		case 0x79:  CMDPRINTF(("ldp1450: %d User Code Inquire\n", ld->parameter));
			/* inquire user's code -- not implemented */
			break;

		case 0x80:  CMDPRINTF(("ldp1450: %d User Index Control\n", ld->parameter));
			/* set user defined index -- not implemented */
			break;

		case 0x81:  CMDPRINTF(("ldp1450: Activate user defined index\n"));
			/* activate eject function -- not implemented */
			break;

		case 0x82:  CMDPRINTF(("ldp1450: Deactivate user defined index\n"));
			/* deactivate eject function -- not implemented */
			break;

		default:	CMDPRINTF(("ldp1450: %d Unknown command %02X\n", ld->parameter, data));
			/* unknown command -- respond with a NAK */
			ldp1450->readtotal = 0;
			ldp1450->readbuf[ldp1450->readtotal++] = 0x0b;
			break;
	}

	/* reset the parameter after executing a command */
	ld->parameter = -1;
}


/*-------------------------------------------------
    ldp1450_data_avail_r - return ASSERT_LINE if
    serial data is available
-------------------------------------------------*/

static UINT8 ldp1450_data_avail_r(laserdisc_state *ld)
{
	ldp1450_info *ldp1450 = &ld->u.ldp1450;
	return (ldp1450->readpos < ldp1450->readtotal) ? ASSERT_LINE : CLEAR_LINE;
}


/*-------------------------------------------------
    ldp1450_data_r - return data from the player
-------------------------------------------------*/

static UINT8 ldp1450_data_r(laserdisc_state *ld)
{
	ldp1450_info *ldp1450 = &ld->u.ldp1450;

	if (ldp1450->readpos < ldp1450->readtotal)
		return ldp1450->readbuf[ldp1450->readpos++];
	return 0xff;
}


/*-------------------------------------------------
    ldp1450_state_changed - Sony LDP-1450-specific
    state changed callback
-------------------------------------------------*/

static void ldp1450_state_changed(laserdisc_state *ld, UINT8 oldstate)
{
	ldp1450_info *ldp1450 = &ld->u.ldp1450;

	/* look for searching -> search finished state */
	if (ld->state == LASERDISC_SEARCH_FINISHED && oldstate == LASERDISC_SEARCHING_FRAME)
	{
		ldp1450->readpos = ldp1450->readtotal = 0;
		ldp1450->readbuf[ldp1450->readtotal++] = 0x01;
	}
}



/***************************************************************************
    PHILPS 22VP932 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    vp932_init - Philips 22VP932-specific
    initialization
-------------------------------------------------*/

static void vp932_init(laserdisc_state *ld)
{
	/* set up the write callbacks */
	ld->writedata = vp932_data_w;

	/* set up the read callbacks */
	ld->readdata = vp932_data_r;
	ld->readline[LASERDISC_LINE_DATA_AVAIL] = vp932_data_avail_r;

	/* use a state changed callback */
	ld->statechanged = vp932_state_changed;

	/* do a soft reset */
	vp932_soft_reset(ld);
}


/*-------------------------------------------------
    vp932_soft_reset - Philips 22VP932-specific
    soft reset
-------------------------------------------------*/

static void vp932_soft_reset(laserdisc_state *ld)
{
	vp932_info *vp932 = &ld->u.vp932;

	ld->audio = AUDIO_CH1_ENABLE  | AUDIO_CH2_ENABLE;
	ld->display = 0;

	/* reset the pending count */
	vp932->incount = 0;
	vp932->outcount = 0;
}


/*-------------------------------------------------
    vp932_data_w - write callback when data is
    written
-------------------------------------------------*/

static void vp932_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data)
{
	vp932_info *vp932 = &ld->u.vp932;

	/* ignore 0's */
	if (data == 0x00)
		return;

	/* if this isn't a CR, just accumulate it */
	if (data != 0x0d)
	{
		if (vp932->incount < ARRAY_LENGTH(vp932->inbuffer))
			vp932->inbuffer[vp932->incount++] = data;
	}

	/* otherwise, execute the command */
	else
	{
		vp932->inbuffer[vp932->incount] = 0;
		CMDPRINTF(("22vp932: %s\n", vp932->inbuffer));

		switch (vp932->inbuffer[vp932->incount - 1])
		{
			case 'R':	/* seek */
				if (laserdisc_ready(ld))
				{
					INT32 targetframe = 0, i;

					for (i = 0; i < vp932->incount - 1; i++)
						if (vp932->inbuffer[i] >= '0' && vp932->inbuffer[i] <= '9')
							targetframe = (targetframe * 10) + (vp932->inbuffer[i] - '0');
					set_state(ld, LASERDISC_SEARCHING_FRAME, VP932_SEARCH_SPEED, targetframe);

					vp932->outcount = 0;
					vp932->outbuffer[vp932->outcount++] = 'A';
					vp932->outbuffer[vp932->outcount++] = '0';
					vp932->outbuffer[vp932->outcount++] = 0x0d;
				}
				break;
		}

		/* reset the command buffer */
		vp932->incount = 0;
	}
}


/*-------------------------------------------------
    vp932_data_r - read callback when data is
    returned
-------------------------------------------------*/

static UINT8 vp932_data_r(laserdisc_state *ld)
{
	vp932_info *vp932 = &ld->u.vp932;
	UINT8 result = 0;

	/* grab data if we can */
	if (vp932->outcount > 0)
	{
		result = vp932->outbuffer[0];
		if (--vp932->outcount > 0)
			memmove(&vp932->outbuffer[0], &vp932->outbuffer[1], vp932->outcount);
	}

	return result;
}


/*-------------------------------------------------
    vp932_data_avail_r - return ASSERT_LINE if
    serial data is available
-------------------------------------------------*/

static UINT8 vp932_data_avail_r(laserdisc_state *ld)
{
	vp932_info *vp932 = &ld->u.vp932;
	return (vp932->outcount > 0) ? ASSERT_LINE : CLEAR_LINE;
}


/*-------------------------------------------------
    vp932_state_changed - Sony LDP-1450-specific
    state changed callback
-------------------------------------------------*/

static void vp932_state_changed(laserdisc_state *ld, UINT8 oldstate)
{
	vp932_info *vp932 = &ld->u.vp932;

	/* look for searching -> search finished state */
	if (ld->state == LASERDISC_SEARCH_FINISHED && oldstate == LASERDISC_SEARCHING_FRAME)
	{
		vp932->outcount = 0;
		vp932->outbuffer[vp932->outcount++] = 'A';
		vp932->outbuffer[vp932->outcount++] = '0';
		vp932->outbuffer[vp932->outcount++] = 0x0d;
	}
}
