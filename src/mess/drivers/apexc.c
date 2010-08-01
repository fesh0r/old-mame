/*
    drivers/apexc.c : APEXC driver

    By Raphael Nabet

    see cpu/apexc.c for background and tech info
*/

#include "emu.h"
#include "cpu/apexc/apexc.h"


class apexc_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, apexc_state(machine)); }

	apexc_state(running_machine &machine) { }

	UINT32 panel_data_reg;	/* value of a data register on the control panel which can
                                be edited - the existence of this register is a personnal
                                guess */

	bitmap_t *bitmap;

	UINT32 old_edit_keys;
	int old_control_keys;

	int letters;
	int pos;
};


static void apexc_teletyper_init(running_machine *machine);
static void apexc_teletyper_putchar(running_machine *machine, int character);


static MACHINE_START(apexc)
{
	apexc_teletyper_init(machine);
}


/*
    APEXC RAM loading/saving from cylinder image

    Note that, in an actual APEXC, the RAM contents are not read from the cylinder :
    the cylinder IS the RAM.

    This feature is important : of course, the tape reader allows to enter programs, but you
    still need an object code loader in memory.  (Of course, the control panel enables
    the user to enter such a loader manually, but it would take hours...)
*/

typedef struct _apexc_cylinder_t apexc_cylinder_t;
struct _apexc_cylinder_t
{
	int writable;
};

/*
    Open cylinder image and read RAM
*/
static DEVICE_IMAGE_LOAD( apexc_cylinder )
{
	/* load RAM contents */
	apexc_cylinder_t *cyl = (apexc_cylinder_t *)downcast<legacy_device_base *>(&image.device())->token();
	cyl->writable = image.is_writable();

	image.fread( memory_region(image.device().machine, "maincpu"), /*0x8000*/0x1000);
#ifdef LSB_FIRST
	{	/* fix endianness */
		UINT32 *RAM;
		int i;

		RAM = (UINT32 *) memory_region(image.device().machine, "maincpu");

		for (i=0; i < /*0x2000*/0x0400; i++)
			RAM[i] = BIG_ENDIANIZE_INT32(RAM[i]);
	}
#endif

	return IMAGE_INIT_PASS;
}

/*
    Save RAM to cylinder image and close it
*/
static DEVICE_IMAGE_UNLOAD( apexc_cylinder )
{
	apexc_cylinder_t *cyl = (apexc_cylinder_t *)downcast<legacy_device_base *>(&image.device())->token();
	if (cyl->writable)
	{	/* save RAM contents */
		/* rewind file */
		image.fseek(0, SEEK_SET);
#ifdef LSB_FIRST
		{	/* fix endianness */
			UINT32 *RAM;
			int i;

			RAM = (UINT32 *) memory_region(image.device().machine, "maincpu");

			for (i=0; i < /*0x2000*/0x0400; i++)
				RAM[i] = BIG_ENDIANIZE_INT32(RAM[i]);
		}
#endif
		/* write */
		image.fwrite(memory_region(image.device().machine, "maincpu"), /*0x8000*/0x1000);
	}
}

static DEVICE_START(apexc_cylinder)
{
}

static DEVICE_RESET(apexc_cylinder)
{
}

DEVICE_GET_INFO( apexc_cylinder )
{
	switch ( state )
	{
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(apexc_cylinder_t);						break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;											break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_CYLINDER;                              	break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 0;                                        	break;
		case DEVINFO_INT_IMAGE_RESET_ON_LOAD:	    info->i = 1;                                        	break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( apexc_cylinder );          	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( apexc_cylinder );				break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( apexc_cylinder );	break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(apexc_cylinder );	break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "APEXC Cylinder");	                    break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Cylinder");	                    	break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "apc");                                 break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright the MESS Team"); 			break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(APEXC_CYLINDER, apexc_cylinder);
DEFINE_LEGACY_IMAGE_DEVICE(APEXC_CYLINDER, apexc_cylinder);

#define MDRV_APEXC_CYLINDER_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, APEXC_CYLINDER, 0)


