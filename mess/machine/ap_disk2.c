/***************************************************************************

  ap_disk2.c

  Machine file to handle emulation of the Apple Disk II controller.

  TODO:
    Allow # of drives and slot to be selectable.
	Redo the code to make it understandable.
	Allow disks to be writeable.
	Support more disk formats.
	Add a description of Apple Disk II Hardware?
	Make it faster?
	Add sound?
	Add proper microsecond timing?


	Note - only one driver light can be on at once; regardless of the motor
	state; if we support drive lights we must take this into consideration
***************************************************************************/

#include "driver.h"
#include "image.h"
#include "devices/flopdrv.h"
#include "includes/apple2.h"
#include "formats/ap2_dsk.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

#define PROFILER_SLOT6	PROFILER_USER1

#define Q6_MASK				0x10
#define Q7_MASK				0x20
#define TWEEN_TRACKS		0x40

#define TRSTATE_LOADED		0x01
#define TRSTATE_DIRTY		0x02

static int disk6byte;		/* byte queued for writing? */
static int read_state;		/* 1 = read, 0 = write */
static int a2_drives_num;

struct apple2_drive
{
	UINT8 state;			/* bits 0-3 are the phase; bits 4-5 is q6-7 */
	UINT8 transient_state;
	int position;
	UINT8 track_data[APPLE2_NIBBLE_SIZE * APPLE2_SECTOR_COUNT];
};

static struct apple2_drive *apple2_drives;

/***************************************************************************
  apple2_slot6_init
***************************************************************************/

void apple2_slot6_init(void)
{
	apple2_drives = auto_malloc(sizeof(struct apple2_drive) * 2);
	if (!apple2_drives)
		return;
	memset(apple2_drives, 0, sizeof(struct apple2_drive) * 2);
}



/**************************************************************************/

static void load_current_track(mess_image *image, struct apple2_drive *disk)
{
	int len = sizeof(disk->track_data);
	floppy_drive_read_track_data_info_buffer(image, 0, disk->track_data, &len);
	disk->transient_state |= TRSTATE_LOADED;
}



static void save_current_track(mess_image *image, struct apple2_drive *disk)
{
	int len = sizeof(disk->track_data);

	if (disk->transient_state & TRSTATE_DIRTY)
	{
		floppy_drive_write_track_data_info_buffer(image, 0, disk->track_data, &len);
		disk->transient_state &= ~TRSTATE_DIRTY;
	}
}



/* reads/writes a byte; write_value is -1 for read only */
static UINT8 process_byte(mess_image *img, struct apple2_drive *disk, int write_value)
{
	UINT8 read_value;

	/* no image initialized for that drive ? */
	if (!image_exists(img))
		return 0xFF;

	/* load track if need be */
	if ((disk->transient_state & TRSTATE_LOADED) == 0)
		load_current_track(img, disk);

	/* perform the read */
	read_value = disk->track_data[disk->position];

	/* perform the write, if applicable */
	if (write_value >= 0)
	{
		disk->track_data[disk->position] = write_value;
		disk->transient_state |= TRSTATE_DIRTY;
	}

	disk->position++;
	disk->position %= (sizeof(disk->track_data) / sizeof(disk->track_data[0]));

	/* when writing; save the current track after every full sector write */
	if ((write_value >= 0) && ((disk->position % APPLE2_NIBBLE_SIZE) == 0))
		save_current_track(img, disk);

	return read_value;
}

static void seek_disk(mess_image *img, struct apple2_drive *disk, signed int step)
{
	int track;
	int pseudo_track;

	save_current_track(img, disk);
	
	track = floppy_drive_get_current_track(img);
	pseudo_track = (track * 2) + (disk->state & TWEEN_TRACKS ? 1 : 0);
	
	pseudo_track += step;
	if (pseudo_track < 0)
		pseudo_track = 0;
	else if (pseudo_track/2 >= APPLE2_TRACK_COUNT)
		pseudo_track = APPLE2_TRACK_COUNT*2-1;

	if (pseudo_track/2 != track)
	{
		floppy_drive_seek(img, pseudo_track/2 - floppy_drive_get_current_track(img));
		disk->transient_state &= ~TRSTATE_LOADED;
	}

	if (pseudo_track & 1)
		disk->state |= TWEEN_TRACKS;
	else
		disk->state &= ~TWEEN_TRACKS;
}



