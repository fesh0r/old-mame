#include "driver.h"
#include "video/vector.h"
#include "machine/6522via.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"
#include "image.h"

#include "includes/vectrex.h"


#define VC_RED      MAKE_RGB(0xff, 0x00, 0x00)
#define VC_GREEN    MAKE_RGB(0x00, 0xff, 0x00)
#define VC_BLUE     MAKE_RGB(0x00, 0x00, 0xff)
#define VC_DARKRED  MAKE_RGB(0x80, 0x00, 0x00)

#define DAMPC (-0.2)
#define MMI (5.0)

enum {
	PORTB = 0,
	PORTA
};


/*********************************************************************

   Global variables

*********************************************************************/

unsigned char vectrex_via_out[2];
rgb_t vectrex_beam_color;      /* the color of the vectrex beam */
int vectrex_imager_status = 0;     /* 0 = off, 1 = right eye, 2 = left eye */
double vectrex_imager_freq;
emu_timer *vectrex_imager_timer;
int vectrex_lightpen_port=0;
int vectrex_reset_refresh;


/*********************************************************************

   Local variables

*********************************************************************/

/* Colors for right and left eye */
static rgb_t imager_colors[6];

/* Starting points of the three colors */
/* Values taken from J. Nelson's drawings*/

static const double minestorm_3d_angles[3] = {0, 0.1692, 0.2086};
static const double narrow_escape_angles[3] = {0, 0.1631, 0.3305};
static const double crazy_coaster_angles[3] = {0, 0.1631, 0.3305};


static const double unknown_game_angles[3] = {0,0.16666666, 0.33333333};
static const double *vectrex_imager_angles = unknown_game_angles;
static unsigned char vectrex_imager_pinlevel;


static int vectrex_verify_cart(char *data)
{
	/* Verify the file is accepted by the Vectrex bios */
	if (!memcmp(data,"g GCE", 5))
		return IMAGE_VERIFY_PASS;
	else
		return IMAGE_VERIFY_FAIL;
}


/*********************************************************************

   ROM load and id functions

*********************************************************************/

DEVICE_IMAGE_LOAD(vectrex_cart)
{
	UINT8 *mem = memory_region(image->machine, "maincpu");
	image_fread(image, mem, 0x8000);

	/* check image! */
	if (vectrex_verify_cart((char*)mem) == IMAGE_VERIFY_FAIL)
	{
		logerror("Invalid image!\n");
		return INIT_FAIL;
	}

	/* If VIA T2 starts, reset refresh timer.
       This is the best strategy for most games. */
	vectrex_reset_refresh = 1;

	vectrex_imager_angles = narrow_escape_angles;

	/* let's do this 3D detection with a strcmp using data inside the cart images */
	/* slightly prettier than having to hardcode CRCs */

	/* handle 3D Narrow Escape but skip the 2-d hack of it from Fred Taft */
	if (!memcmp(mem + 0x11,"NARROW",6) && (((char*)mem)[0x39] == 0x0c))
	{
		vectrex_imager_angles = narrow_escape_angles;
	}

	if (!memcmp(mem + 0x11,"CRAZY COASTER", 13))
	{
		vectrex_imager_angles = crazy_coaster_angles;
	}

	if (!memcmp(mem + 0x11,"3D MINE STORM", 13))
	{
		vectrex_imager_angles = minestorm_3d_angles;

		/* Don't reset T2 each time it's written.
           This would cause jerking in mine3. */
		vectrex_reset_refresh = 0;
	}

	return INIT_PASS;
}


/*********************************************************************

   Vectrex configuration (mainly 3D Imager)

*********************************************************************/

void vectrex_configuration(running_machine *machine)
{
	unsigned char cport = input_port_read(machine, "3DCONF");

	/* Vectrex 'dipswitch' configuration */

	/* Imager control */
	if (cport & 0x01) /* Imager enabled */
	{
		if (vectrex_imager_status == 0)
			vectrex_imager_status = cport & 0x01;

		vector_add_point_function = cport & 0x02 ? vectrex_add_point_stereo: vectrex_add_point;

		switch ((cport >> 2) & 0x07)
		{
		case 0x00:
			imager_colors[0] = imager_colors[1] = imager_colors[2] = RGB_BLACK;
			break;
		case 0x01:
			imager_colors[0] = imager_colors[1] = imager_colors[2] = VC_DARKRED;
			break;
		case 0x02:
			imager_colors[0] = imager_colors[1] = imager_colors[2] = VC_GREEN;
			break;
		case 0x03:
			imager_colors[0] = imager_colors[1] = imager_colors[2] = VC_BLUE;
			break;
		case 0x04:
			/* mine3 has a different color sequence */
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[0] = VC_GREEN;
				imager_colors[1] = VC_RED;
			}
			else
			{
				imager_colors[0] = VC_RED;
				imager_colors[1] = VC_GREEN;
			}
			imager_colors[2]=VC_BLUE;
			break;
		}

		switch ((cport >> 5) & 0x07)
		{
		case 0x00:
			imager_colors[3] = imager_colors[4] = imager_colors[5] = RGB_BLACK;
			break;
		case 0x01:
			imager_colors[3] = imager_colors[4] = imager_colors[5] = VC_DARKRED;
			break;
		case 0x02:
			imager_colors[3] = imager_colors[4] = imager_colors[5] = VC_GREEN;
			break;
		case 0x03:
			imager_colors[3] = imager_colors[4] = imager_colors[5] = VC_BLUE;
			break;
		case 0x04:
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[3] = VC_GREEN;
				imager_colors[4] = VC_RED;
			}
			else
			{
				imager_colors[3] = VC_RED;
				imager_colors[4] = VC_GREEN;
			}
			imager_colors[5]=VC_BLUE;
			break;
		}
	}
	else
	{
		vector_add_point_function = vectrex_add_point;
		vectrex_beam_color = RGB_WHITE;
		imager_colors[0] = imager_colors[1] = imager_colors[2] = imager_colors[3] = imager_colors[4] = imager_colors[5] = RGB_WHITE;
	}
	vectrex_lightpen_port = input_port_read(machine, "LPENCONF") & 0x03;
}


