/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Business Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "includes/amiga.h"

#define LOG_CUSTOM				0
#define LOG_CIA					0
#define OLD_INTERRUPT_SYSTEM	0

/***************************************************************************

	General routines and registers

***************************************************************************/

custom_regs_def custom_regs;

#if OLD_INTERRUPT_SYSTEM
static int get_int_level( void ) {

	int ints = custom_regs.INTENA & custom_regs.INTREQ;

	if ( custom_regs.INTENA & 0x4000 ) { /* Master interrupt switch */

		/* Level 7 = NMI Can only be triggered from outside the hardware */

		if ( ints & 0x2000 )
			return m68_level6_irq();

		if ( ints & 0x1800 )
			return m68_level5_irq();

		if ( ints & 0x0780 )
			return m68_level4_irq();

		if ( ints & 0x0070 )
			return m68_level3_irq();

		if ( ints & 0x0008 )
			return m68_level2_irq();

		if ( ints & 0x0007 )
			return m68_level1_irq();
	}

	return 0;
}

static void trigger_int( void ) {

	int level = get_int_level();

	if ( level )
		cpu_cause_interrupt( 0, level );
}

#else

static int translate_ints( void ) {

	int ints = custom_regs.INTENA & custom_regs.INTREQ;
	int ret = 0;

	if ( custom_regs.INTENA & 0x4000 ) { /* Master interrupt switch */

		/* Level 7 = NMI Can only be triggered from outside the hardware */

		if ( ints & 0x2000 )
			ret |= ( 1 << 5 );

		if ( ints & 0x1800 )
			ret |= ( 1 << 4 );

		if ( ints & 0x0780 )
			ret |= ( 1 << 3 );

		if ( ints & 0x0070 )
			ret |= ( 1 << 2 );

		if ( ints & 0x0008 )
			ret |= ( 1 << 1 );

		if ( ints & 0x0007 )
			ret |= ( 1 << 0 );
	}

	return ret;
}

static void check_ints( void ) {

	int ints = translate_ints(), i;

	for ( i = 0; i < 7; i++ ) {
		if ( ( ints >> i ) & 1 )
			cpu_set_irq_line( 0, i + 1, ASSERT_LINE );
		else
			cpu_set_irq_line( 0, i + 1, CLEAR_LINE );
	}
}
#endif

/***************************************************************************

	Blitter emulation

***************************************************************************/

static unsigned short blitter_fill( unsigned short src, int mode, int *fc ) {
	int i, data;
	unsigned short dst = 0;

	for ( i = 0; i < 16; i++ ) {
		data = ( src >> i ) & 1;
		if ( data ) {
			*fc ^= 1;
			if ( mode & 0x0010 ) /* Exclusive mode */
				dst |= ( data ^ *fc ) << i;
			if ( mode & 0x0008 ) /* Inclusive mode */
				dst |= ( data | *fc ) << i;
		} else
			dst |= ( *fc << i );
	}

	return dst;
}

