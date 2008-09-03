/*************************************************************************

    ldpr8210.c

    Pioneer PR-8210 laserdisc emulation.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*************************************************************************/

#include "ldcore.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define PRINTF_COMMANDS				1
#define CMDPRINTF(x)				do { if (PRINTF_COMMANDS) mame_printf_debug x; } while (0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* Player-specific states */
enum
{
	LDSTATE_STEPPING_BY_PARAMETER = LDSTATE_OTHER
};

/* Pioneer PR-8210 specific information */
#define PR8210_SCAN_SPEED			(2000 / 30)			/* 2000 frames/second */
#define PR8210_SCAN_DURATION		(10)				/* scan for 10 vsyncs each time */
#define PR8210_SLOW_SPEED			(5)					/* 1/5x normal speed */
#define PR8210_FAST_SPEED			(3)					/* 3x normal speed */
#define PR8210_JUMP_REV_SPEED		(15)				/* 15x normal speed */
#define PR8210_SEEK_FAST_SPEED		(1000)				/* 1000x normal speed */

/* Us vs. Them requires at least this much "non-video" time in order to work */
#define PR8210_MINIMUM_SEEK_TIME	ATTOTIME_IN_MSEC(150)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* player-specific data */
struct _ldplayer_data
{
	/* general modes and states */
	UINT8				audio1disable;			/* explicit disable for audio 1 */
	UINT8				audio2disable;			/* explicit disable for audio 2 */
	UINT8				framedisplay;			/* frame display */
	INT32				parameter;				/* parameter for numbers */

	/* serial/command interface processing */
	UINT8				lastcommand;			/* last command byte received */
	UINT8				seekstate;				/* state of the seek command */
	UINT16				accumulator;			/* bit accumulator */
	attotime			lastbittime;			/* time of last bit received */
	attotime			firstbittime;			/* time of first bit in command */

	/* Simutrek-specific data */
	UINT8				cmdcnt;					/* counter for multi-byte command */
	UINT8				cmdbytes[3];			/* storage for multi-byte command */
	void				(*cmd_ack_callback)(void); /* callback to clear game command write flag */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void pr8210_init(laserdisc_state *ld);
static void pr8210_soft_reset(laserdisc_state *ld);
static void pr8210_update_squelch(laserdisc_state *ld);
static int pr8210_switch_state(laserdisc_state *ld, UINT8 newstate, INT32 stateparam);
static INT32 pr8210_update(laserdisc_state *ld, const vbi_metadata *vbi, int fieldnum, attotime curtime);
static void pr8210_command(laserdisc_state *ld);
static void pr8210_control_w(laserdisc_state *ld, UINT8 prev, UINT8 data);

static void simutrek_init(laserdisc_state *ld);
static UINT8 simutrek_status_r(laserdisc_state *ld);
static void simutrek_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data);



/***************************************************************************
    INTERFACES
***************************************************************************/

const ldplayer_interface pr8210_interface =
{
	LASERDISC_TYPE_PIONEER_PR8210,				/* type of the player */
	sizeof(ldplayer_data),						/* size of the state */
	"Pioneer PR-8210",							/* name of the player */
	pr8210_init,								/* initialization callback */
	pr8210_update,								/* update callback */
	NULL,										/* parallel data write */
	{											/* single line write: */
		NULL,									/*    LASERDISC_LINE_ENTER */
		pr8210_control_w						/*    LASERDISC_LINE_CONTROL */
	},
	NULL,										/* parallel data read */
	{											/* single line read: */
		NULL,									/*    LASERDISC_LINE_READY */
		NULL,									/*    LASERDISC_LINE_STATUS */
		NULL,									/*    LASERDISC_LINE_COMMAND */
		NULL,									/*    LASERDISC_LINE_DATA_AVAIL */
	}
};


