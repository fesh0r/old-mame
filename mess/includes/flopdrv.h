#ifndef __FLOP_DRIVE_HEADER_INCLUDED__
#define __FLOP_DRIVE_HEADER_INCLUDED__

/* flopdrv provides simple emulation of a disc drive */
/* the 8271, nec765 and wd179x use this */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        DEN_FM_LO = 0,
        DEN_FM_HI,
        DEN_MFM_LO,
        DEN_MFM_HI
} DENSITY;

/* sector has a deleted data address mark */
#define ID_FLAG_DELETED_DATA	0x0001
/* CRC error in id field */
#define ID_FLAG_CRC_ERROR_IN_ID_FIELD 0x0002
/* CRC error in data field */
#define ID_FLAG_CRC_ERROR_IN_DATA_FIELD 0x0004

int 	floppy_status(int id, int new_status);

typedef struct chrn_id
{
	unsigned char C;
	unsigned char H;
	unsigned char R;
	unsigned char N;
	int data_id;			// id for read/write data command
	unsigned long flags;
} chrn_id;

/* set if drive is present */
#define FLOPPY_DRIVE_CONNECTED					0x0008
/* set if disc is in drive */
#define FLOPPY_DRIVE_DISK_INSERTED				0x0001
/* set if disc is write protected - also set if drive is present but no disc in drive */
#define FLOPPY_DRIVE_DISK_WRITE_PROTECTED		0x0002
/* set if drive is connected and head is positioned over track 0 */
#define FLOPPY_DRIVE_HEAD_AT_TRACK_0			0x0004
/* set if drive is ready */
#define FLOPPY_DRIVE_READY						0x0010
/* set if index has just occured */
#define FLOPPY_DRIVE_INDEX						0x0020
/* motor state */
#define FLOPPY_DRIVE_MOTOR_ON					0x0040
/* set if disk image is read only */
#define FLOPPY_DRIVE_DISK_IMAGE_READ_ONLY		0x0100


typedef struct floppy_interface
{
	/* seek to physical track */
	void (*seek_callback)(int drive, int physical_track);

	/* the following are not strictly floppy drive operations, but are used by the
	nec765 to get data from the track - really the whole track should be constructed
	into the raw format the nec765 would normally see and this would be totally accurate */
	/* the disc image would then have to re-interpret this back and update the image
	with the data */

	/* get number of sectors per track on side specified */
	int (*get_sectors_per_track)(int drive, int physical_side);
	/* get id from current track and specified side */
	void (*get_id_callback)(int drive, chrn_id *, int id_index, int physical_side);

	/* read sector data into buffer, length = number of bytes to read */
	void	(*read_sector_data_into_buffer)(int drive, int side,int data_id,char *, int length);
	/* write sector data from buffer, length = number of bytes to read  */
	void	(*write_sector_data_from_buffer)(int drive, int side,int data_id, char *, int length, int ddam);
	/* Read track in buffer, length = number of bytes to read */
	void	(*read_track_data_info_buffer)(int drive, int side, char *ptr, int *length );
	/* format */
	void (*format_sector)(int drive, int side, int sector_index,int c, int h, int r, int n, int filler);
} floppy_interface;

struct floppy_drive
{
	/* flags */
	int flags;
	/* maximum track allowed */
	int max_track;
	/* num sides */
	int num_sides;
	/* current track - this may or may not relate to the present cylinder number
	stored by the fdc */
	int current_track;

	/* index pulse timer */
	void	*index_timer;
	/* index pulse callback */
	void	(*index_pulse_callback)(int id);

	void	(*ready_state_change_callback)(int drive, int state);
    /* physical real drive unit */
    int fdd_unit;

	unsigned char id_buffer[4];

	int id_index;
    chrn_id ids[32];

	floppy_interface interface;
};

/* a callback which will be executed if the ready state of the drive changes e.g. not ready->ready, ready->not ready */
void	floppy_drive_set_ready_state_change_callback(int drive, void (*callback)(int drive, int state));

/* floppy drive types */
typedef enum
{
	FLOPPY_DRIVE_SS_40,
	FLOPPY_DRIVE_DS_80
} floppy_type;

void	floppy_drive_set_index_pulse_callback(int drive, void (*callback)(int id));

/* set flag state */
int floppy_drive_get_flag_state(int drive, int flag);
/* get flag state */
void floppy_drive_set_flag_state(int drive, int flag, int state);
/* get current physical track drive is on */
int floppy_drive_get_current_track(int drive);

void	floppy_drive_set_geometry(int,floppy_type type);
void	floppy_drive_set_geometry_absolute(int id, int tracks, int sides);

void	floppy_drives_init(void);
void	floppy_drives_exit(void);

/* get next id from track, 1 if got a id, 0 if no id was got */
int floppy_drive_get_next_id(int drive, int side, chrn_id *);
/* set ready state of drive. If flag == 1, set ready state only if drive present,
disk is in drive, and motor is on. Otherwise set ready state to the state passed */
void	floppy_drive_set_ready_state(int drive, int state, int flag);

void	floppy_drive_set_motor_state(int drive, int state);

/* set interface for disk image functions */
void	floppy_drive_set_disk_image_interface(int, floppy_interface *);

/* set real fdd unit */
void	floppy_drive_set_real_fdd_unit(int, unsigned char);

/* seek up or down */
void floppy_drive_seek(int drive, signed int signed_tracks);


void	floppy_drive_read_track_data_info_buffer(int drive, int side, char *ptr, int *length );
void	floppy_drive_format_sector(int drive, int side, int sector_index, int c, int h, int r, int n, int filler);
void    floppy_drive_read_sector_data(int drive, int side, int index1, char *pBuffer, int length);
void    floppy_drive_write_sector_data(int drive, int side, int index1, char *pBuffer, int length, int ddam);
int		floppy_drive_get_datarate_in_us(DENSITY density);

#ifdef __cplusplus
}
#endif


#endif
