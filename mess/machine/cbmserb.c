#include <stdio.h>
#include <stdlib.h>
#include "snprintf.h"
#include <ctype.h>
#include <string.h>
#include "driver.h"

#define VERBOSE_DBG 0				   /* general debug messages */
#include "includes/cbm.h"
#include "includes/cbmdrive.h"

#include "includes/cbmserb.h"

static void vc1541_reset_write (CBM_Drive * vc1541, int level);

CBM_Drive cbm_drive[2];

CBM_Serial cbm_serial = {0};

/* must be called before other functions */
void cbm_drive_open (void)
{
	int i;

	cbm_drive_open_helper ();

	cbm_serial.count = 0;
	for (i = 0; i < sizeof (cbm_serial.atn) / sizeof (int); i++)

	{
		cbm_serial.atn[i] =
			cbm_serial.data[i] =
			cbm_serial.clock[i] = 1;
	}
}

void cbm_drive_close (void)
{
	int i;

	cbm_serial.count = 0;
	for (i = 0; i < sizeof (cbm_drive) / sizeof (CBM_Drive); i++)
	{
		cbm_drive[i].interface = 0;

		if (cbm_drive[i].drive == D64_IMAGE)
		{
			if (cbm_drive[i].d.d64.image)
				free (cbm_drive[i].d.d64.image);
		}
		cbm_drive[i].drive = 0;
	}
}

static void cbm_drive_config (CBM_Drive * drive, int interface, int serialnr)
{
	int i;

	if (interface==SERIAL)
		drive->i.serial.device=serialnr;

	if (interface==IEEE)
		drive->i.ieee.device=serialnr;

	if (drive->interface == interface)
		return;

	if (drive->interface == SERIAL)
	{
		for (i = 0; (i < cbm_serial.count) && (cbm_serial.drives[i] != drive); i++) ;
		for (; i + 1 < cbm_serial.count; i++)
			cbm_serial.drives[i] = cbm_serial.drives[i + 1];
		cbm_serial.count--;
	}

	drive->interface = interface;

	if (drive->interface == IEC)
	{
		drive->i.iec.handshakein =
			drive->i.iec.handshakeout = 0;
		drive->i.iec.status = 0;
		drive->i.iec.dataout = drive->i.iec.datain = 0xff;
		drive->i.iec.state = 0;
	}
	else if (drive->interface == SERIAL)
	{
		cbm_serial.drives[cbm_serial.count++] = drive;
		vc1541_reset_write(drive, 0);
	}
}

void cbm_drive_0_config (int interface, int serialnr)
{
	cbm_drive_config (cbm_drive, interface, serialnr);
}
void cbm_drive_1_config (int interface, int serialnr)
{
	cbm_drive_config (cbm_drive + 1, interface, serialnr);
}

/* load *.prg files directy from filesystem (rom directory) */
int cbm_drive_attach_fs (int id)
{
	CBM_Drive *drive = cbm_drive + id;

	if (drive->drive == D64_IMAGE)
	{
		return 1;					   /* as long as floppy system is called before driver init */
		if (drive->d.d64.image)
			free (drive->d.d64.image);
	}
	memset (&(drive->d.fs), 0, sizeof (drive->d.fs));
	drive->drive = FILESYSTEM;
	return 0;
}

static int d64_open (int id)
{
	FILE *in;
	int size;

	memset (&(cbm_drive[id].d.d64), 0, sizeof (cbm_drive[id].d.d64));

	cbm_drive[id].d.d64.image_type = IO_FLOPPY;
	cbm_drive[id].d.d64.image_id = id;
	if (!(in = (FILE*)image_fopen (IO_FLOPPY, id, OSD_FILETYPE_IMAGE, 0)))
	{
		logerror(" image %s not found\n", device_filename(IO_FLOPPY,id));
		return 1;
	}
	size = osd_fsize (in);
	if (!(cbm_drive[id].d.d64.image = (UINT8*)malloc (size)))
	{
		osd_fclose (in);
		return 1;
	}
	if (size != osd_fread (in, cbm_drive[id].d.d64.image, size))
	{
		free (cbm_drive[id].d.d64.image);
		osd_fclose (in);
		return 1;
	}
	osd_fclose (in);

	logerror("floppy image %s loaded\n",
				 device_filename(IO_FLOPPY,id));

	cbm_drive[id].drive = D64_IMAGE;
	return 0;
}

/* open an d64 image */
int cbm_drive_attach_image (int id)
{
#if 1
	if (device_filename(IO_FLOPPY,id)==NULL)
		return cbm_drive_attach_fs (id);
#else
    CBM_Drive *drive = cbm_drive + id;
	if (drive->drive == FILESYSTEM) {

	}
#endif
	return d64_open (id);
}


static void c1551_write_data (CBM_Drive * c1551, int data)
{
	c1551->i.iec.datain = data;
	c1551_state (c1551);
}

static int c1551_read_data (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.dataout;
}

static void c1551_write_handshake (CBM_Drive * c1551, int data)
{
	c1551->i.iec.handshakein = data&0x40?1:0;
	c1551_state (c1551);
}

static int c1551_read_handshake (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.handshakeout?0x80:0;
}

static int c1551_read_status (CBM_Drive * c1551)
{
	c1551_state (c1551);
	return c1551->i.iec.status;
}

void c1551_0_write_data (int data)
{
	c1551_write_data (cbm_drive, data);
}
int c1551_0_read_data (void)
{
	return c1551_read_data (cbm_drive);
}
void c1551_0_write_handshake (int data)
{
	c1551_write_handshake (cbm_drive, data);
}
int c1551_0_read_handshake (void)
{
	return c1551_read_handshake (cbm_drive);
}
int c1551_0_read_status (void)
{
	return c1551_read_status (cbm_drive);
}