const ldplayer_interface simutrek_interface =
{
	LASERDISC_TYPE_SIMUTREK_SPECIAL,			/* type of the player */
	sizeof(ldplayer_data),						/* size of the state */
	"Simutrek Modified PR-8210",				/* name of the player */
	simutrek_init,								/* initialization callback */
	pr8210_update,								/* update callback */
	simutrek_data_w,							/* parallel data write */
	{											/* single line write: */
		NULL,									/*    LASERDISC_LINE_ENTER */
		NULL									/*    LASERDISC_LINE_CONTROL */
	},
	NULL,										/* parallel data read */
	{											/* single line read: */
		NULL,									/*    LASERDISC_LINE_READY */
		simutrek_status_r,						/*    LASERDISC_LINE_STATUS */
		NULL,									/*    LASERDISC_LINE_COMMAND */
		NULL,									/*    LASERDISC_LINE_DATA_AVAIL */
	}
};



/***************************************************************************
    PIONEER PR-8210 IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    requires_state_save - returns TRUE if the
    given state will return to the previous
    state when done
-------------------------------------------------*/

INLINE int requires_state_save(UINT8 state)
{
	return (state == LDSTATE_SCANNING);
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
	/* do a soft reset */
	pr8210_soft_reset(ld);
}


/*-------------------------------------------------
    pr8210_soft_reset - Pioneer PR-8210-specific
    soft reset
-------------------------------------------------*/

static void pr8210_soft_reset(laserdisc_state *ld)
{
	ldplayer_data *player = ld->player;
	attotime curtime = timer_get_time();

	ld->state.state = LDSTATE_NONE;
	ld->state.substate = 0;
	ld->state.param = 0;
	ld->state.endtime = curtime;

	player->audio1disable = 0;
	player->audio2disable = 0;
	player->parameter = 0;

	player->lastcommand = 0;
	player->seekstate = 0;
	player->accumulator = 0;
	player->firstbittime = curtime;
	player->lastbittime = curtime;

	pr8210_switch_state(ld, LDSTATE_PARKED, 0);
}


/*-------------------------------------------------
    pr8210_update_squelch - update the squelch
    settings based on the current state
-------------------------------------------------*/

static void pr8210_update_squelch(laserdisc_state *ld)
{
	ldplayer_data *player = ld->player;

	switch (ld->state.state)
	{
		/* video on, audio on */
		case LDSTATE_PLAYING:
			ldcore_set_audio_squelch(ld, player->audio1disable, player->audio2disable);
			ldcore_set_video_squelch(ld, FALSE);
			break;

		/* video on, audio off */
		case LDSTATE_PAUSING:
		case LDSTATE_PAUSED:
		case LDSTATE_PLAYING_SLOW_REVERSE:
		case LDSTATE_PLAYING_SLOW_FORWARD:
		case LDSTATE_PLAYING_FAST_REVERSE:
		case LDSTATE_PLAYING_FAST_FORWARD:
		case LDSTATE_STEPPING_REVERSE:
		case LDSTATE_STEPPING_FORWARD:
		case LDSTATE_SCANNING:
			ldcore_set_audio_squelch(ld, TRUE, TRUE);
			ldcore_set_video_squelch(ld, FALSE);
			break;

		/* video off, audio off */
		case LDSTATE_NONE:
		case LDSTATE_EJECTED:
		case LDSTATE_EJECTING:
		case LDSTATE_PARKED:
		case LDSTATE_LOADING:
		case LDSTATE_SPINUP:
		case LDSTATE_SEEKING:
			ldcore_set_audio_squelch(ld, TRUE, TRUE);
			ldcore_set_video_squelch(ld, TRUE);
			break;

		/* Simutrek: stepping by parameter with explicit audio squelch controls */
		case LDSTATE_STEPPING_BY_PARAMETER:
			ldcore_set_video_squelch(ld, FALSE);
			break;
	}
}


/*-------------------------------------------------
    pr8210_switch_state - attempt to switch states
    if appropriate, and ensure the squelch values
    are correct
-------------------------------------------------*/

