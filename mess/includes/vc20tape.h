#ifndef __VC20_TAPE_H_
#define __VC20_TAPE_H_

/* put this into your gamedriver */
extern struct DACinterface vc20tape_sound_interface;

#define CONFIG_DEVICE_VC20TAPE	\
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 1, "wav\0", DEVICE_LOAD_RESETS_NONE, OSD_FOPEN_NONE, NULL, NULL, device_load_vc20_tape, device_unload_vc20_tape, NULL)

/* the function which should be called by change on readline */
extern void vc20_tape_open (void (*read_callback) (UINT32, UINT8));
extern void c16_tape_open (void);
extern void vc20_tape_close (void);

/* call this with the name of the tape image */
DEVICE_LOAD(vc20_tape);
DEVICE_UNLOAD(vc20_tape);

/* must be high active keys */
/* set it frequently */
extern void vc20_tape_buttons (int play, int record, int stop);
extern void vc20_tape_config (int on, int noise);

extern int vc20_tape_read (void);
extern void vc20_tape_write (int data);
extern void vc20_tape_motor (int data);

/* pressed wenn cassette is in */
extern int vc20_tape_switch (void);

/* delivers status for displaying */
extern void vc20_tape_status (char *text, int size);

#endif