/* Ascending */
#define BLIT_FUNCA( num, op ) static int blit_func_a##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
														 	int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= custom_regs.BLTAFWM;														\
																							\
	if ( last )																				\
		dataA &= custom_regs.BLTALWM;														\
																							\
	shiftA = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;											\
	shiftB = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;											\
																							\
	if ( shiftA ) {																			\
		dataA >>= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] & ( ( 1 << shiftA ) - 1 ) ) << ( 16 - shiftA );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB >>= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] & ( ( 1 << shiftB ) - 1 ) ) << ( 16 - shiftB );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCA( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCA( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCA( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCA( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCA( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCA( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCA( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCA( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_a[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_a0,
	blit_func_a1,
	blit_func_a2,
	blit_func_a3,
	blit_func_a4,
	blit_func_a5,
	blit_func_a6,
	blit_func_a7
};

/* Descending */
#define BLIT_FUNCD( num, op ) static int blit_func_d##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
															int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= custom_regs.BLTAFWM;														\
																							\
	if ( last )																				\
		dataA &= custom_regs.BLTALWM;														\
																							\
	shiftA = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;											\
	shiftB = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;											\
																							\
	if ( shiftA ) {																			\
		dataA <<= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] >> ( 16 - shiftA ) ) & ( ( 1 << shiftA ) - 1 );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB <<= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] >> ( 16 - shiftB ) ) & ( ( 1 << shiftB ) - 1 );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCD( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCD( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCD( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCD( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCD( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCD( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCD( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCD( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_d[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_d0,
	blit_func_d1,
	blit_func_d2,
	blit_func_d3,
	blit_func_d4,
	blit_func_d5,
	blit_func_d6,
	blit_func_d7
};

/* General for Ascending/Descending */
static int (**blit_func)( unsigned short *old_data, unsigned short *new_data, int first, int last );

/* Line mode */
#define BLIT_FUNCL( num, op ) static int blit_func_line##num ( unsigned short dataA,		\
															   unsigned short dataB,		\
															   unsigned short dataC ) {		\
	return ( op );																			\
}

BLIT_FUNCL( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCL( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCL( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCL( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCL( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCL( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCL( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCL( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_line[8])( unsigned short dataA, unsigned short dataB, unsigned short dataC ) = {
	blit_func_line0,
	blit_func_line1,
	blit_func_line2,
	blit_func_line3,
	blit_func_line4,
	blit_func_line5,
	blit_func_line6,
	blit_func_line7
};

static void blitter_proc( int param ) {
	/* Now we do the real blitting */
	unsigned char *RAM = memory_region( REGION_CPU1 );
	int	blt_total = 0;

	custom_regs.DMACON |= 0x2000; /* Blit Zero, we modify it later */

	if ( custom_regs.BLTCON1 & 1 ) { /* Line mode */
		int linesize, single_bit, single_flag, texture, sign, start, ptr[4], temp_ptr3;
		unsigned short dataA, dataB;

		if ( ( custom_regs.BLTSIZE & 0x003f ) != 0x0002 ) {
			logerror("Blitter: BLTSIZE.w != 2 in line mode!\n" );
		}

		if ( ( custom_regs.BLTCON0 & 0x0b00 ) != 0x0b00 ) {
			logerror("Blitter: Channel selection incorrect in line mode!\n" );
		}

		linesize = ( custom_regs.BLTSIZE >> 6 ) & 0x3ff;
		if ( linesize == 0 )
			linesize = 0x400;

		single_bit = custom_regs.BLTCON1 & 0x0002;
		single_flag = 0;
		sign = custom_regs.BLTCON1 & 0x0040;
		texture = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;
		start = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;

		dataA = ( custom_regs.BLTxDAT[0] >> start );

		ptr[0] = ( custom_regs.BLTxPTH[0] << 16 ) | custom_regs.BLTxPTL[0];
		ptr[2] = ( custom_regs.BLTxPTH[2] << 16 ) | custom_regs.BLTxPTL[2];
		ptr[3] = ( custom_regs.BLTxPTH[3] << 16 ) | custom_regs.BLTxPTL[3];

		dataB = ( custom_regs.BLTxDAT[1] >> texture ) | ( custom_regs.BLTxDAT[1] << ( 16 - texture ) );

		while ( linesize-- ) {
			int dst_data = 0;
			int i;

			temp_ptr3 = ptr[3];

			if ( custom_regs.BLTCON0 & 0x0200 )
				custom_regs.BLTxDAT[2] = READ_WORD( &RAM[ptr[2]] );

			dataA = ( custom_regs.BLTxDAT[0] >> start );

			if ( single_bit && single_flag ) dataA = 0;
			single_flag = 1;

			for ( i = 0; i < 8; i++ ) {
				if ( custom_regs.BLTCON0 & ( 1 << i ) ) {
					dst_data |= (*blit_func_line[i])( dataA, ( dataB & 1 ) * 0xffff, custom_regs.BLTxDAT[2] );
				}
			}

			if ( !sign ) {
				ptr[0] += custom_regs.BLTxMOD[0];
				if ( custom_regs.BLTCON1 & 0x0010 ) {
					if ( custom_regs.BLTCON1 & 0x0008 ) { /* Decrement Y */
						ptr[2] -= custom_regs.BLTxMOD[2];
						ptr[3] -= custom_regs.BLTxMOD[2]; /* ? */
						single_flag = 0;
					} else { /* Increment Y */
						ptr[2] += custom_regs.BLTxMOD[2];
						ptr[3] += custom_regs.BLTxMOD[2]; /* ? */
						single_flag = 0;
					}
				} else {
					if ( custom_regs.BLTCON1 & 0x0008 ) { /* Decrement X */
						if ( start-- == 0 ) {
							start = 15;
							ptr[2] -= 2;
							ptr[3] -= 2;
						}
					} else { /* Increment X */
						if ( ++start == 16 ) {
							start = 0;
							ptr[2] += 2;
							ptr[3] += 2;
						}
					}
				}
			} else
				ptr[0] += custom_regs.BLTxMOD[1];

			if ( custom_regs.BLTCON1 & 0x0010 ) {
				if ( custom_regs.BLTCON1 & 0x0004 ) { /* Decrement X */
					if ( start-- == 0 ) {
						start = 15;
						ptr[2] -= 2;
						ptr[3] -= 2;
					}
				} else {
					if ( ++start == 16 ) { /* Increment X */
						start = 0;
						ptr[2] += 2;
						ptr[3] += 2;
					}
				}
			} else {
				if ( custom_regs.BLTCON1 & 0x0004 ) { /* Decrement Y */
					ptr[2] -= custom_regs.BLTxMOD[2];
					ptr[3] -= custom_regs.BLTxMOD[2]; /* ? */
					single_flag = 0;
				} else { /* Increment Y */
					ptr[2] += custom_regs.BLTxMOD[2];
					ptr[3] += custom_regs.BLTxMOD[2]; /* ? */
					single_flag = 0;
				}
			}

			sign = 0 > ( signed short )ptr[0];

			blt_total |= dst_data;

			if ( custom_regs.BLTCON0 & 0x0100 )
				WRITE_WORD( &RAM[temp_ptr3], dst_data );

			dataB = ( dataB << 1 ) | ( dataB >> 15 );
		}

		custom_regs.BLTxPTH[0] = ( ptr[0] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[0] = ptr[0] & 0xffff;
		custom_regs.BLTxPTH[2] = ( ptr[2] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[2] = ptr[2] & 0xffff;
		custom_regs.BLTxPTH[3] = ( ptr[3] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[3] = ptr[3] & 0xffff;

//		custom_regs.BLTxDAT[0] = dataA;
//		custom_regs.BLTxDAT[1] = dataB;

	} else { /* Blit mode */
		if ( custom_regs.BLTCON0 & 0x0f00 ) {
			int i, x, y;
			int ptr[4] = { -1, -1, -1, -1 };
			int width = ( custom_regs.BLTSIZE & 0x3f );
			int height = ( ( custom_regs.BLTSIZE >> 6 ) & 0x3ff );
			unsigned short old_data[3];
			unsigned short new_data[3];
			int fc;

			if ( custom_regs.BLTCON0 & 0x800 )
				ptr[0] = ( custom_regs.BLTxPTH[0] << 16 ) | custom_regs.BLTxPTL[0];

			if ( custom_regs.BLTCON0 & 0x400 )
				ptr[1] = ( custom_regs.BLTxPTH[1] << 16 ) | custom_regs.BLTxPTL[1];

			if ( custom_regs.BLTCON0 & 0x200 )
				ptr[2] = ( custom_regs.BLTxPTH[2] << 16 ) | custom_regs.BLTxPTL[2];

			if ( custom_regs.BLTCON0 & 0x100 )
				ptr[3] = ( custom_regs.BLTxPTH[3] << 16 ) | custom_regs.BLTxPTL[3];

			if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
				blit_func = blit_func_d;
				fc = ( custom_regs.BLTCON1 >> 2 ) & 1; /* fill carry setup */
			} else {
				blit_func = blit_func_a;
			}

			if ( width == 0 )
				width = 0x40;

			if ( height == 0 )
				height = 0x400;

			for ( y = 0; y < height; y++ ) {
				for ( x = 0; x < width; x++ ) {
					int dst_data = 0;

					/* get old data */
					new_data[0] = old_data[0] = custom_regs.BLTxDAT[0];
					new_data[1] = old_data[1] = custom_regs.BLTxDAT[1];
					new_data[2] = old_data[2] = custom_regs.BLTxDAT[2];

					/* get new data */
					if ( ptr[0] != -1 )
						new_data[0] = READ_WORD( &RAM[ptr[0]] );

					if ( ptr[1] != -1 )
						new_data[1] = READ_WORD( &RAM[ptr[1]] );

					if ( ptr[2] != -1 )
						new_data[2] = READ_WORD( &RAM[ptr[2]] );

					for ( i = 0; i < 8; i++ ) {
						if ( custom_regs.BLTCON0 & ( 1 << i ) ) {
							dst_data |= (*blit_func[i])( old_data, new_data, x == 0, x == (width - 1) );
						}
					}

					/* store new data */
					custom_regs.BLTxDAT[0] = new_data[0];
					custom_regs.BLTxDAT[1] = new_data[1];
					custom_regs.BLTxDAT[2] = new_data[2];

					if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
						if ( custom_regs.BLTCON1 & 0x0018 ) /* Fill mode */
							dst_data = blitter_fill( dst_data, custom_regs.BLTCON1 & 0x18, &fc );

						if ( ptr[3] != -1 ) {
							WRITE_WORD( &RAM[ptr[3]], dst_data );
							ptr[3] -= 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] -= 2;

						if ( ptr[1] != -1 )
							ptr[1] -= 2;

						if ( ptr[2] != -1 )
							ptr[2] -= 2;
					} else {
						if ( ptr[3] != -1 ) {
							WRITE_WORD( &RAM[ptr[3]], dst_data );
							ptr[3] += 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] += 2;

						if ( ptr[1] != -1 )
							ptr[1] += 2;

						if ( ptr[2] != -1 )
							ptr[2] += 2;
					}
					blt_total |= dst_data;
				}

				if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
					if ( ptr[0] != -1 )
						ptr[0] -= custom_regs.BLTxMOD[0];

					if ( ptr[1] != -1 )
						ptr[1] -= custom_regs.BLTxMOD[1];

					if ( ptr[2] != -1 )
						ptr[2] -= custom_regs.BLTxMOD[2];

					if ( ptr[3] != -1 )
						ptr[3] -= custom_regs.BLTxMOD[3];
				} else {
					if ( ptr[0] != -1 )
						ptr[0] += custom_regs.BLTxMOD[0];

					if ( ptr[1] != -1 )
						ptr[1] += custom_regs.BLTxMOD[1];

					if ( ptr[2] != -1 )
						ptr[2] += custom_regs.BLTxMOD[2];

					if ( ptr[3] != -1 )
						ptr[3] += custom_regs.BLTxMOD[3];
				}
			}

			/* We're done, update the ptr's now */
			if ( ptr[0] != -1 ) {
				custom_regs.BLTxPTH[0] = ptr[0] >> 16;
				custom_regs.BLTxPTL[0] = ptr[0];
			}

			if ( ptr[1] != -1 ) {
				custom_regs.BLTxPTH[1] = ptr[1] >> 16;
				custom_regs.BLTxPTL[1] = ptr[1];
			}

			if ( ptr[2] != -1 ) {
				custom_regs.BLTxPTH[2] = ptr[2] >> 16;
				custom_regs.BLTxPTL[2] = ptr[2];
			}

			if ( ptr[3] != -1 ) {
				custom_regs.BLTxPTH[3] = ptr[3] >> 16;
				custom_regs.BLTxPTL[3] = ptr[3];
			}
		}
	}

	custom_regs.DMACON ^= 0x4000; /* signal we're done */

	if ( blt_total )
		custom_regs.DMACON ^= 0x2000;


	amiga_custom_w( 0x009c>>1, 0x8040, 0);
}

static void blitter_setup( void ) {
	int ticks, width, height;
	double blit_time;

	if ( ( custom_regs.DMACON & 0x0240 ) != 0x0240 ) /* Enabled ? */
		return;

	if ( custom_regs.DMACON & 0x4000 ) { /* Is there another blitting in progress? */
		logerror("This program is playing tricks with the blitter\n" );
		return;
	}

	if ( custom_regs.BLTCON1 & 1 ) { /* Line mode */
		ticks = 8;
	} else {
		ticks = 4;
		if ( custom_regs.BLTCON0 & 0x0400 ) /* channel B, +2 ticks */
			ticks += 2;
		if ( ( custom_regs.BLTCON0 & 0x0300 ) == 0x0300 ) /* Both channel C and D, +2 ticks */
			ticks += 2;
	}

	custom_regs.DMACON |= 0x4000; /* signal blitter busy */

	width = ( custom_regs.BLTSIZE & 0x3f );
	if ( width == 0 )
		width = 0x40;

	height = ( ( custom_regs.BLTSIZE >> 6 ) & 0x3ff );
	if ( height == 0 )
		height = 0x400;

	blit_time = ( (double)ticks * (double)width * (double)height );

	blit_time /= ( (double)Machine->drv->cpu[0].cpu_clock / 1000000.0 );

	timer_set( TIME_IN_USEC( blit_time ), 0, blitter_proc );
}

/***************************************************************************

	Floppy disk controller emulation

***************************************************************************/

/* required prototype */
static void cia_issue_index( void );
static void setup_fdc_buffer( int drive );

typedef struct {
	int motor_on;
	int side;
	int dir;
	int wprot;
	int disk_changed;
	void *f;
	int cyl;
	unsigned char mfm[544*2*11];
	int	cached;
	void *rev_timer;
	int pos;
} fdc_def;

static fdc_def fdc_status[4];
/* signals */
static int fdc_sel = 0x0f;
static int fdc_dir = 0;
static int fdc_side = 1;
static int fdc_step = 1;
static int fdc_rdy = 1;

int amiga_fdc_init( int id ) {

	fdc_status[id].motor_on = 0;
	fdc_status[id].side = 0;
	fdc_status[id].dir = 0;
	fdc_status[id].wprot = 1;
	fdc_status[id].cyl = 0;
	fdc_status[id].rev_timer = 0;
	fdc_status[id].cached = -1;
	fdc_status[id].pos = 0;

	memset( fdc_status[id].mfm, 0xaa, 544*2*11 );

	fdc_status[id].disk_changed = 1;
	fdc_status[id].f = image_fopen(IO_FLOPPY,id,OSD_FILETYPE_IMAGE,0);
	if ( fdc_status[id].f == NULL ) {
		logerror("Could not open image %s\n", device_filename(IO_FLOPPY,id) );
		return INIT_FAIL;
	}
	fdc_status[id].disk_changed = 0;

	fdc_sel = 0x0f;
	fdc_dir = 0;
	fdc_side = 1;
	fdc_step = 1;
	fdc_rdy = 1;

	return INIT_PASS;
}

static int fdc_get_curpos( int drive ) {
	double elapsed;
	int speed;
	int bytes;
	int pos;

	if ( fdc_status[drive].rev_timer == 0 ) {
		logerror("Rev timer not started on drive %d, cant get position!\n", drive );
		return 0;
	}

	elapsed = timer_timeelapsed( fdc_status[drive].rev_timer );
	speed = ( custom_regs.ADKCON & 0x100 ) ? 2 : 4;

	bytes = elapsed / ( TIME_IN_USEC( speed * 8 ) );
	pos = bytes % ( 544*2*11 );

	return pos;
}

static unsigned short fdc_get_byte( void ) {
	int pos;
	int i, drive = -1;
	unsigned short ret;

	ret = ( ( custom_regs.DSKLEN >> 1 ) & 0x4000 ) & ( ( custom_regs.DMACON << 10 ) & 0x4000 );
	ret |= ( custom_regs.DSKLEN >> 1 ) & 0x2000;

	for ( i = 0; i < 4; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 )
		return ret;

	if ( fdc_status[drive].disk_changed )
		return ret;

	setup_fdc_buffer( drive );

	pos = fdc_get_curpos( drive );

	if ( fdc_status[drive].mfm[pos] == ( custom_regs.DSKSYNC >> 8 ) &&
		 fdc_status[drive].mfm[pos+1] == ( custom_regs.DSKSYNC & 0xff ) )
			ret |= 0x1000;

	if ( pos != fdc_status[drive].pos ) {
		ret |= 0x8000;
		fdc_status[drive].pos = pos;
	}

	ret |= fdc_status[drive].mfm[pos];

	return ret;
}

static void fdc_dma_proc( int drive ) {

	if ( fdc_status[drive].disk_changed )
		return;

	setup_fdc_buffer( drive );

	if ( custom_regs.DSKLEN & 0x4000 ) {
		logerror("Write to disk unsupported yet\n" );
	} else {
		unsigned char *RAM = &memory_region(REGION_CPU1)[( custom_regs.DSKPTH << 16 ) | custom_regs.DSKPTL];
		int cur_pos = fdc_status[drive].pos;
		int len = custom_regs.DSKLEN & 0x3fff;

		while ( len-- ) {
			int dat = ( fdc_status[drive].mfm[cur_pos++] ) << 8;

			cur_pos %= ( 544 * 2 * 11 );

			dat |= fdc_status[drive].mfm[cur_pos++];

			cur_pos %= ( 544 * 2 * 11 );

			WRITE_WORD( RAM, dat );

			RAM += 2;
		}
	}

	amiga_custom_w( 0x009c>>1, 0x8002, 0);
}

static void fdc_setup_dma( void ) {
	int i, cur_pos, drive = -1;
	int time = 0;

	if ( ( custom_regs.DSKLEN & 0x8000 ) == 0 )
		return;

	if ( ( custom_regs.DMACON & 0x0210 ) == 0 )
		return;

	for ( i = 0; i < 4; i++ ) {
		if ( !( fdc_sel & ( 1 << i ) ) )
			drive = i;
	}

	if ( drive == -1 ) {
		logerror("Disk DMA started with no drive selected!\n" );
		return;
	}

	if ( fdc_status[drive].disk_changed )
		return;

	setup_fdc_buffer( drive );

	fdc_status[drive].pos = cur_pos = fdc_get_curpos( drive );

	if ( custom_regs.ADKCON & 0x0400 ) { /* Wait for sync */
		if ( custom_regs.DSKSYNC != 0x4489 ) {
			logerror("Attempting to read a non-standard SYNC\n" );
		}

		i = cur_pos;
		do {
			if ( fdc_status[drive].mfm[i] == ( custom_regs.DSKSYNC >> 8 ) &&
				 fdc_status[drive].mfm[i+1] == ( custom_regs.DSKSYNC & 0xff ) )
				 	break;

			i++;
			i %= ( 544 * 2 * 11 );
			time++;
		} while( i != cur_pos );

		if ( i == cur_pos && time != 0 ) {
			logerror("SYNC not found on track!\n" );
			return;
		} else {
			fdc_status[drive].pos = i + 2;
		}

		time += ( custom_regs.DSKLEN & 0x3fff ) * 2;
		time *= ( custom_regs.ADKCON & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	} else {
		time = ( custom_regs.DSKLEN & 0x3fff ) * 2;
		time *= ( custom_regs.ADKCON & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	}
}

static void setup_fdc_buffer( int drive ) {
	int sector, offset, len;
	static unsigned char temp_cyl[512*11];

	/* no disk in drive */
	if ( fdc_status[drive].disk_changed )
		return;

	if ( fdc_status[drive].f == NULL ) {
		fdc_status[drive].disk_changed = 1;
		return;
	}

	len = 512*11;

	offset = ( fdc_status[drive].cyl << 1 ) | fdc_side;

	if ( fdc_status[drive].cached == offset )
		return;

	if ( osd_fseek( fdc_status[drive].f, offset * len, SEEK_SET ) ) {
		logerror("FDC: osd_fseek failed!\n" );
		osd_fclose( fdc_status[drive].f );
		fdc_status[drive].f = NULL;
		fdc_status[drive].disk_changed = 1;
	}

	osd_fread( fdc_status[drive].f, temp_cyl, len );

	for ( sector = 0; sector < 11; sector++ ) {
		unsigned char secbuf[544];
	    int i;
	    unsigned char *mfmbuf = ( &fdc_status[drive].mfm[544*2*sector] );
		unsigned long deven,dodd;
		unsigned long hck = 0,dck = 0;

	    secbuf[0] = secbuf[1] = 0x00;
	    secbuf[2] = secbuf[3] = 0xa1;
	    secbuf[4] = 0xff;
	    secbuf[5] = offset;
	    secbuf[6] = sector;
	    secbuf[7] = 11 - sector;

	    for ( i = 8; i < 24; i++ )
			secbuf[i] = 0;

	    memcpy( &secbuf[32], &temp_cyl[sector*512], 512 );

		mfmbuf[0*2] = 0xaa;
		mfmbuf[0*2+1] = 0xaa;
		mfmbuf[1*2] = 0xaa;
		mfmbuf[1*2+1] = 0xaa;
		mfmbuf[2*2] = 0x44;
		mfmbuf[2*2+1] = 0x89;
		mfmbuf[3*2] = 0x44;
		mfmbuf[3*2+1] = 0x89;

	    deven = ( ( secbuf[4] << 24) | ( secbuf[5] << 16 ) | ( secbuf[6] << 8 ) | ( secbuf[7] ) );
	    dodd = deven >> 1;
	    deven &= 0x55555555; dodd &= 0x55555555;

		mfmbuf[4*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[4*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[5*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[5*2+1] = dodd & 0xff;
		mfmbuf[6*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[6*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[7*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[7*2+1] = deven & 0xff;

		for ( i = 8; i < 48; i++ )
			mfmbuf[i*2] = mfmbuf[i*2+1] = 0xaa;

	    for (i = 0; i < 512; i += 4) {
			deven = ((secbuf[i+32] << 24) | (secbuf[i+33] << 16)
				 | (secbuf[i+34] << 8) | (secbuf[i+35]));
			dodd = deven >> 1;
			deven &= 0x55555555; dodd &= 0x55555555;

			mfmbuf[i + ( 32 * 2 )] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
			mfmbuf[i + ( 32 * 2 ) + 1] = ( dodd >> 16 ) & 0xff;
			mfmbuf[i + ( 33 * 2 )] = ( dodd >> 8 ) & 0xff;
			mfmbuf[i + ( 33 * 2 ) + 1] = dodd & 0xff;

			mfmbuf[i + ( ( 256 + 32 ) * 2 )] = ( ( deven >> 16 ) >> 8 ) & 0xff;
			mfmbuf[i + ( ( 256 + 32 ) * 2 ) + 1] = ( deven >> 16 ) & 0xff;
			mfmbuf[i + ( ( 256 + 33 ) * 2 )] = ( deven >> 8 ) & 0xff;
			mfmbuf[i + ( ( 256 + 33 ) * 2 ) + 1] = deven & 0xff;
	    }

	    for(i = 4; i < 24; i += 2)
			hck ^= ( ( mfmbuf[i*2] << 24) | ( mfmbuf[i*2+1] << 16 ) ) | ( ( mfmbuf[i*2+2] << 8 ) | mfmbuf[i*2+3] );

	    deven = dodd = hck; dodd >>= 1;

		mfmbuf[24*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[24*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[25*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[25*2+1] = dodd & 0xff;
		mfmbuf[26*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[26*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[27*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[27*2+1] = deven & 0xff;

	    for(i = 32; i < 544; i += 2)
			dck ^= ( ( mfmbuf[i*2] << 24) | ( mfmbuf[i*2+1] << 16 ) ) | ( ( mfmbuf[i*2+2] << 8 ) | mfmbuf[i*2+3] );

	    deven = dodd = dck; dodd >>= 1;

		mfmbuf[28*2] = ( ( dodd >> 16 ) >> 8 ) & 0xff;
		mfmbuf[28*2+1] = ( ( dodd >> 16 ) ) & 0xff;
		mfmbuf[29*2] = ( dodd >> 8 ) & 0xff;
		mfmbuf[29*2+1] = dodd & 0xff;
		mfmbuf[30*2] = ( ( deven >> 16 ) >> 8 ) & 0xff;
		mfmbuf[30*2+1] = ( ( deven >> 16 ) ) & 0xff;
		mfmbuf[31*2] = ( deven >> 8 ) & 0xff;
		mfmbuf[31*2+1] = deven & 0xff;
	}

	fdc_status[drive].cached = offset;
}

static void fdc_rev_proc( int drive ) {
	int time;

	cia_issue_index();

	time = ( custom_regs.ADKCON & 0x100 ) ? 2 : 4;
	time *= ( 544 * 2 * 11 );
	time *= 8;
	fdc_status[drive].rev_timer = timer_set( TIME_IN_USEC( time ) , drive, fdc_rev_proc );
}

static void start_rev_timer( int drive ) {
	int time;

	if ( fdc_status[drive].rev_timer ) {
		logerror("Revolution timer started twice?!\n" );
		return;
	}

	time = ( custom_regs.ADKCON & 0x100 ) ? 2 : 4;
	time *= ( 544 * 2 * 11 );
	time *= 8;

	fdc_status[drive].rev_timer = timer_set( TIME_IN_USEC( time ) , drive, fdc_rev_proc );
}

static void stop_rev_timer( int drive ) {
	if ( fdc_status[drive].rev_timer == 0 ) {
		logerror("Revolution timer never started?!\n" );
		return;
	}

	timer_remove( fdc_status[drive].rev_timer );
	fdc_status[drive].rev_timer = 0;
}

static void fdc_setup_leds( int drive ) {

	if ( drive == 0 )
		set_led_status( 1, fdc_status[drive].motor_on ); /* update internal drive led */

	if ( drive == 1 )
		set_led_status( 2, fdc_status[drive].motor_on ); /* update external drive led */
}

static void fdc_stepdrive( int drive ) {
	if ( fdc_dir ) {
		if ( fdc_status[drive].cyl )
			fdc_status[drive].cyl--;
	} else {
		if ( fdc_status[drive].cyl < 79 )
			fdc_status[drive].cyl++;
	}
}

static void fdc_motor( int drive, int off ) {
	int on = !off;

	if ( ( fdc_status[drive].motor_on == 0 ) && on ) {
		fdc_status[drive].pos = 0;

		start_rev_timer( drive );

	} else {
		if ( fdc_status[drive].motor_on && off )
			stop_rev_timer( drive );
	}

	fdc_status[drive].motor_on = on;
}

static void fdc_control_w( int data ) {
	int step_pulse;
	int drive;

	if ( fdc_sel != ( ( data >> 3 ) & 15 ) )
		fdc_rdy = 0;

	fdc_sel = ( data >> 3 ) & 15;
    fdc_side = 1 - ( ( data >> 2 ) & 1 );
	fdc_dir = ( data >> 1 ) & 1;

	step_pulse = data & 1;

    if ( fdc_step != step_pulse ) {
		fdc_step = step_pulse;

    	if ( fdc_step == 0 ) {
		    for ( drive = 0; drive < 4; drive++ ) {
				if ( !( fdc_sel & ( 1 << drive ) ) )
				    fdc_stepdrive( drive );
			}
		}
	}

	for ( drive = 0; drive < 4; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			fdc_motor( drive, ( data >> 7 ) & 1 );
			fdc_setup_leds( drive );
		}
    }
}

static int fdc_status_r( void ) {
	int drive, ret = 0x3c;

	for ( drive = 0; drive < 4; drive++ ) {
		if ( !( fdc_sel & ( 1 << drive ) ) ) {
			if ( fdc_status[drive].motor_on ) {
				if ( fdc_rdy )
					ret &= ~0x20;
				fdc_rdy = 1;
			} else {
				/* if we got a floppy image, then return we're on */
				if ( fdc_status[drive].disk_changed == 0 )
					ret &= ~0x20;
			}

			if ( fdc_status[drive].cyl == 0 )
				ret &= ~0x10;

			if ( fdc_status[drive].wprot )
				ret &= ~0x08;

			if ( fdc_status[drive].disk_changed )
				ret &= ~0x04;
		}
	}

	return ret;
}

/***************************************************************************

	8520 CIA emulation

***************************************************************************/

/* required prototype */
static void cia_fire_timer( int cia, int timer );
static void *cia_hblank_timer = 0;

typedef struct {
	unsigned char 	ddra;
	unsigned char 	ddrb;
	int	(*portA_read)( void );
	int	(*portB_read)( void );
	void (*portA_write)( int data );
	void (*portB_write)( int data );
	unsigned char	data_latchA;
	unsigned char	data_latchB;
	/* Timer A */
	unsigned short	timerA_latch;
	unsigned short	timerA_count;
	unsigned char	timerA_mode;
	/* Timer B */
	unsigned short	timerB_latch;
	unsigned short	timerB_count;
	unsigned char	timerB_mode;
	/* Time Of the Day clock (TOD) */
	unsigned long	tod;
	int				tod_running;
	unsigned long	alarm;
	/* Interrupts */
	int				icr;
	int				ics;
	/* MESS timers */
	void			*timerA;
	void			*timerB;
} cia_8520_def;

static cia_8520_def cia_8520[2];

static void cia_timer_proc( int param ) {
	int cia = param >> 8;
	int timer = param & 1;

	if ( timer == 0 )
		cia_8520[cia].timerA = 0;
	else
		cia_8520[cia].timerB = 0;

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c>>1, 0x8008, 0);
				} else {
					amiga_custom_w( 0x009c>>1, 0xa000, 0);
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_8520[cia].timerA_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c>>1, 0x8008, 0);
				} else {
					amiga_custom_w( 0x009c>>1, 0xa000, 0);
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_fire_timer( cia, 0 ); /* keep going */
		}
	} else {
		if ( cia_8520[cia].timerB_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c>>1, 0x8008, 0);
				} else {
					amiga_custom_w( 0x009c>>1, 0xa000, 0);
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_8520[cia].timerB_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c>>1, 0x8008, 0);
				} else {
					amiga_custom_w( 0x009c>>1, 0xa000, 0);
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_fire_timer( cia, 1 ); /* keep going */
		}
	}
}

static int cia_get_count( int cia, int timer ) {
	int time;

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA )
			time = cia_8520[cia].timerA_count - ( int )( timer_timeelapsed( cia_8520[cia].timerA ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerA_count;
	} else {
		if ( cia_8520[cia].timerB )
			time = cia_8520[cia].timerB_count - ( int )( timer_timeelapsed( cia_8520[cia].timerB ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerB_count;
	}

	return time;
}

static void cia_stop_timer( int cia, int timer ) {

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA )
			timer_remove( cia_8520[cia].timerA );
		cia_8520[cia].timerA = 0;
	} else {
		if ( cia_8520[cia].timerB )
			timer_remove( cia_8520[cia].timerB );
		cia_8520[cia].timerB = 0;
	}
}

static void cia_fire_timer( int cia, int timer ) {

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA == 0 )
			cia_8520[cia].timerA = timer_set( ( double )cia_8520[cia].timerA_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, cia_timer_proc );
	} else {
		if ( cia_8520[cia].timerB == 0 )
			cia_8520[cia].timerB = timer_set( ( double )cia_8520[cia].timerB_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, cia_timer_proc );
	}
}

/* Update TOD on CIA A */
static void cia_vblank_update( void ) {
	if ( cia_8520[0].tod_running ) {
		cia_8520[0].tod++;
		if ( cia_8520[0].tod == cia_8520[0].alarm ) {
			cia_8520[0].ics |= 0x84;
			if ( cia_8520[0].icr & 0x04 ) {
				amiga_custom_w( 0x009c>>1, 0x8008, 0);
			}
		}
	}
}

/* Update TOD on CIA B (each hblank) */
static void cia_hblank_update( int param ) {
	if ( cia_8520[1].tod_running ) {
		int i;

		for ( i = 0; i < Machine->drv->screen_height; i++ ) {
			cia_8520[1].tod++;
			if ( cia_8520[1].tod == cia_8520[1].alarm ) {
				cia_8520[1].ics |= 0x84;
				if ( cia_8520[1].icr & 0x04 ) {
				    amiga_custom_w( 0x009c>>1, 0xa000, 0 /* could also be hibyte only 0xff */);
				}
			}
		}
	}

	cia_hblank_timer = timer_set( cpu_getscanlineperiod(), 0, cia_hblank_update );
}

/* Issue a index pulse when a disk revolution completes */
static void cia_issue_index( void ) {
	cia_8520[1].ics |= 0x90;
	if ( cia_8520[1].icr & 0x10 ) {
	    amiga_custom_w( 0x009c>>1, 0xa000, 0 /* could also be hibyte only 0xff*/);
	}
}

static int cia_0_portA_r( void ) {
	int ret = readinputport( 0 ) & 0xc0;

	ret |= fdc_status_r();

	return ret; /* Gameport 1 and 0 buttons */
}

static int cia_0_portB_r( void ) {
	logerror("Program read from the parallel port\n" );
	return 0;
}

static void cia_0_portA_w( int data ) {
	/* bit 1 = overlay (data & 1 ) */
	set_led_status( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led on Amiga*/
}

static void cia_0_portB_w( int data ) {
	logerror("Program wrote to the parallel port\n" );
}

static int cia_1_portA_r( void ) {
	return 0x00;
}

static int cia_1_portB_r( void ) {
	return 0x00;
}

static void cia_1_portA_w( int data ) {
}

static void cia_1_portB_w( int data ) {
	fdc_control_w( data );
}

static void cia_init( void ) {
	int i;

	/* Initialize port handlers */
	cia_8520[0].portA_read = cia_0_portA_r;
	cia_8520[0].portB_read = cia_0_portB_r;
	cia_8520[0].portA_write = cia_0_portA_w;
	cia_8520[0].portB_write = cia_0_portB_w;
	cia_8520[1].portA_read = cia_1_portA_r;
	cia_8520[1].portB_read = cia_1_portB_r;
	cia_8520[1].portA_write = cia_1_portA_w;
	cia_8520[1].portB_write = cia_1_portB_w;

	/* Initialize data direction registers */
	cia_8520[0].ddra = 0x03;
	cia_8520[0].ddrb = 0x00; /* undefined */
	cia_8520[1].ddra = 0xff;
	cia_8520[1].ddrb = 0xff;

	/* Initialize timer's, TOD's and interrupts */
	for ( i = 0; i < 2; i++ ) {
		cia_8520[i].data_latchA = 0;
		cia_8520[i].data_latchB = 0;
		cia_8520[i].timerA_latch = 0xffff;
		cia_8520[i].timerA_count = 0;
		cia_8520[i].timerA_mode = 0;
		cia_8520[i].timerB_latch = 0xffff;
		cia_8520[i].timerB_count = 0;
		cia_8520[i].timerB_mode = 0;
		cia_8520[i].tod = 0;
		cia_8520[i].tod_running = 0;
		cia_8520[i].alarm = 0;
		cia_8520[i].icr = 0;
		cia_8520[i].ics = 0;
		if ( cia_8520[i].timerA )
			timer_remove( cia_8520[i].timerA );
		cia_8520[i].timerA = 0;
		if ( cia_8520[i].timerB )
			timer_remove( cia_8520[i].timerA );
		cia_8520[i].timerB = 0;
	}
}

READ16_HANDLER ( amiga_cia_r ) {
	int cia_sel = 1, mask, data;

	offset<<=1; //PeT offset with new memory system now in word counts!
	if ( offset >= 0x1000 )
		cia_sel = 0;

	switch ( offset & 0xffe ) {
		case 0x000:
			data = (*cia_8520[cia_sel].portA_read)();
			mask = ~( cia_8520[cia_sel].ddra );
			data &= mask;
			mask = cia_8520[cia_sel].ddra & cia_8520[cia_sel].data_latchA;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x100:
			data = (*cia_8520[cia_sel].portB_read)();
			mask = ~( cia_8520[cia_sel].ddrb );
			data &= mask;
			mask = cia_8520[cia_sel].ddrb & cia_8520[cia_sel].data_latchB;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x200:
			data = cia_8520[cia_sel].ddra;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x300:
			data = cia_8520[cia_sel].ddrb;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x400:
			data = cia_get_count( cia_sel, 0 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x500:
			data = ( cia_get_count( cia_sel, 0 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x600:
			data = cia_get_count( cia_sel, 1 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x700:
			data = ( cia_get_count( cia_sel, 1 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x800:
			data = cia_8520[cia_sel].tod & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x900:
			data = ( cia_8520[cia_sel].tod >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xa00:
			data = ( cia_8520[cia_sel].tod >> 16 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xd00:
			data = cia_8520[cia_sel].ics;
			cia_8520[cia_sel].ics = 0; /* clear on read */
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xe00:
			data = cia_8520[cia_sel].timerA_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xf00:
			data = cia_8520[cia_sel].timerB_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;
	}

#if LOG_CIA
	logerror("PC = %06x - Read from CIA %01x\n", cpu_get_pc(), cia_sel );
#endif

	return 0;
}

WRITE16_HANDLER ( amiga_cia_w ) {
	int cia_sel = 1, mask;

	offset<<=1;
	if ( offset >= 0x1000 )
		cia_sel = 0;
	else
		data >>= 8;

	data &= 0xff;

	switch ( offset & 0xffe ) {
		case 0x000:
			mask = cia_8520[cia_sel].ddra;
			cia_8520[cia_sel].data_latchA = data;
			(*cia_8520[cia_sel].portA_write)( data & mask );
		break;

		case 0x100:
			mask = cia_8520[cia_sel].ddrb;
			cia_8520[cia_sel].data_latchB = data;
			(*cia_8520[cia_sel].portB_write)( data & mask );
		break;

		case 0x200:
			cia_8520[cia_sel].ddra = data;
		break;

		case 0x300:
			cia_8520[cia_sel].ddrb = data;
		break;

		case 0x400:
			cia_8520[cia_sel].timerA_latch &= 0xff00;
			cia_8520[cia_sel].timerA_latch |= data;
		break;

		case 0x500:
			cia_8520[cia_sel].timerA_latch &= 0x00ff;
			cia_8520[cia_sel].timerA_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerA_mode & 0x08 ) {
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_8520[cia_sel].timerA_mode |= 1;
				cia_fire_timer( cia_sel, 0 );
			}
		break;

		case 0x600:
			cia_8520[cia_sel].timerB_latch &= 0xff00;
			cia_8520[cia_sel].timerB_latch |= data;
		break;

		case 0x700:
			cia_8520[cia_sel].timerB_latch &= 0x00ff;
			cia_8520[cia_sel].timerB_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerB_mode & 0x08 ) {
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_8520[cia_sel].timerB_mode |= 1;
				cia_fire_timer( cia_sel, 1 );
			}
		break;

		case 0x800:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xffff00;
				cia_8520[cia_sel].alarm |= data;
			} else {
				cia_8520[cia_sel].tod &= 0xffff00;
				cia_8520[cia_sel].tod |= data;
				cia_8520[cia_sel].tod_running = 1;
			}
		break;

		case 0x900:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xff00ff;
				cia_8520[cia_sel].alarm |= data << 8;
			} else {
				cia_8520[cia_sel].tod &= 0xff00ff;
				cia_8520[cia_sel].tod |= data << 8;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xa00:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0x00ffff;
				cia_8520[cia_sel].alarm |= data << 16;
			} else {
				cia_8520[cia_sel].tod &= 0x00ffff;
				cia_8520[cia_sel].tod |= data << 16;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xd00:
			if ( data & 0x80 ) /* set */
				cia_8520[cia_sel].icr |= ( data & 0x7f );
			else /* clear */
				cia_8520[cia_sel].icr &= ~( data & 0x7f );
		break;

		case 0xe00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_stop_timer( cia_sel, 0 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 0 );
			else
				cia_stop_timer( cia_sel, 0 );

			cia_8520[cia_sel].timerA_mode = data & 0xef;
		break;

		case 0xf00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_stop_timer( cia_sel, 1 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 1 );
			else
				cia_stop_timer( cia_sel, 1 );

			cia_8520[cia_sel].timerB_mode = data & 0xef;
		break;
	}

#if LOG_CIA
	logerror("PC = %06x - Wrote to CIA %01x (%02x)\n", cpu_get_pc(), cia_sel, data );
#endif
}


/***************************************************************************

	Custom chips emulation

***************************************************************************/

static void amiga_custom_init( void ) {
	custom_regs.DDFSTRT = 0x18;
	custom_regs.DDFSTOP = 0xd8;
}

READ16_HANDLER ( amiga_custom_r ) {

    offset<<=1;
	offset &= 0xfff;

	switch ( offset ) {
		case 0x0002: /* DMACON */
			return custom_regs.DMACON;
		break;

		case 0x0004: /* VPOSR */
			return ( ( cpu_getscanline() >> 8 ) & 1 );
		break;

		case 0x0006: /* VHPOSR */
			{
				int h, v;

				h = ( cpu_gethorzbeampos() >> 1 ); /* in color clocks */
				v = ( cpu_getscanline() & 0xff );

				return ( v << 8 ) | h;
			}
		break;

		case 0x000a: /* JOY0DAT */
			if ( readinputport( 0 ) & 0x20 ) {
				int input = ( readinputport( 1 ) >> 4 );
				int	top,bot,lft,rgt;

				top = ( input >> 3 ) & 1;
				bot = ( input >> 2 ) & 1;
				lft = ( input >> 1 ) & 1;
				rgt = input & 1;

				if ( lft ) top ^= 1;
				if ( rgt ) bot ^= 1;

				return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
			} else {
				int input = ( readinputport( 2 ) & 0xff );

				input |= ( readinputport( 3 ) & 0xff ) << 8;

				return input;
			}
		break;

		case 0x000c: /* JOY1DAT */
			if ( readinputport( 0 ) & 0x10 ) {
				int input = ( readinputport( 1 ) & 0x0f );
				int	top,bot,lft,rgt;

				top = ( input >> 3 ) & 1;
				bot = ( input >> 2 ) & 1;
				lft = ( input >> 1 ) & 1;
				rgt = input & 1;

				if ( lft ) top ^= 1;
				if ( rgt ) bot ^= 1;

				return ( bot | ( rgt << 1 ) | ( top << 8 ) | ( lft << 9 ) );
			} else {
				int input = ( readinputport( 4 ) & 0xff );

				input |= ( readinputport( 5 ) & 0xff ) << 8;

				return input;
			}
		break;

		case 0x0010: /* ADKCONR */
			return custom_regs.ADKCON;
		break;

		case 0x0012: /* POT0DAT */
		case 0x0014: /* POT0DAT */
			return 0;
		break;

		case 0x0016: /* POTGOR */
			custom_regs.POTGOR = custom_regs.POTGO & 0xaa00;
			custom_regs.POTGOR |= ( readinputport( 0 ) & 1 ) << 10;
			custom_regs.POTGOR |= ( readinputport( 0 ) & 2 ) << 13;
			return custom_regs.POTGOR;
		break;

		case 0x001a: /* DSKBYTR */
			return fdc_get_byte();
		break;


		case 0x001c: /* INTENA */
			return custom_regs.INTENA;
		break;

		case 0x001e: /* INTREQ */
			return custom_regs.INTREQ;
		break;

		case 0x0088: /* COPJMPA */
			copper_setpc( ( custom_regs.COPLCH[0] << 16 ) | custom_regs.COPLCL[0] );
		break;

		case 0x008a: /* COPJMPB */
			copper_setpc( ( custom_regs.COPLCH[1] << 16 ) | custom_regs.COPLCL[1] );
		break;

		default:
#if LOG_CUSTOM
			logerror("PC = %06x - Read from Custom %04x\n", cpu_get_pc(), offset );
#endif
		break;
	}

	return 0;
}

#define SETCLR( reg, data ) { \
	if ( data & 0x8000 ) \
		reg |= ( data & 0x7fff ); \
	else \
		reg &= ~( data & 0x7fff ); }

WRITE16_HANDLER ( amiga_custom_w ) {
    offset<<=1;
	offset &= 0xfff;

	switch ( offset ) {
		case 0x0020: /* DSKPTH */
			custom_regs.DSKPTH = data;
		break;

		case 0x0022: /* DSKPTL */
			custom_regs.DSKPTL = data;
		break;

		case 0x0024: /* DSKLEN */
			if ( data & 0x8000 ) {
				if ( custom_regs.DSKLEN & 0x8000 )
					fdc_setup_dma();
			}
			custom_regs.DSKLEN = data;
		break;

		case 0x002e: /* COPCON */
			custom_regs.COPCON = data & 2;
		break;

		case 0x0034: /* POTGO */
			custom_regs.POTGO = data;
		break;

		case 0x0040: /* BLTCON0 */
			custom_regs.BLTCON0 = data;
		break;

		case 0x0042: /* BLTCON1 */
			custom_regs.BLTCON1 = data;
		break;

		case 0x0044: /* BLTAFWM */
			custom_regs.BLTAFWM = data;
		break;

		case 0x0046: /* BLTALWM */
			custom_regs.BLTALWM = data;
		break;

		case 0x0048: /* BLTCPTH */
		case 0x004a: /* BLTCPTL */
		case 0x004c: /* BLTBPTH */
		case 0x004e: /* BLTBPTL */
		case 0x0050: /* BLTAPTH */
		case 0x0052: /* BLTAPTL */
		case 0x0054: /* BLTDPTH */
		case 0x0056: /* BLTDPTL */
			{
				int lo = ( offset & 2 );
				int loc = ( offset - 0x48 ) >> 2;
				int order[4] = { 2, 1, 0, 3 };

				if ( lo )
					custom_regs.BLTxPTL[order[loc]] = ( data & 0xfffe ); /* should be word aligned, we make sure is is */
				else
					custom_regs.BLTxPTH[order[loc]] = ( data & 0x1f );
			}
		break;

		case 0x0058: /* BLTSIZE */
			custom_regs.BLTSIZE = data;
			blitter_setup();
		break;

		case 0x0060: /* BLTCMOD */
		case 0x0062: /* BLTBMOD */
		case 0x0064: /* BLTAMOD */
		case 0x0066: /* BLTDMOD */
			{
				int loc = ( offset >> 1 ) & 3;
				int order[4] = { 2, 1, 0, 3 };

				custom_regs.BLTxMOD[order[loc]] = ( signed short )( data & ~1 ); /* strip off lsb */
			}
		break;

		case 0x0070: /* BLTCDAT */
		case 0x0072: /* BLTBDAT */
		case 0x0074: /* BLTADAT */
			{
				int loc = ( offset >> 1 ) & 3;
				int order[3] = { 2, 1, 0 };

				custom_regs.BLTxDAT[order[loc]] = data;
			}
		break;

		case 0x007e: /* DSKSYNC */
			custom_regs.DSKSYNC = data;
		break;

		case 0x0080: /* COP1LCH */
		case 0x0082: /* COP1LCL */
		case 0x0084: /* COP2LCH */
		case 0x0086: /* COP1LCL */
			{
				int lo = ( offset & 2 );
				int loc = ( offset >> 2 ) & 1;

				if ( lo )
					custom_regs.COPLCL[loc] = ( data & ~1 ); /* should be word aligned, we make sure it is */
				else
					custom_regs.COPLCH[loc] = ( data & 0x1f );
			}
		break;

		case 0x0088: /* COPJMPA */
			copper_setpc( ( custom_regs.COPLCH[0] << 16 ) | custom_regs.COPLCL[0] );
		break;

		case 0x008a: /* COPJMPB */
			copper_setpc( ( custom_regs.COPLCH[1] << 16 ) | custom_regs.COPLCL[1] );
		break;

		case 0x008e: /* DIWSTRT */
			custom_regs.DIWSTRT = data;
		break;

		case 0x0090: /* DIWSTOP */
			custom_regs.DIWSTOP = data;
		break;

		case 0x0092: /* DDFSTRT */
			custom_regs.DDFSTRT = data & 0xfc;
			/* impose hardware limits ( HRM, page 75 ) */
			if ( custom_regs.DDFSTRT < 0x18 )
				custom_regs.DDFSTRT = 0x18;
		break;

		case 0x0094: /* DDFSTOP */
			custom_regs.DDFSTOP = data & 0xfc;
			/* impose hardware limits ( HRM, page 75 ) */
			if ( custom_regs.DDFSTOP > 0xd8 )
				custom_regs.DDFSTOP = 0xd8;
		break;

		case 0x0096: /* DMACON */
			SETCLR( custom_regs.DMACON, data )
			copper_enable();
		break;

		case 0x009a: /* INTENA */
			SETCLR( custom_regs.INTENA, data )
#if OLD_INTERRUPT_SYSTEM
			if ( data & 0x8000 )
				trigger_int();
#else
			check_ints();
#endif
		break;

		case 0x009c: /* INTREQ */
			SETCLR( custom_regs.INTREQ, data )
#if OLD_INTERRUPT_SYSTEM
			if ( data & 0x8000 )
				trigger_int();
#else
			check_ints();
#endif
		break;

		case 0x009e: /* ADKCON */
			SETCLR( custom_regs.ADKCON, data )
		break;

		case 0x00e0: /* BPL1PTH */
		case 0x00e2: /* BPL1PTL */
		case 0x00e4: /* BPL2PTH */
		case 0x00e6: /* BPL2PTL */
		case 0x00e8: /* BPL3PTH */
		case 0x00ea: /* BPL3PTL */
		case 0x00ec: /* BPL4PTH */
		case 0x00ee: /* BPL4PTL */
		case 0x00f0: /* BPL5PTH */
		case 0x00f2: /* BPL5PTL */
		case 0x00f4: /* BPL6PTH */
		case 0x00f6: /* BPL6PTL */
			{
				int lo = ( offset & 2 );
				int plane = ( offset >> 2 ) & 0x07;

				if ( lo ) {
					custom_regs.BPLPTR[plane] &= 0x001f0000;
					custom_regs.BPLPTR[plane] |= ( data & 0xfffe );
				} else {
					custom_regs.BPLPTR[plane] &= 0x0000ffff;
					custom_regs.BPLPTR[plane] |= ( data & 0x1f ) << 16;
				}
			}
		break;

		case 0x0100: /* BPLCON0 */
			custom_regs.BPLCON0 = data;

			if ( ( custom_regs.BPLCON0 & ( BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 ) ) == ( BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 ) ) {
				/* planes go from 0 to 6, inclusive */
				logerror("This game is doing funky planes stuff. (planes > 6)\n" );
				custom_regs.BPLCON0 &= ~BPLCON0_BPU0;
			}
		break;

		case 0x0102: /* BPLCON1 */
			custom_regs.BPLCON1 = data & 0xff;
		break;

		case 0x0104: /* BPLCON2 */
			custom_regs.BPLCON2 = data & 0x7f;
		break;

		case 0x0108: /* BPL1MOD */
			custom_regs.BPL1MOD = data;
		break;

		case 0x010a: /* BPL2MOD */
			custom_regs.BPL2MOD = data;
		break;

		case 0x0110: /* BPL1DAT */
		case 0x0112: /* BPL2DAT */
		case 0x0114: /* BPL3DAT */
		case 0x0116: /* BPL4DAT */
		case 0x0118: /* BPL5DAT */
		case 0x011a: /* BPL6DAT */
			/* in our current implementation of the screen update, we ignore this */
		break;

		case 0x0120: /* SPR0PTH */
		case 0x0122: /* SPR0PTL */
		case 0x0124: /* SPR1PTH */
		case 0x0126: /* SPR1PTL */
		case 0x0128: /* SPR2PTH */
		case 0x012a: /* SPR2PTL */
		case 0x012c: /* SPR3PTH */
		case 0x012e: /* SPR3PTL */
		case 0x0130: /* SPR4PTH */
		case 0x0132: /* SPR4PTL */
		case 0x0134: /* SPR5PTH */
		case 0x0136: /* SPR5PTL */
		case 0x0138: /* SPR6PTH */
		case 0x013a: /* SPR6PTL */
		case 0x013c: /* SPR7PTH */
		case 0x013e: /* SPR7PTL */
			{
				int lo = ( offset & 2 );
				int num = ( offset >> 2 ) & 0x07;

				if ( lo ) {
					custom_regs.SPRxPT[num] &= 0x001f0000;
					custom_regs.SPRxPT[num] |= ( data & 0xfffe );
					amiga_reload_sprite_info( num );
				} else {
					custom_regs.SPRxPT[num] &= 0x0000ffff;
					custom_regs.SPRxPT[num] |= ( data & 0x1f ) << 16;
				}
			}
		break;

		case 0x180: /* COLOR00 */
		case 0x182: /* COLOR01 */
		case 0x184: /* COLOR02 */
		case 0x186: /* COLOR03 */
		case 0x188: /* COLOR04 */
		case 0x18a: /* COLOR05 */
		case 0x18c: /* COLOR06 */
		case 0x18e: /* COLOR07 */
		case 0x190: /* COLOR08 */
		case 0x192: /* COLOR09 */
		case 0x194: /* COLOR10 */
		case 0x196: /* COLOR11 */
		case 0x198: /* COLOR12 */
		case 0x19a: /* COLOR13 */
		case 0x19c: /* COLOR14 */
		case 0x19e: /* COLOR15 */
		case 0x1a0: /* COLOR16 */
		case 0x1a2: /* COLOR17 */
		case 0x1a4: /* COLOR18 */
		case 0x1a6: /* COLOR19 */
		case 0x1a8: /* COLOR20 */
		case 0x1aa: /* COLOR21 */
		case 0x1ac: /* COLOR22 */
		case 0x1ae: /* COLOR23 */
		case 0x1b0: /* COLOR24 */
		case 0x1b2: /* COLOR25 */
		case 0x1b4: /* COLOR26 */
		case 0x1b6: /* COLOR27 */
		case 0x1b8: /* COLOR28 */
		case 0x1ba: /* COLOR29 */
		case 0x1bc: /* COLOR30 */
		case 0x1be: /* COLOR31 */
			{
				int color = ( offset - 0x180 ) >> 1;

				custom_regs.COLOR[color] = data;
			}
		break;

		default:
#if LOG_CUSTOM
		logerror("PC = %06x - Wrote to Custom %04x (%04x)\n", cpu_get_pc(), offset, data );
#endif
		break;
	}
}

/***************************************************************************

	Interrupt handling

***************************************************************************/

int amiga_vblank_irq( void ) {
	/* Update TOD on CIA A */
	cia_vblank_update();

	if ( cia_hblank_timer == 0 )
		cia_hblank_timer = timer_set( cpu_getscanlineperiod(), 0, cia_hblank_update );

	amiga_custom_w( 0x009c>>1, 0x8020, 0);

	return ignore_interrupt();
}

/***************************************************************************

	Init Machine

***************************************************************************/

void amiga_init_machine( void ) {

	/* Initialize the CIA's */
	cia_init();

	/* Initialize the Custom chips */
	amiga_custom_init();

	/* setup the ROM bank */
	cpu_setbank( 2, &memory_region( REGION_CPU1 )[0x100000] );

	/* Fake our reset pointer */
	/* This is done with a hardware overlay from the 8520 CIA A in the real hardware */
	memcpy( memory_region( REGION_CPU1 ), &memory_region( REGION_CPU1 )[0x180000], 8 );
}