static int pr8210_switch_state(laserdisc_state *ld, UINT8 newstate, INT32 parameter)
{
	attotime curtime = timer_get_time();

	/* if this is the same state, ignore */
	if (ld->state.state == newstate)
		return TRUE;

	/* if we're in a timed state, we ignore changes until the time elapses */
	if (attotime_compare(curtime, ld->state.endtime) < 0)
		return FALSE;

	/* if moving to a state that requires it, save the old state */
	if (requires_state_save(newstate) && !requires_state_save(ld->state.state))
		ld->savestate = ld->state;

	/* set up appropriate squelch and timing */
	ld->state.state = newstate;
	ld->state.substate = 0;
	ld->state.param = parameter;
	ld->state.endtime = curtime;
	pr8210_update_squelch(ld);

	/* if we're switching to a timed state, set the appropriate timer */
	switch (ld->state.state)
	{
		case LDSTATE_EJECTING:
			ld->state.endtime = attotime_add(curtime, GENERIC_EJECT_TIME);
			break;

		case LDSTATE_LOADING:
			ld->state.endtime = attotime_add(curtime, GENERIC_LOAD_TIME);
			break;

		case LDSTATE_SPINUP:
			ld->state.endtime = attotime_add(curtime, GENERIC_SPINUP_TIME);
			break;

		case LDSTATE_SEEKING:
			ld->state.endtime = attotime_add(curtime, PR8210_MINIMUM_SEEK_TIME);
			break;
	}
	return TRUE;
}


/*-------------------------------------------------
    pr8210_update - Pioneer PR-8210-specific
    update callback
-------------------------------------------------*/

static INT32 pr8210_update(laserdisc_state *ld, const vbi_metadata *vbi, int fieldnum, attotime curtime)
{
	ldplayer_state newstate;
	INT32 advanceby = 0;
	int frame;

	/* handle things based on the state */
	switch (ld->state.state)
	{
		case LDSTATE_EJECTING:
		case LDSTATE_EJECTED:
		case LDSTATE_PARKED:
		case LDSTATE_LOADING:
		case LDSTATE_SPINUP:
		case LDSTATE_PAUSING:
		case LDSTATE_PAUSED:
		case LDSTATE_PLAYING:
		case LDSTATE_PLAYING_SLOW_REVERSE:
		case LDSTATE_PLAYING_SLOW_FORWARD:
		case LDSTATE_PLAYING_FAST_REVERSE:
		case LDSTATE_PLAYING_FAST_FORWARD:
		case LDSTATE_STEPPING_REVERSE:
		case LDSTATE_STEPPING_FORWARD:
		case LDSTATE_SCANNING:
			/* generic behaviors are appropriate for all of these commands */
			advanceby = ldcore_generic_update(ld, vbi, fieldnum, curtime, &newstate);
			pr8210_switch_state(ld, newstate.state, newstate.param);
			break;

		case LDSTATE_SEEKING:
			/* if we're in the final state, look for a matching frame and pause there */
			frame = frame_from_metadata(vbi);
			if (ld->state.substate == 3 && is_start_of_frame(vbi) && frame == ld->state.param)
				pr8210_switch_state(ld, LDSTATE_PAUSED, fieldnum);

			/* otherwise, if we got frame data from the VBI, update our seeking logic */
			else if (ld->state.substate < 3 && frame != FRAME_NOT_PRESENT)
			{
				static const INT32 seekspeed[] = { -PR8210_SEEK_FAST_SPEED, PR8210_SEEK_FAST_SPEED, -PR8210_JUMP_REV_SPEED };
				INT32 curseekspeed = seekspeed[ld->state.substate];
				INT32 delta = (ld->state.param - 1) - frame;

				if ((curseekspeed < 0 && delta <= 0) || (curseekspeed > 0 && delta >= 0))
					advanceby = curseekspeed;
				else
					ld->state.substate++;
			}

			/* otherwise, keep advancing until we know what's up */
			else if (fieldnum == 1)
				advanceby = 1;
			break;

		case LDSTATE_STEPPING_BY_PARAMETER:
			/* advance after the second field of each frame */
			if (fieldnum == 1)
			{
				/* note that we switch directly to PAUSED as we are not looking for frames */
				advanceby = ld->state.param;
				pr8210_switch_state(ld, LDSTATE_PAUSED, 0);
			}
			break;
	}

	return advanceby;
}


/*-------------------------------------------------
    pr8210_command - Pioneer PR-8210-specific
    command processing
-------------------------------------------------*/

