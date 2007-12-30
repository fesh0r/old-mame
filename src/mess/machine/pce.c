
#include "driver.h"
#include "video/vdc.h"
#include "cpu/h6280/h6280.h"
#include "includes/pce.h"
#include "devices/chd_cd.h"
#include "sound/msm5205.h"
#include "sound/cdda.h"
#include "image.h"

/* the largest possible cartridge image (street fighter 2 - 2.5MB) */
#define PCE_ROM_MAXSIZE				(0x280000)
#define PCE_BRAM_SIZE				0x800
#define PCE_ADPCM_RAM_SIZE			0x10000
#define PCE_CD_COMMAND_BUFFER_SIZE	0x100

#define PCE_CD_IRQ_TRANSFER_READY	0x40
#define PCE_CD_IRQ_TRANSFER_DONE	0x20

#define PCE_CD_DATA_FRAMES_PER_SECOND	75

enum {
	PCE_CD_CDDA_OFF=0,
	PCE_CD_CDDA_PLAYING,
	PCE_CD_CDDA_PAUSED
};

/* system RAM */
unsigned char *pce_user_ram;    /* scratch RAM at F8 */

/* CD Unit RAM */
UINT8	*pce_cd_ram;			/* 64KB RAM from a CD unit */
static UINT8	pce_sys3_card;	/* Is a Super CD System 3 card present */
static struct {
	UINT8	regs[16];
	UINT8	*bram;
	UINT8	*adpcm_ram;
	int		bram_locked;
	int		adpcm_read_ptr;
	int		adpcm_write_ptr;
	int		adpcm_length;
	int		adpcm_clock_count;
	int		adpcm_clock_divider;
	/* SCSI signals */
	int		scsi_BSY;	/* Busy. Bus in use */
	int		scsi_SEL;	/* Select. Initiator has won arbitration and has selected a target */
	int		scsi_CD;	/* Control/Data. Target is sending control (data) information */
	int		scsi_IO;	/* Input/Output. Target is sending (receiving) information */
	int		scsi_MSG;	/* Message. Target is sending or receiving a message */
	int		scsi_REQ;	/* Request. Target is requesting a data transfer */
	int		scsi_ACK;	/* Acknowledge. Initiator acknowledges that it is ready for a data transfer */
	int		scsi_ATN;	/* Attention. Initiator has a message ready for the target */
	int		scsi_RST;	/* Reset. Initiator forces all targets and any other initiators to do a warm reset */
	int		scsi_last_RST;	/* To catch setting of RST signal */
	int		cd_motor_on;
	int		selected;
	UINT8	*command_buffer;
	int		command_buffer_index;
	int		status_sent;
	int		message_after_status;
	int		message_sent;
	UINT8	*data_buffer;
	int		data_buffer_size;
	int		data_buffer_index;
	int		data_transferred;
	UINT32	current_frame;
	UINT32	end_frame;
	UINT32	last_frame;
	UINT8	cdda_status;
	UINT8	cdda_play_mode;
	UINT8	*subcode_buffer;
	cdrom_file	*cd;
	const cdrom_toc*	toc;
	emu_timer	*data_timer;
	emu_timer	*adpcm_dma_timer;
} pce_cd;

/* MSM5205 ADPCM decoder definition */
static void pce_cd_msm5205_int( int data );
const struct MSM5205interface pce_cd_msm5205_interface = {
	pce_cd_msm5205_int,	/* interrupt function */
	MSM5205_S48_4B		/* 1/48 prescaler, 4bit data */
};

struct pce_struct pce;

static UINT8 *cartridge_ram;

/* joystick related data*/

#define JOY_CLOCK   0x01
#define JOY_RESET   0x02

static int joystick_port_select;        /* internal index of joystick ports */
static int joystick_data_select;        /* which nibble of joystick data we want */

/* prototypes */
static void pce_cd_init( void );
static void pce_cd_set_irq_line( int num, int state );
static TIMER_CALLBACK( pce_cd_adpcm_dma_timer_callback );


static mess_image *cdrom_device_image( void ) {
	return image_from_devtype_and_index( IO_CDROM, 0 );
}

static WRITE8_HANDLER( pce_sf2_banking_w ) {
	memory_set_bankptr( 2, memory_region(REGION_USER1) + offset * 0x080000 + 0x080000 );
	memory_set_bankptr( 3, memory_region(REGION_USER1) + offset * 0x080000 + 0x088000 );
	memory_set_bankptr( 4, memory_region(REGION_USER1) + offset * 0x080000 + 0x0D0000 );
}

static WRITE8_HANDLER( pce_cartridge_ram_w ) {
	cartridge_ram[ offset ] = data;
}