/*
    APEXC tape support

    APEXC does all I/O on paper tape.  There are 5 punch rows on tape.

    Both a reader (read-only), and a puncher (write-only) are provided.

    Tape output can be fed into a teletyper, in order to have text output :

    code                    Symbol
    (binary)        Letters         Figures
    00000                           0
    00001           T               1
    00010           B               2
    00011           O               3
    00100           E               4
    00101           H               5
    00110           N               6
    00111           M               7
    01000           A               8
    01001           L               9
    01010           R               +
    01011           G               -
    01100           I               z
    01101           P               .
    01110           C               d
    01111           V               =
    10000                   Space
    10001           Z               y
    10010           D               theta (greek letter)
    10011                   Line Space (i.e. LF)
    10100           S               ,
    10101           Y               Sigma (greek letter)
    10110           F               x
    10111           X               /
    11000                   Carriage Return
    11001           W               phi (greek letter)
    11010           J               - (dash ?)
    11011                   Figures
    11100           U               pi (greek letter)
    11101           Q               )
    11110           K               (
    11111                   Letters
*/

typedef struct _apexc_tape_t apexc_tape_t;
struct _apexc_tape_t
{
	int dummy;
};
static DEVICE_START(apexc_tape_puncher)
{
}

static DEVICE_RESET(apexc_tape_puncher)
{
}
static DEVICE_GET_INFO(apexc_tape_puncher)
{
	switch ( state )
	{
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(apexc_tape_t);							break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;											break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_PUNCHTAPE;                             	break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 0;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                        	break;
		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME(apexc_tape_puncher);        	break;
		case DEVINFO_FCT_STOP:							/* Nothing */								break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME(apexc_tape_puncher);				break;

		case DEVINFO_STR_NAME:		                strcpy( info->s, "APEXC Tape Puncher");                break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Punch tape");	                    	break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "tap");                                 break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright the MESS Team"); 			break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(APEXC_TAPE_PUNCHER, apexc_tape_puncher);
DEFINE_LEGACY_IMAGE_DEVICE(APEXC_TAPE_PUNCHER, apexc_tape_puncher);

#define MDRV_APEXC_TAPE_PUNCHER_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, APEXC_TAPE_PUNCHER, 0)

static DEVICE_GET_INFO(apexc_tape_reader)
{
	switch ( state )
	{
		case DEVINFO_STR_NAME:		                strcpy(info->s, "APEXC Tape Reader");	                    break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 0;                                        	break;
		default:									DEVICE_GET_INFO_CALL(apexc_tape_puncher);	break;
	}
}

DECLARE_LEGACY_IMAGE_DEVICE(APEXC_TAPE_READER, apexc_tape_reader);
DEFINE_LEGACY_IMAGE_DEVICE(APEXC_TAPE_READER, apexc_tape_reader);

#define MDRV_APEXC_TAPE_READER_ADD(_tag) \
	MDRV_DEVICE_ADD(_tag, APEXC_TAPE_READER, 0)

/*
    Open a tape image
*/

static READ8_DEVICE_HANDLER(tape_read)
{
	UINT8 reply;
	device_image_interface *image = dynamic_cast<device_image_interface *>(device);

	if (image->exists() && (image->fread(& reply, 1) == 1))
		return reply & 0x1f;
	else
		return 0;	/* unit not ready - I don't know what we should do */
}

static WRITE8_DEVICE_HANDLER(tape_write)
{
	UINT8 data5 = (data & 0x1f);
	device_image_interface *image = dynamic_cast<device_image_interface *>(device);

	if (image->exists())
		image->fwrite(& data5, 1);

	apexc_teletyper_putchar(device->machine, data & 0x1f);	/* display on screen */
}

/*
    APEXC control panel

    I know really little about the details, although the big picture is obvious.

    Things I know :
    * "As well as starting and stopping the machine, [it] enables information to be inserted
    manually and provides for the inspection of the contents of the memory via various
    storage registers." (Booth, p. 2)
    * "Data can be inserted manually from the control panel [into the control register]".
    (Booth, p. 3)
    * The contents of the R register can be edited, too.  A button allows to clear
    a complete X (or Y ???) field.  (forgot the reference, but must be somewhere in Booth)
    * There is no trace mode (Booth, p. 213)

    Since the control panel is necessary for the operation of the APEXC, I tried to
    implement a commonplace control panel.  I cannot tell how close the feature set and
    operation of this control panel is to the original APEXC control panel, but it
    cannot be too different in the basic principles.
*/


#if 0
/* defines for input port numbers */
enum
{
	panel_control = 0,
	panel_edit1,
	panel_edit2
};
#endif


/* defines for each bit and mask in input port panel_control */
enum
{
	/* bit numbers */
	panel_run_bit = 0,
	panel_CR_bit,
	panel_A_bit,
	panel_R_bit,
	panel_HB_bit,
	panel_ML_bit,
	panel_mem_bit,
	panel_write_bit,

	/* masks */
	panel_run = (1 << panel_run_bit),
	panel_CR  = (1 << panel_CR_bit),
	panel_A   = (1 << panel_A_bit),
	panel_R   = (1 << panel_R_bit),
	panel_HB  = (1 << panel_HB_bit),
	panel_ML  = (1 << panel_ML_bit),
	panel_mem = (1 << panel_mem_bit),
	panel_write = (1 << panel_write_bit)
};

/* fake input ports with keyboard keys */
static INPUT_PORTS_START(apexc)

	PORT_START("panel")	/* 0 : panel control */
	PORT_BIT(panel_run, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Run/Stop")                PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(panel_CR,  IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read CR")                 PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(panel_A,   IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read A")                  PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(panel_R,   IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read R")                  PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT(panel_HB,  IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read HB")                 PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(panel_ML,  IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read ML")                 PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(panel_mem, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Read mem")                PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT(panel_write, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Write instead of read") PORT_CODE(KEYCODE_LSHIFT)

	PORT_START("data")	/* data edit */
	PORT_BIT(0x80000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #1")              PORT_CODE(KEYCODE_1)
	PORT_BIT(0x40000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #2")              PORT_CODE(KEYCODE_2)
	PORT_BIT(0x20000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #3")              PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #4")              PORT_CODE(KEYCODE_4)
	PORT_BIT(0x08000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #5")              PORT_CODE(KEYCODE_5)
	PORT_BIT(0x04000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #6")              PORT_CODE(KEYCODE_6)
	PORT_BIT(0x02000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #7")              PORT_CODE(KEYCODE_7)
	PORT_BIT(0x01000000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #8")              PORT_CODE(KEYCODE_8)
	PORT_BIT(0x00800000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #9")              PORT_CODE(KEYCODE_9)
	PORT_BIT(0x00400000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #10")             PORT_CODE(KEYCODE_0)
	PORT_BIT(0x00200000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #11")             PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x00100000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #12")             PORT_CODE(KEYCODE_W)
	PORT_BIT(0x00080000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #13")             PORT_CODE(KEYCODE_E)
	PORT_BIT(0x00040000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #14")             PORT_CODE(KEYCODE_R)
	PORT_BIT(0x00020000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #15")             PORT_CODE(KEYCODE_T)
	PORT_BIT(0x00010000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #16")             PORT_CODE(KEYCODE_Y)

	PORT_BIT(0x00008000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #17")             PORT_CODE(KEYCODE_U)
	PORT_BIT(0x00004000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #18")             PORT_CODE(KEYCODE_I)
	PORT_BIT(0x00002000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #19")             PORT_CODE(KEYCODE_O)
	PORT_BIT(0x00001000, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #20")             PORT_CODE(KEYCODE_P)
	PORT_BIT(0x00000800, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #21")             PORT_CODE(KEYCODE_A)
	PORT_BIT(0x00000400, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #22")             PORT_CODE(KEYCODE_S)
	PORT_BIT(0x00000200, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #23")             PORT_CODE(KEYCODE_D)
	PORT_BIT(0x00000100, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #24")             PORT_CODE(KEYCODE_F)
	PORT_BIT(0x00000080, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #25")             PORT_CODE(KEYCODE_G)
	PORT_BIT(0x00000040, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #26")             PORT_CODE(KEYCODE_H)
	PORT_BIT(0x00000020, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #27")             PORT_CODE(KEYCODE_J)
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #28")             PORT_CODE(KEYCODE_K)
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #29")             PORT_CODE(KEYCODE_L)
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #30")             PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #31")             PORT_CODE(KEYCODE_X)
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("Toggle bit #32")             PORT_CODE(KEYCODE_C)
INPUT_PORTS_END


/*
    Not a real interrupt - just handle keyboard input
*/
static INTERRUPT_GEN( apexc_interrupt )
{
	apexc_state *state = (apexc_state *)device->machine->driver_data;
	const address_space* space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT32 edit_keys;
	int control_keys;

	int control_transitions;


	/* read new state of edit keys */
	edit_keys = input_port_read(device->machine, "data");

	/* toggle data reg according to transitions */
	state->panel_data_reg ^= edit_keys & (~state->old_edit_keys);

	/* remember new state of edit keys */
	state->old_edit_keys = edit_keys;


	/* read new state of control keys */
	control_keys = input_port_read(device->machine, "panel");

	/* compute transitions */
	control_transitions = control_keys & (~state->old_control_keys);

	/* process commands */

	if (control_transitions & panel_run)
	{	/* toggle run/stop state */
		cpu_set_reg(device, APEXC_STATE, ! cpu_get_reg(device, APEXC_STATE));
	}

	while (control_transitions & (panel_CR | panel_A | panel_R | panel_ML | panel_HB))
	{	/* read/write a register */
		/* note that we must take into account the possibility of simulteanous keypresses
        (which would be a goofy thing to do when reading, but a normal one when writing,
        if the user wants to clear several registers at once) */
		int reg_id = -1;

		/* determinate value of reg_id */
		if (control_transitions & panel_CR)
		{	/* CR register selected ? */
			control_transitions &= ~panel_CR;	/* clear so that it is ignored on next iteration */
			reg_id = APEXC_CR;			/* matching register ID */
		}
		else if (control_transitions & panel_A)
		{
			control_transitions &= ~panel_A;
			reg_id = APEXC_A;
		}
		else if (control_transitions & panel_R)
		{
			control_transitions &= ~panel_R;
			reg_id = APEXC_R;
		}
		else if (control_transitions & panel_HB)
		{
			control_transitions &= ~panel_HB;
			reg_id = APEXC_WS;
		}
		else if (control_transitions & panel_ML)
		{
			control_transitions &= ~panel_ML;
			reg_id = APEXC_ML;
		}

		if (-1 != reg_id)
		{
			/* read/write register #reg_id */
			if (control_keys & panel_write)
				/* write reg */
				cpu_set_reg(device, reg_id, state->panel_data_reg);
			else
				/* read reg */
				state->panel_data_reg = cpu_get_reg(device, reg_id);
		}
	}

	if (control_transitions & panel_mem)
	{	/* read/write memory */

		if (control_keys & panel_write) {
			/* write memory */
			memory_write_dword_32be(space, cpu_get_reg(device, APEXC_ML_FULL)<<2, state->panel_data_reg);
		}
		else {
			/* read memory */
			state->panel_data_reg = memory_read_dword_32be(space, cpu_get_reg(device, APEXC_ML_FULL)<<2);
		}
	}

	/* remember new state of control keys */
	state->old_control_keys = control_keys;
}

/*
    apexc video emulation.

    Since the APEXC has no video display, we display the control panel.

    Additionnally, We display one page of teletyper output.
*/

static const rgb_t apexc_palette[] =
{
	RGB_WHITE,
	RGB_BLACK,
	MAKE_RGB(255, 0, 0),
	MAKE_RGB(50, 0, 0)
};

static const unsigned short apexc_colortable[] =
{
	0, 1
};

#define APEXC_PALETTE_SIZE ARRAY_LENGTH(apexc_palette)
#define APEXC_COLORTABLE_SIZE sizeof(apexc_colortable)/2

enum
{
	/* size and position of panel window */
	panel_window_width = 256,
	panel_window_height = 64,
	panel_window_offset_x = 0,
	panel_window_offset_y = 0,
	/* size and position of teletyper window */
	teletyper_window_width = 256,
	teletyper_window_height = 128,
	teletyper_window_offset_x = 0,
	teletyper_window_offset_y = panel_window_height
};

static const rectangle panel_window =
{
	panel_window_offset_x,	panel_window_offset_x+panel_window_width-1,	/* min_x, max_x */
	panel_window_offset_y,	panel_window_offset_y+panel_window_height-1,/* min_y, max_y */
};
static const rectangle teletyper_window =
{
	teletyper_window_offset_x,	teletyper_window_offset_x+teletyper_window_width-1,	/* min_x, max_x */
	teletyper_window_offset_y,	teletyper_window_offset_y+teletyper_window_height-1,/* min_y, max_y */
};
enum
{
	teletyper_scroll_step = 8
};
static const rectangle teletyper_scroll_clear_window =
{
	teletyper_window_offset_x,	teletyper_window_offset_x+teletyper_window_width-1,	/* min_x, max_x */
	teletyper_window_offset_y+teletyper_window_height-teletyper_scroll_step,	teletyper_window_offset_y+teletyper_window_height-1,	/* min_y, max_y */
};
static const int var_teletyper_scroll_step = - teletyper_scroll_step;

static PALETTE_INIT( apexc )
{
	palette_set_colors(machine, 0, apexc_palette, APEXC_PALETTE_SIZE);
}

static VIDEO_START( apexc )
{
	apexc_state *state = (apexc_state *)machine->driver_data;
	screen_device *screen = screen_first(*machine);
	int width = screen->width();
	int height = screen->height();

	state->bitmap = auto_bitmap_alloc(machine, width, height, BITMAP_FORMAT_INDEXED16);
	bitmap_fill(state->bitmap, &/*machine->visible_area*/teletyper_window, 0);
}

/* draw a small 8*8 LED (well, there were no LEDs at the time, so let's call this a lamp ;-) ) */
static void apexc_draw_led(bitmap_t *bitmap, int x, int y, int state)
{
	int xx, yy;

	for (yy=1; yy<7; yy++)
		for (xx=1; xx<7; xx++)
			*BITMAP_ADDR16(bitmap, y+yy, x+xx) = state ? 2 : 3;
}

/* write a single char on screen */
static void apexc_draw_char(running_machine *machine, bitmap_t *bitmap, char character, int x, int y, int color)
{
	drawgfx_transpen(bitmap, NULL, machine->gfx[0], character-32, color, 0, 0,
				x+1, y, 0);
}

/* write a string on screen */
static void apexc_draw_string(running_machine *machine, bitmap_t *bitmap, const char *buf, int x, int y, int color)
{
	while (* buf)
	{
		apexc_draw_char(machine, bitmap, *buf, x, y, color);

		x += 8;
		buf++;
	}
}


static VIDEO_UPDATE( apexc )
{
	apexc_state *state = (apexc_state *)screen->machine->driver_data;
	int i;
	char the_char;

	bitmap_fill(bitmap, &/*machine->visible_area*/panel_window, 0);
	apexc_draw_string(screen->machine, bitmap, "power", 8, 0, 0);
	apexc_draw_string(screen->machine, bitmap, "running", 8, 8, 0);
	apexc_draw_string(screen->machine, bitmap, "data :", 0, 24, 0);

	copybitmap(bitmap, state->bitmap, 0, 0, 0, 0, &teletyper_window);


	apexc_draw_led(bitmap, 0, 0, 1);

	apexc_draw_led(bitmap, 0, 8, cpu_get_reg(screen->machine->device("maincpu"), APEXC_STATE));

	for (i=0; i<32; i++)
	{
		apexc_draw_led(bitmap, i*8, 32, (state->panel_data_reg << i) & 0x80000000UL);
		the_char = '0' + ((i + 1) % 10);
		apexc_draw_char(screen->machine, bitmap, the_char, i*8, 40, 0);
		if (((i + 1) % 10) == 0)
		{
			the_char = '0' + ((i + 1) / 10);
			apexc_draw_char(screen->machine, bitmap, the_char, i*8, 48, 0);
		}
	}
	return 0;
}

static void apexc_teletyper_init(running_machine *machine)
{
	apexc_state *state = (apexc_state *)machine->driver_data;

	state->letters = FALSE;
	state->pos = 0;
}

static void apexc_teletyper_linefeed(running_machine *machine)
{
	apexc_state *state = (apexc_state *)machine->driver_data;
	UINT8 buf[teletyper_window_width];
	int y;

	for (y=teletyper_window_offset_y; y<teletyper_window_offset_y+teletyper_window_height-teletyper_scroll_step; y++)
	{
		extract_scanline8(state->bitmap, teletyper_window_offset_x, y+teletyper_scroll_step, teletyper_window_width, buf);
		draw_scanline8(state->bitmap, teletyper_window_offset_x, y, teletyper_window_width, buf, machine->pens);
	}

	bitmap_fill(state->bitmap, &teletyper_scroll_clear_window, 0);
}

static void apexc_teletyper_putchar(running_machine *machine, int character)
{
	static const char ascii_table[2][32] =
	{
		{
			'0',				'1',				'2',				'3',
			'4',				'5',				'6',				'7',
			'8',				'9',				'+',				'-',
			'z',				'.',				'd',				'=',
			' ',				'y',				/*'@'*/'\200'/*theta*/,'\n'/*Line Space*/,
			',',				/*'&'*/'\201'/*Sigma*/,'x',				'/',
			'\r'/*Carriage Return*/,/*'!'*/'\202'/*Phi*/,'_'/*???*/,	'\0'/*Figures*/,
			/*'#'*/'\203'/*pi*/,')',				'(',				'\0'/*Letters*/
		},
		{
			' '/*???*/,			'T',				'B',				'O',
			'E',				'H',				'N',				'M',
			'A',				'L',				'R',				'G',
			'I',				'P',				'C',				'V',
			' ',				'Z',				'D',				'\n'/*Line Space*/,
			'S',				'Y',				'F',				'X',
			'\r'/*Carriage Return*/,'W',			'J',				'\0'/*Figures*/,
			'U',				'Q',				'K',				'\0'/*Letters*/
		}
	};

	apexc_state *state = (apexc_state *)machine->driver_data;
	char buffer[2] = "x";

	character &= 0x1f;

	switch (character)
	{
	case 19:
		/* Line Space */
		apexc_teletyper_linefeed(machine);
		break;

	case 24:
		/* Carriage Return */
		state->pos = 0;
		break;

	case 27:
		/* Figures */
		state->letters = FALSE;
		break;

	case 31:
		/* Letters */
		state->letters = TRUE;
		break;

	default:
		/* Any printable character... */

		if (state->pos >= 32)
		{	/* if past right border, wrap around */
			apexc_teletyper_linefeed(machine);	/* next line */
			state->pos = 0;					/* return to start of line */
		}

		/* print character */
		buffer[0] = ascii_table[state->letters][character];	/* lookup ASCII equivalent in table */
		buffer[1] = '\0';								/* terminate string */
		apexc_draw_string(machine, state->bitmap, buffer, 8*state->pos, 176, 0);	/* print char */
		state->pos++;											/* step carriage forward */

		break;
	}
}

enum
{
	apexc_charnum = /*96+4*/128,	/* ASCII set + 4 special characters */
									/* for whatever reason, 96+4 breaks greek characters */

	apexcfontdata_size = 8 * apexc_charnum
};

/* apexc driver init : builds a font for use by the teletyper */
static DRIVER_INIT(apexc)
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[apexcfontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,

		/* theta */
		0x70,
		0x88,
		0x88,
		0xF8,
		0x88,
		0x88,
		0x70,
		0x00,

		/* Sigma */
		0xf8,
		0x40,
		0x20,
		0x10,
		0x20,
		0x40,
		0xf8,
		0x00,

		/* Phi */
		0x20,
		0x70,
		0xA8,
		0xA8,
		0xA8,
		0xA8,
		0x70,
		0x20,

		/* pi */
		0x00,
		0x00,
		0xF8,
		0x50,
		0x50,
		0x50,
		0x50,
		0x00
	};

	dst = memory_region(machine, "gfx1");

	memcpy(dst, fontdata6x8, apexcfontdata_size);
}

static const gfx_layout fontlayout =
{
	6, 8,			/* 6*8 characters */
	apexc_charnum,	/* 96+4 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static GFXDECODE_START( apexc )
	GFXDECODE_ENTRY( "gfx1", 0, fontlayout, 0, 2 )
GFXDECODE_END


static ADDRESS_MAP_START(apexc_mem_map, ADDRESS_SPACE_PROGRAM, 32)
#if 0
	AM_RANGE(0x0000, 0x03ff) AM_RAM	/* 1024 32-bit words (expandable to 8192) */
	AM_RANGE(0x0400, 0x1fff) AM_NOP
#else
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x1000, 0x7fff) AM_NOP
#endif
ADDRESS_MAP_END

static ADDRESS_MAP_START(apexc_io_map, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x00) AM_DEVREAD("tape_reader",tape_read)
	AM_RANGE(0x00, 0x00) AM_DEVWRITE("tape_puncher",tape_write)
ADDRESS_MAP_END


static MACHINE_DRIVER_START(apexc)

	MDRV_DRIVER_DATA( apexc_state )

	/* basic machine hardware */
	/* APEXC CPU @ 2.0 kHz (memory word clock frequency) */
	MDRV_CPU_ADD("maincpu", APEXC, 2000)
	/*MDRV_CPU_CONFIG(NULL)*/
	MDRV_CPU_PROGRAM_MAP(apexc_mem_map)
	MDRV_CPU_IO_MAP(apexc_io_map)
	/* dummy interrupt: handles the control panel */
	MDRV_CPU_VBLANK_INT("screen", apexc_interrupt)
	/*MDRV_CPU_PERIODIC_INT(func, rate)*/

	MDRV_MACHINE_START( apexc )

	/* video hardware does not exist, but we display a control panel and the typewriter output */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 192-1)

	MDRV_GFXDECODE(apexc)
	MDRV_PALETTE_LENGTH(APEXC_PALETTE_SIZE)

	MDRV_PALETTE_INIT(apexc)
	MDRV_VIDEO_START(apexc)
	MDRV_VIDEO_UPDATE(apexc)

	MDRV_APEXC_CYLINDER_ADD("cylinder")
	MDRV_APEXC_TAPE_PUNCHER_ADD("tape_puncher")
	MDRV_APEXC_TAPE_READER_ADD("tape_reader")
MACHINE_DRIVER_END

ROM_START(apexc)
	/*CPU memory space*/
	ROM_REGION32_BE(0x10000, "maincpu", ROMREGION_ERASEFF)
		/* Note this computer has no ROM... */

	ROM_REGION(apexcfontdata_size, "gfx1", ROMREGION_ERASEFF)
		/* space filled with our font */
ROM_END

/*         YEAR     NAME        PARENT          COMPAT  MACHINE     INPUT   INIT  COMPANY     FULLNAME */
//COMP(      1951,    apexc53,    0,            0,      apexc53,    apexc,  apexc, "Booth", "All Purpose Electronic X-ray Computer (as described in 1953)" , GAME_NOT_WORKING | GAME_NO_SOUND)
COMP(      1955,    apexc,	/*apexc53*/0,	0,	apexc,      apexc,  apexc, "Booth", "All Purpose Electronic X-ray Computer (as described in 1957)" , GAME_NO_SOUND)