static void pr8210_command(laserdisc_state *ld)
{
	ldplayer_data *player = ld->player;
	UINT8 cmd = player->lastcommand;

	switch (cmd)
	{
		case 0x00:	CMDPRINTF(("pr8210: EOC\n"));
			/* EOC marker - can be safely ignored */
			break;

		case 0x01:	CMDPRINTF(("pr8210: 0\n"));
			player->parameter = (player->parameter == -1) ? 0 : (player->parameter * 10 + 0);
			break;

		case 0x02:	CMDPRINTF(("pr8210: Slow reverse\n"));
			pr8210_switch_state(ld, LDSTATE_PLAYING_SLOW_REVERSE, PR8210_SLOW_SPEED);
			break;

		case 0x03:	CMDPRINTF(("pr8210: 8\n"));
			player->parameter = (player->parameter == -1) ? 8 : (player->parameter * 10 + 8);
			break;

		case 0x04:	CMDPRINTF(("pr8210: Step forward\n"));
			pr8210_switch_state(ld, LDSTATE_STEPPING_FORWARD, 0);
			break;

		case 0x05:	CMDPRINTF(("pr8210: 4\n"));
			player->parameter = (player->parameter == -1) ? 4 : (player->parameter * 10 + 4);
			break;

		case 0x06 :	CMDPRINTF(("pr8210: Chapter\n"));
			/* chapter -- not implemented */
			break;

		case 0x08:	CMDPRINTF(("pr8210: Scan forward\n"));
			if (ld->state.state != LDSTATE_SCANNING)
				pr8210_switch_state(ld, LDSTATE_SCANNING, SCANNING_PARAM(PR8210_SCAN_SPEED, PR8210_SCAN_DURATION));
			else
				ld->state.substate = 0;
			break;

		case 0x09:	CMDPRINTF(("pr8210: 2\n"));
			player->parameter = (player->parameter == -1) ? 2 : (player->parameter * 10 + 2);
			break;

		case 0x0a:	CMDPRINTF(("pr8210: Pause\n"));
			pr8210_switch_state(ld, LDSTATE_PAUSING, 0);
			break;

		case 0x0b :	CMDPRINTF(("pr8210: Frame\n"));
			/* frame -- not implemented */
			break;

		case 0x0c:	CMDPRINTF(("pr8210: Fast reverse\n"));
			pr8210_switch_state(ld, LDSTATE_PLAYING_FAST_REVERSE, PR8210_FAST_SPEED);
			break;

		case 0x0d:	CMDPRINTF(("pr8210: 6\n"));
			player->parameter = (player->parameter == -1) ? 6 : (player->parameter * 10 + 6);
			break;

		case 0x0e:	CMDPRINTF(("pr8210: Ch1 toggle\n"));
			player->audio1disable = !player->audio1disable;
			pr8210_update_squelch(ld);
			break;

		case 0x10:	CMDPRINTF(("pr8210: Fast forward\n"));
			pr8210_switch_state(ld, LDSTATE_PLAYING_FAST_FORWARD, PR8210_FAST_SPEED);
			break;

		case 0x11:	CMDPRINTF(("pr8210: 1\n"));
			player->parameter = (player->parameter == -1) ? 1 : (player->parameter * 10 + 1);
			break;

		case 0x12:	CMDPRINTF(("pr8210: Step reverse\n"));
			pr8210_switch_state(ld, LDSTATE_STEPPING_REVERSE, 0);
			break;

		case 0x13:	CMDPRINTF(("pr8210: 9\n"));
			player->parameter = (player->parameter == -1) ? 9 : (player->parameter * 10 + 9);
			break;

		case 0x14:  CMDPRINTF(("pr8210: Play\n"));
			if (ld->state.state == LDSTATE_EJECTED)
				pr8210_switch_state(ld, LDSTATE_LOADING, 0);
			else if (ld->state.state == LDSTATE_PARKED)
				pr8210_switch_state(ld, LDSTATE_SPINUP, 0);
			else
				pr8210_switch_state(ld, LDSTATE_PLAYING, 0);
			break;

		case 0x15:	CMDPRINTF(("pr8210: 5\n"));
			player->parameter = (player->parameter == -1) ? 5 : (player->parameter * 10 + 5);
			break;

		case 0x16:	CMDPRINTF(("pr8210: Ch2 toggle\n"));
			player->audio2disable = !player->audio2disable;
			pr8210_update_squelch(ld);
			break;

		case 0x18:	CMDPRINTF(("pr8210: Slow forward\n"));
			pr8210_switch_state(ld, LDSTATE_PLAYING_SLOW_FORWARD, PR8210_SLOW_SPEED);
			break;

		case 0x19:	CMDPRINTF(("pr8210: 3\n"));
			player->parameter = (player->parameter == -1) ? 3 : (player->parameter * 10 + 3);
			break;

		case 0x1a:	CMDPRINTF(("pr8210: Seek\n"));
			if (player->seekstate)
			{
				/* we're ready to seek */
				CMDPRINTF(("pr8210: Seeking to frame %d\n", player->parameter));
				pr8210_switch_state(ld, LDSTATE_SEEKING, player->parameter);
			}
			else
			{
				/* waiting for digits indicating position */
				player->parameter = 0;
			}
			player->seekstate ^= 1;
			break;

		case 0x1c:	CMDPRINTF(("pr8210: Scan reverse\n"));
			if (ld->state.state != LDSTATE_SCANNING)
				pr8210_switch_state(ld, LDSTATE_SCANNING, SCANNING_PARAM(-PR8210_SCAN_SPEED, PR8210_SCAN_DURATION));
			else
				ld->state.substate = 0;
			break;

		case 0x1d:	CMDPRINTF(("pr8210: 7\n"));
			player->parameter = (player->parameter == -1) ? 7 : (player->parameter * 10 + 7);
			break;

		case 0x1e:	CMDPRINTF(("pr8210: Reject\n"));
			pr8210_switch_state(ld, LDSTATE_EJECTING, 0);
			break;

		default:	CMDPRINTF(("pr8210: Unknown command %02X\n", cmd));
			break;
	}
}


