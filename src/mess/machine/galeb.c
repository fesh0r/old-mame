/***************************************************************************

		Galeb machine driver by Miodrag Milanovic

		23/02/2008 Sound support added.
		22/02/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "sound/dac.h"
#include "includes/galeb.h"
  
  
/* Driver initialization */
DRIVER_INIT(galeb)
{
}

MACHINE_RESET( galeb )
{
}

READ8_HANDLER( galeb_keyboard_r )
{
	static const char *keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	return input_port_read(machine, keynames[offset]);
}

WRITE8_HANDLER( galeb_speaker_w )
{	 
	dac_data_w(0,data);
}