DEVICE_LOAD(pce_cart)
{
	int size;
	int split_rom = 0;
	const char *extrainfo;
	unsigned char *ROM;
	logerror("*** DEVICE_LOAD(pce_cart) : %s\n", image_filename(image));

	/* open file to get size */
	new_memory_region(Machine, REGION_USER1,PCE_ROM_MAXSIZE,0);
	ROM = memory_region(REGION_USER1);

	size = image_length( image );

	/* handle header accordingly */
	if((size/512)&1)
	{
		logerror("*** DEVICE_LOAD(pce_cart) : Header present\n");
		size -= 512;
		image_fseek(image, 512, SEEK_SET);
	}
	if ( size > PCE_ROM_MAXSIZE )
		size = PCE_ROM_MAXSIZE;

	image_fread(image, ROM, size);

	extrainfo = image_extrainfo( image );
	if ( extrainfo )
	{
		logerror( "extrainfo: %s\n", extrainfo );
		if ( strstr( extrainfo, "ROM_SPLIT" ) )
			split_rom = 1;
	}

	if ( ROM[0x1FFF] < 0xE0 )
	{
		int i;
		UINT8 decrypted[256];

		logerror( "*** DEVICE_LOAD(pce_cart) : ROM image seems encrypted, decrypting...\n" );

		/* Initialize decryption table */
		for( i = 0; i < 256; i++ )
			decrypted[i] = ( ( i & 0x01 ) << 7 ) | ( ( i & 0x02 ) << 5 ) | ( ( i & 0x04 ) << 3 ) | ( ( i & 0x08 ) << 1 ) | ( ( i & 0x10 ) >> 1 ) | ( ( i & 0x20 ) >> 3 ) | ( ( i & 0x40 ) >> 5 ) | ( ( i & 0x80 ) >> 7 );

		/* Decrypt ROM image */
		for( i = 0; i < size; i++ )
			ROM[i] = decrypted[ROM[i]];
	}

	/* check if we're dealing with a split rom image */
	if ( size == 384 * 1024 )
	{
		split_rom = 1;
		/* Mirror the upper 128KB part of the image */
		memcpy( ROM + 0x060000, ROM + 0x040000, 0x020000 );	/* Set up 060000 - 07FFFF mirror */
	}

	/* set up the memory for a split rom image */
	if ( split_rom )
	{
		logerror( "Split rom detected, setting up memory accordingly\n" );
		/* Set up ROM address space as follows:          */
		/* 000000 - 03FFFF : ROM data 000000 - 03FFFF    */
		/* 040000 - 07FFFF : ROM data 000000 - 03FFFF    */
		/* 080000 - 0BFFFF : ROM data 040000 - 07FFFF    */
		/* 0C0000 - 0FFFFF : ROM data 040000 - 07FFFF    */
		memcpy( ROM + 0x080000, ROM + 0x040000, 0x040000 );	/* Set up 080000 - 0BFFFF region */
		memcpy( ROM + 0x0C0000, ROM + 0x040000, 0x040000 );	/* Set up 0C0000 - 0FFFFF region */
		memcpy( ROM + 0x040000, ROM, 0x040000 );		/* Set up 040000 - 07FFFF region */
	}
	else
	{
		/* mirror 256KB rom data */
		if ( size <= 0x040000 )
			memcpy( ROM + 0x040000, ROM, 0x040000 );

		/* mirror 512KB rom data */
		if ( size <= 0x080000 )
			memcpy( ROM + 0x080000, ROM, 0x080000 );
	}

	memory_set_bankptr( 1, ROM );
	memory_set_bankptr( 2, ROM + 0x080000 );
	memory_set_bankptr( 3, ROM + 0x088000 );
	memory_set_bankptr( 4, ROM + 0x0D0000 );

	/* Check for Street fighter 2 */
	if ( size == PCE_ROM_MAXSIZE ) {
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x01ff0, 0x01ff3, 0, 0, pce_sf2_banking_w );
	}

	/* Check for Populous */
	if ( ! memcmp( ROM + 0x1F26, "POPULOUS", 8 ) ) {
		cartridge_ram = auto_malloc( 0x8000 );
		memory_set_bankptr( 2, cartridge_ram );
		memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x080000, 0x087FFF, 0, 0, pce_cartridge_ram_w );
	}

	/* Check for CD system card */
	pce_sys3_card = 0;
	if ( ! memcmp( ROM + 0x3FFB6, "PC Engine CD-ROM SYSTEM", 23 ) ) {
		/* Check if 192KB additional system card ram should be used */
		if ( ! memcmp( ROM + 0x29D1, "VER. 3.", 7 ) || ! memcmp( ROM + 0x29C4, "VER. 3.", 7 ) ) {
			pce_sys3_card = 1;
			cartridge_ram = auto_malloc( 0x30000 );
			memory_set_bankptr( 4, cartridge_ram );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0D0000, 0x0FFFFF, 0, 0, pce_cartridge_ram_w );
		}
	}
	return 0;
}

DRIVER_INIT( pce ) {
	pce.io_port_options = PCE_JOY_SIG | CONST_SIG;
}

DRIVER_INIT( tg16 ) {
	pce.io_port_options = TG_16_JOY_SIG | CONST_SIG;
}

DRIVER_INIT( sgx ) {
	pce.io_port_options = PCE_JOY_SIG | CONST_SIG;
}

MACHINE_RESET( pce ) {
	pce_cd_init();
}

/* todo: how many input ports does the PCE have? */
WRITE8_HANDLER ( pce_joystick_w )
{
	h6280io_set_buffer(data);
    /* bump counter on a low-to-high transition of bit 1 */
    if((!joystick_data_select) && (data & JOY_CLOCK))
    {
        joystick_port_select = (joystick_port_select + 1) & 0x07;
    }

    /* do we want buttons or direction? */
    joystick_data_select = data & JOY_CLOCK;

    /* clear counter if bit 2 is set */
    if(data & JOY_RESET)
    {
        joystick_port_select = 0;
    }
}

READ8_HANDLER ( pce_joystick_r )
{
	UINT8 ret;
	int data = readinputport(0);
	if(joystick_data_select) data >>= 4;
	ret = (data & 0x0F) | pce.io_port_options;
#ifdef UNIFIED_PCE
	ret &= ~0x40;
#endif
	return (ret);
}