/*-------------------------------------------------
    pr8210_control_w - write callback when the
    CONTROL line is toggled
-------------------------------------------------*/

static void pr8210_control_w(laserdisc_state *ld, UINT8 prev, UINT8 data)
{
	ldplayer_data *player = ld->player;

	if (data == ASSERT_LINE)
	{
		attotime curtime = timer_get_time();
		attotime delta;
		int longpulse;

		/* if we timed out, reset the accumulator */
		delta = attotime_sub(curtime, player->firstbittime);
		if (delta.attoseconds > ATTOTIME_IN_USEC(25320).attoseconds)
		{
			player->firstbittime = curtime;
			player->accumulator = 0x5555;
//          printf("Reset accumulator\n");
		}

		/* get the time difference from the last assert */
		/* and update our internal command time */
		delta = attotime_sub(curtime, player->lastbittime);
		player->lastbittime = curtime;

		/* 0 bit delta is 1.05 msec, 1 bit delta is 2.11 msec */
		longpulse = (delta.attoseconds < ATTOTIME_IN_USEC(1500).attoseconds) ? 0 : 1;
		player->accumulator = (player->accumulator << 1) | longpulse;

#if 0
		{
			int usecdiff = (int)(delta.attoseconds / ATTOSECONDS_IN_USEC(1));
			printf("bitdelta = %5d (%d) - accum = %04X\n", usecdiff, longpulse, player->accumulator);
		}
#endif

		/* if we have a complete command, signal it */
		/* a complete command is 0,0,1 followed by 5 bits, followed by 0,0 */
		if ((player->accumulator & 0x383) == 0x80)
		{
			UINT8 newcommand = (player->accumulator >> 2) & 0x1f;

//printf("New command = %02X (last=%02X)\n", newcommand, player->lastcommand);

			/* if we got a double command, act on it */
			if (newcommand == player->lastcommand)
			{
				pr8210_command(ld);
				player->lastcommand = 0;
			}
			else
				player->lastcommand = newcommand;
		}
	}
}