/*********************************************************************

   VIA interface functions

*********************************************************************/

void vectrex_via_irq(const device_config *device, int level)
{
	cputag_set_input_line(device->machine, "maincpu", M6809_IRQ_LINE, level);
}


READ8_DEVICE_HANDLER(vectrex_via_pb_r)
{
	int pot;
	static const char *const ctrlnames[] = { "CONTR1X", "CONTR1Y", "CONTR2X", "CONTR2Y" };

	pot = input_port_read(device->machine, ctrlnames[(vectrex_via_out[PORTB] & 0x6) >> 1]) - 0x80;

	if (pot > (signed char)vectrex_via_out[PORTA])
		vectrex_via_out[PORTB] |= 0x20;
	else
		vectrex_via_out[PORTB] &= ~0x20;

	return vectrex_via_out[PORTB];
}


READ8_DEVICE_HANDLER(vectrex_via_pa_r)
{
	if ((!(vectrex_via_out[PORTB] & 0x10)) && (vectrex_via_out[PORTB] & 0x08))
		/* BDIR inactive, we can read the PSG. BC1 has to be active. */
	{
		const device_config *ay = devtag_get_device(device->machine, "ay8912");

		vectrex_via_out[PORTA] = ay8910_r(ay, 0)
			& ~(vectrex_imager_pinlevel & 0x80);
	}
	return vectrex_via_out[PORTA];
}


READ8_DEVICE_HANDLER(vectrex_s1_via_pb_r)
{
	return (vectrex_via_out[PORTB] & ~0x40) | (input_port_read(device->machine, "COIN") & 0x40);
}


/*********************************************************************

   3D Imager support

*********************************************************************/

static TIMER_CALLBACK(vectrex_imager_change_color)
{
	vectrex_beam_color = param;
}


static TIMER_CALLBACK(update_level)
{
	if (ptr)
		* (UINT8 *) ptr = param;
}


TIMER_CALLBACK(vectrex_imager_eye)
{
	const device_config *via_0 = devtag_get_device(machine, "via6522_0");
	int coffset;
	double rtime = (1.0 / vectrex_imager_freq);

	if (vectrex_imager_status > 0)
	{
		vectrex_imager_status = param;
		coffset = param > 1? 3: 0;
		timer_set (machine, double_to_attotime(rtime * vectrex_imager_angles[0]), NULL, imager_colors[coffset+2], vectrex_imager_change_color);
		timer_set (machine, double_to_attotime(rtime * vectrex_imager_angles[1]), NULL, imager_colors[coffset+1], vectrex_imager_change_color);
		timer_set (machine, double_to_attotime(rtime * vectrex_imager_angles[2]), NULL, imager_colors[coffset], vectrex_imager_change_color);

		if (param == 2)
		{
			timer_set (machine, double_to_attotime(rtime * 0.50), NULL, 1, vectrex_imager_eye);

			/* Index hole sensor is connected to IO7 which triggers also CA1 of VIA */
			via_ca1_w(via_0, 1);
			via_ca1_w(via_0, 0);
			vectrex_imager_pinlevel |= 0x80;
			timer_set (machine, double_to_attotime(rtime / 360.0), &vectrex_imager_pinlevel, 0, update_level);
		}
	}
}


WRITE8_HANDLER(vectrex_psg_port_w)
{
	static int state;
	static double sl, pwl;
	double wavel, ang_acc, tmp;
	int mcontrol;

	mcontrol = data & 0x40; /* IO6 controls the imager motor */

	if (!mcontrol && mcontrol ^ state)
	{
		state = mcontrol;
		tmp = attotime_to_double(timer_get_time(space->machine));
		wavel = tmp - sl;
		sl = tmp;

		if (wavel < 1)
		{
			/* The Vectrex sends a stream of pulses which control the speed of
               the motor using Pulse Width Modulation. Guessed parameters are MMI
               (mass moment of inertia) of the color wheel, DAMPC (damping coefficient)
               of the whole thing and some constants of the motor's torque/speed curve.
               pwl is the negative pulse width and wavel is the whole wavelength. */

			ang_acc = (50.0 - 1.55 * vectrex_imager_freq) / MMI;
			vectrex_imager_freq += ang_acc * pwl + DAMPC * vectrex_imager_freq / MMI * wavel;

			if (vectrex_imager_freq > 1)
			{
				timer_adjust_periodic(vectrex_imager_timer,
									  double_to_attotime(MIN(1.0 / vectrex_imager_freq, attotime_to_double(timer_timeleft(vectrex_imager_timer)))),
									  2,
									  double_to_attotime(1.0 / vectrex_imager_freq));
			}
		}
	}
	if (mcontrol && mcontrol ^ state)
	{
		state = mcontrol;
		pwl = attotime_to_double(timer_get_time(space->machine)) - sl;
	}
}


DRIVER_INIT(vectrex)
{
	int i;

	vectrex_beam_color = RGB_WHITE;
	for (i=0; i<ARRAY_LENGTH(imager_colors); i++)
		imager_colors[i] = RGB_WHITE;

	/*
     * Uninitialized RAM needs to return 0xff. Otherwise the mines in
     * the first level of Minestorm are not evenly distributed.
     */

	memset(vectorram, 0xff, vectorram_size);
}