NVRAM_HANDLER( pce )
{
	if (read_or_write)
	{
		mame_fwrite(file, pce_cd.bram, PCE_BRAM_SIZE);
	}
	else
	{
		/* load battery backed memory from disk */
		if (file)
			mame_fread(file, pce_cd.bram, PCE_BRAM_SIZE);
	}
}

static void pce_set_cd_bram( void ) {
	memory_set_bankptr( 10, pce_cd.bram + ( pce_cd.bram_locked ? PCE_BRAM_SIZE : 0 ) );
}

/* Callback for new data from the MSM5205.
  The PCE cd unit actually divides the clock signal supplied to
  the MSM5205. Currently we can only use static clocks for the
  MSM5205.
 */
static void pce_cd_msm5205_int( int chip ) {
	pce_cd.adpcm_clock_count = ( pce_cd.adpcm_clock_count + 1 ) % pce_cd.adpcm_clock_divider;
	if ( ! pce_cd.adpcm_clock_count ) {
		/* Supply new ADPCM data */
	} else {
		/* Make sure the sample does not change */
	}
}

#define	SCSI_STATUS_OK			0x00
#define SCSI_CHECK_CONDITION	0x02

static void pce_cd_reply_status_byte( UINT8 status ) {
logerror("Setting CD in reply_status_byte\n");
	pce_cd.scsi_CD = pce_cd.scsi_IO = pce_cd.scsi_REQ = 1;
	pce_cd.scsi_MSG = 0;
	pce_cd.message_after_status = 1;
	pce_cd.status_sent = pce_cd.message_sent = 0;

	if ( status == SCSI_STATUS_OK ) {
		pce_cd.regs[0x01] = 0x00;
	} else {
		pce_cd.regs[0x01] = 0x01;
	}
}

/* 0x00 - TEST UNIT READY */
static void pce_cd_test_unit_ready( void ) {
	logerror("test unit ready\n");
	if ( pce_cd.cd ) {
		logerror( "Sending STATUS_OK status\n" );
		pce_cd_reply_status_byte( SCSI_STATUS_OK );
	} else {
		logerror( "Sending CHECK_CONDITION status\n" );
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
	}
}