void c1551_1_write_data (int data)
{
	c1551_write_data (cbm_drive + 1, data);
}
int c1551_1_read_data (void)
{
	return c1551_read_data (cbm_drive + 1);
}
void c1551_1_write_handshake (int data)
{
	c1551_write_handshake (cbm_drive + 1, data);
}
int c1551_1_read_handshake (void)
{
	return c1551_read_handshake (cbm_drive + 1);
}
int c1551_1_read_status (void)
{
	return c1551_read_status (cbm_drive + 1);
}

static void vc1541_reset_write (CBM_Drive * vc1541, int level)
{
	if (level == 0)
	{
		vc1541->i.serial.data =
			vc1541->i.serial.clock =
			vc1541->i.serial.atn = 1;
		vc1541->i.serial.state = 0;
	}
}

static int vc1541_atn_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.atn;
}

static int vc1541_data_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.data;
}

static int vc1541_clock_read (CBM_Drive * vc1541)
{
	vc1541_state (vc1541);
	return vc1541->i.serial.clock;
}

static void vc1541_data_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}

static void vc1541_clock_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}

static void vc1541_atn_write (CBM_Drive * vc1541, int level)
{
	vc1541_state (vc1541);
}


/* bus handling */
void cbm_serial_reset_write (int level)
{
	int i;

	for (i = 0; i < cbm_serial.count; i++)
		vc1541_reset_write (cbm_serial.drives[i], level);
	/* init bus signals */
}

int cbm_serial_request_read (void)
{
	/* in c16 not connected */
	return 1;
}

void cbm_serial_request_write (int level)
{
}

int cbm_serial_atn_read (void)
{
	int i;

	cbm_serial.atn[0] = cbm_serial.atn[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2] =
			vc1541_atn_read (cbm_serial.drives[i]);
	return cbm_serial.atn[0];
}

int cbm_serial_data_read (void)
{
	int i;

	cbm_serial.data[0] = cbm_serial.data[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2] =
			vc1541_data_read (cbm_serial.drives[i]);
	return cbm_serial.data[0];
}

int cbm_serial_clock_read (void)
{
	int i;

	cbm_serial.clock[0] = cbm_serial.clock[1];
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2] =
			vc1541_clock_read (cbm_serial.drives[i]);
	return cbm_serial.clock[0];
}

void cbm_serial_data_write (int level)
{
	int i;

	cbm_serial.data[0] =
		cbm_serial.data[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.data[0] &= cbm_serial.data[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_data_write (cbm_serial.drives[i], cbm_serial.data[0]);
}

void cbm_serial_clock_write (int level)
{
	int i;

	cbm_serial.clock[0] =
		cbm_serial.clock[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.clock[0] &= cbm_serial.clock[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_clock_write (cbm_serial.drives[i], cbm_serial.clock[0]);
}

void cbm_serial_atn_write (int level)
{
	int i;

	cbm_serial.atn[0] =
		cbm_serial.atn[1] = level;
	/* update line */
	for (i = 0; i < cbm_serial.count; i++)
		cbm_serial.atn[0] &= cbm_serial.atn[i + 2];
	/* inform drives */
	for (i = 0; i < cbm_serial.count; i++)
		vc1541_atn_write (cbm_serial.drives[i], cbm_serial.atn[0]);
}

/* delivers status for displaying */
static void cbm_drive_status (CBM_Drive * c1551, char *text, int size)
{
	text[0] = 0;
#if VERBOSE_DBG
	if ((c1551->interface == SERIAL) /*&&(c1551->i.serial.device==8) */ )
	{
		snprintf (text, size, "%d state:%d %d %d %s %s %s",
				  c1551->state, c1551->i.serial.state, c1551->pos, c1551->size,
				  cbm_serial.atn[0] ? "ATN" : "atn",
				  cbm_serial.clock[0] ? "CLOCK" : "clock",
				  cbm_serial.data[0] ? "DATA" : "data");
		return;
	}
	if ((c1551->interface == IEC) /*&&(c1551->i.serial.device==8) */ )
	{
		snprintf (text, size, "%d state:%d %d %d",
				  c1551->state, c1551->i.iec.state, c1551->pos, c1551->size);
		return;
	}
#endif
	if (c1551->drive == FILESYSTEM)
	{
		switch (c1551->state)
		{
		case OPEN:
			snprintf (text, size, "Romdir File %s open", c1551->d.fs.filename);
			break;
		case READING:
			snprintf (text, size, "Romdir File %s loading %d",
					  c1551->d.fs.filename, c1551->size - c1551->pos - 1);
			break;
		case WRITING:
			snprintf (text, size, "Romdir File %s saving %d",
					  c1551->d.fs.filename, c1551->pos);
			break;
		}
	}
	else if (c1551->drive == D64_IMAGE)
	{
		switch (c1551->state)
		{
		case OPEN:
			snprintf (text, size, "Image File %s open",
					  c1551->d.d64.filename);
			break;
		case READING:
			snprintf (text, size, "Image File %s loading %d",
					  c1551->d.d64.filename,
					  c1551->size - c1551->pos - 1);
			break;
		case WRITING:
			snprintf (text, size, "Image File %s saving %d",
					  c1551->d.d64.filename, c1551->pos);
			break;
		}
	}
}

void cbm_drive_0_status (char *text, int size)
{
	cbm_drive_status (cbm_drive, text, size);
}

void cbm_drive_1_status (char *text, int size)
{
	cbm_drive_status (cbm_drive + 1, text, size);
}
