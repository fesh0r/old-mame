/***************************************************************************
	vtech1.c

	Video Technology Models (series 1)
	Laser 110 monochrome
	Laser 210
		Laser 200 (same hardware?)
		aka VZ 200 (Australia)
		aka Salora Fellow (Finland)
		aka Texet8000 (UK)
	Laser 310
        aka VZ 300 (Australia)

    video hardware
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	Thanks go to:
	- Guy Thomason
	- Jason Oakley
	- Bushy Maunder
	- and anybody else on the vzemu list :)
    - Davide Moretti for the detailed description of the colors.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/vtech1.h"
#include "vidhrdw/m6847.h"

/* bit 3 of cassette/speaker/vdp control latch defines if the mode is text or
graphics */

static void vtech1_charproc(UINT8 c)
{
	/* bit 7 defines if semigraphics/text used */
	/* bit 6 defines if chars should be inverted */

	m6847_inv_w(0,      (c & 0x40));
	m6847_as_w(0,		(c & 0x80));
	/* it appears this is fixed */
	m6847_intext_w(0,	0);
}

VIDEO_START( vtech1 )
{
	struct m6847_init_params p;

	m6847_vh_normalparams(&p);
	p.version = M6847_VERSION_ORIGINAL_PAL;
	p.ram = memory_region(REGION_CPU1) + 0x7000;
	p.ramsize = 0x10000;
	p.charproc = vtech1_charproc;

	if (video_start_m6847(&p))
		return 1;

	return (0);
}