/***************************************************************************
  apple2_c0xx_slot6_r
***************************************************************************/

READ_HANDLER ( apple2_c0xx_slot6_r )
{
	struct apple2_drive *cur_disk;
	mess_image *cur_image;
	unsigned int phase;
	data8_t result = 0x00;

	profiler_mark(PROFILER_SLOT6);

	cur_image = image_from_devtype_and_index(IO_FLOPPY, a2_drives_num);
	cur_disk = &apple2_drives[a2_drives_num];

	switch (offset) {
	case 0x00:		/* PHASE0OFF */
	case 0x01:		/* PHASE0ON */
	case 0x02:		/* PHASE1OFF */
	case 0x03:		/* PHASE1ON */
	case 0x04:		/* PHASE2OFF */
	case 0x05:		/* PHASE2ON */
	case 0x06:		/* PHASE3OFF */
	case 0x07:		/* PHASE3ON */
		phase = (offset >> 1);
		if ((offset & 1) == 0)
		{
			/* phase OFF */
			cur_disk->state &= ~(1 << phase);
		}
		else
		{
			/* phase ON */
			cur_disk->state |= (1 << phase);

			phase -= floppy_drive_get_current_track(cur_image) * 2;
			if (cur_disk->state & TWEEN_TRACKS)
				phase--;
			phase %= 4;

			switch(phase) {
			case 1:
				seek_disk(cur_image, cur_disk, +1);
				break;
			case 3:
				seek_disk(cur_image, cur_disk, -1);
				break;
			}
		}
		break;

	case 0x08:		/* MOTOROFF */
	case 0x09:		/* MOTORON */
		floppy_drive_set_motor_state(cur_image, (offset & 1));
		break;
		
	case 0x0A:		/* DRIVE1 */
	case 0x0B:		/* DRIVE2 */
		a2_drives_num = (offset & 1);
		break;
	
	case 0x0C:		/* Q6L - set transistor Q6 low */
		cur_disk->state &= ~Q6_MASK;
		if (read_state)
			result = process_byte(cur_image, cur_disk, -1);
		else
			process_byte(cur_image, cur_disk, disk6byte);
		break;
	
	case 0x0D:		/* Q6H - set transistor Q6 high */
		cur_disk->state |= Q6_MASK;
		if (!image_is_writable(cur_image))
			result = 0x80;
		break;

	case 0x0E:		/* Q7L - set transistor Q7 low */
		cur_disk->state &= ~Q7_MASK;
		read_state = 1;
		if (!image_is_writable(cur_image))
			result = 0x80;
		break;

	case 0x0F:		/* Q7H - set transistor Q7 high */
		cur_disk->state |= Q7_MASK;
		read_state = 0;
		break;
	}

	profiler_mark(PROFILER_END);
	return result;
}



/***************************************************************************
  apple2_c0xx_slot6_w
***************************************************************************/

WRITE_HANDLER ( apple2_c0xx_slot6_w )
{
	profiler_mark(PROFILER_SLOT6);
	switch (offset)
	{
		case 0x0D:	/* Store byte for writing */
			/* TODO: remove following ugly hacked-in code */
			disk6byte = data;
			break;
		default:	/* Otherwise, do same as slot6_r ? */
			LOG(("slot6_w\n"));
			apple2_c0xx_slot6_r(offset);
			break;
	}

	profiler_mark(PROFILER_END);
}



/***************************************************************************
  apple2_slot6_w
***************************************************************************/

WRITE_HANDLER (  apple2_slot6_w )
{
}