/***************************************************************************
    SIMUTREK MODIFIED PR-8210 PLAYER IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------

    Command Set:

    FX XX XX  : Seek to frame XXXXX
    01-19     : Skip forward 1-19 frames
    99-81     : Skip back 1-19 frames
    5a        : Toggle frame display

-------------------------------------------------*/

/*-------------------------------------------------
    simutrek_init - Simutrek-specific
    initialization
-------------------------------------------------*/

static void simutrek_init(laserdisc_state *ld)
{
	/* do a soft reset */
	pr8210_soft_reset(ld);
}


/*-------------------------------------------------
    simutrek_status_r - Simutrek-specific
    command processing
-------------------------------------------------*/

static UINT8 simutrek_status_r(laserdisc_state *ld)
{
	return (ld->state.state != LDSTATE_SEEKING) ? ASSERT_LINE : CLEAR_LINE;
}


/*-------------------------------------------------
    simutrek_data_w - Simutrek-specific
    data processing
-------------------------------------------------*/

static void simutrek_data_w(laserdisc_state *ld, UINT8 prev, UINT8 data)
{
	ldplayer_data *player = ld->player;

	/* Acknowledge every command byte */
	if (player->cmd_ack_callback != NULL)
		(*player->cmd_ack_callback)();

	/* Is this byte part of a multi-byte seek command? */
	if (player->cmdcnt > 0)
	{
		CMDPRINTF(("Simutrek: Seek to frame byte %d of 3\n", player->cmdcnt + 1));
		player->cmdbytes[player->cmdcnt++] = data;

		if (player->cmdcnt == 3)
		{
			player->parameter = ((player->cmdbytes[0] & 0xf) * 10000) +
								((player->cmdbytes[1] >> 4)  * 1000) +
								((player->cmdbytes[1] & 0xf) * 100) +
								((player->cmdbytes[2] >> 4)  * 10) +
								(player->cmdbytes[2] & 0xf);

			CMDPRINTF(("Simutrek: Seek to frame %d\n", player->parameter));

			pr8210_switch_state(ld, LDSTATE_SEEKING, player->parameter);
			player->cmdcnt = 0;
		}
	}
	else if (data == 0)
	{
		CMDPRINTF(("Simutrek: 0 ?\n"));
	}
	else if ((data & 0xf0) == 0xf0)
	{
		CMDPRINTF(("Simutrek: Seek to frame byte 1 of 3\n"));
		player->cmdbytes[player->cmdcnt++] = data;
	}
	else if (data >= 1 && data <= 0x19)
	{
		int count = ((data >> 4) * 10) + (data & 0xf);
		CMDPRINTF(("Simutrek: Step forwards by %d frame(s)\n", count));
		pr8210_switch_state(ld, LDSTATE_STEPPING_BY_PARAMETER, count);
	}
	else if (data >= 0x81 && data <= 0x99)
	{
		int count = (((data >> 4) * 10) + (data & 0xf)) - 100;
		CMDPRINTF(("Simutrek: Step backwards by %d frame(s)\n", count));
		pr8210_switch_state(ld, LDSTATE_STEPPING_BY_PARAMETER, count);
	}
	else if (data == 0x5a)
	{
		CMDPRINTF(("Simutrek: Frame window toggle\n"));
		player->framedisplay ^= 1;
	}
	else
	{
		CMDPRINTF(("Simutrek: Unknown command (%.2x)\n", data));
	}
}


/*-------------------------------------------------
    simutrek_set_audio_squelch - Simutrek-specific
    command to enable/disable audio squelch
-------------------------------------------------*/

void simutrek_set_audio_squelch(const device_config *device, int state)
{
	laserdisc_state *ld = ldcore_get_safe_token(device);
	int squelch = (state != ASSERT_LINE);
	ldcore_set_audio_squelch(ld, squelch, squelch);
}


/*-------------------------------------------------
    simutrek_set_audio_squelch - Simutrek-specific
    command to set callback function for
    player/interface command acknowledge
-------------------------------------------------*/

void simutrek_set_cmd_ack_callback(const device_config *device, void (*callback)(void))
{
	laserdisc_state *ld = ldcore_get_safe_token(device);
	ldplayer_data *player = ld->player;

	player->cmd_ack_callback = callback;
}
