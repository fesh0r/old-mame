#ifndef ATARI_H
#define ATARI_H

#define NEW_POKEY	1

typedef struct {
	int  serout_count;
	int  serout_offs;
	UINT8 serout_buff[512];
	UINT8 serout_chksum;
	int  serout_delay;

	int  serin_count;
	int  serin_offs;
	UINT8 serin_buff[512];
	UINT8 serin_chksum;
	int  serin_delay;
}   POKEY;
extern POKEY pokey;

typedef struct {
	struct {
		UINT8 painp; 	/* pia port A input*/
		UINT8 pbinp; 	/* pia port B input */
	}	r;	/* read registers */
	struct {
		UINT8 paout; 	/* pia port A output */
		UINT8 pbout; 	/* pia port B output */
	}	w;	/* write registers */
	struct {
		UINT8 pactl; 	/* pia port A control */
		UINT8 pbctl; 	/* pia port B control */
	}	rw; /* read/write registers */
	struct {
		UINT8 pamsk; 	/* pia port A mask */
		UINT8 pbmsk; 	/* pia port B mask */
	}	h;	/* helper variables */
}	PIA;
extern	PIA 	pia;

extern void a800_init_machine(void);
extern int	a800_load_rom(void);
extern int	a800_id_rom(const char *name, const char *gamename);

extern void a800xl_init_machine(void);
extern int	a800xl_load_rom(void);
extern int	a800xl_id_rom(const char *name, const char *gamename);

extern void a5200_init_machine(void);
extern int	a5200_load_rom(void);
extern int	a5200_id_rom(const char *name, const char *gamename);

extern void a800_close_floppy(void);

extern int  MRA_GTIA(int offset);
extern int	MRA_PIA(int offset);
extern int	MRA_ANTIC(int offset);

extern void MWA_GTIA(int offset, int data);
extern void MWA_PIA(int offset, int data);
extern void MWA_ANTIC(int offset, int data);

extern int	atari_serin_r(int offset);
extern void atari_serout_w(int offset, int data);
extern void atari_interrupt_cb(int mask);

extern void a800_handle_keyboard(void);
extern void a5200_handle_keypads(void);

#endif