/* 0x08 - READ (6) */
static void pce_cd_read_6( void ) {
	UINT32 frame = ( ( pce_cd.command_buffer[1] & 0x1F ) << 16 ) | ( pce_cd.command_buffer[2] << 8 ) | pce_cd.command_buffer[3];
	UINT32 frame_count = pce_cd.command_buffer[4];

	/* Check for presence of a CD */
	if ( ! pce_cd.cd ) {
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	if ( pce_cd.cdda_status != PCE_CD_CDDA_OFF ) {
		pce_cd.cdda_status = PCE_CD_CDDA_OFF;
		cdda_stop_audio( 0 );
	}

	pce_cd.current_frame = frame;
	pce_cd.end_frame = frame + frame_count;

	if ( frame_count == 0 ) {
		pce_cd_reply_status_byte( SCSI_STATUS_OK );
	} else {
		timer_adjust( pce_cd.data_timer, ATTOTIME_IN_HZ( PCE_CD_DATA_FRAMES_PER_SECOND ), 0, ATTOTIME_IN_HZ( PCE_CD_DATA_FRAMES_PER_SECOND ) );
	}
}

/* 0xD8 - SET AUDIO PLAYBACK START POSITION (NEC) */
static void pce_cd_nec_set_audio_start_position( void ) {
	UINT32	frame = 0;

	if ( ! pce_cd.cd ) {
		/* Throw some error here */
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	switch( pce_cd.command_buffer[9] & 0xC0 ) {
	case 0x00:
		frame = ( pce_cd.command_buffer[3] << 16 ) | ( pce_cd.command_buffer[4] << 8 ) | pce_cd.command_buffer[5];
		break;
	case 0x40:
		frame = bcd_2_dec( pce_cd.command_buffer[4] ) + 75 * ( bcd_2_dec( pce_cd.command_buffer[3] ) + 60 * bcd_2_dec( pce_cd.command_buffer[2] ) );
		break;
	case 0x80:
		frame = pce_cd.toc->tracks[ bcd_2_dec( pce_cd.command_buffer[2] ) - 1 ].physframeofs;
		break;
	default:
		assert( NULL == pce_cd_nec_set_audio_start_position );
		break;
	}

	pce_cd.current_frame = frame;
	pce_cd.cdda_play_mode = pce_cd.command_buffer[1];
	if ( pce_cd.cdda_play_mode ) {
		pce_cd.cdda_status = PCE_CD_CDDA_PLAYING;
		cdda_start_audio( 0, pce_cd.current_frame, pce_cd.end_frame - pce_cd.current_frame );
	} else {
		pce_cd.cdda_status = PCE_CD_CDDA_OFF;
		cdda_stop_audio( 0 );
		pce_cd.end_frame = pce_cd.last_frame;
	}

	pce_cd_reply_status_byte( SCSI_STATUS_OK );
	pce_cd_set_irq_line( PCE_CD_IRQ_TRANSFER_DONE, ASSERT_LINE );
}

/* 0xD9 - SET AUDIO PLAYBACK END POSITION (NEC) */
static void pce_cd_nec_set_audio_stop_position( void ) {
	UINT32  frame = 0;

	if ( ! pce_cd.cd ) {
		/* Throw some error here */
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	switch( pce_cd.command_buffer[9] & 0xC0 ) {
	case 0x00:
		frame = ( pce_cd.command_buffer[3] << 16 ) | ( pce_cd.command_buffer[4] << 8 ) | pce_cd.command_buffer[5];
		break;
	case 0x40:
		frame = bcd_2_dec( pce_cd.command_buffer[4] ) + 75 * ( bcd_2_dec( pce_cd.command_buffer[3] ) + 60 * bcd_2_dec( pce_cd.command_buffer[2] ) );
		break;
	case 0x80:
		frame = pce_cd.toc->tracks[ bcd_2_dec( pce_cd.command_buffer[2] ) - 1 ].physframeofs;
		break;
	default:
		assert( NULL == pce_cd_nec_set_audio_start_position );
		break;
	}

	pce_cd.end_frame = frame;
	pce_cd.cdda_play_mode = pce_cd.command_buffer[1];
	if ( pce_cd.cdda_play_mode ) {
		if ( pce_cd.cdda_status == PCE_CD_CDDA_PAUSED ) {
			cdda_pause_audio( 0, 0 );
		} else {
			cdda_start_audio( 0, pce_cd.current_frame, pce_cd.end_frame - pce_cd.current_frame );
		}
		pce_cd.cdda_status = PCE_CD_CDDA_PLAYING;
	} else {
		pce_cd.cdda_status = PCE_CD_CDDA_OFF;
		cdda_stop_audio( 0 );
		pce_cd.end_frame = pce_cd.last_frame;
		assert( NULL == pce_cd_nec_set_audio_stop_position );
	}

	pce_cd_reply_status_byte( SCSI_STATUS_OK );
	pce_cd_set_irq_line( PCE_CD_IRQ_TRANSFER_DONE, ASSERT_LINE );
}

/* 0xDA - PAUSE (NEC) */
static void pce_cd_nec_pause( void ) {

	/* If no cd mounted throw an error */
	if ( ! pce_cd.cd ) {
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	/* If there was no cdda playing, throw an error */
	if ( pce_cd.cdda_status == PCE_CD_CDDA_OFF ) {
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	pce_cd.cdda_status = PCE_CD_CDDA_PAUSED;
	pce_cd.current_frame = cdda_get_audio_lba( 0 );
	cdda_pause_audio( 0, 1 );
	pce_cd_reply_status_byte( SCSI_STATUS_OK );
}

/* 0xDD - READ SUBCHANNEL Q (NEC) */
static void pce_cd_nec_get_subq( void ) {
	/* WP - I do not have access to chds with subchannel information yet, so I'm faking something here */
	UINT32 msf_abs, msf_rel, track, frame;

	if ( ! pce_cd.cd ) {
		/* Throw some error here */
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
		return;
	}

	frame = pce_cd.current_frame;

	switch( pce_cd.cdda_status ) {
	case PCE_CD_CDDA_PAUSED:
		pce_cd.data_buffer[0] = 2;
		frame = cdda_get_audio_lba( 0 );
		break;
	case PCE_CD_CDDA_PLAYING:
		pce_cd.data_buffer[0] = 0;
		frame = cdda_get_audio_lba( 0 );
		break;
	default:
		pce_cd.data_buffer[0] = 3;
		break;
	}

	msf_abs = lba_to_msf( frame );
	track = cdrom_get_track( pce_cd.cd, frame );
	msf_rel = lba_to_msf( frame - cdrom_get_track_start( pce_cd.cd, track ) );

	pce_cd.data_buffer[1] = 0;
	pce_cd.data_buffer[2] = track+1;					/* track */
	pce_cd.data_buffer[3] = 1;							/* index */
	pce_cd.data_buffer[4] = ( msf_rel >> 16 ) & 0xFF;	/* M (relative) */
	pce_cd.data_buffer[5] = ( msf_rel >> 8 ) & 0xFF;	/* S (relative) */
	pce_cd.data_buffer[6] = msf_rel & 0xFF;;			/* F (relative) */
	pce_cd.data_buffer[7] = ( msf_abs >> 16 ) & 0xFF;	/* M (absolute) */
	pce_cd.data_buffer[8] = ( msf_abs >> 8 ) & 0xFF;	/* S (absolute) */
	pce_cd.data_buffer[9] = msf_abs & 0xFF;				/* F (absolute) */
	pce_cd.data_buffer_size = 10;

	pce_cd.data_buffer_index = 0;
	pce_cd.data_transferred = 1;
	pce_cd.scsi_IO = 1;
	pce_cd.scsi_CD = 0;
}

/* 0xDE - GET DIR INFO (NEC) */
static void pce_cd_nec_get_dir_info( void ) {
	UINT32 frame, msf, track = 0;
	mess_image *img = cdrom_device_image();
	const cdrom_toc	*toc;
	logerror("nec get dir info\n");

	if ( ! image_exists( img ) ) {
		/* Throw some error here */
		pce_cd_reply_status_byte( SCSI_CHECK_CONDITION );
	}

	toc = cdrom_get_toc( mess_cd_get_cdrom_file(img) );

	switch( pce_cd.command_buffer[1] ) {
	case 0x00:		/* Get first and last track numbers */
		pce_cd.data_buffer[0] = dec_2_bcd(1);
		pce_cd.data_buffer[1] = dec_2_bcd(toc->numtrks);
		pce_cd.data_buffer_size = 2;
		break;
	case 0x01:		/* Get total disk size in MSF format */
		frame = toc->tracks[toc->numtrks-1].physframeofs;
		frame += toc->tracks[toc->numtrks-1].frames;
		msf = lba_to_msf( frame );

		pce_cd.data_buffer[0] = ( msf >> 16 ) & 0xFF;	/* M */
		pce_cd.data_buffer[1] = ( msf >> 8 ) & 0xFF;	/* S */
		pce_cd.data_buffer[2] = msf & 0xFF;				/* F */
		pce_cd.data_buffer_size = 3;
		break;
	case 0x02:		/* Get track information */
		if ( pce_cd.command_buffer[2] == 0xAA ) {
			frame = toc->tracks[toc->numtrks-1].physframeofs;
			frame += toc->tracks[toc->numtrks-1].frames;
			pce_cd.data_buffer[3] = 0x04;	/* correct? */
		} else {
			track = MAX( bcd_2_dec( pce_cd.command_buffer[2] ), 1 );
			frame = toc->tracks[track-1].physframeofs;
			pce_cd.data_buffer[3] = ( toc->tracks[track-1].trktype == CD_TRACK_AUDIO ) ? 0x00 : 0x04;
		}
		logerror("track = %d, frame = %d\n", track, frame );
		msf = lba_to_msf( frame + 150 );
		pce_cd.data_buffer[0] = ( msf >> 16 ) & 0xFF;	/* M */
		pce_cd.data_buffer[1] = ( msf >> 8 ) & 0xFF;	/* S */
		pce_cd.data_buffer[2] = msf & 0xFF;				/* F */
		pce_cd.data_buffer_size = 4;
		break;
	default:
		assert( pce_cd_nec_get_dir_info == NULL );  // Not implemented yet
		break;
	}

	pce_cd.data_buffer_index = 0;
	pce_cd.data_transferred = 1;
	pce_cd.scsi_IO = 1;
	pce_cd.scsi_CD = 0;
}

static void pce_cd_handle_data_output( void ) {
	static const struct {
		UINT8	command_byte;
		UINT8	command_size;
		void	(*command_handler)(void);
	} pce_cd_commands[] = {
		{ 0x00, 6, pce_cd_test_unit_ready },				/* TEST UNIT READY */
		{ 0x08, 6, pce_cd_read_6 },							/* READ (6) */
		{ 0xD8,10, pce_cd_nec_set_audio_start_position },	/* NEC SET AUDIO PLAYBACK START POSITION */
		{ 0xD9,10, pce_cd_nec_set_audio_stop_position },	/* NEC SET AUDIO PLAYBACK END POSITION */
		{ 0xDA,10, pce_cd_nec_pause },						/* NEC PAUSE */
		{ 0xDD,10, pce_cd_nec_get_subq },					/* NEC GET SUBCHANNEL Q */
		{ 0xDE,10, pce_cd_nec_get_dir_info },				/* NEC GET DIR INFO */
		{ 0xFF, 0, NULL }									/* end of list marker */
	};

	if ( pce_cd.scsi_REQ && pce_cd.scsi_ACK ) {
		/* Command byte received */
		logerror( "Command byte $%02X received\n", pce_cd.regs[0x01] );

		/* Check for buffer overflow */
		assert( pce_cd.command_buffer_index < PCE_CD_COMMAND_BUFFER_SIZE );

		pce_cd.command_buffer[pce_cd.command_buffer_index] = pce_cd.regs[0x01];
		pce_cd.command_buffer_index++;
		pce_cd.scsi_REQ = 0;
	}

	if ( ! pce_cd.scsi_REQ && ! pce_cd.scsi_ACK && pce_cd.command_buffer_index ) {
		int i = 0;

		logerror( "Check if command done\n" );

		for( i = 0; pce_cd.command_buffer[0] > pce_cd_commands[i].command_byte; i++ );

		/* Check for unknown commands */
		if ( pce_cd.command_buffer[0] != pce_cd_commands[i].command_byte ) {
			logerror("Unrecognized command: %02X\n", pce_cd.command_buffer[0] );
		}
		assert( pce_cd.command_buffer[0] == pce_cd_commands[i].command_byte );

		if ( pce_cd.command_buffer_index == pce_cd_commands[i].command_size ) {
			(pce_cd_commands[i].command_handler)();
			pce_cd.command_buffer_index = 0;
		} else {
			pce_cd.scsi_REQ = 1;
		}
	}
}

static void pce_cd_handle_data_input( void ) {
	if ( pce_cd.scsi_CD ) {
		/* Command / Status byte */
		if ( pce_cd.scsi_REQ && pce_cd.scsi_ACK ) {
			logerror( "status sent\n" );
			pce_cd.scsi_REQ = 0;
			pce_cd.status_sent = 1;
		}

		if ( ! pce_cd.scsi_REQ && ! pce_cd.scsi_ACK && pce_cd.status_sent ) {
			pce_cd.status_sent = 0;
			if ( pce_cd.message_after_status ) {
				logerror( "message after status\n" );
				pce_cd.message_after_status = 0;
				pce_cd.scsi_MSG = pce_cd.scsi_REQ = 1;
				pce_cd.regs[0x01] = 0;
			}
		}
	} else {
		/* Data */
		if ( pce_cd.scsi_REQ && pce_cd.scsi_ACK ) {
			pce_cd.scsi_REQ = 0;
		}

		if ( ! pce_cd.scsi_REQ && ! pce_cd.scsi_ACK ) {
			if ( pce_cd.data_buffer_index == pce_cd.data_buffer_size ) {
				pce_cd_set_irq_line( PCE_CD_IRQ_TRANSFER_READY, CLEAR_LINE );
				if ( pce_cd.data_transferred ) {
					pce_cd.data_transferred = 0;
					pce_cd_reply_status_byte( SCSI_STATUS_OK );
					pce_cd_set_irq_line( PCE_CD_IRQ_TRANSFER_DONE, ASSERT_LINE );
				}
			} else {
				logerror("Transfer byte from offset %d\n", pce_cd.data_buffer_index);
				pce_cd.regs[0x01] = pce_cd.data_buffer[pce_cd.data_buffer_index];
				pce_cd.data_buffer_index++;
				pce_cd.scsi_REQ = 1;
			}
		}
	}
}

static void pce_cd_handle_message_output( void ) {
	if ( pce_cd.scsi_REQ && pce_cd.scsi_ACK ) {
		pce_cd.scsi_REQ = 0;
	}
}

static void pce_cd_handle_message_input( void ) {
	if ( pce_cd.scsi_REQ && pce_cd.scsi_ACK ) {
		pce_cd.scsi_REQ = 0;
		pce_cd.message_sent = 1;
	}

	if ( ! pce_cd.scsi_REQ && ! pce_cd.scsi_ACK && pce_cd.message_sent ) {
		pce_cd.message_sent = 0;
		pce_cd.scsi_BSY = 0;
	}
}

/* Update internal CD statuses */
static void pce_cd_update( void ) {
	/* Check for reset of CD unit */
	if ( pce_cd.scsi_RST != pce_cd.scsi_last_RST ) {
		if ( pce_cd.scsi_RST ) {
			logerror("Performing CD reset\n");
			/* Reset internal data */
			pce_cd.scsi_BSY = pce_cd.scsi_SEL = pce_cd.scsi_CD = pce_cd.scsi_IO = 0;
			pce_cd.scsi_MSG = pce_cd.scsi_REQ = pce_cd.scsi_ATN = 0;
			pce_cd.cd_motor_on = 0;
			pce_cd.selected = 0;
		}
		pce_cd.scsi_last_RST = pce_cd.scsi_RST;
	}

	/* Check if bus can be freed */
	if ( ! pce_cd.scsi_SEL && ! pce_cd.scsi_BSY && pce_cd.selected ) {
		logerror( "freeing bus\n" );
		pce_cd.selected = 0;
		pce_cd.scsi_CD = pce_cd.scsi_MSG = pce_cd.scsi_IO = pce_cd.scsi_REQ = 0;
		pce_cd_set_irq_line( PCE_CD_IRQ_TRANSFER_DONE, CLEAR_LINE );
	}

	/* Select the CD device */
	if ( pce_cd.scsi_SEL ) {
		if ( ! pce_cd.selected ) {
			pce_cd.selected = 1;
logerror("Setting CD in device selection\n");
			pce_cd.scsi_BSY = pce_cd.scsi_REQ = pce_cd.scsi_CD = 1;
			pce_cd.scsi_MSG = pce_cd.scsi_IO = 0;
		}
	}

	if ( pce_cd.scsi_ATN ) {
	} else {
		/* Check for data and pessage phases */
		if ( pce_cd.scsi_BSY ) {
			if ( pce_cd.scsi_MSG ) {
				/* message phase */
				if ( pce_cd.scsi_IO ) {
					pce_cd_handle_message_input();
				} else {
					pce_cd_handle_message_output();
				}
			} else {
				/* data phase */
				if ( pce_cd.scsi_IO ) {
					/* Reading data from target */
					pce_cd_handle_data_input();
				} else {
					/* Sending data to target */
					pce_cd_handle_data_output();
				}
			}
		}
	}
}

static void pce_cd_set_irq_line( int num, int state ) {
	switch( num ) {
	case PCE_CD_IRQ_TRANSFER_DONE:
		if ( state == ASSERT_LINE ) {
			pce_cd.regs[0x03] |= PCE_CD_IRQ_TRANSFER_DONE;
		} else {
			pce_cd.regs[0x03] &= ~ PCE_CD_IRQ_TRANSFER_DONE;
		}
		break;
	case PCE_CD_IRQ_TRANSFER_READY:
		if ( state == ASSERT_LINE ) {
			pce_cd.regs[0x03] |= PCE_CD_IRQ_TRANSFER_READY;
		} else {
			pce_cd.regs[0x03] &= ~ PCE_CD_IRQ_TRANSFER_READY;
		}
		break;
	default:
		break;
	}

	if ( pce_cd.regs[0x02] & pce_cd.regs[0x03] & ( PCE_CD_IRQ_TRANSFER_DONE | PCE_CD_IRQ_TRANSFER_READY ) ) {
		cpunum_set_input_line( 0, 1, ASSERT_LINE );
	} else {
		cpunum_set_input_line( 0, 1, CLEAR_LINE );
	}
}

static TIMER_CALLBACK( pce_cd_data_timer_callback ) {
	if ( pce_cd.data_buffer_index == pce_cd.data_buffer_size ) {
		/* Read next data sector */
		logerror("read sector %d\n", pce_cd.current_frame );
		if ( ! cdrom_read_data( pce_cd.cd, pce_cd.current_frame, pce_cd.data_buffer, CD_TRACK_MODE1 ) ) {
			logerror("Mode1 CD read failed for frame #%d\n", pce_cd.current_frame );
		} else {
			logerror("Succesfully read mode1 frame #%d\n", pce_cd.current_frame );
		}

		pce_cd.data_buffer_index = 0;
		pce_cd.data_buffer_size = 2048;
		pce_cd.current_frame++;

		pce_cd.scsi_IO = 1;
		pce_cd.scsi_CD = 0;

		if ( pce_cd.current_frame == pce_cd.end_frame ) {
			/* We are done, disable the timer */
			logerror("Last frame read from CD\n");
			pce_cd.data_transferred = 1;
			timer_adjust( pce_cd.data_timer, attotime_never, 0, attotime_never );
		} else {
			pce_cd.data_transferred = 0;
		}
	}
}

static void pce_cd_init( void ) {
	/* Initialize pce_cd struct */
	memset( &pce_cd, 0, sizeof(pce_cd) );

	/* Initialize BRAM */
	pce_cd.bram = auto_malloc( PCE_BRAM_SIZE * 2 );
	memset( pce_cd.bram, 0, PCE_BRAM_SIZE );
	memset( pce_cd.bram + PCE_BRAM_SIZE, 0xFF, PCE_BRAM_SIZE );
	pce_cd.bram_locked = 1;
	pce_set_cd_bram();

	/* set up adpcm related things */
	pce_cd.adpcm_ram = auto_malloc( PCE_ADPCM_RAM_SIZE );
	memset( pce_cd.adpcm_ram, 0, PCE_ADPCM_RAM_SIZE );
	pce_cd.adpcm_clock_divider = 1;

	/* Set up cd command buffer */
	pce_cd.command_buffer = auto_malloc( PCE_CD_COMMAND_BUFFER_SIZE );
	memset( pce_cd.command_buffer, 0, PCE_CD_COMMAND_BUFFER_SIZE );
	pce_cd.command_buffer_index = 0;

	pce_cd.data_buffer = auto_malloc( 8192 );
	memset( pce_cd.data_buffer, 0, 8192 );
	pce_cd.data_buffer_size = 0;
	pce_cd.data_buffer_index = 0;

	pce_cd.subcode_buffer = auto_malloc( 96 );

	pce_cd.cd = mess_cd_get_cdrom_file_by_number( 0 );
	if ( pce_cd.cd ) {
		pce_cd.toc = cdrom_get_toc( pce_cd.cd );
		cdda_set_cdrom( 0, pce_cd.cd );
		pce_cd.last_frame = cdrom_get_track_start( pce_cd.cd, cdrom_get_last_track( pce_cd.cd ) - 1 );
		pce_cd.last_frame += pce_cd.toc->tracks[ cdrom_get_last_track( pce_cd.cd ) - 1 ].frames;
		pce_cd.end_frame = pce_cd.last_frame;
	}

	pce_cd.data_timer = timer_alloc( pce_cd_data_timer_callback , NULL);
	timer_adjust( pce_cd.data_timer, attotime_never, 0, attotime_never );
	pce_cd.adpcm_dma_timer = timer_alloc( pce_cd_adpcm_dma_timer_callback , NULL);
	timer_adjust( pce_cd.adpcm_dma_timer, attotime_never, 0, attotime_never );
}

WRITE8_HANDLER( pce_cd_bram_w ) {
	if ( ! pce_cd.bram_locked ) {
		pce_cd.bram[ offset ] = data;
	}
}

WRITE8_HANDLER( pce_cd_intf_w ) {
	logerror("%04X: write to CD interface offset %02X, data %02X\n", activecpu_get_pc(), offset, data );
	pce_cd_update();

	switch( offset ) {
	case 0x00:	/* CDC status */
		/* select device (which bits??) */
		pce_cd.scsi_SEL = 1;
		pce_cd_update();
		pce_cd.scsi_SEL = 0;
		break;
	case 0x01:	/* CDC command / status / data */
		break;
	case 0x02:	/* ADPCM / CD control / IRQ enable/disable */
				/* bit 6 - transfer ready irq */
				/* bit 5 - transfer done irq */
				/* bit 4 - ?? irq */
				/* bit 3 - ?? irq */
				/* bit 2 - ?? irq */
		pce_cd.scsi_ACK = data & 0x80;
		/* Don't set or reset any irq lines, but just verify the current state */
		pce_cd_set_irq_line( 0, 0 );
		break;
	case 0x03:	/* BRAM lock / CD status / IRQ - Read Only register */
		break;
	case 0x04:	/* CD reset */
		pce_cd.scsi_RST = data & 0x02;
		break;
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
		break;
	case 0x07:	/* BRAM unlock / CD status */
		if ( data & 0x80 ) {
			pce_cd.bram_locked = 0;
			pce_set_cd_bram();
		}
		break;
	case 0x08:	/* ADPCM address (LSB) / CD data */
		break;
	case 0x09:	/* ADPCM address (MSB) */
	case 0x0A:	/* ADPCM RAM data port */
		break;
	case 0x0B:	/* ADPCM DMA control */
		if ( ! ( pce_cd.regs[0x0B] & 0x02 ) && ( data & 0x02 ) ) {
			/* Start CD to ADPCM transfer */
			timer_adjust( pce_cd.adpcm_dma_timer, ATTOTIME_IN_HZ( PCE_CD_DATA_FRAMES_PER_SECOND * 2048 ), 0, ATTOTIME_IN_HZ( PCE_CD_DATA_FRAMES_PER_SECOND * 2048 ) );
		}
		if ( ( pce_cd.regs[0x0B] & 0x02 ) && ! ( data & 0x02 ) ) {
			/* Stop CD to ADPCM transfer (?) */
			timer_adjust( pce_cd.adpcm_dma_timer, attotime_never, 0, attotime_never );
		}
		break;
	case 0x0C:	/* ADPCM status */
		break;
	case 0x0D:	/* ADPCM address control */
		if ( ( pce_cd.regs[0x0D] & 0x80 ) && ! ( data & 0x80 ) ) {
			/* Reset ADPCM hardware */
			pce_cd.adpcm_read_ptr = 0;
			pce_cd.adpcm_write_ptr = 0;
			MSM5205_reset_w( 0, 0 );
		}
		if ( data & 0x10 ) {
			pce_cd.adpcm_length = ( pce_cd.regs[0x09] << 8 ) | pce_cd.regs[0x08];
		}
		if ( data & 0x08 ) {
			pce_cd.adpcm_read_ptr = ( pce_cd.regs[0x09] << 8 ) | pce_cd.regs[0x08];
		}
		if ( ( data & 0x03 ) == 0x03 ) {
			pce_cd.adpcm_write_ptr = ( pce_cd.regs[0x09] << 8 ) | pce_cd.regs[0x08];
		}
		break;
	case 0x0E:	/* ADPCM playback rate */
		pce_cd.adpcm_clock_divider = 16 - ( data & 0x0F );
		pce_cd.adpcm_clock_count = pce_cd.adpcm_clock_divider - 1;
		break;
	case 0x0F:	/* ADPCM and CD audio fade timer */
		break;
	default:
		return;
	}
	pce_cd.regs[offset] = data;
	pce_cd_update();
}

static TIMER_CALLBACK( pce_cd_clear_ack ) {
	pce_cd_update();
	pce_cd.scsi_ACK = 0;
	pce_cd_update();
	if ( pce_cd.scsi_CD ) {
		pce_cd.regs[0x0B] &= 0xFE;
	}
}

static UINT8 pce_cd_get_cd_data_byte( void ) {
	UINT8 data = pce_cd.regs[0x01];
	if ( pce_cd.scsi_REQ && ! pce_cd.scsi_ACK && ! pce_cd.scsi_CD ) {
		if ( pce_cd.scsi_IO ) {
			pce_cd.scsi_ACK = 1;
			timer_set( ATTOTIME_IN_CYCLES( 15, 0 ), NULL, 0, pce_cd_clear_ack );
		}
	}
	return data;
}

static TIMER_CALLBACK( pce_cd_adpcm_dma_timer_callback ) {
	if ( pce_cd.scsi_REQ && ! pce_cd.scsi_ACK && ! pce_cd.scsi_CD && pce_cd.scsi_IO ) {
		pce_cd.adpcm_ram[pce_cd.adpcm_write_ptr] = pce_cd_get_cd_data_byte();
		pce_cd.adpcm_write_ptr = ( pce_cd.adpcm_write_ptr + 1 ) & 0xFFFF;
	}
}

READ8_HANDLER( pce_cd_intf_r ) {
	UINT8 data = pce_cd.regs[offset & 0x0F];

	pce_cd_update();

	logerror("%04X: read from CD interface offset %02X\n", activecpu_get_pc(), offset );
	switch( offset ) {
	case 0x00:	/* CDC status */
		data &= 0x07;
		data |= pce_cd.scsi_BSY ? 0x80 : 0;
		data |= pce_cd.scsi_REQ ? 0x40 : 0;
		data |= pce_cd.scsi_MSG ? 0x20 : 0;
		data |= pce_cd.scsi_CD  ? 0x10 : 0;
		data |= pce_cd.scsi_IO  ? 0x08 : 0;
		break;
	case 0x01:	/* CDC command / status / data */
		break;
	case 0x02:	/* ADPCM / CD control */
		break;
	case 0x03:	/* BRAM lock / CD status */
		/* bit 4 set when CD motor is on */
		/* bit 2 set when less than half of the ADPCM data is remaining ?? */
		pce_cd.bram_locked = 1;
		pce_set_cd_bram();
		data = data & 0x6E;
		data |= ( pce_cd.cd_motor_on ? 0x10 : 0 );
		pce_cd.regs[0x03] ^= 0x02;			/* TODO: get rid of this hack */
		break;
	case 0x04:	/* CD reset */
		break;
	case 0x05:	/* Convert PCM data / PCM data */
	case 0x06:	/* PCM data */
	case 0x07:	/* BRAM unlock / CD status */
		data = ( pce_cd.bram_locked ? ( data & 0x7F ) : ( data | 0x80 ) );
		break;
	case 0x08:	/* ADPCM address (LSB) / CD data */
		data = pce_cd_get_cd_data_byte();
		break;
	case 0x09:	/* ADPCM address (MSB) */
	case 0x0A:	/* ADPCM RAM data port */
	case 0x0B:	/* ADPCM DMA control */
	case 0x0C:	/* ADPCM status */
	case 0x0D:	/* ADPCM address control */
	case 0x0E:	/* ADPCM playback rate */
	case 0x0F:	/* ADPCM and CD audio fade timer */
		break;
	case 0xC1:
		data = pce_sys3_card ? 0xAA : 0xFF;
		break;
	case 0xC2:
		data = pce_sys3_card ? 0x55 : 0xFF;
		break;
	case 0xC5:
		data = pce_sys3_card ? 0x55 : 0xFF;
		break;
	case 0xC6:
		data = pce_sys3_card ? 0xAA : 0xFF;
		break;
	default:
		data = 0xFF;
		break;
	}

	return data;
}

